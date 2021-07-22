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
 * $Id: qmsParseTree.h 90311 2021-03-24 09:46:45Z ahra.cho $
 **********************************************************************/

#ifndef _O_QMS_PARSE_TREE_H_
#define _O_QMS_PARSE_TREE_H_ 1

#include <ida.h>
#include <idTypes.h>
#include <qc.h>
#include <qtcDef.h>
#include <qmc.h>
#include <qcmTableInfo.h>
#include <qmoHint.h>


// PROJ-1502 PARTITIONED DISK TABLE
// partition key condition value type count
#define QMS_PARTKEYCONDVAL_TYPE_COUNT           (3)

/***********************************************************************
 * [Parse Tree를 위한 상수 정의]
 *
 ***********************************************************************/

// qmsTarget.flag
#define QMS_TARGET_IS_NULLABLE_MASK             (0x00000001)
#define QMS_TARGET_IS_NULLABLE_TRUE             (0x00000001)
#define QMS_TARGET_IS_NULLABLE_FALSE            (0x00000000)

#define QMS_TARGET_IS_UPDATABLE_MASK            (0x00000002)
#define QMS_TARGET_IS_UPDATABLE_TRUE            (0x00000002)
#define QMS_TARGET_IS_UPDATABLE_FALSE           (0x00000000)

// This flag is not usable after expanding asterisk in validation
#define QMS_TARGET_ASTERISK_MASK                (0x00000004)
#define QMS_TARGET_ASTERISK_TRUE                (0x00000004)
#define QMS_TARGET_ASTERISK_FALSE               (0x00000000)

// qmsTarget.flag
#define QMS_TARGET_DUMMY_PIVOT_MASK             (0x00000008)
#define QMS_TARGET_DUMMY_PIVOT_TRUE             (0x00000008)
#define QMS_TARGET_DUMMY_PIVOT_FALSE            (0x00000000)

// qmsTarget.flag
#define QMS_TARGET_ORDER_BY_MASK                (0x00000010)
#define QMS_TARGET_ORDER_BY_TRUE                (0x00000010)
#define QMS_TARGET_ORDER_BY_FALSE               (0x00000000)

// PROJ-2469 Optimize View Materialization
// qmsTarget.flag
#define QMS_TARGET_IS_USELESS_MASK              (0x00000060)
#define QMS_TARGET_IS_USELESS_UNKNOWN           (0x00000000)
#define QMS_TARGET_IS_USELESS_FALSE             (0x00000020)
#define QMS_TARGET_IS_USELESS_TRUE              (0X00000040)

// PROJ-2523 Unpivot clause
// qmsTarget.flag
#define QMS_TARGET_UNPIVOT_COLUMN_MASK          (0x00000080)
#define QMS_TARGET_UNPIVOT_COLUMN_TRUE          (0x00000080)
#define QMS_TARGET_UNPIVOT_COLUMN_FALSE         (0x00000000)

/* TASK-7219 */
/* qmsTarget.flag */
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK        (0x00000F00)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_NONE        (0x00000000)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_COLUMN      (0x00000100)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_EXPRESSION  (0x00000200)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGREGATION (0x00000300)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_POSITION    (0x00000400)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_GROUPKEY    (0x00000500)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_NONKEY      (0x00000600)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGR        (0x00000700)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP        (0x00000800)
#define QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR       (0x00000900)

// for qmsTableRef.flag
#define QMS_TABLE_REF_CREATED_VIEW_MASK         (0x00000001)
#define QMS_TABLE_REF_CREATED_VIEW_FALSE        (0x00000000)
#define QMS_TABLE_REF_CREATED_VIEW_TRUE         (0x00000001)

// PROJ-1495
// for qmsTableRef.flag
#define QMS_TABLE_REF_PUSH_PRED_HINT_MASK       (0x00000002)
#define QMS_TABLE_REF_PUSH_PRED_HINT_FALSE      (0x00000000)
#define QMS_TABLE_REF_PUSH_PRED_HINT_TRUE       (0x00000002)

// PROJ-1502 PARTITIONED DISK TABLE
// for qmsTableRef.flag
#define QMS_TABLE_REF_PRE_PRUNING_MASK          (0x00000004)
#define QMS_TABLE_REF_PRE_PRUNING_FALSE         (0x00000000)
#define QMS_TABLE_REF_PRE_PRUNING_TRUE          (0x00000004)

// PROJ-1502 PARTITIONED DISK TABLE
// for qmsTableRef.flag
#define QMS_TABLE_REF_PARTITION_MADE_MASK       (0x00000008)
#define QMS_TABLE_REF_PARTITION_MADE_FALSE      (0x00000000)
#define QMS_TABLE_REF_PARTITION_MADE_TRUE       (0x00000008)

// BUG-17409
// for qmsTableRef.flag
#define QMS_TABLE_REF_SCAN_FOR_NON_SELECT_MASK  (0x00000010)
#define QMS_TABLE_REF_SCAN_FOR_NON_SELECT_FALSE (0x00000000)
#define QMS_TABLE_REF_SCAN_FOR_NON_SELECT_TRUE  (0x00000010)

// for qmsTableRef.flag
// exec_remote hint에 의해 생성된 tableRef임을 설정
#define QMS_TABLE_REF_EXEC_REMOTE_MASK          (0x00000020)
#define QMS_TABLE_REF_EXEC_REMOTE_FALSE         (0x00000000)
#define QMS_TABLE_REF_EXEC_REMOTE_TRUE          (0x00000020)

// BUG-26800
// inner join에 적용된 push_pred hint임을 설정
// for qmsTableRef.flag
#define QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_MASK  (0x00000040)
#define QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_FALSE (0x00000000)
#define QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_TRUE  (0x00000040)

// BUG-29598
// for qmsTableRef.flag
// dual table의 tableRef임을 설정
#define QMS_TABLE_REF_BASE_TABLE_DUAL_MASK      (0x00000080)
#define QMS_TABLE_REF_BASE_TABLE_DUAL_FALSE     (0x00000000)
#define QMS_TABLE_REF_BASE_TABLE_DUAL_TRUE      (0x00000080)

// BUG-37237
// for qmsTableRef.flag
// hierarchy query에서 사용된 view임을 설정
#define QMS_TABLE_REF_HIER_VIEW_MASK            (0x00000100)
#define QMS_TABLE_REF_HIER_VIEW_FALSE           (0x00000000)
#define QMS_TABLE_REF_HIER_VIEW_TRUE            (0x00000100)

// PROJ-2418 Cross/Outer APPLY & Lateral View
// for qmsTableRef.flag
// 해당 view가 Lateral View임을 설정
#define QMS_TABLE_REF_LATERAL_VIEW_MASK         (0x00001000)
#define QMS_TABLE_REF_LATERAL_VIEW_FALSE        (0x00000000)
#define QMS_TABLE_REF_LATERAL_VIEW_TRUE         (0x00001000)

// PROJ-2582 recursive with
// for qmsTableRef.flag
#define QMS_TABLE_REF_RECURSIVE_VIEW_MASK       (0x00000200)
#define QMS_TABLE_REF_RECURSIVE_VIEW_FALSE      (0x00000000)
#define QMS_TABLE_REF_RECURSIVE_VIEW_TRUE       (0x00000200)

/* TASK-7219 */
/* for qmsTableRef.flag */
#define QMS_TABLE_REF_SHARD_TRANSFROM_MASK      (0x00000400)
#define QMS_TABLE_REF_SHARD_TRANSFROM_FALSE     (0x00000000)
#define QMS_TABLE_REF_SHARD_TRANSFROM_TRUE      (0x00000400)

/* BUG-48090 */
/* for qmsTableRef.flag */
#define QMS_TABLE_REF_WITH_VIEW_MASK            (0x00000800)
#define QMS_TABLE_REF_WITH_VIEW_FALSE           (0x00000000)
#define QMS_TABLE_REF_WITH_VIEW_TRUE            (0x00000800)

// for qmsHierarchy.flag
#define QMS_HIERARCHY_IGNORE_LOOP_MASK          (0x00000001)
#define QMS_HIERARCHY_IGNORE_LOOP_FALSE         (0x00000000)
#define QMS_HIERARCHY_IGNORE_LOOP_TRUE          (0x00000001)

/* TASK-7219 */
#define QMS_ORDER_BY_TRANSFORM_AGGREGATION (1)
#define QMS_ORDER_BY_TRANSFORM_QUERYSET    (2)

//----------------------------------------------
// BUCKET COUNT 관련 상수
//----------------------------------------------

