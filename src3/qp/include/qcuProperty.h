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
 

/*****************************************************************************
 * $Id: qcuProperty.h 90787 2021-05-07 00:50:48Z ahra.cho $
 *
 * QP에서 사용하는 System Property에 대한 정의
 * A4에서 제공하는 Property 관리자를 이용하여 처리한다.
 *
 * 주의 : Readable Property만을 관리한다.
 *        Writable Property의 경우, MM의 Session에서 관리되어야 함.
 *        TraceLog의 경우, 개념적으로 RunTime property의 의미임
 *
 **********************************************************************/

#ifndef _O_QCU_PROPERTY_H_
#define _O_QCU_PROPERTY_H_ 1

#include <idl.h>
#include <idp.h>
#include <smi.h>
#include <qcs.h>
#include <qtc.h>

/* BUG-40867 QP SharedProperty */
#define QCU_PROPERTY( aProperty ) (qcuProperty::mStaticProperty.aProperty)

#define DEFAULT__STORED_PROC_MODE 0
#define QCU_SPM_NORM 0
#define QCU_SPM_MASK_DISABLE  (1<<0)
#define QCU_SPM_MASK_VERVOSE1 (1<<1)
#define QCU_SPM_MASK_VERVOSE2 (1<<2)
#define QCU_SPM_MASK_VERVOSE3 (1<<3)

/* BUG-48336 __optimizer_unnest_compatibility mode mask */
#define QCU_UNNEST_COMPATIBILITY_MASK_MODE1 (1)
#define QCU_UNNEST_COMPATIBILITY_MASK_MODE2 (2)

/* BUG-48941 __OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE */
#define QCU_OBYE_MASK_MODE1 (1)
#define QCU_OBYE_MASK_MODE2 (2)

#define QCU_TRCLOG_DML_SENTENCE        ( QCU_PROPERTY(mTraceLog_DML_Sentence) )
#define QCU_TRCLOG_DETAIL_PREDICATE    ( QCU_PROPERTY(mTraceLog_Detail_Predicate) )
#define QCU_TRCLOG_DETAIL_MTRNODE      ( QCU_PROPERTY(mTraceLog_Detail_MtrNode) )
#define QCU_TRCLOG_EXPLAIN_GRAPH       ( QCU_PROPERTY(mTraceLog_Explain_Graph) )
#define QCU_TRCLOG_RESULT_DESC         ( QCU_PROPERTY(mTraceLog_Result_Desc) )
#define QCU_TRCLOG_DISPLAY_CHILDREN    ( QCU_PROPERTY(mTraceLogDisplayChildren) )
#define QCU_TRCLOG_DETAIL_RESULTCACHE  ( QCU_PROPERTY(mTraceLog_Detail_ResultCache) )

#define QCU_NORMAL_FORM_MAXIMUM     ( QCU_PROPERTY(mNormalFormMaximum) )
#define QCU_EXEC_DDL_DISABLE        ( QCU_PROPERTY(mExecDDLDisable) )

#define QCM_ISOLATION_LEVEL         ( QCU_PROPERTY(mIsolationLevel) )

#define QCU_UPDATE_IN_PLACE         ( QCU_PROPERTY(mUpdateInPlace) )

#define QCU_STORED_PROC_MODE        ( QCU_PROPERTY(mSpMode) )

// BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
// 관련프로퍼티를 참조하지 못하는 경우 있음.
#define QCU_OPTIMIZER_MODE             ( QCU_PROPERTY(mOptimizerMode) )

#define QCU_AUTO_REMOTE_EXEC           ( QCU_PROPERTY(mAutoRemoteExec) )

// PROJ-1358 Stack Size의 조정
#define QCU_QUERY_STACK_SIZE        ( QCU_PROPERTY(mQueryStackSize) )

// PR-13395
// 가상 통계 정보를 위한 Scale Factor
#define QCU_FAKE_TPCH_SCALE_FACTOR  ( QCU_PROPERTY(mFakeTpchScaleFactor) )
#define QCU_FAKE_BUFFER_SIZE        ( QCU_PROPERTY(mFakeBufferSize) )

// PROJ-1446 Host variable을 포함한 질의 최적화
#define QCU_HOST_OPTIMIZE_ENABLE    ( QCU_PROPERTY(mHostOptimizeEnable) )

// BUG-13068
// session당 open할 수 있는 filehandle개수 제한
#define QCU_PSM_FILE_OPEN_LIMIT      ( QCU_PROPERTY(mFileOpenLimit) )

// BUG-40854
#define QCU_CONNECT_TYPE_OPEN_LIMIT ( QCU_PROPERTY(mSocketOpenLimit) )

/* BUG-41307 User Lock 지원 */
#define QCU_USER_LOCK_POOL_INIT_SIZE         ( QCU_PROPERTY(mUserLockPoolInitSize) )
#define QCU_USER_LOCK_REQUEST_TIMEOUT        ( QCU_PROPERTY(mUserLockRequestTimeout) )
#define QCU_USER_LOCK_REQUEST_CHECK_INTERVAL ( QCU_PROPERTY(mUserLockRequestCheckInterval) )
#define QCU_USER_LOCK_REQUEST_LIMIT          ( QCU_PROPERTY(mUserLockRequestLimit) )

// PR-14325
#define QCU_REPLICATION_UPDATE_PK   ( QCU_PROPERTY(mReplUpdatePK) )

// PROJ-1557
#define QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE  ( QCU_PROPERTY(mMemVarColumnInRowSize) )

// PROJ-1362
#define QCU_MEMORY_LOB_COLUMN_IN_ROW_SIZE  ( QCU_PROPERTY(mMemLobColumnInRowSize) )

// PROJ-1862 Disk Lob Column In Row Size
#define QCU_DISK_LOB_COLUMN_IN_ROW_SIZE  ( QCU_PROPERTY(mDiskLobColumnInRowSize) )

// BUG-18851 disable transitive predicate generation
#define QCU_OPTIMIZER_TRANSITIVITY_DISABLE  ( QCU_PROPERTY(mOptimizerTransitivityDisable) )

/* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
#define QCU_OPTIMIZER_TRANSITIVITY_OLD_RULE ( QCU_PROPERTY(mOptimizerTransitivityOldRule) )

// PROJ-1473
#define QCU_OPTIMIZER_PUSH_PROJECTION  ( QCU_PROPERTY(mOptimizerPushProjection) )

// BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
#define QCU_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE  ( QCU_PROPERTY(mOptimizerDefaultTempTbsType) )

// BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
#define QCU_OPTIMIZER_FIXED_GROUP_MEMORY_TEMP ( QCU_PROPERTY(mOptimizerFixedGroupMemoryTemp) )
// BUG-38339 Outer Join Elimination
#define QCU_OPTIMIZER_OUTERJOIN_ELIMINATION ( QCU_PROPERTY(mOptimizerOuterJoinElimination) )

// PROJ-1413 Simple View Merging
#define QCU_OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE  ( QCU_PROPERTY(mOptimizerSimpleViewMergingDisable) )

// PROJ-1718
#define QCU_OPTIMIZER_UNNEST_SUBQUERY ( QCU_PROPERTY(mOptimizerUnnestSubquery) )
#define QCU_OPTIMIZER_UNNEST_COMPLEX_SUBQUERY ( QCU_PROPERTY(mOptimizerUnnestComplexSubquery) )
#define QCU_OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY ( QCU_PROPERTY(mOptimizerUnnestAggregationSubquery) )

//PROJ-1583 large geometry
#define QCU_ST_OBJECT_BUFFER_SIZE   ( QCU_PROPERTY(mSTObjBufSize) )

// BUG-19089
// FK가 있는 상태에서 CREATE REPLICATION 구문이 가능하도록 한다.
#define QCU_CHECK_FK_IN_CREATE_REPLICATION_DISABLE ( QCU_PROPERTY(mCheckFkInCreateReplicationDisable) )

// PROJ-1436
#define QCU_SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT  ( QCU_PROPERTY(mSqlPlanCachePreparedExecutionContextCnt) )

// PROJ-2002 Column Security
#define QCU_SECURITY_MODULE_NAME      ( QCU_PROPERTY(mSecurityModuleName) )
#define QCU_SECURITY_MODULE_LIBRARY   ( QCU_PROPERTY(mSecurityModuleLibrary) )
#define QCU_SECURITY_ECC_POLICY_NAME  ( QCU_PROPERTY(mSecurityEccPolicyName) )

// BUG-29209
#define QCU_DISPLAY_PLAN_FOR_NATC ( QCU_PROPERTY(mDisplayPlanForNATC) )

// BUG-32101
#define QCU_OPTIMIZER_DISK_INDEX_COST_ADJ   ( QCU_PROPERTY(mOptimizerDiskIndexCostAdj) )

// BUG-43736
#define QCU_OPTIMIZER_MEMORY_INDEX_COST_ADJ   ( QCU_PROPERTY(mOptimizerMemoryIndexCostAdj) )

// BUG-34441
#define QCU_OPTIMIZER_HASH_JOIN_COST_ADJ    ( QCU_PROPERTY(mOptimizerHashJoinCostAdj) )

// BUG-34235
#define QCU_OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD ( QCU_PROPERTY(mOptimizerSubqueryOptimizeMethod) )

// fix BUG-33589
#define QCU_PLAN_REBUILD_ENABLE ( QCU_PROPERTY(mPlanRebuildEnable) )

/* PROJ-2361 */
#define QCU_AVERAGE_TRANSFORM_ENABLE ( QCU_PROPERTY(mAvgTransformEnable) )