// BUCKET COUNT HINTS가 정의되지 않은 경우를 의미
#define QMS_NOT_DEFINED_BUCKET_CNT                       (0)
#define QMS_NOT_DEFINED_TEMP_TABLE_CNT                   (0)
// BUCKET COUNT의 최대값
#define QMS_MAX_BUCKET_CNT                       (102400000)

//PROJ-1583 large geometry
#define QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE            (0)
#define QMS_MIN_ST_OBJECT_BUFFER_SIZE                (32000)
#define QMS_MAX_ST_OBJECT_BUFFER_SIZE            (104857600)

//PROJ-2242
#define QMS_NOT_DEFINED_FIRST_ROWS_N                     (0)
#define QMS_MIN_FIRST_ROWS_N                             (1)
#define QMS_MAX_FIRST_ROWS_N                   (ID_UINT_MAX)

// SET 연산자의 종류
typedef enum qmsSetOpType
{
    QMS_NONE,
    QMS_UNION,
    QMS_UNION_ALL,
    QMS_MINUS,
    QMS_INTERSECT
} qmsSetOpType;

// SELECT Target의 종류
typedef enum qmsSelectType
{
    QMS_ALL,        // SELECT ALL ...
    QMS_DISTINCT    // SELECT DISTINCT ...
} qmsSelectType;

// Join의 종류
typedef enum qmsJoinType
{
    QMS_NO_JOIN,
    QMS_INNER_JOIN,
    QMS_FULL_OUTER_JOIN,
    QMS_LEFT_OUTER_JOIN,
    QMS_RIGHT_OUTER_JOIN
} qmsJoinType;

// GROUP BY 의 종류
typedef enum qmsGroupByType
{
    QMS_GROUPBY_NORMAL = 0,
    QMS_GROUPBY_ROLLUP,
    QMS_GROUPBY_CUBE,
    QMS_GROUPBY_GROUPING_SETS,
    // PROJ-2415 Grouping Sets Clause
    // QMS_GROUPBY_NULL Type은 Grouping Sets Transform시 사용 되며
    // Validation 단계가 지나면 QMS_GROUPBY_NULL Type은 존재하지 않는다.
    QMS_GROUPBY_NULL,
    QMS_GROUPBY_WITH_ROLLUP
} qmsGroupByType;

// for detecting current parent's validation phase.
typedef enum qmsProcessPhase
{
    QMS_VALIDATE_INIT,
    QMS_VALIDATE_FROM,
    QMS_VALIDATE_TARGET,
    QMS_VALIDATE_WHERE,
    QMS_VALIDATE_HIERARCHY,
    QMS_VALIDATE_GROUPBY,
    QMS_VALIDATE_HAVING,
    QMS_VALIDATE_ORDERBY,
    QMS_VALIDATE_SET,
    QMS_VALIDATE_INSERT,
    QMS_VALIDATE_FINAL,
    QMS_OPTIMIZE_PUSH_DOWN_PRED,
    QMS_OPTIMIZE_TRANS_PRED,
    QMS_OPTIMIZE_NODE_TRANS,
    QMS_MAKEPLAN_JOIN,
    QMS_MAKEPLAN_GROUPBY,
    QMS_MAKEPLAN_WINDOW,
    QMS_MAKEPLAN_DISTINCT,
    QMS_MAKEPLAN_ORDERBY,
    QMS_MAKEPLAN_PROJECT,
    QMS_MAKEPLAN_UPDATE,
    QMS_MAKEPLAN_DELETE,
    QMS_MAKEPLAN_MOVE,
    QMS_MAKEPLAN_MERGE
} qmsProcessPhase;

/***********************************************************************
 * [Hints 를 위한 정의]
 *
 ***********************************************************************/

typedef struct qmsHintTables
{
    qcNamePosition       userName;
    qcNamePosition       tableName;
    struct qmsFrom     * table;
    qmsHintTables      * next;
} qmsHintTables;

typedef struct qmsViewOptHints
{
    qmoViewOptType       viewOptType; // VMTR, PUSH
    qmsHintTables      * table;
    qmsViewOptHints    * next;
} qmsViewOptHints;

typedef struct qmsJoinMethodHints
{
    UInt                 flag;  // QMO_JOIN_METHOD_MASK
    qcDepInfo            depInfo;
    qmsHintTables      * joinTables;
    // USE_TWO_PASS_HASH( table_name, table_name [, temp_table_count] )
    UInt                 tempTableCnt;
    idBool               isUndirected; // PROJ-2339 Inverse Hash Join
    idBool               isNoUse;      // BUG-42413 NO_USE JoinMethod
    qmsJoinMethodHints * next;
} qmsJoinMethodHints;

// BUG-42447 leading hint support
typedef struct qmsLeadingHints
{
    qmsHintTables      * mLeadingTables;
} qmsLeadingHints;

typedef struct qmsHintIndices
{
    qcNamePosition             indexName;
    // for use in optimizer
    qcmIndex                 * indexPtr;
    qmsHintIndices           * next;
} qmsHintIndices;

typedef struct qmsTableAccessHints
{
    UInt                       flag; // validate hint done flag ( done : 1 )
    qmoTableAccessType         accessType;
    qmsHintTables            * table;
    qmsHintIndices           * indices;
    
    /* BUG-39306 partial scan */
    UInt                       count;
    UInt                       id;
    
    qmsTableAccessHints      * next;
} qmsTableAccessHints;

// PROJ-1495
// PUSH_PRED( view_name )
// view 내부로 join predicate을 내린다.
typedef struct qmsPushPredHints
{
    qmsHintTables            * table;
    qmsPushPredHints         * next;
} qmsPushPredHints;

// PROJ-1413
// NO_MERGE( view_name )
typedef struct qmsNoMergeHints
{
    qmsHintTables            * table;
    qmsNoMergeHints          * next;
} qmsNoMergeHints;

/*
 * PROJ-1665
 * PROJ-1071 Parallel Query
 *
 * PARALLEL( table_name, parallel_degree ) 
 */
typedef struct qmsParallelHints
{
    qmsHintTables          * table;
    UInt                     parallelDegree;
    struct qmsParallelHints* next;
} qmsParallelHints;

/* BUG-39399 remove search key preserved table */
typedef struct qmsKeyPreservedHints
{
    qmsHintTables               * table;
    struct qmsKeyPreservedHints * next;
} qmsKeyPreservedHints;

// BUG-41615 simple query hint
typedef enum qmsExecFastHint
{
    QMS_EXEC_FAST_NONE,
    QMS_EXEC_FAST_TRUE,
    QMS_EXEC_FAST_FALSE
} qmsExecFastHint;

// BUG-43493
typedef enum qmsDelayHint
{
    QMS_DELAY_NONE,
    QMS_DELAY_TRUE,
    QMS_DELAY_FALSE
} qmsDelayHint;

/* PROJ-2632 */
typedef enum qmsSerialFilterHint
{
    QMS_SERIAL_FILTER_NONE,
    QMS_SERIAL_FILTER_TRUE,
    QMS_SERIAL_FILTER_FALSE
} qmsSerialFilter;

// HINTS 전체를 위한 자료 구조
// TODO : tableAccess Hint가 존재하면 qmsTableRef::tableAccessHints에 연결,
//         qmsHints::viewOptType을 qmsTableRef::viewOptType에 설정
typedef struct qmsHints
{
    qmoOptGoalType                 optGoalType;        // RULE 또는 COST
    qmoNormalType                  normalType;         // CNF 또는 DNF
    // tupleset 또는 push projection이 적용된 레코드저장방식의 처리방법지정 
    qmoMaterializeType             materializeType;  
    qmoJoinOrderType               joinOrderType;      // ORDERED
    qmoInterResultType             interResultType;    // DISK/MEMORY
    qmoGroupMethodType             groupMethodType;    // HASH/SORT
    qmoDistinctMethodType          distinctMethodType; // HASH/SORT
    qmoSubqueryUnnestType          subqueryUnnestType; // UNNEST/NO_UNNEST
    qmoSemiJoinMethod              semiJoinMethod;     // NL_SJ/HASH_SJ/MERGE_SJ/SORT_SJ
    qmoAntiJoinMethod              antiJoinMethod;     // NL_AJ/HASH_AJ/MERGE_AJ/SORT_AJ
    qmoInverseJoinOption           inverseJoinOption;  // INVERSE_JOIN/NO_INVERSE_JOIN

    //------------------------------------------------------------------
    // View Optimization Hint
    //
    //    - PUSH_SELECTION_VIEW, PUSH_SELECTION_VIEW(table_name)
    //    - NO_PUSH_SELECTION_VIEW, NO_PUSH_SELECTION_VIEW([talbe_name])
    //-------------------------------------------------------------------
    qmsViewOptHints                * viewOptHint;        // VMTR, PUSH

    //-------------------------------------------------------------------
    // View Optimization Hint
    //    : VIEW 내부로 join predicate을 내린다.
    //
    //    - PUSH_PRED, PUSH_PRED( view_name )
    //------------------------------------------------------------------
    qmsPushPredHints               * pushPredHint;      // PROJ-1495

    //-------------------------------------------------------------------
    // No View Merging Hint
    //    : View를 merge하지 않는다.
    //
    //    - NO_MERGE ( v1, v2, ... )
    //------------------------------------------------------------------
    qmsNoMergeHints                * noMergeHint;      // PROJ-1413

    //------------------------------------------------------------------
    // Join Method Hint
    //
    // - USE_NL ( t1, t2 )
    // - USE_HASH ( t1, t2 )
    // - USE_SORT ( t1, t2 )
    // - USE_MERGE ( t1, t2 )
    //------------------------------------------------------------------
    struct qmsJoinMethodHints      * joinMethod;

    //------------------------------------------------------------------
    // Leading Hint
    //
    // - LEADING ( t1, t2, ... )
    //------------------------------------------------------------------
    struct qmsLeadingHints         * leading;

    //------------------------------------------------------------------
    // Table Access Hint
    //
    //   - FULL SCAN ( t1 )
    //   - NO INDEX ( t1, i1, i2, ..., i(n) )
    //   - INDEX ( t1, i1, i2, ..., i(n) )
    //   - INDEX ASC ( t1, i1, i2, ..., i(n) )
    //   - INDEX DESC ( t1, i1, i2, ..., i(n) )
    //------------------------------------------------------------------
    struct qmsTableAccessHints     * tableAccess;

    // HASH BUCKET COUNT ( n )
    // ---- for HASH, HSDS node
    UInt                           hashBucketCnt; // default 0

    // GROUP BUCKET COUNT ( n )
    // ---- for GRAG, AGGR node
    UInt                           groupBucketCnt; // default 0

    // SET BUCKET COUNT ( n )
    // ---- for SITS, SDIF node
    UInt                           setBucketCnt; // default 0

    // PROJ-1566 : INSERT 방식에 대한 Hint
    //             APPEND일 경우, 내부적으로 direct-path INSERT로 처리됨
    UInt                           directPathInsHintFlag;

    //PROJ-1583 large geometry
    UInt                           STObjBufSize;

    // PROJ-1404
    qmoTransitivePredType          transitivePredType;

    /*
     * PROJ-1665 Direct-Path INSERT
     * PROJ-1071 Parallel Query
     */
    qmsParallelHints             * parallelHint;

    // PROJ-1436
    // no_plan_cache : plan cache 대상이더라도 plan을 cache하지 않게 함
    // keep_plan : 생성된 plan에 대하여 통계정보 변경에도 그대로 사용하게 함
    idBool                         noPlanCache;
    idBool                         keepPlan;

    // PROJ-2206 MATERIALIZE, NO_MATERIALIZE 힌트
    qmoViewOptMtrType              viewOptMtrType;

    /* PROJ-2211 Materialized View */
    idBool                         refreshMView;

    //PROJ-2242 first_rows_n
    UInt                           firstRowsN;

    // PROJ-1784 DML Without Retry
    idBool                         withoutRetry;

    /* BUG-39399 remove search key preserved table */
    qmsKeyPreservedHints         * keyPreservedHint;

    /* BUG-39600 Grouping Sets View Materialization Hint */
    idBool                         GBGSOptViewMtr;

    // BUG-41615 simple query hint
    qmsExecFastHint                execFastHint;

    // PROJ-2462 Reuslt Cache
    qmoResultCacheType             resultCacheType;
    idBool                         topResultCache;

    // BUG-43493
    qmsDelayHint                   delayedExec;

    // PROJ-2673 insert before/after trigger를 동작시키지 않는다. (비공개)
    idBool                         disableInsTrigger;

    // BUG-46137
    idBool                         planCacheKeep;

    /* PROJ-2632 */
    qmsSerialFilterHint            mSerialFilter;

    // BUG-48348
    idBool                         partialCSE;
} qmsHints;

#define QCP_SET_INIT_HINTS(_dst_)                                           \
{                                                                           \
    (_dst_)->optGoalType           = QMO_OPT_GOAL_TYPE_UNKNOWN;             \
    (_dst_)->normalType            = QMO_NORMAL_TYPE_NOT_DEFINED;           \
    (_dst_)->materializeType       = QMO_MATERIALIZE_TYPE_NOT_DEFINED;      \
    (_dst_)->joinOrderType         = QMO_JOIN_ORDER_TYPE_OPTIMIZED;         \
    (_dst_)->interResultType       = QMO_INTER_RESULT_TYPE_NOT_DEFINED;     \
    (_dst_)->groupMethodType       = QMO_GROUP_METHOD_TYPE_NOT_DEFINED;     \
    (_dst_)->distinctMethodType    = QMO_DISTINCT_METHOD_TYPE_NOT_DEFINED;  \
    (_dst_)->viewOptHint           = NULL;                                  \
    (_dst_)->pushPredHint          = NULL;                                  \
    (_dst_)->noMergeHint           = NULL;                                  \
    (_dst_)->joinMethod            = NULL;                                  \
    (_dst_)->leading               = NULL;                                  \
    (_dst_)->tableAccess           = NULL;                                  \
    (_dst_)->hashBucketCnt         = QMS_NOT_DEFINED_BUCKET_CNT;            \
    (_dst_)->groupBucketCnt        = QMS_NOT_DEFINED_BUCKET_CNT;            \
    (_dst_)->setBucketCnt          = QMS_NOT_DEFINED_BUCKET_CNT;            \
    (_dst_)->directPathInsHintFlag = SMI_INSERT_METHOD_NORMAL;              \
    (_dst_)->STObjBufSize          = QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE; \
    (_dst_)->transitivePredType    = QMO_TRANSITIVE_PRED_TYPE_ENABLE;       \
    (_dst_)->parallelHint          = NULL;                                  \
    (_dst_)->noPlanCache           = ID_FALSE;                              \
    (_dst_)->keepPlan              = ID_FALSE;                              \
    (_dst_)->viewOptMtrType        = QMO_VIEW_OPT_MATERIALIZE_NOT_DEFINED;  \
    (_dst_)->refreshMView          = ID_FALSE;                              \
    (_dst_)->withoutRetry          = ID_FALSE;                              \
    (_dst_)->GBGSOptViewMtr        = ID_FALSE;                              \
    (_dst_)->subqueryUnnestType    = QMO_SUBQUERY_UNNEST_TYPE_NOT_DEFINED;  \
    (_dst_)->semiJoinMethod        = QMO_SEMI_JOIN_METHOD_NOT_DEFINED;      \
    (_dst_)->antiJoinMethod        = QMO_ANTI_JOIN_METHOD_NOT_DEFINED;      \
    (_dst_)->inverseJoinOption     = QMO_INVERSE_JOIN_METHOD_ALLOWED;       \
    (_dst_)->firstRowsN            = QMS_NOT_DEFINED_FIRST_ROWS_N;          \
    (_dst_)->keyPreservedHint      = NULL;                                  \
    (_dst_)->execFastHint          = QMS_EXEC_FAST_NONE;                    \
    (_dst_)->resultCacheType       = QMO_RESULT_CACHE_NOT_DEFINED;          \
    (_dst_)->topResultCache        = ID_FALSE;                              \
    (_dst_)->delayedExec           = QMS_DELAY_NONE;                        \
    (_dst_)->disableInsTrigger     = ID_FALSE;                              \
    (_dst_)->planCacheKeep         = ID_FALSE;                              \
    (_dst_)->mSerialFilter         = QMS_SERIAL_FILTER_NONE;                \
    (_dst_)->partialCSE            = ID_FALSE;                              \
}

#define QCP_SET_INIT_JOIN_METHOD_HINTS(_dst_)                 \
{                                                             \
    (_dst_)->flag         = 0;                                \
    qtc::dependencyClear( & (_dst_)->depInfo );               \
    (_dst_)->joinTables   = NULL;                             \
    (_dst_)->tempTableCnt = QMS_NOT_DEFINED_TEMP_TABLE_CNT;   \
    (_dst_)->isUndirected = ID_FALSE;                         \
    (_dst_)->isNoUse      = ID_FALSE;                         \
    (_dst_)->next         = NULL;                             \
}