/* PROJ-1071 Parallel Query */
#define QCU_PARALLEL_QUERY_THREAD_MAX      ( QCU_PROPERTY(mParallelQueryThreadMax) )
#define QCU_PARALLEL_QUERY_QUEUE_SLEEP_MAX (QCU_PROPERTY(mParallelQueryQueueSleepMax) )
#define QCU_PARALLEL_QUERY_QUEUE_SIZE      (QCU_PROPERTY(mParallelQueryQueueSize) )

/* PROJ-2207 Password policy support */
#define QCU_SYSDATE_FOR_NATC ( QCU_PROPERTY(mSysdateForNATC) )

#define QCU_FAILED_LOGIN_ATTEMPTS    ( QCU_PROPERTY(mFailedLoginAttempts) )
#define QCU_PASSWORD_LOCK_TIME       ( QCU_PROPERTY(mPasswLockTime) )
#define QCU_PASSWORD_LIFE_TIME       ( QCU_PROPERTY(mPasswLifeTime) )
#define QCU_PASSWORD_GRACE_TIME      ( QCU_PROPERTY(mPasswGraceTime) )
#define QCU_PASSWORD_REUSE_MAX       ( QCU_PROPERTY(mPasswReuseMax) )
#define QCU_PASSWORD_REUSE_TIME      ( QCU_PROPERTY(mPasswReuseTime) )
#define QCU_PASSWORD_VERIFY_FUNCTION ( QCU_PROPERTY(mPasswVerifyFunc) )

// BUG-36203
#define QCU_PSM_TEMPLATE_CACHE_COUNT ( QCU_PROPERTY(mPSMTemplateCacheCount) )

// BUG-34234
#define QCU_COERCE_HOST_VAR_TO_VARCHAR ( QCU_PROPERTY(mCoerceHostVarToVarchar) )

// BUG-34295
#define QCU_OPTIMIZER_ANSI_JOIN_ORDERING ( QCU_PROPERTY(mOptimizerAnsiJoinOrdering) )
// BUG-38402
#define QCU_OPTIMIZER_ANSI_INNER_JOIN_CONVERT ( QCU_PROPERTY(mOptimizerAnsiInnerJoinConvert) )

// BUG-34350
#define QCU_OPTIMIZER_REFINE_PREPARE_MEMORY   ( QCU_PROPERTY(mOptimizerRefinePrepareMemory) )

// BUG-35155
#define QCU_OPTIMIZER_PARTIAL_NORMALIZE ( QCU_PROPERTY(mOptimizerPartialNormalize) )

// PROJ-2264 Dictionary table
#define QCU_FORCE_COMPRESSION_COLUMN       ( QCU_PROPERTY(mForceCompressionColumn) )

// PROJ-1071 Parallel query
#define QCU_FORCE_PARALLEL_DEGREE   ( QCU_PROPERTY(mForceParallelDegree) )

/* PROJ-1090 Function-based Index */
#define QCU_QUERY_REWRITE_ENABLE ( QCU_PROPERTY(mQueryRewriteEnable) )

// BUG-35713
#define QCU_PSM_IGNORE_NO_DATA_FOUND_ERROR ( QCU_PROPERTY(mPSMIgnoreNoDataFoundError) )

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
#define QCU_INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT ( QCU_PROPERTY(mIntermediateTupleLobObjectLimit) )

/* PROJ-2242 CSE, CFS, Feature enable */
#define QCU_OPTIMIZER_FEATURE_VERSION_LEN   (25)
#define QCU_OPTIMIZER_FEATURE_ENABLE        ( QCU_PROPERTY(mOptimizerFeatureEnable) )
#define QCU_OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION ( QCU_PROPERTY(mOptimizerEliminateCommonSubexpression) )
#define QCU_OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION    ( QCU_PROPERTY(mOptimizerConstantFilterSubsumption) )

#define QCU_DML_WITHOUT_RETRY_ENABLE ( QCU_PROPERTY(mDmlWithoutRetryEnable) )

// fix BUG-36522
#define QCU_PSM_SHOW_ERROR_STACK ( QCU_PROPERTY(mPSMShowErrorStack) )

// fix BUG-36793
#define QCU_BIND_PARAM_DEFAULT_PRECISION ( QCU_PROPERTY(mBindParamDefaultPrecision) )

// BUG-37247
#define QCU_SYS_CONNECT_BY_PATH_PRECISION ( QCU_PROPERTY(mSysConnectByPathPrecision) )

// BUG-37252
#if defined(DEBUG)
#define QCU_EXECUTION_PLAN_MEMORY_CHECK ( (UInt)1 )
#else
#define QCU_EXECUTION_PLAN_MEMORY_CHECK ( QCU_PROPERTY(mExecutionPlanMemoryCheck) )
#endif

// BUG-37302
#define QCU_SQL_INFO_SIZE ( QCU_PROPERTY(mSQLErrorInfoSize) )

// PROJ-2362
#define QCU_REDUCE_TEMP_MEMORY_ENABLE ( QCU_PROPERTY(mReduceTempMemoryEnable) )

// BUG-38434
#define QCU_OPTIMIZER_DNF_DISABLE ( QCU_PROPERTY(mOptimizerDnfDisable) )

/* BUG-38952 type null */
#define QCU_TYPE_NULL ( QCU_PROPERTY(mTypeNull) )

/* BUG-38946 display name */
#define QCU_COMPAT_DISPLAY_NAME ( QCU_PROPERTY(mCompatDisplayName) )

// fix BUG-39754
#define QCU_FOREIGN_KEY_LOCK_ROW ( QCU_PROPERTY(mForeignKeyLockRow) )

// BUG-40022
#define QCU_OPTIMIZER_JOIN_DISABLE ( QCU_PROPERTY(mOptimizerJoinDisable) )

// BUG-40042 oracle outer join property
#define QCU_OUTER_JOIN_OPER_TO_ANSI_JOIN ( QCU_PROPERTY(mOuterJoinOperToAnsiJoinTransEnable) )

// PROJ-2469 Optimize View Materialization
#define QCU_OPTIMIZER_VIEW_TARGET_ENABLE ( QCU_PROPERTY(mOptimizerViewTargetEnable) )

/* PROJ-2451 Concurrent Execute Package */
#define QCU_CONC_EXEC_DEGREE_MAX     (QCU_PROPERTY(mConcExecDegreeMax) )
#define QCU_CONC_EXEC_DEGREE_DEFAULT (QCU_PROPERTY(mConcExecDegreeDefault) )
#define QCU_CONC_EXEC_WAIT_INTERVAL  (QCU_PROPERTY(mConcExecWaitInterval) )

/* PROJ-2452 Caching for DETERMINISTIC Function */
#define QCU_QUERY_EXECUTION_CACHE_MAX_COUNT    ( QCU_PROPERTY(mQueryExecutionCacheMaxCount) )
#define QCU_QUERY_EXECUTION_CACHE_MAX_SIZE     ( QCU_PROPERTY(mQueryExecutionCacheMaxSize) )
#define QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT ( QCU_PROPERTY(mQueryExecutionCacheBucketCount) )
#define QCU_FORCE_FUNCTION_CACHE               ( QCU_PROPERTY(mForceFunctionCache) )

/* PROJ-2441 flashback */
#define QCU_RECYCLEBIN_MEM_MAX_SIZE     ( QCU_PROPERTY(mRecyclebinMemMaxSize) )
#define QCU_RECYCLEBIN_DISK_MAX_SIZE    ( QCU_PROPERTY(mRecyclebinDiskMaxSize) )
#define QCU_RECYCLEBIN_ENABLE           ( QCU_PROPERTY(mRecyclebinEnable) )
#define QCU_RECYCLEBIN_FOR_NATC         ( QCU_PROPERTY(mRecyclebinNATC) )

#define QCU_RECYCLEBIN_ON         (1)
#define QCU_RECYCLEBIN_OFF        (0)

/* PROJ-2448 Subquery caching */
#define QCU_FORCE_SUBQUERY_CACHE_DISABLE          ( QCU_PROPERTY(mForceSubqueryCacheDisable) )

// PROJ-2551 simple query 최적화
#define QCU_EXECUTOR_FAST_SIMPLE_QUERY  ( QCU_PROPERTY(mExecutorFastSimpleQuery) )

/* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
#define QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE   ( QCU_PROPERTY(mHashJoinMemTempPartitioningDisable) )
#define QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE ( QCU_PROPERTY(mHashJoinMemTempAutoBucketCntDisable) )
#define QCU_HSJN_MEM_TEMP_TLB_COUNT              ( QCU_PROPERTY(mHashJoinMemTempTLBCnt) )
#define QCU_FORCE_HSJN_MEM_TEMP_FANOUT_MODE      ( QCU_PROPERTY(mForceHashJoinMemTempFanoutMode) )

// BUG-41183 ORDER BY Elimination
#define QCU_OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE ( QCU_PROPERTY(mOptimizerOrderByEliminationEnable) )

// BUG-41398 use old sort
#define QCU_USE_OLD_SORT                         ( QCU_PROPERTY(mUseOldSort) )

#define QCU_DDL_SUPPLEMENTAL_LOG                 ( QCU_PROPERTY(mDDLSupplementalLog) )

// BUG-41249 DISTINCT Elimination
#define QCU_OPTIMIZER_DISTINCT_ELIMINATION_ENABLE ( QCU_PROPERTY(mOptimizerDistinctEliminationEnable) )

/* PROJ-2462 Result Cache */
#define QCU_RESULT_CACHE_ENABLE         ( QCU_PROPERTY( mResultCacheEnable ) )
#define QCU_TOP_RESULT_CACHE_MODE       ( QCU_PROPERTY( mTopResultCacheMode ) )
#define QCU_RESULT_CACHE_MEMORY_MAXIMUM ( QCU_PROPERTY( mResultCacheMemoryMax ) )

// BUG-36438 LIST transformation
#define QCU_OPTIMIZER_LIST_TRANSFORMATION ( QCU_PROPERTY(mOptimizerListTransformation) )

/* PROJ-2492 Dynamic sample selection */
#define QCU_OPTIMIZER_AUTO_STATS          ( QCU_PROPERTY( mOptimizerAutoStats ) )

// BUG-41248 DBMS_SQL package
#define QCU_PSM_CURSOR_OPEN_LIMIT         ( QCU_PROPERTY(mPSMCursorOpenLimit ) )

// PROJ-2582 recursive with
#define QCU_RECURSION_LEVEL_MAXIMUM       ( QCU_PROPERTY( mRecursionLevelMaximum ) )

// fix BUG-42752
#define QCU_OPTIMIZER_ESTIMATE_KEY_FILTER_SELECTIVITY ( QCU_PROPERTY( mOptimizerEstimateKeyFilterSelectivity ) )

// BUG-43059 Target subquery unnest/removal disable
#define QCU_OPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE  ( QCU_PROPERTY( mOptimizerTargetSubqueryUnnestDisable ) )
#define QCU_OPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE ( QCU_PROPERTY( mOptimizerTargetSubqueryRemovalDisable ) )

/* PROJ-2465 Tablespace Alteration for Table */
#define QCU_DDL_MEM_USAGE_THRESHOLD    ( QCU_PROPERTY( mDDLMemUsageThreshold ) )
#define QCU_DDL_TBS_USAGE_THRESHOLD    ( QCU_PROPERTY( mDDLTBSUsageThreshold ) )
#define QCU_ANALYZE_USAGE_MIN_ROWCOUNT ( QCU_PROPERTY( mAnalyzeUsageMinRowCount ) )
#define QCU_MEM_MAX_DB_SIZE            ( QCU_PROPERTY( mMemMaxDBSize ) )

/* PROJ-2586 PSM Parameters and return without precision */
#define QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE  ( QCU_PROPERTY( mPSMParamAndReturnWithoutPrecisionEnable ) )
#define QCU_PSM_CHAR_DEFAULT_PRECISION                     ( QCU_PROPERTY( mPSMCharDefaultPrecision ) )
#define QCU_PSM_VARCHAR_DEFAULT_PRECISION                  ( QCU_PROPERTY( mPSMVarcharDefaultPrecision ) )
#define QCU_PSM_NCHAR_UTF16_DEFAULT_PRECISION              ( QCU_PROPERTY( mPSMNcharUTF16DefaultPrecision ) )
#define QCU_PSM_NVARCHAR_UTF16_DEFAULT_PRECISION           ( QCU_PROPERTY( mPSMNvarcharUTF16DefaultPrecision ) )
#define QCU_PSM_NCHAR_UTF8_DEFAULT_PRECISION               ( QCU_PROPERTY( mPSMNcharUTF8DefaultPrecision ) )
#define QCU_PSM_NVARCHAR_UTF8_DEFAULT_PRECISION            ( QCU_PROPERTY( mPSMNvarcharUTF8DefaultPrecision ) )

/* BUG-42639 Monitoring query */
#define QCU_OPTIMIZER_PERFORMANCE_VIEW ( QCU_PROPERTY( mOptimizerPerformanceView ) )

// BUG-42322
#define QCU_PSM_FORMAT_CALL_STACK_OID ( QCU_PROPERTY(mPSMFormatCallStackOID) )

// BUG-43039 inner join push down
#define QCU_OPTIMIZER_INNER_JOIN_PUSH_DOWN                  ( QCU_PROPERTY( mOptimizerInnerJoinPushDown ) )

/* BUG-42928 No Partition Lock */
#define QCU_TABLE_LOCK_MODE     ( QCU_PROPERTY(mTableLockMode) )

// BUG-43068 Indexable order by 개선
#define QCU_OPTIMIZER_ORDER_PUSH_DOWN                       ( QCU_PROPERTY( mOptimizerOrderPushDown ) )

/* BUG-43112 */
#define QCU_FORCE_AUTONOMOUS_TRANSACTION_PRAGMA ( QCU_PROPERTY( mForceAutonomousTransactionPragma ) )
#define QCU_AUTONOMOUS_TRANSACTION_PRAGMA_DISABLE ( QCU_PROPERTY( mAutonomousTransactionPragmaDisable ) )

// BUG-43258
#define QCU_OPTIMIZER_INDEX_CLUSTERING_FACTOR_ADJ ( QCU_PROPERTY( mOptimizerIndexClusteringFactorAdj ) )

// BUG-43158 Enhance statement list caching in PSM
#define QCU_PSM_STMT_LIST_COUNT                             ( QCU_PROPERTY( mPSMStmtListCount ) )
#define QCU_PSM_STMT_POOL_COUNT                             ( QCU_PROPERTY( mPSMStmtPoolCount ) )

// BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
#define QCU_DISK_TEMP_SIZE_ESTIMATE                         ( QCU_PROPERTY( mDiskTempSizeEstimate ) )

// BUG-43421
#define QCU_OPTIMIZER_SEMI_JOIN_TRANSITIVITY_DISABLE  (QCU_PROPERTY(mOptimizerSemiJoinTransitivityDisable) )

// BUG-43493
#define QCU_OPTIMIZER_DELAYED_EXECUTION ( QCU_PROPERTY(mOptimizerDelayedExecution) )

/* BUG-43495 */
#define QCU_OPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE   ( QCU_PROPERTY(mOptimizerLikeIndexScanWithOuterColumnDisable) )

/* BUG-43737 */
#define QCU_FORCE_TABLESPACE_DEFAULT ( QCU_PROPERTY(mForceTablespaceDefault) )

/* BUG-43769 */
#define QCU_IS_CPU_AFFINITY ( QCU_PROPERTY(mIsCPUAffinity) )

/* TASK-6744 */
#define QCU_PLAN_RANDOM_SEED ( QCU_PROPERTY(mPlanRandomSeed) )

/* BUG-44499 */
#define QCU_OPTIMIZER_BUCKET_COUNT_MIN ( QCU_PROPERTY( mOptimizerBucketCountMin ) )

/* BUG-48162 */
#define QCU_OPTIMIZER_BUCKET_COUNT_MAX ( QCU_PROPERTY( mOptimizerBucketCountMax ) )

/* PROJ-2641 Hierarchy Query Index */
#define QCU_OPTIMIZER_HIERARCHY_TRANSFORMATION ( QCU_PROPERTY(mOptimizerHierarchyTransformation) )

// BUG-44692
#define QCU_OPTIMIZER_BUG_44692 ( QCU_PROPERTY(mOptimizerBug44692) )

// BUG-44795
#define QCU_OPTIMIZER_DBMS_STAT_POLICY ( QCU_PROPERTY(mOptimizerDBMSStatPolicy) )

/* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
#define QCU_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY ( QCU_PROPERTY( mOptimizerIndexNLJoinAccessMethodPolicy ) )

#define QCU_OPTIMIZER_SEMI_JOIN_REMOVE ( QCU_PROPERTY( mOptimizerSemiJoinRemove ) )

#define QCU_PRINT_OUT_ENABLE           ( QCU_PROPERTY(mPrintOutEnable) )

#define QCU_KEY_PRESERVED_TABLE        ( QCU_PROPERTY(mKeyPreservedTable) )

/* BUG-46544 Unnest hit */
#define QCU_OPTIMIZER_UNNEST_COMPATIBILITY ( QCU_PROPERTY(mOptimizerUnnestCompatibility) )

/* PROJ-2632 */
#define QCU_SERIAL_EXECUTE_MODE ( QCU_PROPERTY( mSerialExecuteMode ) )
#define QCU_TRCLOG_DETAIL_INFORMATION ( QCU_PROPERTY( mTraceLogDetailInformation ) )

/* BUG-46932 */ 
#define QCU_OPTIMIZER_INVERSE_JOIN_ENABLE ( QCU_PROPERTY(mOptimizerInverseJoinEnable) )

/* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
#define QCU_REDUCE_PART_PREPARE_MEMORY ( QCU_PROPERTY(mReducePartPrepareMemory) )

#define QCU_SHARD_IN_PSM_ENABLE        ( QCU_PROPERTY( mShardInPSMEnable ) )

/* BUG-47986 */
#define QCU_OPTIMIZER_OR_VALUE_INDEX ( QCU_PROPERTY(mOptimizerOrValueIndex) )