// BUG-42447 leading hint support
#define QCP_SET_INIT_LEADING_HINTS(_dst_)                     \
{                                                             \
    (_dst_)->mLeadingTables   = NULL;                         \
}

// qmsQuerySet 자료 구조의 초기화
#define QCP_SET_INIT_QMS_QUERY_SET(_dst_)                         \
{                                                                 \
    (_dst_)->setOp  = QMS_NONE;                                   \
    (_dst_)->SFWGH  = NULL;                                       \
    (_dst_)->materializeType = QMO_MATERIALIZE_TYPE_NOT_DEFINED;  \
    qtc::dependencyClear( & (_dst_)->depInfo );                   \
    qtc::dependencyClear( & (_dst_)->outerDepInfo );              \
    qtc::dependencyClear( & (_dst_)->lateralDepInfo );            \
    (_dst_)->parentTupleID = ID_USHORT_MAX;                       \
    (_dst_)->outerSFWGH = NULL;                                   \
    (_dst_)->left   = NULL;                                       \
    (_dst_)->right  = NULL;                                       \
    (_dst_)->target = NULL;                                       \
    (_dst_)->lflag  = ID_ULONG( 0 );                              \
    SET_EMPTY_POSITION((_dst_)->startPos);                        \
    SET_EMPTY_POSITION((_dst_)->endPos);                          \
    (_dst_)->nonCrtPath = NULL;                                   \
    (_dst_)->processPhase = QMS_VALIDATE_INIT;                    \
    (_dst_)->analyticFuncList = NULL;                             \
    (_dst_)->loopNode = NULL;                                     \
    (_dst_)->mShardAnalysis = NULL;                               \
    (_dst_)->mPreAnalysis = NULL;                                 \
}

/* TASK-7219 Shard Transformer Refactoring */
#define QCP_SET_RESET_QMS_QUERY_SET(_dst_)                        \
{                                                                 \
    (_dst_)->setOp  = QMS_NONE;                                   \
    (_dst_)->SFWGH  = NULL;                                       \
    (_dst_)->materializeType = QMO_MATERIALIZE_TYPE_NOT_DEFINED;  \
    qtc::dependencyClear( & (_dst_)->depInfo );                   \
    qtc::dependencyClear( & (_dst_)->outerDepInfo );              \
    qtc::dependencyClear( & (_dst_)->lateralDepInfo );            \
    (_dst_)->parentTupleID = ID_USHORT_MAX;                       \
    (_dst_)->outerSFWGH = NULL;                                   \
    (_dst_)->left   = NULL;                                       \
    (_dst_)->right  = NULL;                                       \
    (_dst_)->target = NULL;                                       \
    (_dst_)->lflag  = ID_ULONG( 0 );                              \
    SET_EMPTY_POSITION((_dst_)->startPos);                        \
    SET_EMPTY_POSITION((_dst_)->endPos);                          \
    (_dst_)->nonCrtPath = NULL;                                   \
    (_dst_)->processPhase = QMS_VALIDATE_INIT;                    \
    (_dst_)->analyticFuncList = NULL;                             \
    (_dst_)->loopNode = NULL;                                     \
}

// qmsSFWGH 자료 구조의 초기화
#define QCP_SET_INIT_QMS_SFWGH(_dst_)                   \
{                                                       \
    (_dst_)->selectType      = QMS_ALL;                 \
    (_dst_)->hints           = NULL;                    \
    (_dst_)->top             = NULL;                    \
    (_dst_)->target          = NULL;                    \
    (_dst_)->intoVariables   = NULL;                    \
    (_dst_)->from            = NULL;                    \
    qtc::dependencyClear( & (_dst_)->depInfo );         \
    (_dst_)->where           = NULL;                    \
    (_dst_)->hierarchy       = NULL;                    \
    (_dst_)->group           = NULL;                    \
    (_dst_)->having          = NULL;                    \
    (_dst_)->aggsDepth1      = NULL;                    \
    (_dst_)->aggsDepth2      = NULL;                    \
    SET_EMPTY_POSITION((_dst_)->startPos);              \
    SET_EMPTY_POSITION((_dst_)->endPos);                \
    (_dst_)->outerQuery      = NULL;                    \
    (_dst_)->outerFrom       = NULL;                    \
    (_dst_)->outerColumns    = NULL;                    \
    qtc::dependencyClear( & (_dst_)->outerDepInfo );    \
    (_dst_)->validatePhase   = QMS_VALIDATE_INIT;       \
    (_dst_)->outerHavingCase = ID_FALSE;                \
    (_dst_)->lflag           = ID_ULONG( 0 );           \
    (_dst_)->level           = NULL;                    \
    (_dst_)->loopLevel       = NULL;                    \
    (_dst_)->isLeaf          = NULL;                    \
    (_dst_)->cnbyStackAddr   = NULL;                    \
    (_dst_)->rownum          = NULL;                    \
    (_dst_)->groupingInfoAddr = NULL;                   \
    (_dst_)->mergedSFWGH     = NULL;                    \
    (_dst_)->isTransformed   = ID_FALSE;                \
    (_dst_)->preservedInfo   = NULL;                    \
    (_dst_)->crtPath         = NULL;                    \
    (_dst_)->currentTargetNum = 0;                      \
    (_dst_)->thisQuerySet    = NULL;                    \
    (_dst_)->outerJoinTree   = NULL;                    \
    (_dst_)->recursiveViewID = ID_USHORT_MAX;           \
}
        


// qmsFrom 자료 구조의 초기화
#define QCP_SET_INIT_QMS_FROM(_dst_)                          \
{                                                             \
    (_dst_)->joinType = QMS_NO_JOIN;                          \
    SET_EMPTY_POSITION( (_dst_)->fromPosition );              \
    (_dst_)->tableRef = NULL;                                 \
    qtc::dependencyClear( &((_dst_)->depInfo) );              \
    qtc::dependencyClear( &((_dst_)->semiAntiJoinDepInfo) );  \
    (_dst_)->onCondition = NULL;                              \
    (_dst_)->next = NULL;                                     \
    (_dst_)->left = NULL;                                     \
    (_dst_)->right = NULL;                                    \
}

// qmsTableRef 자료 구조의 초기화
#define QCP_SET_INIT_QMS_TABLE_REF(_dst_)                           \
{                                                                   \
    SET_EMPTY_POSITION((_dst_)->position);                          \
    SET_EMPTY_POSITION((_dst_)->userName);                          \
    SET_EMPTY_POSITION((_dst_)->tableName);                         \
    SET_EMPTY_POSITION((_dst_)->aliasName);                         \
    (_dst_)->mDumpObjList       = NULL;                             \
    (_dst_)->view               = NULL;                             \
    (_dst_)->recursiveView      = NULL;                             \
    (_dst_)->tempRecursiveView  = NULL;                             \
    (_dst_)->sameViewRef        = NULL;                             \
    (_dst_)->withStmt           = NULL;                             \
    (_dst_)->flag               = 0;                                \
    (_dst_)->tableInfo          = NULL;                             \
    (_dst_)->viewColumnRefList  = NULL;                             \
    (_dst_)->isMerged           = ID_FALSE;                         \
    (_dst_)->isNewAliasName     = ID_FALSE;                         \
    (_dst_)->defaultExprList    = NULL;                             \
    (_dst_)->tableAccessHints   = NULL;                             \
    (_dst_)->tableHandle        = NULL;                             \
    (_dst_)->tableType          = QCM_USER_TABLE;                   \
    (_dst_)->tableFlag          = 0;                                \
    (_dst_)->tableOID           = SMI_NULL_OID;                     \
    (_dst_)->viewOptType        = QMO_VIEW_OPT_TYPE_NOT_DEFINED;    \
    (_dst_)->noMergeHint        = ID_FALSE;                         \
    (_dst_)->statInfo           = NULL;                             \
    (_dst_)->partitionRef       = NULL;                             \
    (_dst_)->partitionCount     = 0;                                \
    (_dst_)->partitionSummary   = NULL;                             \
    (_dst_)->mEmptyPartRef       = NULL;                             \
    (_dst_)->indexTableRef      = NULL;                             \
    (_dst_)->indexTableCount    = 0;                                \
    (_dst_)->selectedIndexTable = NULL;                             \
    (_dst_)->userID             = QC_EMPTY_USER_ID;                 \
    (_dst_)->columnsName        = NULL;                             \
    (_dst_)->pivot              = NULL;                             \
    (_dst_)->unpivot            = NULL;                             \
    (_dst_)->remoteTable        = NULL;                             \
    (_dst_)->remoteQuery        = NULL;                             \
    (_dst_)->funcNode           = NULL;                             \
    (_dst_)->mParallelDegree    = 0;                                \
    (_dst_)->mShardObjInfo      = NULL;                             \
}