/* BUG-48132 */
#define QCU_OPTIMIZER_PLAN_HASH_OR_SORT_METHOD ( QCU_PROPERTY(mOptimizerPlanHashOrSortMethod) )

/* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
#define QCU_OPTIMIZER_INDEX_NL_JOIN_PENALTY ( QCU_PROPERTY(mOptimizerIndexNlJoinPenalty) )

/* BUG-48120 */
#define QCU_OPTIMIZER_INDEX_COST_MODE ( QCU_PROPERTY(mOptimizerIndexCostMode) )

/* BUG-48594 */
#define QCU_SQL_PLAN_CACHE_VALID_MODE ( QCU_PROPERTY(mSqlPlanCacheValidMode) )

/* BUG-48776 */
#define QCU_SUBQUERY_MODE ( QCU_PROPERTY(mSubqueryMode) )

/* PROJ-2750 */
#define QCU_LEFT_OUTER_SKIP_RIGHT_ENABLE ( QCU_PROPERTY(mLeftOuterSkipRightEnable) )

// 참조 : mmuPropertyArgument
typedef struct qcuPropertyArgument
{
    idpArgument mArg;
    UInt        mUserID;
    smiTrans   *mTrans;
} qcuPropertyArgument;

/* BUG-40867 QP SharedProperty */
typedef struct qcuProperties
{
    idShmAddr mAddrSelf;

    //----------------------------------------------
    // Trace Log 관련 Properties
    //    - Writable Property의 경우, MM 에서 관리하여야 하나,
    //    - Trace Log는 System Property라기 보다는
    //    - RunTime Property의 의미이다.
    //----------------------------------------------

    // 수행한 DML 구문의 출력
    // TODO - Optimizer에 해당 Trace에 대한 처리가 필요함.
    UInt   mTraceLog_DML_Sentence;

    // Predicate의 보다 자세한 정보 출력
    UInt   mTraceLog_Detail_Predicate;

    // PROJ-1473
    // 레코드저장방식의 질의처리과정에 대한 정보를 얻기위함.
    // mtrNode 정보 출력
    UInt   mTraceLog_Detail_MtrNode;

    // PROJ-2462 Result Cache
    UInt   mTraceLog_Detail_ResultCache;

    // Graph에 대한 자세한 정보 출력
    UInt   mTraceLog_Explain_Graph;

    // PROJ-2179
    // Execution plan의 각 operator별 결과 정보 출력
    UInt   mTraceLog_Result_Desc;

    // BUG-38192
    // Partition graph 의 children 정보 출력
    UInt   mTraceLogDisplayChildren;

    //----------------------------------------------
    // Query Processor 관련 Properties
    //     - Read Only Property만 관리해야 한다.
    //     - Writable Property는 MM 단에서 관리하며,
    //     - Session 정보로부터 그 값을 얻는다.
    //----------------------------------------------

    // Normalization에 대한 Node수 예측 시의 한계값
    UInt   mNormalFormMaximum;

    // DDL 수행 가능 여부
    UInt   mExecDDLDisable;

    ULong  mIsolationLevel;

    // UPDATE_IN_PLACE
    UInt   mUpdateInPlace;

    UInt   mSpMode;

    // DEFAULT SEGMENT MANAGEMENT  ( PROJ-1671 )
    UInt   mSegMgmtType;
    // DEFAULT STORAGE ATTRIBUTEES
    UInt   mSegStoInitExtCnt;
    UInt   mSegStoNextExtCnt;
    UInt   mSegStoMinExtCnt;
    UInt   mSegStoMaxExtCnt;

    UInt   mPctFree;
    UInt   mPctUsed;

    // *_TRANS ( To Fix PROJ-1704 )
    UInt   mTableInitTrans;
    UInt   mTableMaxTrans;
    UInt   mIndexInitTrans;
    UInt   mIndexMaxTrans;

    // BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
    //           관련프로퍼티를 참조하지 못하는 경우 있음.
    UInt   mOptimizerMode;

    UInt   mAutoRemoteExec;

    // PROJ-1358 Stack Size의 조정
    UInt   mQueryStackSize;

    // PR-13395 TPC-H Scale Factor
    UInt   mFakeTpchScaleFactor;

    // PR-13395 Fake Buffer Size
    UInt   mFakeBufferSize;

    // PROJ-1446 Host variable을 포함한 질의 최적화
    UInt   mHostOptimizeEnable;

    // BUG-13608 Filehandle Open Limit
    UInt   mFileOpenLimit;
    
    // BUG-40854
    UInt mSocketOpenLimit;

    /* BUG-41307 User Lock 지원 */
    UInt mUserLockPoolInitSize;
    UInt mUserLockRequestTimeout;
    UInt mUserLockRequestCheckInterval;
    UInt mUserLockRequestLimit;

    // PR-14325 PK Update in Replicated table.
    UInt   mReplUpdatePK;

    // PROJ-1557 memory variable column in row size.
    UInt   mMemVarColumnInRowSize;

    // PROJ-1362 memory lob column in row size.
    UInt   mMemLobColumnInRowSize;

    // PROJ-1862 disk lob column in row size.
    UInt   mDiskLobColumnInRowSize;

    // BUG-18851 disable transitive predicate generation
    UInt   mOptimizerTransitivityDisable;

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    UInt   mOptimizerTransitivityOldRule;

    // PROJ-1473
    UInt   mOptimizerPushProjection;

    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    UInt   mOptimizerDefaultTempTbsType;

    // BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
    UInt   mOptimizerFixedGroupMemoryTemp;
    // BUG-38339 Outer Join Elimination
    UInt   mOptimizerOuterJoinElimination;

    // PROJ-1413 Simple View Merging
    UInt   mOptimizerSimpleViewMergingDisable;

    // PROJ-1718 Subquery Unnesting
    UInt   mOptimizerUnnestSubquery;
    UInt   mOptimizerUnnestComplexSubquery;
    UInt   mOptimizerUnnestAggregationSubquery;

    // PROJ-1583 large geometry
    UInt   mSTObjBufSize;

    // BUG-19089
    // FK가 있는 상태에서 CREATE REPLICATION 구문이 가능하도록 한다.
    UInt   mCheckFkInCreateReplicationDisable;

    // PROJ-1436
    UInt   mSqlPlanCachePreparedExecutionContextCnt;

    // PROJ-2002 Column Security
    SChar  mSecurityModuleName[QCS_MODULE_NAME_SIZE + 1];
    SChar  mSecurityModuleLibrary[QCS_MODULE_LIBRARY_SIZE + 1];
    SChar  mSecurityEccPolicyName[QCS_POLICY_NAME_SIZE + 1];

    // BUG-29209
    // Plan에서 변경은 자주 일어나나 display가 꼭 필요하지 않는
    // 정보는 보여주지 않도록 함
    UInt   mDisplayPlanForNATC;

    // BUG-31040
    SInt   mMaxSetOpRecursionDepth;

    // BUG-32101
    SInt   mOptimizerDiskIndexCostAdj;

    // BUG-43736
    SInt   mOptimizerMemoryIndexCostAdj;

    // BUG-34441
    UInt   mOptimizerHashJoinCostAdj;

    //BUG-34235
    UInt   mOptimizerSubqueryOptimizeMethod;

    // fix BUG-33589
    UInt   mPlanRebuildEnable;

    /* PROJ-1071 Parallel Query */
    UInt   mParallelQueryThreadMax;
    UInt   mParallelQueryQueueSleepMax;
    UInt   mParallelQueryQueueSize;

    UInt   mForceParallelDegree;

    /* PROJ-2361 */
    UInt   mAvgTransformEnable;

    /* PROJ-2207 Password policy support */
    SChar  mSysdateForNATC[QTC_SYSDATE_FOR_NATC_LEN + 1];

    UInt   mFailedLoginAttempts;
    UInt   mPasswLockTime;
    UInt   mPasswLifeTime;
    UInt   mPasswGraceTime;
    UInt   mPasswReuseMax;
    UInt   mPasswReuseTime;
    SChar  mPasswVerifyFunc[QC_PASSWORD_OPT_LEN + 1];

    // BUG-34234
    SInt   mCoerceHostVarToVarchar;

    // BUG-34295
    UInt   mOptimizerAnsiJoinOrdering;
    // BUG-38402
    UInt   mOptimizerAnsiInnerJoinConvert;

    // BUG-34350
    UInt   mOptimizerRefinePrepareMemory;

    // BUG-35155
    UInt   mOptimizerPartialNormalize;

    // PROJ-2264 Dictionary table
    UInt   mForceCompressionColumn;

    /* PROJ-1090 Function-based Index */
    UInt   mQueryRewriteEnable;

    // BUG-35713
    // sql로 부터 invoke되는 function에서 발생하는 no_data_found 에러무시
    UInt   mPSMIgnoreNoDataFoundError;

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    UInt   mIntermediateTupleLobObjectLimit;

    /* PROJ-2242 CSE, FS, Feature enable */
    UInt   mOptimizerEliminateCommonSubexpression;
    UInt   mOptimizerConstantFilterSubsumption;
    SChar  mOptimizerFeatureEnable[QCU_OPTIMIZER_FEATURE_VERSION_LEN + 1];

    UInt   mDmlWithoutRetryEnable;

    // fix BUG-36522
    UInt  mPSMShowErrorStack;

    // fix BUG-36793
    UInt  mBindParamDefaultPrecision;

    // BUG-36203
    UInt  mPSMTemplateCacheCount;

    // BUG-37247
    UInt  mSysConnectByPathPrecision;

    // BUG-37252
    UInt  mExecutionPlanMemoryCheck;

    // BUG-37302
    UInt  mSQLErrorInfoSize;

    // PROJ-2362
    UInt  mReduceTempMemoryEnable;

    // BUG-38434
    UInt  mOptimizerDnfDisable;

    /* BUG-38952 type null */
    UInt  mTypeNull;

    /* BUG-38946 display name */
    UInt  mCompatDisplayName;
   
    // fix BUG-39754 
    UInt  mForeignKeyLockRow;

    // BUG-40022
    UInt  mOptimizerJoinDisable;

    // BUG-40042 oracle outer join property
    UInt  mOuterJoinOperToAnsiJoinTransEnable;

    // PROJ-2469 Optimize View Materialization
    UInt  mOptimizerViewTargetEnable;    

    /* PROJ-2451 Concurrent Execute Package */
    UInt  mConcExecDegreeMax;
    UInt  mConcExecDegreeDefault;
    UInt  mConcExecWaitInterval;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    UInt  mQueryExecutionCacheMaxCount;
    UInt  mQueryExecutionCacheMaxSize;
    UInt  mQueryExecutionCacheBucketCount;
    UInt  mForceFunctionCache;

    /* PROJ-2441 flashback */
    ULong mRecyclebinMemMaxSize;
    ULong mRecyclebinDiskMaxSize;
    UInt  mRecyclebinEnable;
    UInt  mRecyclebinNATC;

    /* PROJ-2448 Subquery caching */
    UInt  mForceSubqueryCacheDisable;

    // PROJ-2551 simple query 최적화
    UInt  mExecutorFastSimpleQuery;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    UInt  mHashJoinMemTempPartitioningDisable;
    UInt  mHashJoinMemTempAutoBucketCntDisable;
    UInt  mHashJoinMemTempTLBCnt;
    UInt  mForceHashJoinMemTempFanoutMode;

    // BUG-41183 ORDER BY Elimination
    UInt  mOptimizerOrderByEliminationEnable;

    // BUG-41398 use old sort
    UInt  mUseOldSort;

    UInt  mDDLSupplementalLog;

    UInt  mOptimizerDistinctEliminationEnable;

    /* PROJ-2462 Result Cache */
    UInt  mResultCacheEnable;
    UInt  mTopResultCacheMode;
    ULong mResultCacheMemoryMax;

    // BUG-36438 LIST transformation
    UInt  mOptimizerListTransformation;

    // PROJ-2492 Dynamic sample selection
    UInt  mOptimizerAutoStats;

    // BUG-41248 DBMS_SQL package
    UInt  mPSMCursorOpenLimit;

    // PROJ-2582 recursive with
    UInt  mRecursionLevelMaximum;

    // fix BUG-42752
    UInt  mOptimizerEstimateKeyFilterSelectivity;

    /* PROJ-2465 Tablespace Alteration for Table */
    UInt  mDDLMemUsageThreshold;
    UInt  mDDLTBSUsageThreshold;
    UInt  mAnalyzeUsageMinRowCount;
    ULong mMemMaxDBSize;

    /* PROJ-2586 PSM Parameters and return without precision */
    UInt  mPSMParamAndReturnWithoutPrecisionEnable;
    UInt  mPSMCharDefaultPrecision;
    UInt  mPSMVarcharDefaultPrecision;
    UInt  mPSMNcharUTF16DefaultPrecision;
    UInt  mPSMNvarcharUTF16DefaultPrecision;
    UInt  mPSMNcharUTF8DefaultPrecision;
    UInt  mPSMNvarcharUTF8DefaultPrecision;

    /* BUG-42639 Monitoring query */
    UInt  mOptimizerPerformanceView;

    // BUG-42322
    ULong mPSMFormatCallStackOID;

    // BUG-43039 inner join push down
    UInt  mOptimizerInnerJoinPushDown;

    // BUG-43068 Indexable order by 개선
    UInt  mOptimizerOrderPushDown;

    // BUG-43059 Target subquery unnest/removal disable
    UInt  mOptimizerTargetSubqueryUnnestDisable;
    UInt  mOptimizerTargetSubqueryRemovalDisable;

    /* BUG-43112 */  
    UInt mForceAutonomousTransactionPragma;
    UInt mAutonomousTransactionPragmaDisable;

    // BUG-43258
    UInt  mOptimizerIndexClusteringFactorAdj;

    // BUG-43158 Enhance statement list caching in PSM
    UInt  mPSMStmtListCount;
    UInt  mPSMStmtPoolCount;

    /* BUG-42928 No Partition Lock */
    UInt  mTableLockMode;

    // BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
    UInt  mDiskTempSizeEstimate;

    // BUG-43421
    UInt mOptimizerSemiJoinTransitivityDisable;

    // BUG-43493
    UInt mOptimizerDelayedExecution;

    /* BUG-43495 */
    UInt mOptimizerLikeIndexScanWithOuterColumnDisable;

    /* BUG-43737 */
    UInt mForceTablespaceDefault;

    /* BUG-43769 */
    idBool mIsCPUAffinity;

    /* TASK-6744 */
    UInt mPlanRandomSeed;

    /* BUG-44499 */
    UInt mOptimizerBucketCountMin;

    /* BUG-48161 */
    UInt mOptimizerBucketCountMax;

    /* PROJ-2641 Hierarchy Query Index */
    UInt   mOptimizerHierarchyTransformation;

    // BUG-44692
    UInt   mOptimizerBug44692;

    // BUG-44795
    UInt   mOptimizerDBMSStatPolicy;

    /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
    UInt mOptimizerIndexNLJoinAccessMethodPolicy;
    
    UInt mOptimizerSemiJoinRemove;
    UInt mPrintOutEnable;
    UInt mKeyPreservedTable;

    UInt mOptimizerUnnestCompatibility;

    /* PROJ-2632 */
    UInt mSerialExecuteMode;
    UInt mTraceLogDetailInformation;

    /* BUG-46932 */ 
    UInt mOptimizerInverseJoinEnable;

    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    UInt mReducePartPrepareMemory;

    idBool mShardInPSMEnable;

    /* BUG-47986 */
    UInt mOptimizerOrValueIndex;

    /* BUG-48132 */
    UInt mOptimizerPlanHashOrSortMethod;

    /* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
    UInt mOptimizerIndexNlJoinPenalty;

    /* BUG-48120 */
    UInt mOptimizerIndexCostMode;

    /* BUG-48594 */
    UInt mSqlPlanCacheValidMode;

    /* BUG-48776 */
    UInt mSubqueryMode;

    /* PROJ-2750 */
    UInt  mLeftOuterSkipRightEnable;
} qcuProperties;