// qmsPartitionRef 자료 구조의 초기화
#define QCP_SET_INIT_QMS_PARTITION_REF(_dst_)   \
{                                               \
    SET_EMPTY_POSITION((_dst_)->position);      \
    SET_EMPTY_POSITION((_dst_)->partitionName); \
    (_dst_)->partitionInfo    = NULL;           \
    (_dst_)->partitionHandle  = NULL;           \
    (_dst_)->statInfo         = NULL;           \
    (_dst_)->next             = NULL;           \
}

#define QCP_SET_INIT_QMS_PIVOT(_dst_)  \
{                                      \
    (_dst_)->aggrNodes  = NULL;        \
    (_dst_)->columnNode = NULL;        \
    (_dst_)->valueNode  = NULL;        \
}

#define QCP_SET_INIT_QMS_PIVOT_AGGREGATION_NODE(_dst_)  \
{                                                       \
    SET_EMPTY_POSITION((_dst_)->aliasName);             \
    SET_EMPTY_POSITION((_dst_)->aggrFunc);              \
    (_dst_)->node = NULL;                               \
    (_dst_)->next = NULL;                               \
}

#define QCP_SET_INIT_QMS_PIVOT_IN(_dst_)       \
{                                              \
    SET_EMPTY_POSITION((_dst_)->aliasName);    \
    (_dst_)->nodeList = NULL;                  \
    (_dst_)->next     = NULL;                  \
}

#define QCP_SET_INIT_QMS_PIVOT_INNODE(_dst_)   \
{                                              \
    (_dst_)->node = NULL;                      \
    (_dst_)->next = NULL;                      \
}

// PROJ-2523 Unpivot clause
#define QCP_SET_INIT_QMS_UNPIVOT(_dst_) \
{                                       \
    (_dst_)->isIncludeNulls = ID_FALSE; \
    (_dst_)->valueColName   = NULL;     \
    (_dst_)->measureColName = NULL;     \
    (_dst_)->inColInfo      = NULL;     \
}

#define QCP_SET_INIT_QMS_UNPIVOT_COL_NAME(_dst_) \
{                                                \
    SET_EMPTY_POSITION( (_dst_)->colName);       \
    (_dst_)->next = NULL;                        \
}

#define QCP_SET_INIT_QMS_UNPIVOT_IN_COL_INFO(_dst_) \
{                                                   \
    (_dst_)->inColumn = NULL;                       \
    (_dst_)->inAlias  = NULL;                       \
    (_dst_)->next = NULL;                           \
}

#define QCP_SET_INIT_QMS_UNPIVOT_IN_NODE(_dst_) \
{                                               \
    (_dst_)->inNode = NULL;                     \
    (_dst_)->next = NULL;                       \
}

#define QCP_SET_INIT_QMS_WITH(_dst_)        \
{                                           \
    SET_EMPTY_POSITION((_dst_)->startPos)   \
    SET_EMPTY_POSITION((_dst_)->stmtName)   \
    SET_EMPTY_POSITION((_dst_)->stmtText)   \
    (_dst_)->columns = NULL;                \
    (_dst_)->stmt    = NULL;                \
    (_dst_)->next    = NULL;                \
}
/***********************************************************************
 * [검색(SELECT) 구문을 위한 Parse Tree 정의]
 *
 ***********************************************************************/