class qcuProperty
{
public:

    /* BUG-40867 QP SharedProperty */
    static qcuProperties    mStaticProperty;
    
public:

    /* BUG-40867 QP SharedProperty */
    static IDE_RC initProperty( idvSQL *aStatistics );
    static IDE_RC finalProperty( idvSQL *aStatistics );

    // System 구동 시 관련 Property를 Loading 함.
    static IDE_RC load();

    // System 구동 시 관련 Property의 callback 을 등록함
    static IDE_RC setupUpdateCallback();

    //----------------------------------------------
    // Writable Property를 위한 Call Back 함수들
    //----------------------------------------------

    static IDE_RC changeTRCLOG_DML_SENTENCE( idvSQL* /* aStatistics */,
					     SChar * aName,
					     void  * aOldValue,
					     void  * aNewValue,
					     void  * /* aArg */);

    static IDE_RC changeTRCLOG_DETAIL_PREDICATE( idvSQL* /* aStatistics */,
						 SChar * aName,
						 void  * aOldValue,
						 void  * aNewValue,
						 void  * /* aArg */);

    static IDE_RC changeTRCLOG_DETAIL_MTRNODE( idvSQL* /* aStatistics */,
					       SChar * /* aName */,
					       void  * /* aOldValue */,
					       void  * aNewValue,
					       void  * /* aArg */);

    static IDE_RC changeTRCLOG_EXPLAIN_GRAPH( idvSQL* /* aStatistics */,
					      SChar * aName,
					      void  * aOldValue,
					      void  * aNewValue,
					      void  * /* aArg */);

    static IDE_RC changeTRCLOG_RESULT_DESC( idvSQL* /* aStatistics */,
					    SChar * aName,
					    void  * aOldValue,
					    void  * aNewValue,
					    void  * /* aArg */);

    // BUG-38192
    static IDE_RC changeTRCLOG_DISPLAY_CHILDREN( idvSQL* /* aStatistics */,
						 SChar * aName,
						 void  * aOldValue,
						 void  * aNewValue,
						 void  * aArg );

    static IDE_RC changeUPDATE_IN_PLACE( idvSQL* /* aStatistics */,
					 SChar * aName,
					 void  * aOldValue,
					 void  * aNewValue,
					 void  * /* aArg */);

    static IDE_RC changeEXEC_DDL_DISABLE( idvSQL* /* aStatistics */,
					  SChar * /* aName */,
					  void  * /* aOldValue */,
					  void  * aNewValue,
					  void  * /* aArg */);

    // BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
    //           관련프로퍼티를 참조하지 못하는 경우 있음.
    static IDE_RC changeOPTIMIZER_MODE( idvSQL* /* aStatistics */,
					SChar * /* aName */,
					void  * /* aOldValue */,
					void  * aNewValue,
					void  * /* aArg */);

    static IDE_RC changeAUTO_REMOTE_EXEC( idvSQL* /* aStatistics */,
					  SChar * /* aName */,
					  void  * /* aOldValue */,
					  void  * aNewValue,
					  void  * /* aArg */);

    // PROJ-1358 Stack Size의 조정
    static IDE_RC changeQUERY_STACK_SIZE( idvSQL* /* aStatistics */,
					  SChar * /* aName */,
					  void  * /* aOldValue */,
					  void  * aNewValue,
					  void  * /* aArg */);

    // PR-24056
    static IDE_RC changeNORMALFORM_MAXIMUM( idvSQL* /* aStatistics */,
					    SChar * /* aName */,
					    void  * /* aOldValue */,
					    void  * aNewValue,
					    void  * /* aArg */);

    // PR-13395 TPC-H Scale Factor 조정
    static IDE_RC changeFAKE_STAT_TPCH_SCALE_FACTOR( idvSQL* /* aStatistics */,
						     SChar * /* aName */,
						     void  * /* aOldValue */,
						     void  * aNewValue,
						     void  * /* aArg */);

    // PR-13395 가상 Buffer Size 조정
    static IDE_RC changeFAKE_STAT_BUFFER_SIZE( idvSQL* /* aStatistics */,
					       SChar * /* aName */,
					       void  * /* aOldValue */,
					       void  * aNewValue,
					       void  * /* aArg */);

    // PROJ-1446 Host variable을 포함한 질의 최적화
    static IDE_RC changeHOST_OPTIMIZE_ENABLE( idvSQL* /* aStatistics */,
					      SChar * /* aName */,
					      void  * /* aOldValue */,
					      void  * aNewValue,
					      void  * /* aArg */);

    // BUG-13068 Filehandle Open Limit 조정
    static IDE_RC changePSM_FILE_OPEN_LIMIT( idvSQL* /* aStatistics */,
					     SChar * /* aName */,
					     void  * /* aOldValue */,
					     void  * aNewValue,
					     void  * /* aArg */);

    // BUG-40854
    static IDE_RC changeCONNECT_TYPE_OPEN_LIMIT( idvSQL* /* aStatistics */,
                        SChar * /* aName */,
                        void  * /* aOldValue */,
                        void  * aNewValue,
                        void  * /* aArg */);

    /* BUG-41307 User Lock 지원 */
    static IDE_RC changeUSER_LOCK_REQUEST_TIMEOUT( idvSQL * /* aStatistics */,
                                                   SChar  * /* aName */,
                                                   void   * /* aOldValue */,
                                                   void   * aNewValue,
                                                   void   * /* aArg */ );

    /* BUG-41307 User Lock 지원 */
    static IDE_RC changeUSER_LOCK_REQUEST_CHECK_INTERVAL( idvSQL * /* aStatistics */,
                                                          SChar  * /* aName */,
                                                          void   * /* aOldValue */,
                                                          void   * aNewValue,
                                                          void   * /* aArg */ );

    /* BUG-41307 User Lock 지원 */
    static IDE_RC changeUSER_LOCK_REQUEST_LIMIT( idvSQL * /* aStatistics */,
                                                 SChar  * /* aName */,
                                                 void   * /* aOldValue */,
                                                 void   * aNewValue,
                                                 void   * /* aArg */ );

    static void* callbackForGettingArgument( idvSQL              * /* aStatistics */,
					     qcuPropertyArgument *aArg,
					     idpArgumentID        aID );

    //PROJ-1583 large geometry
    static IDE_RC changeST_OBJECT_BUFFER_SIZE( idvSQL* /* aStatistics */,
					       SChar * /* aName */,
					       void  * /* aOldValue */,
					       void  * aNewValue,
					       void  * /* aArg */);

    static IDE_RC changeDML_WITHOUT_RETRY_ENABLE(idvSQL* /* aStatistics */,
						 SChar * /*aName*/,
						 void  * /*aOldValue*/,
						 void  * aNewValue,
						 void  * /*aArg*/);

    // BUG-19089
    static IDE_RC changeCHECK_FK_IN_CREATE_REPLICATION_DISABLE( idvSQL* /* aStatistics */,
								SChar * /* aName */,
								void  * /* aOldValue */,
								void  * aNewValue,
								void  * /* aArg */);