// BUG-25109
// simple select query에서 참조된 base table name
typedef struct qmsBaseTableInfo
{
    SChar           tableOwnerName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar           tableName[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool          isUpdatable;
} qmsBaseTableInfo;

typedef struct qmsOrderBy
{
    struct qmsSortColumns * sortColumns;
    idBool                  isSiblings;
} qmsOrderBy;

// 검색(SELECT) 구문을 위한 Parse Tree
typedef struct qmsParseTree
{
    qcParseTree               common;

    struct qmsWithClause    * withClause; // WITH QUERY1 AS SELECT ..
    struct qmsQuerySet      * querySet;   // SELECT UNION SELECT MINUS SELECT ..
    struct qmsSortColumns   * orderBy;    // ORDER BY

    struct qmsForUpdate     * forUpdate;

    // members for LIMIT clause
    struct qmsLimit         * limit;

    // for loop clause
    struct qtcNode          * loopNode;

    // Proj-1360 Queue
    // Dequeue statement를 수행할 경우에만 설정됨.
    struct qmsQueue         * queue;

    // BUG-25109
    // 쿼리에서 참조된 base table 정보
    struct qmsBaseTableInfo   baseTableInfo;

    // PROJ-1413
    // 현재 parseTree에 merge가 수행되었는지 여부
    // order by절에 노드변화가 발생되었음을 나타낸다.
    idBool                    isTransformed;
    idBool                    isSiblings;

    /* PROJ-2598 Shard pilot(shard Analyze) */
    struct sdiShardAnalysis * mShardAnalysis;
    idBool                    isView;       /* view인지 여부 */
    idBool                    isShardView;  /* PROJ-2646 shard view인지 여부 */

} qmsParseTree;


// Proj-1360 Queue
typedef struct qmsQueue
{
    idBool     isFifo;
    // dequeue를 수행할 큐 테이블의 ID (mm에서 사용)
    UInt       tableID;
    // dequeue시 대기 시간 (mm에서 사용)
    ULong      waitSec;
} qmsQueue;


# define QMS_LIMIT_UNKNOWN    ID_ULONG_MAX

// LIMIT 표현을 위한 자료 구조
typedef struct qmsLimitValue {
    ULong     constant;
    qtcNode * hostBindNode;
    qcNamePosition mPosition; /* TASK-7219 */
} qmsLimitValue;

typedef struct qmsLimit
{
    qmsLimitValue   start;
    qmsLimitValue   count;
    qcNamePosition  limitPos;  /* BUG-36580 supported TOP */
    UInt            flag;      /* for parsing */
    idBool          mIsShard;  /* TASK-7219 */
} qmsLimit;

class qmsLimitI
{
public:

    static inline void setStart( qmsLimit * aLimit, qmsLimitValue aStart )
    {
        aLimit->start = aStart;
    }

    static inline void setCount( qmsLimit * aLimit, qmsLimitValue aCount )
    {
        aLimit->count = aCount;
    }

    static inline qmsLimitValue getStart( qmsLimit * aLimit )
    {
        return aLimit->start;
    }

    static inline qmsLimitValue getCount( qmsLimit * aLimit )
    {
        return aLimit->count;
    }

    static inline idBool hasHostBind( qmsLimit * aLimit )
    {
        if( hasHostBind( getStart( aLimit ) ) == ID_TRUE ||
            hasHostBind( getCount( aLimit ) ) == ID_TRUE )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }

    static inline void setStartValue( qmsLimit * aLimit, ULong aStartValue )
    {
        setStart( aLimit, makeLimitValue( aStartValue ) );
    }

    static inline void setCountValue( qmsLimit * aLimit, ULong aCountValue )
    {
        setCount( aLimit, makeLimitValue( aCountValue ) );
    }

    static inline ULong getStartConstant( qmsLimit * aLimit )
    {
        return getConstant( getStart( aLimit ) );
    }

    static inline ULong getCountConstant( qmsLimit * aLimit )
    {
        return getConstant( getCount( aLimit ) );
    }

    static IDE_RC getStartValue( qcTemplate * aTemplate,
                                 qmsLimit   * aLimit,
                                 ULong      * aResultValue );

    static IDE_RC getCountValue( qcTemplate * aTemplate,
                                 qmsLimit   * aLimit,
                                 ULong      * aResultValue );

    /////////////////////////////////////////////////////////////////////////////
    //
    // qmsLimitValue에 대한 interface function들
    //
    /////////////////////////////////////////////////////////////////////////////

    static inline idBool hasHostBind( qmsLimitValue aLimitValue )
    {
        return (aLimitValue.hostBindNode == NULL ) ? ID_FALSE : ID_TRUE;
    }

    static inline qmsLimitValue makeLimitValue( ULong aPrimitiveValue )
    {
        qmsLimitValue sLimitValue;

        sLimitValue.constant = aPrimitiveValue;
        sLimitValue.hostBindNode = NULL;

        return sLimitValue;
    }

    static inline ULong getConstant( qmsLimitValue aLimitValue )
    {
        return aLimitValue.constant;
    }

    static inline qtcNode * getHostNode( qmsLimitValue aLimitValue )
    {
        return aLimitValue.hostBindNode;
    }

private:
    static IDE_RC getPrimitiveValue(qcTemplate   * aTemplate,
                                    qmsLimitValue  aLimitValue,
                                    mtdBigintType* aResultValue);
};

typedef struct qmsLimitOrLoop
{
    qmsLimit   * limit;
    qtcNode    * loopNode;
} qmsLimitOrLoop;

// FOR UPDATE 구문을 위한 자료 구조
typedef struct qmsForUpdate
{
// Proj-1360 Queue
    idBool      isQueue;
    idBool      isMoveAndDelete;

    ULong       lockWaitMicroSec;
} qmsForUpdate;

/* PROJ-2435 ORDERY NULLS FRIST/LAST */
typedef enum qmsNullsOption
{
    QMS_NULLS_NONE  = 0,
    QMS_NULLS_FIRST = 1,
    QMS_NULLS_LAST  = 2
} qmsNullsOption;

// ORDER BY 구문을 위한 자료 구조
typedef struct qmsSortColumns
{
    qtcNode             * sortColumn;
    idBool                isDESC;        // ASC, DESC
    SInt                  targetPosition;
    qmsNullsOption        nullsOption;  /* PROJ-2435 ORDERY NULLS FRIST/LAST */
    qcNamePosition        mSortModePos; /* TASK-7219 */
    qcNamePosition        mNullsOptPos; /* TASK-7219 */
    qmsSortColumns      * next;
} qmsSortColumns;

// SET 구문을 위한 자료 구조
typedef struct qmsQuerySet
{
    qmsSetOpType        setOp;
    
    struct qmsSFWGH   * SFWGH;

    //----------------------------------------------------
    // [Query Set의 dependencies]
    //
    // dependencies : 하위 qmsSFWGH의 dependencies의 조합
    // SET이 아닌 경우
    //     - dependencies = SFWGH->myDependencies;
    // SET인 경우
    //     - dependencies =
    //       left->dependencies | right->dependencies | view dependencies
    //----------------------------------------------------

    qmoMaterializeType  materializeType;    

    qcDepInfo           depInfo;
    qcDepInfo           outerDepInfo;    // PROJ-1413 outer column dependencies
    UShort              parentTupleID;   // PROJ-2179 subquery의 경우에 필요

    // PROJ-2415 Grouping Sets Clause
    // Grouping Sets Transform이 생성한 View는 개념적으로 투명한( Dependency를 가질 수 있는 )View 이며
    // 이 투명한 View가 Dependency를 가질 수 있도록 상위 SFWGH를 하위 inLineView에게 전달하기 위해 추가.
    qmsSFWGH          * outerSFWGH;

    // PROJ-2418 
    // Lateral View가 외부 참조해야 하는 depInfo를 나타낸다.
    // Subquery의 outerDepInfo와는 구별된다.
    qcDepInfo           lateralDepInfo;  

    qmsQuerySet       * left;
    qmsQuerySet       * right;

    // for SET node
    struct qmsTarget  * target;

    // position of SELECT keyword for SP error message
    qcNamePosition      startPos;
    qcNamePosition      endPos;

    /* TASK-7219 Shard Transformer Refactoring
     *  ULong Type 으로 확장
     *   qmv::validateSelect에 인자를 추가하지 못하므로 생긴 member data.
     *    qmsSFWGH에 flag를 넘겨주기 위함.
     */
    ULong               lflag;

    //------------------------------
    // Non Critical Path : optimizer에서 사용
    //------------------------------

    struct qmoNonCrtPath     * nonCrtPath;

    // PROJ-1762 ( SFWGH에서 QuerySet으로 위치 옮김, Set의 OrderBy Column에
    // 대한 estimate 시에는 SFWGH 정보를 넘기지 않기 때문 )
    // for detecting current parent's validation phase
    qmsProcessPhase      processPhase;
    
    //--------------------------------
    // Analytic Function 정보 ( PROJ-1762 ) 
    //--------------------------------

    struct qmsAnalyticFunc * analyticFuncList;

    // loop node 정보
    qtcNode           * loopNode;
    mtcStack            loopStack;

    /* PROJ-2646 shard analyzer enhancement */
    struct sdiShardAnalysis  * mShardAnalysis;

    /* TASK-7219 Sahrd ransformer Refactoring */
    struct sdiShardAnalysis  * mPreAnalysis;

} qmsQuerySet;



/***********************************************************************
 * [GROUP BY를 위한 자료 구조]
 *
 * group by is composed of lists of qmsConcantElement
 *
 * qmsConcatElement can be
 *    ( arithmetic/ list-of-arithmetic )
 ***********************************************************************/

typedef struct qmsConcatElement
{
    qmsGroupByType     type;
    qtcNode          * arithmeticOrList;
    qmsConcatElement * arguments;
    qmsConcatElement * next;
    qcNamePosition     position;
} qmsConcatElement;


/***********************************************************************
 * [Hierarchy 구문을 위한 자료 구조]
 *
 ***********************************************************************/

typedef struct qmsHierarchy
{
    qtcNode         * startWith;     // START WITH 구문
    qtcNode         * connectBy;     // CONNECT BY 구문
    qmsSortColumns  * siblings;      // order siblings by
    UInt              flag;

    UShort            originalTable;
    UShort            priorTable;
} qmsHierarchy;

typedef struct qmsConnectBy
{
    idBool    noCycle;
    qtcNode * expression;
} qmsConnectBy;

/***********************************************************************
 * [Analytic Function을 위한 자료 구조]
 *
 ***********************************************************************/

typedef struct qmsAnalyticFunc
{
    qtcNode         * analyticFuncNode;
    qmsAnalyticFunc * next;
} qmsAnalyticFunc;

/***********************************************************************
 * [Aggregation을 위한 자료 구조]
 *
 ***********************************************************************/

typedef struct qmsAggNode
{
    qtcNode     * aggr;

    qmsAggNode  * next;
} qmsAggNode;

#define QMS_SFWGH_HAVE_AGGREGATE(_SFWGH_)                       \
    ( (_SFWGH_->aggsDepth1 != NULL) ? ID_TRUE : ID_FALSE )

/***********************************************************************
 * [SELECT Target을 위한 자료 구조]
 *
 ***********************************************************************/

// The following structure is needed for "SELECT * "
typedef struct qmsNamePosition
{
    SChar       * name;
    SInt          size;
} qmsNamePosition;

typedef struct qmsTarget
{
    qtcNode          * targetColumn;    // set in parser or validation

    qmsNamePosition    userName;
    qmsNamePosition    tableName;
    qmsNamePosition    aliasTableName;
    qmsNamePosition    columnName;
    qmsNamePosition    aliasColumnName; // set in parser or validation
    qmsNamePosition    displayName;

    UInt               flag; // isNullable, isUpdatable

    qmsTarget        * next;
} qmsTarget;

#define QMS_TARGET_INIT(aTarget) \
    (aTarget)->targetColumn         = NULL;              \
    (aTarget)->userName.name        = NULL;              \
    (aTarget)->userName.size        = QC_POS_EMPTY_SIZE; \
    (aTarget)->tableName.name       = NULL;              \
    (aTarget)->tableName.size       = QC_POS_EMPTY_SIZE; \
    (aTarget)->aliasTableName.name  = NULL;              \
    (aTarget)->aliasTableName.size  = QC_POS_EMPTY_SIZE; \
    (aTarget)->columnName.name      = NULL;              \
    (aTarget)->columnName.size      = QC_POS_EMPTY_SIZE; \
    (aTarget)->aliasColumnName.name = NULL;              \
    (aTarget)->aliasColumnName.size = QC_POS_EMPTY_SIZE; \
    (aTarget)->displayName.name     = NULL;              \
    (aTarget)->displayName.size     = QC_POS_EMPTY_SIZE; \
    (aTarget)->flag                 = 0;                 \
    (aTarget)->next                 = NULL;

// PROJ-1584 DML returning clause
typedef struct qmsInto
{
    qtcNode          * intoNodes;
    qcNamePosition     intoPosition;
    idBool             bulkCollect;
} qmsInto;

// To fix BUG-17642
// CASE ... WHEN 구문을 처리하기 위한 구조체
typedef struct qmsCaseExpr {
    struct qmsWhenThenList * whenThenList;
    qtcNode                * elseNode;
} qmsCaseExpr;

typedef struct qmsWhenThenList {
    qtcNode         * whenNode;
    qtcNode         * thenNode;
    qmsWhenThenList * next;
} qmsWhenThenList;

/***********************************************************************
 * [FROM 구문을 위한 자료 구조]
 *
 ***********************************************************************/

typedef struct qmsFrom
{
    qmsJoinType     joinType;

    qcNamePosition  fromPosition;

    qmsTableRef   * tableRef;     // one table information
    qtcNode       * onCondition;  // join condition
    // from절이 on condtion join인 경우에 on condition join을 구성하는
    // table들의 dependencies의 값을 설정.
    qcDepInfo       depInfo;

    // PROJ-1718 Subquery unnesting
    // Semi/anti join시 outer/inner table을 모두 포함하는 값을 설정한다.
    qcDepInfo       semiAntiJoinDepInfo;

    qmsFrom       * next;

    qmsFrom       * left;
    qmsFrom       * right;
} qmsFrom;

//----------------------------------
// PROJ-1618 Online Dump
// Dump Object List
// ex) D$DISK_INDEX_BTREE( obj_name )
//----------------------------------

typedef struct qmsDumpObjList
{
    qcNamePosition            mUserNamePos;
    qcNamePosition            mDumpObjPos;
    void                    * mObjInfo;
    struct qmsPartitionRef  * mDumpPartitionRef;
    qmsDumpObjList          * mNext;
} qmsDumpObjList;

#define QCP_SET_INIT_QMS_DUMP_OBJECT(_dst_)             \
    {                                                   \
        SET_EMPTY_POSITION((_dst_)->mUserNamePos);      \
        SET_EMPTY_POSITION((_dst_)->mDumpObjPos);       \
        (_dst_)->mObjInfo           = NULL;             \
        (_dst_)->mDumpPartitionRef  = NULL;             \
        (_dst_)->mNext              = NULL;             \
    }

//----------------------------------
// PROJ-1413 Simple View Merging
// View Column Reference List
//----------------------------------

typedef struct qmsColumnRefList
{
    qtcNode             * column;
    qtcNode             * orgColumn;
    idBool                isMerged;
    
    // PROJ-2469 Optimize View Materialization
    idBool                isUsed;          // Default:TRUE 로 생성되며, Optimization 시에 사용 
    idBool                usedInTarget;    // Target Validation 때 등록된 Column인지 여부
    UShort                targetOrder;     // 해당 Node가 Target으로부터 참조 될 때, Target의 순번
    UShort                viewTargetOrder; // 해당 Node가 가리키는 View의 Target 순번
    
    qmsColumnRefList    * next;
} qmsColumnRefList;

typedef struct qmsPivotAggr
{
    qcNamePosition   aliasName;
    qcNamePosition   aggrFunc;
    qtcNode        * node;
    qmsPivotAggr   * next;
} qmsPivotAggr;

typedef struct qmsPivot
{
    qmsPivotAggr   * aggrNodes;
    qtcNode        * columnNode;
    qtcNode        * valueNode;
    qcNamePosition   position;   // for parsing
} qmsPivot;

/*
 * PROJ-2523 Unpviot Clause
 */

typedef struct qmsUnpivotColName
{
    qcNamePosition      colName;
    qmsUnpivotColName * next;

} qmsUnpivotCol;

typedef struct qmsUnpivotInNode
{
    qtcNode          * inNode;
    qmsUnpivotInNode * next;    
} qmsUnpivotInNode;

typedef struct qmsUnpivotInColInfo
{
    qmsUnpivotInNode    * inColumn;
    qmsUnpivotInNode    * inAlias;
    qmsUnpivotInColInfo * next;

} qmsUnpivotInColInfo;

typedef struct qmsUnpivot
{
    idBool                isIncludeNulls;
    qmsUnpivotColName   * valueColName;
    qmsUnpivotColName   * measureColName;
    qmsUnpivotInColInfo * inColInfo;
    qcNamePosition        position;   // for parsing

} qmsUnpviot;

typedef struct qmsPivotOrUnpivot
{
    qmsPivot   * pivot;
    qmsUnpivot * unpivot;

} qmsPivotOrUnPivot;

/*
 * PROJ-1832 New database link
 */

typedef struct qmsRemoteTable
{
    idBool         mIsStore;    /* BUG-37077 REMOTE_TABLE_STORE */
    qcNamePosition tableName;
    qcNamePosition linkName;
    qcNamePosition remoteQuery;

} qmsRemoteTable;

#define QCP_SET_INIT_QMS_REMOTE_TABLE(_dst_)           \
    {                                                  \
        (_dst_)->mIsStore = ID_FALSE;                  \
        SET_EMPTY_POSITION( (_dst_)->tableName );      \
        SET_EMPTY_POSITION( (_dst_)->linkName );       \
        SET_EMPTY_POSITION( (_dst_)->remoteQuery );    \
    }

/* PROJ-2464 hybrid partitioned table 지원 */
typedef struct qmsPartitionSummary
{
    UInt                     memoryPartitionCount;
    UInt                     volatilePartitionCount;
    UInt                     diskPartitionCount;
    idBool                   isHybridPartitionedTable;

    struct qmsPartitionRef * diskPartitionRef;
    struct qmsPartitionRef * memoryOrVolatilePartitionRef;
} qmsPartitionSummary;

// one table information in FROM clause
typedef struct qmsTableRef
{
    //---------------------------
    // parsing information
    //---------------------------

    qcNamePosition        position;  // for dml_table_reference
    qcNamePosition        userName;
    qcNamePosition        tableName;
    qcNamePosition        aliasName;

    // PROJ-2188 Pivot
    qmsPivot            * pivot;
    // PROJ-2523 Unpivot clause
    qmsUnpivot          * unpivot;

    // PROJ-1618 Online Dump
    qmsDumpObjList      * mDumpObjList;

    qcStatement         * view;          // created (or inline) view
    qcStatement         * recursiveView;     // created (or inline) recursive view
    qcStatement         * tempRecursiveView; // 재귀호출을 위해 임시로 사용하는 recursive view
    
    qmsTableRef         * sameViewRef;   // 상위 Query에 존재하는 동일한 View
    qcWithStmt          * withStmt;      // with절에 검색된 withStmt

    UInt                  flag;

    /* PROJ-1832 New database link */
    qmsRemoteTable      * remoteTable;

    // BUG-41311 table function
    qtcNode             * funcNode;
    
    //---------------------------
    // validation information
    //---------------------------

    UInt                  userID;
    UShort                table; // position of this table from the tuple set.

    // The member of qcmTableInfo is READ-ONLY.
    // but, if a table is view, then tableInfo is maded.
    qcmTableInfo        * tableInfo;
    smSCN                 tableSCN;

    // PROJ-1350 Plan Tree 자동 재구성
    // Execution 시점에 tableInfo내용을 참조할 수 없다.
    // 따라서, Execution 시점에 tableHandle을 접근하기 위하여
    // 이곳에 table handle 정보를 저장해 둔다.
    void                * tableHandle;
    // BUG-29598
    // table type을 tableRef에 기록해둔다.
    qcmTableType          tableType;
    UInt                  tableFlag;
    smOID                 tableOID;

    /* PROJ-1832 New database link */
    SChar               * remoteQuery;
    
    //---------------------------
    // transformation information
    //---------------------------

    // PROJ-1413 Simple View Merging
    qmsColumnRefList    * viewColumnRefList; // view 컬럼 참조 노드를 기록
    idBool                isMerged;          // merge된 view임을 표시
    idBool                isNewAliasName;    // merge에 의해 생성된 alias임을 표시
    
    /* PROJ-1090 Function-based Index */
    struct qmsExprNode  * defaultExprList;
    
    //---------------------------
    // optimization hint
    //---------------------------

    qmsTableAccessHints * tableAccessHints;
    qmoViewOptType        viewOptType;      // VMTR or PUSH
    idBool                noMergeHint;

    //---------------------------
    // 통계 정보
    // - 일반 Table인 경우, Vaildation과정
    // - View의 경우, Validation 과정 NULL
    //                Optimization 과정 중에 정보 설정
    //---------------------------

    struct qmoStatistics   * statInfo;  // Table의 통계 정보

    //---------------------------
    // PROJ-1502 PARTITIONED DISK TABLE
    //---------------------------
    
    struct qmsPartitionRef * partitionRef;
    UInt                     partitionCount;
    UInt                     selectedPartitionID;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qmsPartitionSummary    * partitionSummary;

    // BUG-47599
    struct qmsPartitionRef * mEmptyPartRef;

    //---------------------------
    // PROJ-1624 global non-partitioned index
    //---------------------------

    struct qmsIndexTableRef * indexTableRef;
    UInt                      indexTableCount;
    // index table scan시 선택된 index table
    struct qmsIndexTableRef * selectedIndexTable;
    
    /* BUG-31570
     * DDL이 빈번한 환경에서 plan text를 안전하게 보여주는 방법이 필요하다.
     */
    SChar              (* columnsName)[QC_MAX_OBJECT_NAME_LEN + 1];

    /* PROJ-1071 Parallel Query */
    UInt                      mParallelDegree;

    //---------------------------
    // PROJ-2646 shard analyzer
    //---------------------------

    struct sdiObjectInfo    * mShardObjInfo;

} qmsTableRef;


// PROJ-1502 PARTITIONED DISK TABLE
typedef enum qmsPartCondValType
{
    QMS_PARTCONDVAL_NORMAL  = 0,
    QMS_PARTCONDVAL_MIN     = 1,
    QMS_PARTCONDVAL_DEFAULT = 2
} qmsPartCondValType;

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qmsPartCondValList
{
    UInt               partCondValCount;
    qmsPartCondValType partCondValType;
    void             * partCondValues[QC_MAX_PARTKEY_COND_COUNT];
} qmsPartCondValList;

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qmsPartitionRef
{
    qcNamePosition             position;  // for parsing (dml_table_reference)
    qcNamePosition             partitionName;
    UInt                       partitionID;
    smOID                      partitionOID;
    UShort                     table; // position of this table from the tuple set.
    qcmTableInfo             * partitionInfo;
    smSCN                      partitionSCN;
    void                     * partitionHandle;
    struct qmsPartCondValList  minPartCondVal;
    struct qmsPartCondValList  maxPartCondVal;
    UInt                       partOrder;
    struct qmoStatistics     * statInfo;
    struct qmsPartitionRef   * next;
} qmsPartitionRef;

// PROJ-1624 global non-partitioned index
typedef struct qmsIndexTableRef
{
    qcmIndex                 * index;
    UInt                       tableID;
    UShort                     table; // position of this table from the tuple set.
    qcmTableInfo             * tableInfo;
    smSCN                      tableSCN;
    void                     * tableHandle;
    struct qmoStatistics     * statInfo;
    struct qmsIndexTableRef  * next;
} qmsIndexTableRef;

// PROJ-2204 Join Update, Delete
// composite unique index를 처리하기 위한 자료구조
// (t1[i1,i2]->t2) -> (t1[i1,i2]->t3) -> null
typedef struct qmsUniqueInfo
{
    UInt                  * fromColumn;       // column id array
    UShort                  fromColumnCount;  // array size
    UShort                  toTable;
    struct qmsUniqueInfo  * next;
} qmsUniqueInfo;
    
// PROJ-2204 Join Update, Delete
// key preserved table을 계산하기 위한 자료구조
typedef struct qmsPreservedInfo
{
    UInt             tableCount;
    qmsTableRef    * tableRef[QC_MAX_REF_TABLE_CNT];
    qmsUniqueInfo  * uniqueInfo[QC_MAX_REF_TABLE_CNT];
    UShort         * tableMap[QC_MAX_REF_TABLE_CNT];
    idBool           result[QC_MAX_REF_TABLE_CNT];
    idBool           isKeyPreserved[QC_MAX_REF_TABLE_CNT]; // BUG-39399
    idBool           stopFlag;
    idBool           useKeyPreservedTable;
    idBool           mIsInValid; /* BUG-46124 */
} qmsPreservedInfo;

/***********************************************************************
 * [기타 정보를 위한 자료 구조]
 *
 ***********************************************************************/

// Validation용도로 사용하며,
// Optimizer에서는 Type J의 판단이 가능함.

typedef struct qmsOuterNode
{
    qtcNode         * column;
    qmsOuterNode    * next;

    //--------------------------------
    // PROJ-2448 Scalar subquery caching
    // outer column 의 materialize node 의 position 획득
    //--------------------------------
    UShort            cacheTable;
    UShort            cacheColumn;
} qmsOuterNode;

/***********************************************************************
// 기본 SELECT 구문을 위한 자료 구조
// SELECT ...
// FROM ...
// WHERE ...
// [START WITH] ...
// [CONNECT BY] ...
// GROUP BY ...
// HAVING ...
***********************************************************************/

typedef struct qmsSFWGH
{
    //--------------------------------
    // SELECT 구문을 위한 자료 구조
    //--------------------------------

    qmsSelectType         selectType;       // ALL, DISTINCT
    struct qmsHints     * hints;            // Hint 정보
    struct qmsTarget    * target;           // Select Target List
    struct qmsInto      * intoVariables;    // for stored procedure

    struct qmsLimit     * top;              /* BUG-36580 supported TOP */
    
    //--------------------------------
    // FROM 구문을 위한 자료 구조
    //--------------------------------

    struct qmsFrom      * from;
    // BUG-34295 Join ordering ANSI style query
    // ANSI style 로 기술된 left outer join 의 qmsFrom 을 담는다.
    struct qmsFrom      * outerJoinTree;
    // FROM 절에 있는 모든 Table만의 Dependency의 합
    qcDepInfo             depInfo;

    //--------------------------------
    // WHERE 구문을 위한 자료 구조
    //--------------------------------

    qtcNode             * where;

    //--------------------------------
    // Hierarchy ([START WITH] [CONNECT BY]) 를 위한 자료 구조
    //--------------------------------

    struct qmsHierarchy * hierarchy;

    //--------------------------------
    // GROUP BY 를 위한 자료 구조
    //--------------------------------
    qmsConcatElement    * group;

    //--------------------------------
    // HAVING을 위한 자료 구조
    //--------------------------------
    qtcNode             * having;

    //--------------------------------
    // Aggregation 정보
    //--------------------------------

    struct qmsAggNode   * aggsDepth1;  // 일반 Aggregation
    struct qmsAggNode   * aggsDepth2;  // Nested Aggregation

    //--------------------------------
    // 기타 정보
    //--------------------------------

    // SELECT 구문의 시작 위치 (for SP error message)
    qcNamePosition        startPos;
    qcNamePosition        endPos;

    // member for outer column or aggregate function reference
    qmsSFWGH            * outerQuery;   // outer query
    struct qmsFrom      * outerFrom;    // BUG-26134 outer parent from
    struct qmsOuterNode * outerColumns; // outer column list
    qcDepInfo             outerDepInfo; // PROJ-1413 outer column dependency
    qmsProcessPhase       validatePhase; // for detecting current parent's validation phase
    idBool                outerHavingCase;

    /* TASK-7219 Shard Transformer Refactoring */
    ULong                 lflag;

    // for LEVEL pseudo column
    qtcNode             * level;

    /* PROJ-1715 Hierarchy Pseudo Column */
    qtcNode             * isLeaf;
    qtcNode             * cnbyStackAddr;

    // for LOOP_LEVEL pseudo column
    qtcNode             * loopLevel;

    // for ROWNUM pseudo column
    qtcNode             * rownum;

    /* PROJ-1353 Rollup, Cube Grouping Set Pseudo Column */
    qtcNode             * groupingInfoAddr;

    // PROJ-1413
    // 하위 SFWGH가 현재 SFWGH로 merge된 후
    // 하위 SFWGH를 기록한 outerColumns에서
    // 현재 SFWGH를 찾을 수 있도록 현재 SFWGH를 기록한다.
    qmsSFWGH            * mergedSFWGH;

    // PROJ-1413
    // 현재 SFWGH에 merge가 수행되었는지 여부
    // 모든 SFWGH에 노드변화가 발생되었음을 나타낸다.
    idBool                isTransformed;

    // PROJ-2204 Join Update, Delete
    qmsPreservedInfo    * preservedInfo;

    //------------------------------------
    // Critical Path : optimizer 에서 사용
    //------------------------------------

    struct qmoCrtPath   * crtPath;

    // PROJ-2469 Optimize View Materialization
    // Target Validation시에 몇 번 째 Target을 Validation하는 중인지 기록한다.
    UShort                currentTargetNum;

    // PROJ-2582 recursive with
    // SFWGH에 bottom recursive view가 있다면, reference id를 기록한다.
    UShort                recursiveViewID;

    qmsQuerySet         * thisQuerySet;

} qmsSFWGH;

/***********************************************************************
 * [WITH 구문을 위한 자료 구조]
 * WITH query_name1 AS ( query_block1 ),
 *      query_name2 AS ( query_block2 )
 * SELECT * FROM query_name1, query_name2;
 ***********************************************************************/

typedef struct qmsWithClause
{
    qcNamePosition   startPos;
    qcNamePosition   stmtName;
    qcNamePosition   stmtText;
    
    qcmColumn      * columns;  // column alias
    qcStatement    * stmt;
    
    qmsWithClause  * next;
} qmsWithClause;


#endif /* _O_QMS_PARSE_TREE_H_ */