    // PROJ-1413
    static IDE_RC changeOPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    static IDE_RC changeOPTIMIZER_DEFAULT_TEMP_TBS_TYPE(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
    static IDE_RC changeOPTIMIZER_FIXED_GROUP_MEMORY_TEMP(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);
    // BUG-38339 Outer Join Elimination
    static IDE_RC changeOPTIMIZER_OUTERJOIN_ELIMINATION(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // PROJ-1436
    static IDE_RC changeSQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // PROJ-2002
    static IDE_RC changeQCU_SECURITY_MODULE_NAME(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // PROJ-2002
    static IDE_RC changeQCU_SECURITY_MODULE_LIBRARY(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // PROJ-2002
    static IDE_RC changeQCU_SECURITY_ECC_POLICY_NAME(
	idvSQL* /* aStatistics */,
	SChar * /* aName */,
	void  * /* aOldValue */,
	void  * aNewValue,
	void  * /* aArg */);

    // BUG-29209
    static IDE_RC changeDISPLAY_PLAN_FOR_NATC( idvSQL* /* aStatistics */,
					       SChar * /* aName */,
					       void  * /* aOldValue */,
					       void  * aNewValue,
					       void  * /* aArg */);

    // BUG-31040
    static IDE_RC change__MAX_SET_OP_RECURSION_DEPTH(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);


    // BUG-32101
    static IDE_RC changeOPTIMIZER_DISK_INDEX_COST_ADJ(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    static IDE_RC changeOPTIMIZER_MEMORY_INDEX_COST_ADJ(
        idvSQL* /* aStatistics */,
        SChar * aName,
        void  * aOldValue,
        void  * aNewValue,
        void  * aArg);

    // BUG-34441
    static IDE_RC changeOPTIMIZER_HASH_JOIN_COST_ADJ(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // BUG-34235
    static IDE_RC changeOPTIMIZER_SUBQUERY_OPTIMIZE_METHOD(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // fix BUG-33589
    static IDE_RC changePLAN_REBUILD_ENABLE( idvSQL* /* aStatistics */,
					     SChar * aName,
					     void  * aOldValue,
					     void  * aNewValue,
					     void  * aArg );

    /* PROJ-2361 */
    static IDE_RC changeAVERAGE_TRANSFORM_ENABLE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    /* PROJ-1071 Parallel Query */
    static IDE_RC changePARALLEL_QUERY_THREAD_MAX(idvSQL* /* aStatistics */,
						  SChar* aName,
						  void * aOldValue,
						  void * aNewValue,
						  void * aArg);

    static IDE_RC changePARALLEL_QUERY_QUEUE_SLEEP_MAX(idvSQL* /* aStatistics */,
						       SChar* aName,
						       void * aOldValue,
						       void * aNewValue,
						       void * aArg);

    static IDE_RC changePARALLEL_QUERY_QUEUE_SIZE(idvSQL* /* aStatistics */,
						  SChar* aName,
						  void * aOldValue,
						  void * aNewValue,
						  void * aArg);

    static IDE_RC changeFORCE_PARALLEL_DEGREE(idvSQL* /* aStatistics */,
					      SChar * aName,
					      void  * aOldValue,
					      void  * aNewValue,
					      void  * aArg);

    /* PROJ-2207 Password policy support */
    static IDE_RC change__SYSDATE_FOR_NATC( idvSQL* /* aStatistics */,
					    SChar * /* aName */,
					    void  * /* aOldValue */,
					    void  * aNewValue,
					    void  * /* aArg */);

    // BUG-36203
    static IDE_RC changePSM_TEMPLATE_CACHE_COUNT(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // BUG-34295
    static IDE_RC changeOPTIMIZER_ANSI_JOIN_ORDERING(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // BUG-38402
    static IDE_RC changeOPTIMIZER_ANSI_INNER_JOIN_CONVERT(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // BUG-34350
    static IDE_RC changeOPTIMIZER_REFINE_PREPARE_MEMORY(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // BUG-35155
    static IDE_RC changeOPTIMIZER_PARTIAL_NORMALIZE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // PROJ-2264 Dictionary table
    static IDE_RC changeFORCE_COMPRESSION_COLUMN(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    /* PROJ-1090 Function-based Index */
    static IDE_RC changeQUERY_REWRITE_ENABLE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // BUG-35713
    static IDE_RC changePSM_IGNORE_NO_DATA_FOUND_ERROR(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // PROJ-1718
    static IDE_RC changeOPTIMIZER_UNNEST_SUBQUERY(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    static IDE_RC changeOPTIMIZER_UNNEST_COMPLEX_SUBQUERY(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    static IDE_RC changeOPTIMIZER_UNNEST_AGGREGATION_SUBQUERY(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    static IDE_RC changeINTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    /* PROJ-2242 CSE, FS, Feature enable */
    static IDE_RC changeOPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    static IDE_RC changeOPTIMIZER_CONSTANT_FILTER_SUBSUMPTION(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    static IDE_RC changeOPTIMIZER_FEATURE_ENABLE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // fix BUG-36522
    static IDE_RC changePSM_SHOW_ERROR_STACK(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // fix BUG-36793
    static IDE_RC changeBIND_PARAM_DEFAULT_PRECISION(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // BUG-37247
    static IDE_RC changeSYS_CONNECT_BY_PATH_PRECISION(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // BUG-37302
    static IDE_RC changeSQL_ERROR_INFO_SIZE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // PROJ-2362
    static IDE_RC changeREDUCE_TEMP_MEMORY_ENABLE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg );

    // BUG-38434
    static IDE_RC changeOPTIMIZER_DNF_DISABLE(
	idvSQL* /* aStatistics */,
	SChar * aName,
	void  * aOldValue,
	void  * aNewValue,
	void  * aArg);

    // fix BUG-39754
    static IDE_RC changeFOREIGN_KEY_LOCK_ROW(
	idvSQL* /* aStatistics */,
        SChar * aName,
        void  * aOldValue,
        void  * aNewValue,
        void  * aArg);

    // fix BUG-40022
    static IDE_RC changeOPTIMIZER_JOIN_DISABLE(
	idvSQL* /* aStatistics */,
        SChar * aName,
        void  * aOldValue,
        void  * aNewValue,
        void  * aArg);

    // PROJ-2469 Optimize View Materialization
    static IDE_RC changeOPTIMIZER_VIEW_TARGET_ENABLE(
  	idvSQL* /* aStatistics */,
        SChar * aName,
        void  * aOldValue,
        void  * aNewValue,
        void  * aArg );    

    /* PROJ-2451 Concurrent Execute Package */
    static IDE_RC changeCONCURRENT_EXEC_DEGREE_DEFAULT(
	idvSQL* /* aStatistics */,
        SChar * aName,
        void  * aOldValue,
        void  * aNewValue,
        void  * aArg );

    /* PROJ-2451 Concurrent Execute Package */
    static IDE_RC changeCONCURRENT_EXEC_WAIT_INTERVAL(
	idvSQL* /* aStatistics */,
        SChar * aName,
        void  * aOldValue,
        void  * aNewValue,
        void  * aArg );

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    static IDE_RC changeQUERY_EXECUTION_CACHE_MAX_COUNT( idvSQL* /* aStatistics */,
                                                         SChar * aName,
                                                         void  * aOldValue,
                                                         void  * aNewValue,
                                                         void  * aArg );

    static IDE_RC changeQUERY_EXECUTION_CACHE_MAX_SIZE( idvSQL* /* aStatistics */,
                                                        SChar * aName,
                                                        void  * aOldValue,
                                                        void  * aNewValue,
                                                        void  * aArg );

    static IDE_RC changeQUERY_EXECUTION_CACHE_BUCKET_COUNT( idvSQL* /* aStatistics */,
                                                            SChar * aName,
                                                            void  * aOldValue,
                                                            void  * aNewValue,
                                                            void  * aArg );

    static IDE_RC changeFORCE_FUNCTION_CACHE( idvSQL* /* aStatistics */,
                                              SChar * aName,
                                              void  * aOldValue,
                                              void  * aNewValue,
                                              void  * aArg );
    
    /* PROJ-2441 flashback */
    static IDE_RC changeRECYCLEBIN_MEM_MAX_SIZE( idvSQL* /* aStatistics */,
                                                 SChar * aName,
                                                 void  * aOldValue,
                                                 void  * aNewValue,
                                                 void  * aArg );

    static IDE_RC changeRECYCLEBIN_DISK_MAX_SIZE( idvSQL* /* aStatistics */,
                                                  SChar * aName,
                                                  void  * aOldValue,
                                                  void  * aNewValue,
                                                  void  * aArg );

    static IDE_RC changeRECYCLEBIN_ENABLE( idvSQL* /* aStatistics */,
                                           SChar * aName,
                                           void  * aOldValue,
                                           void  * aNewValue,
                                           void  * aArg );

    static IDE_RC change__RECYCLEBIN_FOR_NATC( idvSQL* /* aStatistics */,
                                               SChar * aName,
                                               void  * aOldValue,
                                               void  * aNewValue,
                                               void  * aArg );
    /* PROJ-2448 Subquery caching */
    static IDE_RC changeFORCE_SUBQUERY_CACHE_DISABLE( idvSQL* /* aStatistics */,
                                                      SChar * aName,
                                                      void  * aOldValue,
                                                      void  * aNewValue,
                                                      void  * aArg );

    static IDE_RC changeEXECUTOR_FAST_SIMPLE_QUERY( idvSQL* /* aStatistics */,
                                                    SChar * aName,
                                                    void  * aOldValue,
                                                    void  * aNewValue,
                                                    void  * aArg );

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    static IDE_RC changeHASH_JOIN_MEM_TEMP_PARTITIONING_DISABLE( idvSQL* /* aStatistics */,
                                                                 SChar * aName,
                                                                 void  * aOldValue,
                                                                 void  * aNewValue,
                                                                 void  * aArg );

    static IDE_RC changeHASH_JOIN_MEM_TEMP_AUTO_BUCKET_CNT_DISABLE( idvSQL* /* aStatistics */,
                                                                    SChar * aName,
                                                                    void  * aOldValue,
                                                                    void  * aNewValue,
                                                                    void  * aArg );

    static IDE_RC changeHASH_JOIN_MEM_TEMP_TLB_COUNT( idvSQL* /* aStatistics */,
                                                      SChar * aName,
                                                      void  * aOldValue,
                                                      void  * aNewValue,
                                                      void  * aArg );

    static IDE_RC changeHASH_JOIN_MEM_TEMP_FANOUT_MODE( idvSQL* /* aStatistics */,
                                                        SChar * aName,
                                                        void  * aOldValue,
                                                        void  * aNewValue,
                                                        void  * aArg );

    // BUG-41183 ORDER BY Elimination
    static IDE_RC changeOPTIMIZER_ORDER_BY_ELIMINATION_ENABLE( idvSQL * /* aStatistics */,
                                                               SChar  * aName,
                                                               void   * aOldValue,
                                                               void   * aNewValue,
                                                               void   * aArg );

    // BUG-41398 use old sort
    static IDE_RC changeUSE_OLD_SORT( idvSQL* /* aStatistics */,
                                      SChar * aName,
                                      void  * aOldValue,
                                      void  * aNewValue,
                                      void  * aArg );

    static IDE_RC changeDDL_SUPPLEMENTAL_LOG_ENABLE( idvSQL* /* aStatistics */,
                                                     SChar * aName,
                                                     void  * aOldValue,
                                                     void  * aNewValue,
                                                     void  * aArg );

    static IDE_RC changeOPTIMIZER_DISTINCT_ELIMINATION_ENABLE( idvSQL* /* aStatistics */,
                                                               SChar * aName,
                                                               void  * aOldValue,
                                                               void  * aNewValue,
                                                               void  * aArg );

    // PROJ-2462 Result Cache
    static IDE_RC changeRESULT_CACHE_ENABLE( idvSQL* /* aStatistics */,
                                             SChar * aName,
                                             void  * aOldValue,
                                             void  * aNewValue,
                                             void  * aArg );
    // PROJ-2462 Result Cache
    static IDE_RC changeTOP_RESULT_CACHE_MODE( idvSQL* /* aStatistics */,
                                               SChar * aName,
                                               void  * aOldValue,
                                               void  * aNewValue,
                                               void  * aArg );
    // PROJ-2462 Result Cache
    static IDE_RC changeTRCLOG_DETAIL_RESULTCACHE( idvSQL* /* aStatistics */,
                                                   SChar * aName,
                                                   void  * aOldValue,
                                                   void  * aNewValue,
                                                   void  * aArg );
    // PROJ-2462 Result Cache
    static IDE_RC changeRESULT_CACHE_MEMORY_MAXIMUM( idvSQL* /* aStatistics */,
                                                     SChar * aName,
                                                     void  * aOldValue,
                                                     void  * aNewValue,
                                                     void  * aArg );

    // BUG-36438 LIST Transformation
    static IDE_RC changeOPTIMIZER_LIST_TRANSFORMATION( idvSQL* /* aStatistics */,
                                                       SChar * aName,
                                                       void  * aOldValue,
                                                       void  * aNewValue,
                                                       void  * aArg);

    /* PROJ-2492 Dynamic sample selection */
    static IDE_RC changeOPTIMIZER_AUTO_STATS( idvSQL* /* aStatistics */,
                                              SChar * aName,
                                              void  * aOldValue,
                                              void  * aNewValue,
                                              void  * aArg );

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    static IDE_RC changeOPTIMIZER_TRANSITIVITY_OLD_RULE( idvSQL * /* aStatistics */,
                                                         SChar  * aName,
                                                         void   * aOldValue,
                                                         void   * aNewValue,
                                                         void   * aArg );

    // PROJ-2582 recursive with
    static IDE_RC changeRECURSION_LEVEL_MAXIMUM( idvSQL* /* aStatistics */,
                                                 SChar * aName,
                                                 void  * aOldValue,
                                                 void  * aNewValue,
                                                 void  * aArg );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC changeDDL_MEM_USAGE_THRESHOLD( idvSQL * /* aStatistics */,
                                                 SChar  * aName,
                                                 void   * aOldValue,
                                                 void   * aNewValue,
                                                 void   * aArg );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC changeDDL_TBS_USAGE_THRESHOLD( idvSQL * /* aStatistics */,
                                                 SChar  * aName,
                                                 void   * aOldValue,
                                                 void   * aNewValue,
                                                 void   * aArg );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC changeANALYZE_USAGE_MIN_ROWCOUNT( idvSQL * /* aStatistics */,
                                                    SChar  * aName,
                                                    void   * aOldValue,
                                                    void   * aNewValue,
                                                    void   * aArg );
    /* BUG-42639 Monitoring query */
    static IDE_RC changeOPTIMIZER_PERFORMANCE_VIEW( idvSQL * /* aStatistics */,
                                                    SChar  * aName,
                                                    void   * aOldValue,
                                                    void   * aNewValue,
                                                    void   * aArg );

    // BUG-43039 inner join push down
    static IDE_RC changeOPTIMIZER_INNER_JOIN_PUSH_DOWN( idvSQL* /* aStatistics */,
                                                        SChar * aName,
                                                        void  * aOldValue,
                                                        void  * aNewValue,
                                                        void  * aArg );

    // BUG-43068 Indexable order by 개선
    static IDE_RC changeOPTIMIZER_ORDER_PUSH_DOWN( idvSQL* /* aStatistics */,
                                                   SChar * aName,
                                                   void  * aOldValue,
                                                   void  * aNewValue,
                                                   void  * aArg );

    // BUG-43059 Target subquery unnest/removal disable
    static IDE_RC changeOPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE( idvSQL* /* aStatistics */,
                                                                  SChar * aName,
                                                                  void  * aOldValue,
                                                                  void  * aNewValue,
                                                                  void  * aArg);

    static IDE_RC changeOPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE( idvSQL* /* aStatistics */,
                                                                   SChar * aName,
                                                                   void  * aOldValue,
                                                                   void  * aNewValue,
                                                                   void  * aArg);

    /* BUG-43112 */
    static IDE_RC changeFORCE_AUTONOMOUS_TRANSACTION_PRAGMA( idvSQL* /* aStatistics */,
                                                             SChar * aName,
                                                             void  * aOldValue,
                                                             void  * aNewValue,
                                                             void  * aArg );

    // BUG-43158 Enhance statement list caching in PSM
    static IDE_RC changePSM_STMT_LIST_COUNT( idvSQL* /* aStatistics */,
                                             SChar * aName,
                                             void  * aOldValue,
                                             void  * aNewValue,
                                             void  * aArg );

    static IDE_RC changePSM_STMT_POOL_COUNT( idvSQL* /* aStatistics */,
                                             SChar * aName,
                                             void  * aOldValue,
                                             void  * aNewValue,
                                             void  * aArg );

    static IDE_RC changeDISK_TEMP_SIZE_ESTIMATE( idvSQL* /* aStatistics */,
                                                 SChar * aName,
                                                 void  * aOldValue,
                                                 void  * aNewValue,
                                                 void  * aArg );

    static IDE_RC changeOPTIMIZER_DELAYED_EXECUTION( idvSQL* /* aStatistics */,
                                                     SChar * aName,
                                                     void  * aOldValue,
                                                     void  * aNewValue,
                                                     void  * aArg );

    /* BUG-43495 */
    static IDE_RC changeOPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE( idvSQL* /* aStatistics */,
                                                                             SChar * aName,
                                                                             void  * aOldValue,
                                                                             void  * aNewValue,
                                                                             void  * aArg );

    /* TASK-6744 */
    static IDE_RC changePlanRandomSeed( idvSQL* /* aStatistics */,
                                        SChar * aName,
                                        void  * aOldValue,
                                        void  * aNewValue,
                                        void  * aArg );

    /* BUG-44499 */
    static IDE_RC changeOPTIMIZER_BUCKET_COUNT_MIN( idvSQL* /* aStatistics */,
                                                    SChar * aName,
                                                    void  * aOldValue,
                                                    void  * aNewValue,
                                                    void  * aArg );

    /* BUG-48161 */
    static IDE_RC changeOPTIMIZER_BUCKET_COUNT_MAX( idvSQL* /* aStatistics */,
                                                    SChar * aName,
                                                    void  * aOldValue,
                                                    void  * aNewValue,
                                                    void  * aArg );

    static IDE_RC changeOPTIMIZER_HIERARCHY_TRANSFORMATION( idvSQL * /* aStatistics */,
                                                            SChar  * aName,
                                                            void   * aOldValue,
                                                            void   * aNewValue,
                                                            void   * aArg );

    static IDE_RC changeOPTIMIZER_BUG_44692( idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg */ );

    // BUG-44795
    static IDE_RC changeOPTIMIZER_DBMS_STAT_POLICY( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */ );

    /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
    static IDE_RC changeOPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY( idvSQL* /* aStatistics */,
                                                                      SChar * aName,
                                                                      void  * aOldValue,
                                                                      void  * aNewValue,
                                                                      void  * aArg );

    static IDE_RC changeOPTIMIZER_SEMI_JOIN_REMOVE(idvSQL* /* aStatistics */,
                                                   SChar * /* aName */,
                                                   void  * /* aOldValue */,
                                                   void  * aNewValue,
                                                   void  * /* aArg */);

    static IDE_RC change__PRINT_OUT_ENABLE( idvSQL* /* aStatistics */,
                                            SChar * aName,
                                            void  * aOldValue,
                                            void  * aNewValue,
                                            void  * aArg );

    static IDE_RC change__KEY_PRESERVED_TABLE( idvSQL* /* aStatistics */,
                                               SChar * aName,
                                               void  * aOldValue,
                                               void  * aNewValue,
                                               void  * aArg );

    static IDE_RC change__OPTIMIZER_UNNEST_COMPATIBILITY( idvSQL* /* aStatistics */,
                                                          SChar * aName,
                                                          void  * aOldValue,
                                                          void  * aNewValue,
                                                          void  * aArg );

    /* PROJ-2632 */
    static IDE_RC changeSERIAL_EXECUTE_MODE( idvSQL * /* aStatistics */,
                                             SChar  * aName,
                                             void   * aOldValue,
                                             void   * aNewValue,
                                             void   * aArg );

    static IDE_RC changeTRCLOG_DETAIL_INFORMATION( idvSQL * /* aStatistics */,
                                                   SChar  * aName,
                                                   void   * aOldValue,
                                                   void   * aNewValue,
                                                   void   * aArg );

    /* BUG-46932 */
    static IDE_RC changeOPTIMIZER_INVERSE_JOIN_ENABLE( idvSQL* /* aStatistics */,
                                                       SChar * aName,
                                                       void  * aOldValue,
                                                       void  * aNewValue,
                                                       void  * aArg );

    static IDE_RC changeREDUCE_PARTITION_PREPARE_MEMORY( idvSQL* /* aStatistics */,
                                                         SChar * aName,
                                                         void  * aOldValue,
                                                         void  * aNewValue,
                                                         void  * aArg );

    /* BUG-47986 */
    static IDE_RC changeOPTIMIZER_OR_VALUE_INDEX( idvSQL * /* aStatistics */,
                                                  SChar  * /* aName */,
                                                  void   * /* aOldValue */,
                                                  void   * aNewValue,
                                                  void   * /* aArg */ );

    /* BUG-48132 */
    static IDE_RC changeOPTIMIZER_PLAN_HASH_OR_SORT_METHOD ( idvSQL * /* aStatistics */,
                                                             SChar  * /* aName */,
                                                             void   * /* aOldValue */,
                                                             void   * aNewValue,
                                                             void   * /* aArg */ );

    /* BUG-48135 */
    static IDE_RC changeOPTIMIZER_INDEX_NL_JOIN_PENALTY ( idvSQL * /* aStatistics */,
                                                          SChar  * /* aName */,
                                                          void   * /* aOldValue */,
                                                          void   * aNewValue,
                                                          void   * /* aArg */ );

    /* BUG-48120 */
    static IDE_RC changeOPTIMIZER_INDEX_COST_MODE ( idvSQL * /* aStatistics */,
                                                    SChar  * /* aName */,
                                                    void   * /* aOldValue */,
                                                    void   * aNewValue,
                                                    void   * /* aArg */ );

    /* BUG-48776 */
    static IDE_RC changeSUBQUERY_MODE( idvSQL * /* aStatistics */,
                                       SChar  * /* aName */,
                                       void   * /* aOldValue */,
                                       void   * aNewValue,
                                       void   * /* aArg */ );

    /* PROJ-2750 */
    static IDE_RC changeLEFT_OUTER_SKIP_RIGHT_ENABLE( idvSQL* /* aStatistics */,
                                                      SChar * aName,
                                                      void  * aOldValue,
                                                      void  * aNewValue,
                                                      void  * aArg );    
};

#endif /* _O_QCU_PROPERTY_H_ */

