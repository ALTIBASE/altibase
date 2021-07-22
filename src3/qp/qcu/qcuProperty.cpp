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
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: qcuProperty.cpp 90785 2021-05-06 07:26:22Z hykim $
 *
 * QP 및 MM에서 사용할 System Property에 대한 정의
 * A4에서 제공하는 Property 관리자를 이용하여 관리한다.
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qc.h>
#include <qcuProperty.h>
#include <qdc.h>

qcuProperties    qcuProperty::mStaticProperty;

IDE_RC qcuProperty::initProperty( idvSQL */*aStatistics*/ )
{
    IDE_TEST( load() != IDE_SUCCESS );
    IDE_TEST( setupUpdateCallback() != IDE_SUCCESS );

    // BUG-43533 OPTIMIZER_FEATURE_ENABLE
    IDE_TEST( qdc::changeFeatureProperty4Startup( QCU_PROPERTY(mOptimizerFeatureEnable) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcuProperty::finalProperty( idvSQL */*aStatistics*/ )
{
    return IDE_SUCCESS;
}

IDE_RC qcuProperty::load()
{
/***********************************************************************
 *
 * Description :
 *    Server 구동 시 System Property들을 Loading한다.
 *
 * Implementation :
 *    Writable Property의 경우 CallBack을 등록해야 함.
 *
 ***********************************************************************/

    SChar  * sSecurityModuleName;
    SChar  * sSecurityModuleLibrary;
    SChar  * sSecurityEccPolicyName;
    UInt     sLength;
    SChar  * sSysdateForNATC;
    SChar  * sPasswVerifyFunc = NULL;
    SChar  * sOptimizerFeatureEnable;

    //--------------------------------------------------------
    // Trace Log 관련 property loading
    //    - Writable Property이므로 CallBack을 등록한다.
    //    - Atomic Operation이므로, BeforeCallBack은 필요 없다.
    //--------------------------------------------------------

    // TRCLOG_DML_SENTENCE 관련 설정
    IDE_ASSERT( idp::read( "TRCLOG_DML_SENTENCE",
                           & QCU_PROPERTY(mTraceLog_DML_Sentence) ) == IDE_SUCCESS );

    // TRCLOG_DETAIL_PREDICATE 관련 설정
    IDE_ASSERT( idp::read( "TRCLOG_DETAIL_PREDICATE",
                           & QCU_PROPERTY(mTraceLog_Detail_Predicate) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "TRCLOG_DETAIL_MTRNODE",
                           & QCU_PROPERTY(mTraceLog_Detail_MtrNode) ) == IDE_SUCCESS );

    // TRCLOG_EXPLAIN_GRAPH 관련 설정
    IDE_ASSERT( idp::read( "TRCLOG_EXPLAIN_GRAPH",
                           & QCU_PROPERTY(mTraceLog_Explain_Graph) ) == IDE_SUCCESS );

    // PROJ-2179
    IDE_ASSERT( idp::read( "TRCLOG_RESULT_DESC",
                           & QCU_PROPERTY(mTraceLog_Result_Desc) ) == IDE_SUCCESS );

    // BUG-38192
    IDE_ASSERT( idp::read( "TRCLOG_DISPLAY_CHILDREN",
                           & QCU_PROPERTY(mTraceLogDisplayChildren) ) == IDE_SUCCESS );

    // UPDATE_IN_PLACE 관련 설정
    IDE_ASSERT( idp::read( "UPDATE_IN_PLACE",
                           & QCU_PROPERTY(mUpdateInPlace) ) == IDE_SUCCESS );

    //--------------------------------------------------------
    // Query Processor 관련 Property Loading
    //
    //    - Read Only Property이므로 CallBack이 필요 없음.
    //--------------------------------------------------------

    IDE_ASSERT( idp::read( "NORMALFORM_MAXIMUM",
                           & QCU_PROPERTY(mNormalFormMaximum) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "EXEC_DDL_DISABLE",
                           & QCU_PROPERTY(mExecDDLDisable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "ISOLATION_LEVEL",
                           & QCU_PROPERTY(mIsolationLevel) ) == IDE_SUCCESS );

    // PR-14325
    IDE_ASSERT( idp::read( "REPLICATION_UPDATE_PK",
                           & QCU_PROPERTY(mReplUpdatePK) ) == IDE_SUCCESS );

    // PROJ-1557
    IDE_ASSERT( idp::read( "MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE",
                           & QCU_PROPERTY(mMemVarColumnInRowSize) ) == IDE_SUCCESS );

    // PROJ-1362
    IDE_ASSERT( idp::read( "MEMORY_LOB_COLUMN_IN_ROW_SIZE",
                           & QCU_PROPERTY(mMemLobColumnInRowSize) ) == IDE_SUCCESS );

    // PROJ-1862 Disk In Mode LOB
    IDE_ASSERT( idp::read( "DISK_LOB_COLUMN_IN_ROW_SIZE",
                           & QCU_PROPERTY(mDiskLobColumnInRowSize) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "_STORED_PROC_MODE",
                           & QCU_PROPERTY(mSpMode) ) == IDE_SUCCESS);

    // Default EXTENT MANAGEMENT TYPE PROJ-1671
    IDE_ASSERT( idp::read( "DEFAULT_SEGMENT_MANAGEMENT_TYPE",
                           & QCU_PROPERTY(mSegMgmtType) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DEFAULT_SEGMENT_STORAGE_INITEXTENTS",
                           & QCU_PROPERTY(mSegStoInitExtCnt) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DEFAULT_SEGMENT_STORAGE_NEXTEXTENTS",
                           & QCU_PROPERTY(mSegStoNextExtCnt) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DEFAULT_SEGMENT_STORAGE_MINEXTENTS",
                           & QCU_PROPERTY(mSegStoMinExtCnt) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DEFAULT_SEGMENT_STORAGE_MAXEXTENTS",
                           & QCU_PROPERTY(mSegStoMaxExtCnt) ) == IDE_SUCCESS );

    // PCTFREE
    IDE_ASSERT( idp::read( "PCTFREE",
                           & QCU_PROPERTY(mPctFree) ) == IDE_SUCCESS );

    // PCTUSED
    IDE_ASSERT( idp::read( "PCTUSED",
                           & QCU_PROPERTY(mPctUsed) ) == IDE_SUCCESS );

    if( (QCU_PROPERTY(mPctFree) + QCU_PROPERTY(mPctUsed)) > 100 )
    {
        IDE_RAISE( ERR_INVALID_PCTFREE_PCTUSED_VALUE );
    }

    // TABLE_INITRANS
    IDE_ASSERT( idp::read( "TABLE_INITRANS",
                           & QCU_PROPERTY(mTableInitTrans) ) == IDE_SUCCESS );
    // TABLE_MAXTRANS
    IDE_ASSERT( idp::read( "TABLE_MAXTRANS",
                           & QCU_PROPERTY(mTableMaxTrans) ) == IDE_SUCCESS );

    if ( QCU_PROPERTY(mTableMaxTrans) < QCU_PROPERTY(mTableInitTrans) )
    {
        IDE_RAISE( err_fatal_invalid_ttl_size );
    }

    // INDEX_INITRANS
    IDE_ASSERT( idp::read( "INDEX_INITRANS",
                           & QCU_PROPERTY(mIndexInitTrans) ) == IDE_SUCCESS );
    // INDEX_MAXTRANS
    IDE_ASSERT( idp::read( "INDEX_MAXTRANS",
                           & QCU_PROPERTY(mIndexMaxTrans) ) == IDE_SUCCESS );

    // BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
    // 관련프로퍼티를 참조하지 못하는 경우 있음.
    IDE_ASSERT( idp::read( "OPTIMIZER_MODE",
                           & QCU_PROPERTY(mOptimizerMode) ) == IDE_SUCCESS );

    // AUTO_REMOTE_EXEC
    IDE_ASSERT( idp::read( "AUTO_REMOTE_EXEC",
                           & QCU_PROPERTY(mAutoRemoteExec) ) == IDE_SUCCESS );

    // PROJ-1358 Query Stack Size의 조정
    IDE_ASSERT( idp::read( "QUERY_STACK_SIZE",
                           & QCU_PROPERTY(mQueryStackSize) ) == IDE_SUCCESS );

    // PR-13395 가상 통계 정보 구축
    // TPC-H Scale Factor
    IDE_ASSERT( idp::read( "__QP_FAKE_STAT_TPCH_SCALE_FACTOR",
                           & QCU_PROPERTY(mFakeTpchScaleFactor) ) == IDE_SUCCESS );

    // PR-13395 가상 통계 정보 구축
    // 가상 Buffer Size
    IDE_ASSERT( idp::read( "__QP_FAKE_STAT_BUFFER_SIZE",
                           & QCU_PROPERTY(mFakeBufferSize) ) == IDE_SUCCESS );

    // BUG-13068 filehandle open limit 조정
    IDE_ASSERT( idp::read( "PSM_FILE_OPEN_LIMIT",
                           & QCU_PROPERTY(mFileOpenLimit) ) == IDE_SUCCESS );
    
    // BUG-40854
    IDE_ASSERT( idp::read( "CONNECT_TYPE_OPEN_LIMIT",
                           & QCU_PROPERTY(mSocketOpenLimit) ) == IDE_SUCCESS );

    /* BUG-41307 User Lock 지원 */
    IDE_ASSERT( idp::read( "USER_LOCK_POOL_INIT_SIZE",
                           & QCU_PROPERTY(mUserLockPoolInitSize) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "USER_LOCK_REQUEST_TIMEOUT",
                           & QCU_PROPERTY(mUserLockRequestTimeout) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "USER_LOCK_REQUEST_CHECK_INTERVAL",
                           & QCU_PROPERTY(mUserLockRequestCheckInterval) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "USER_LOCK_REQUEST_LIMIT",
                           & QCU_PROPERTY(mUserLockRequestLimit) ) == IDE_SUCCESS );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    IDE_ASSERT( idp::read( "HOST_OPTIMIZE_ENABLE",
                           &QCU_PROPERTY(mHostOptimizeEnable) ) == IDE_SUCCESS );

    // BUG-18851 disable transitive predicate generation
    // Read Only Property이므로 CallBack이 필요 없음.
    IDE_ASSERT( idp::read( "__OPTIMIZER_TRANSITIVITY_DISABLE",
                           &QCU_PROPERTY(mOptimizerTransitivityDisable) ) == IDE_SUCCESS );

    // PROJ-1473
    IDE_ASSERT( idp::read( "__OPTIMIZER_PUSH_PROJECTION",
                           &QCU_PROPERTY(mOptimizerPushProjection) ) == IDE_SUCCESS );

    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    IDE_ASSERT( idp::read( "__OPTIMIZER_DEFAULT_TEMP_TBS_TYPE",
                           &QCU_PROPERTY(mOptimizerDefaultTempTbsType) ) == IDE_SUCCESS );

    // BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
    IDE_ASSERT( idp::read( "__OPTIMIZER_FIXED_GROUP_MEMORY_TEMP",
                           &QCU_PROPERTY(mOptimizerFixedGroupMemoryTemp) ) == IDE_SUCCESS );

    // BUG-38339 Outer Join Elimination
    IDE_ASSERT( idp::read( "__OPTIMIZER_OUTERJOIN_ELIMINATION",
                           &QCU_PROPERTY(mOptimizerOuterJoinElimination) ) == IDE_SUCCESS );

    // PROJ-1413 Simple View Merging
    IDE_ASSERT( idp::read( "__OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE",
                           &QCU_PROPERTY(mOptimizerSimpleViewMergingDisable) ) == IDE_SUCCESS );

    //PROJ-1583 large geometry
    IDE_ASSERT( idp::read( "ST_OBJECT_BUFFER_SIZE",
                           & QCU_PROPERTY(mSTObjBufSize) ) == IDE_SUCCESS );

    // BUG-19089
    // FK가 있는 상태에서 CREATE REPLICATION 구문이 가능하도록 한다.
    IDE_ASSERT( idp::read( "CHECK_FK_IN_CREATE_REPLICATION_DISABLE",
                           & QCU_PROPERTY(mCheckFkInCreateReplicationDisable) ) == IDE_SUCCESS );

    // PROJ-1436
    IDE_ASSERT( idp::read( "SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT",
                           & QCU_PROPERTY(mSqlPlanCachePreparedExecutionContextCnt) ) == IDE_SUCCESS );

    // PROJ-2002 Column Security
    QCU_PROPERTY(mSecurityModuleName)[0] = '\0';

    if ( idp::readPtr( "SECURITY_MODULE_NAME",
                       (void**) &sSecurityModuleName ) == IDE_SUCCESS )
    {
        if ( sSecurityModuleName != NULL )
        {
            sLength = idlOS::strlen( sSecurityModuleName );

            if ( sLength <= QCS_MODULE_NAME_SIZE )
            {
                idlOS::strncpy( QCU_PROPERTY(mSecurityModuleName),
                                sSecurityModuleName,
                                sLength );
                QCU_PROPERTY(mSecurityModuleName)[sLength] = '\0';
            }
        }
    }

    // PROJ-2002 Column Security
    QCU_PROPERTY(mSecurityModuleLibrary)[0] = '\0';

    if ( idp::readPtr( "SECURITY_MODULE_LIBRARY",
                       (void**) &sSecurityModuleLibrary ) == IDE_SUCCESS )
    {
        if ( sSecurityModuleLibrary != NULL )
        {
            sLength = idlOS::strlen( sSecurityModuleLibrary );

            if ( sLength <= QCS_MODULE_LIBRARY_SIZE )
            {
                idlOS::strncpy( QCU_PROPERTY(mSecurityModuleLibrary),
                                sSecurityModuleLibrary,
                                sLength );
                QCU_PROPERTY(mSecurityModuleLibrary)[sLength] = '\0';
            }
        }
    }

    // PROJ-2002 Column Security
    QCU_PROPERTY(mSecurityEccPolicyName)[0] = '\0';

    if ( idp::readPtr( "SECURITY_ECC_POLICY_NAME",
                       (void**) &sSecurityEccPolicyName ) == IDE_SUCCESS )
    {
        if ( sSecurityEccPolicyName != NULL )
        {
            sLength = idlOS::strlen( sSecurityEccPolicyName );

            if ( sLength <= QCS_POLICY_NAME_SIZE )
            {
                idlOS::strncpy( QCU_PROPERTY(mSecurityEccPolicyName),
                                sSecurityEccPolicyName,
                                sLength );
                QCU_PROPERTY(mSecurityEccPolicyName)[sLength] = '\0';
            }
        }
    }

    // BUG-29209 : natc test를 위하여 Plan Display에서
    //             특정 정보( DISK_PAGE_COUNT, ITEM_SIZE)를 보여주지
    //             않게 하는 프로퍼티
    IDE_ASSERT( idp::read( "__DISPLAY_PLAN_FOR_NATC",
                           & QCU_PROPERTY(mDisplayPlanForNATC) ) == IDE_SUCCESS );

    // BUG-31040
    IDE_ASSERT( idp::read( "__MAX_SET_OP_RECURSION_DEPTH",
                           & QCU_PROPERTY(mMaxSetOpRecursionDepth) ) == IDE_SUCCESS);

    // BUG-32101
    IDE_ASSERT( idp::read( "OPTIMIZER_DISK_INDEX_COST_ADJ",
                           & QCU_PROPERTY(mOptimizerDiskIndexCostAdj) ) == IDE_SUCCESS);

    // BUG-32101
    IDE_ASSERT( idp::read( "OPTIMIZER_MEMORY_INDEX_COST_ADJ",
                           & QCU_PROPERTY(mOptimizerMemoryIndexCostAdj) ) == IDE_SUCCESS);

    // BUG-34441
    IDE_ASSERT(idp::read( "OPTIMIZER_HASH_JOIN_COST_ADJ",
                          & QCU_PROPERTY(mOptimizerHashJoinCostAdj) ) == IDE_SUCCESS);

    // BUG-34235
    IDE_ASSERT(idp::read( "OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD",
                          & QCU_PROPERTY(mOptimizerSubqueryOptimizeMethod) ) == IDE_SUCCESS);

    // fix BUG-33589
    IDE_ASSERT( idp::read( "PLAN_REBUILD_ENABLE",
                           & QCU_PROPERTY(mPlanRebuildEnable) ) == IDE_SUCCESS );

    /* PROJ-2361 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_AVG_TRANSFORM_ENABLE",
                           & QCU_PROPERTY(mAvgTransformEnable) ) == IDE_SUCCESS );

    /* PROJ-1071 Parallel Query */
    IDE_ASSERT(idp::read( "PARALLEL_QUERY_THREAD_MAX",
                          & QCU_PROPERTY(mParallelQueryThreadMax) ) == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "PARALLEL_QUERY_QUEUE_SLEEP_MAX",
                          & QCU_PROPERTY(mParallelQueryQueueSleepMax) ) == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "PARALLEL_QUERY_QUEUE_SIZE",
                          & QCU_PROPERTY(mParallelQueryQueueSize) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__FORCE_PARALLEL_DEGREE",
                           & QCU_PROPERTY(mForceParallelDegree) ) == IDE_SUCCESS);

    /* PROJ-2207 Password policy support */
    QCU_PROPERTY(mSysdateForNATC)[0] = '\0';

    if ( idp::readPtr( "__SYSDATE_FOR_NATC",
                       (void**) &sSysdateForNATC ) == IDE_SUCCESS )
    {
        if ( sSysdateForNATC != NULL )
        {
            sLength = idlOS::strlen( sSysdateForNATC );

            if ( (sLength > 0) && (sLength <= QTC_SYSDATE_FOR_NATC_LEN) )
            {
                idlOS::strncpy( QCU_PROPERTY(mSysdateForNATC),
                                sSysdateForNATC,
                                sLength );
                QCU_PROPERTY(mSysdateForNATC)[sLength] = '\0';
            }
        }
    }

    IDE_ASSERT( idp::read( "FAILED_LOGIN_ATTEMPTS",
                           & QCU_PROPERTY(mFailedLoginAttempts) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "PASSWORD_LOCK_TIME",
                           & QCU_PROPERTY(mPasswLockTime) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "PASSWORD_LIFE_TIME",
                           & QCU_PROPERTY(mPasswLifeTime) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "PASSWORD_GRACE_TIME",
                           & QCU_PROPERTY(mPasswGraceTime) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "PASSWORD_REUSE_MAX",
                           & QCU_PROPERTY(mPasswReuseMax) ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "PASSWORD_REUSE_TIME",
                           & QCU_PROPERTY(mPasswReuseTime) ) == IDE_SUCCESS );

    QCU_PROPERTY(mPasswVerifyFunc)[0] = '\0';

    if ( idp::readPtr( "PASSWORD_VERIFY_FUNCTION",
                       (void**) &sPasswVerifyFunc ) == IDE_SUCCESS )
    {
        if ( sPasswVerifyFunc != NULL )
        {
            sLength = idlOS::strlen( sPasswVerifyFunc );

            if ( sLength <= QC_PASSWORD_OPT_LEN )
            {
                idlOS::strncpy( QCU_PROPERTY(mPasswVerifyFunc),
                                sPasswVerifyFunc,
                                sLength );
                QCU_PROPERTY(mPasswVerifyFunc)[sLength] = '\0';
            }
        }
    }

    // BUG-34234
    IDE_ASSERT( idp::read( "COERCE_HOST_VAR_IN_SELECT_LIST_TO_VARCHAR",
                           & QCU_PROPERTY(mCoerceHostVarToVarchar) ) == IDE_SUCCESS );

    // BUG-34295
    IDE_ASSERT( idp::read( "OPTIMIZER_ANSI_JOIN_ORDERING",
                           & QCU_PROPERTY(mOptimizerAnsiJoinOrdering) ) == IDE_SUCCESS);
    // BUG-38402
    IDE_ASSERT( idp::read( "__OPTIMIZER_ANSI_INNER_JOIN_CONVERT",
                           & QCU_PROPERTY(mOptimizerAnsiInnerJoinConvert) ) == IDE_SUCCESS);

    // BUG-34350
    IDE_ASSERT( idp::read( "OPTIMIZER_REFINE_PREPARE_MEMORY",
                           & QCU_PROPERTY(mOptimizerRefinePrepareMemory) ) == IDE_SUCCESS);

    // BUG-35155
    IDE_ASSERT( idp::read( "OPTIMIZER_PARTIAL_NORMALIZE",
                           & QCU_PROPERTY(mOptimizerPartialNormalize) ) == IDE_SUCCESS);

    // PROJ-2264 Dictionary table
    IDE_ASSERT( idp::read( "__FORCE_COMPRESSION_COLUMN",
                           & QCU_PROPERTY(mForceCompressionColumn) ) == IDE_SUCCESS);

    /* PROJ-1090 Function-based Index */
    IDE_ASSERT( idp::read( "QUERY_REWRITE_ENABLE",
                           & QCU_PROPERTY(mQueryRewriteEnable) ) == IDE_SUCCESS );

    // BUG-35713
    IDE_ASSERT( idp::read( "PSM_IGNORE_NO_DATA_FOUND_ERROR",
                           & QCU_PROPERTY(mPSMIgnoreNoDataFoundError) ) == IDE_SUCCESS );

    // PROJ-1718
    IDE_ASSERT( idp::read( "OPTIMIZER_UNNEST_SUBQUERY",
                           & QCU_PROPERTY(mOptimizerUnnestSubquery) ) == IDE_SUCCESS);

    IDE_ASSERT( idp::read( "OPTIMIZER_UNNEST_COMPLEX_SUBQUERY",
                           & QCU_PROPERTY(mOptimizerUnnestComplexSubquery) ) == IDE_SUCCESS);

    IDE_ASSERT( idp::read( "OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY",
                           & QCU_PROPERTY(mOptimizerUnnestAggregationSubquery) ) == IDE_SUCCESS);

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    IDE_ASSERT( idp::read( "__INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT",
                           & QCU_PROPERTY(mIntermediateTupleLobObjectLimit) ) == IDE_SUCCESS );

    /* PROJ-2242 CSE, CFS, Feature enable */
    IDE_ASSERT( idp::read( "__OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION",
                           & QCU_PROPERTY(mOptimizerEliminateCommonSubexpression) )
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( "__OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION",
                           & QCU_PROPERTY(mOptimizerConstantFilterSubsumption) )
                == IDE_SUCCESS);

    QCU_PROPERTY(mOptimizerFeatureEnable)[0] = '\0';

    if ( idp::readPtr( "OPTIMIZER_FEATURE_ENABLE",
                       (void**) &sOptimizerFeatureEnable ) == IDE_SUCCESS )
    {
        if( sOptimizerFeatureEnable != NULL )
        {
            sLength = idlOS::strlen( sOptimizerFeatureEnable );

            if( ( sLength > 0 ) &&
                ( sLength <= QCU_OPTIMIZER_FEATURE_VERSION_LEN ) )
            {
                idlOS::strncpy( QCU_PROPERTY(mOptimizerFeatureEnable),
                                sOptimizerFeatureEnable,
                                sLength );
                QCU_PROPERTY(mOptimizerFeatureEnable)[sLength] = '\0';
            }
        }
    }

    IDE_ASSERT( idp::read( "__DML_WITHOUT_RETRY_ENABLE",
                           & QCU_PROPERTY(mDmlWithoutRetryEnable) ) == IDE_SUCCESS);

    // BUG-36203 PSM Optimize
    IDE_ASSERT( idp::read( "PSM_TEMPLATE_CACHE_COUNT",
                           & QCU_PROPERTY(mPSMTemplateCacheCount) ) == IDE_SUCCESS );

    // fix BUG-36522
    IDE_ASSERT( idp::read( "__PSM_SHOW_ERROR_STACK",
                           & QCU_PROPERTY(mPSMShowErrorStack) ) == IDE_SUCCESS );

    // fix BUG-36793
    IDE_ASSERT( idp::read( "__BIND_PARAM_DEFAULT_PRECISION",
                           & QCU_PROPERTY(mBindParamDefaultPrecision) ) == IDE_SUCCESS );

    // BUG-37247
    IDE_ASSERT( idp::read( "SYS_CONNECT_BY_PATH_PRECISION",
                           & QCU_PROPERTY(mSysConnectByPathPrecision) ) == IDE_SUCCESS );

    // BUG-37247
    IDE_ASSERT( idp::read( "EXECUTION_PLAN_MEMORY_CHECK",
                           &QCU_PROPERTY(mExecutionPlanMemoryCheck) ) == IDE_SUCCESS );

    // BUG-37302
    IDE_ASSERT( idp::read( "SQL_ERROR_INFO_SIZE",
                           & QCU_PROPERTY(mSQLErrorInfoSize) ) == IDE_SUCCESS );

    // PROJ-2362
    IDE_ASSERT( idp::read( "REDUCE_TEMP_MEMORY_ENABLE",
                           & QCU_PROPERTY(mReduceTempMemoryEnable) ) == IDE_SUCCESS );

    // BUG-38434
    IDE_ASSERT( idp::read( "__OPTIMIZER_DNF_DISABLE",
                           & QCU_PROPERTY(mOptimizerDnfDisable) ) == IDE_SUCCESS);

    /* BUG-38952 type null */
    IDE_ASSERT( idp::read( "TYPE_NULL",
                           & QCU_PROPERTY(mTypeNull) ) == IDE_SUCCESS );

    /* BUG-38946 display name */
    IDE_ASSERT( idp::read( "COMPATIBLE_DISPLAY_NAME",
                           & QCU_PROPERTY(mCompatDisplayName) ) == IDE_SUCCESS );

    // fix BUG-39754    
    IDE_ASSERT( idp::read( "__FOREIGN_KEY_LOCK_ROW", &QCU_PROPERTY(mForeignKeyLockRow) )
                == IDE_SUCCESS);

    // BUG-40022
    IDE_ASSERT( idp::read( "__OPTIMIZER_JOIN_DISABLE",
                           & QCU_PROPERTY(mOptimizerJoinDisable) ) == IDE_SUCCESS);

    // BUG-40042 oracle outer join property
    IDE_ASSERT( idp::read( "OUTER_JOIN_OPERATOR_TRANSFORM_ENABLE",
                           & QCU_PROPERTY(mOuterJoinOperToAnsiJoinTransEnable) ) == IDE_SUCCESS );

    // PROJ-2469 Optimize View Materialization
    IDE_ASSERT( idp::read( "__OPTIMIZER_VIEW_TARGET_ENABLE",
                           & QCU_PROPERTY(mOptimizerViewTargetEnable) ) == IDE_SUCCESS);

    /* PROJ-2451 Concurrent Execute Package */
    IDE_ASSERT(idp::read( "CONCURRENT_EXEC_DEGREE_MAX",
                          & QCU_PROPERTY(mConcExecDegreeMax) ) == IDE_SUCCESS );

    /* PROJ-2451 Concurrent Execute Package */
    IDE_ASSERT(idp::read( "CONCURRENT_EXEC_DEGREE_DEFAULT",
                          & QCU_PROPERTY(mConcExecDegreeDefault) ) == IDE_SUCCESS );

    /* PROJ-2451 Concurrent Execute Package */
    IDE_ASSERT(idp::read( "CONCURRENT_EXEC_WAIT_INTERVAL",
                          & QCU_PROPERTY(mConcExecWaitInterval) ) == IDE_SUCCESS );

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    IDE_ASSERT( idp::read( "__QUERY_EXECUTION_CACHE_MAX_COUNT",
                           & QCU_PROPERTY(mQueryExecutionCacheMaxCount) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__QUERY_EXECUTION_CACHE_MAX_SIZE",
                           & QCU_PROPERTY(mQueryExecutionCacheMaxSize) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__QUERY_EXECUTION_CACHE_BUCKET_COUNT",
                           & QCU_PROPERTY(mQueryExecutionCacheBucketCount) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__FORCE_FUNCTION_CACHE",
                           & QCU_PROPERTY(mForceFunctionCache) ) == IDE_SUCCESS );

    /* PROJ-2441 flashback */
    IDE_ASSERT( idp::read( "RECYCLEBIN_MEM_MAX_SIZE",
                           & QCU_PROPERTY(mRecyclebinMemMaxSize) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "RECYCLEBIN_DISK_MAX_SIZE",
                           & QCU_PROPERTY(mRecyclebinDiskMaxSize) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "RECYCLEBIN_ENABLE",
                           & QCU_PROPERTY(mRecyclebinEnable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__RECYCLEBIN_FOR_NATC",
                           & QCU_PROPERTY(mRecyclebinNATC) ) == IDE_SUCCESS );

    /* PROJ-2448 Subquery caching */
    IDE_ASSERT( idp::read( "__FORCE_SUBQUERY_CACHE_DISABLE",
                           & QCU_PROPERTY(mForceSubqueryCacheDisable) ) == IDE_SUCCESS );

    // PROJ-2551 simple query 최적화
    IDE_ASSERT( idp::read( "EXECUTOR_FAST_SIMPLE_QUERY",
                           & QCU_PROPERTY(mExecutorFastSimpleQuery) ) == IDE_SUCCESS );

    // BUG-41183 ORDER BY Elimination
    IDE_ASSERT( idp::read( "__OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE",
                           & QCU_PROPERTY(mOptimizerOrderByEliminationEnable) ) == IDE_SUCCESS );
    
    // PROJ-2553
    IDE_ASSERT( idp::read( "HASH_JOIN_MEM_TEMP_PARTITIONING_DISABLE", 
                           & QCU_PROPERTY(mHashJoinMemTempPartitioningDisable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "HASH_JOIN_MEM_TEMP_AUTO_BUCKET_COUNT_DISABLE", 
                           & QCU_PROPERTY(mHashJoinMemTempAutoBucketCntDisable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__HASH_JOIN_MEM_TEMP_TLB_ENTRY_COUNT", 
                           & QCU_PROPERTY(mHashJoinMemTempTLBCnt) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__FORCE_HASH_JOIN_MEM_TEMP_FANOUT_MODE", 
                           & QCU_PROPERTY(mForceHashJoinMemTempFanoutMode) ) == IDE_SUCCESS );

    // BUG-41398 use old sort
    IDE_ASSERT( idp::read( "__USE_OLD_SORT",
                           & QCU_PROPERTY(mUseOldSort) ) == IDE_SUCCESS );

    IDE_ASSERT(idp::read("DDL_SUPPLEMENTAL_LOG_ENABLE",
                         & QCU_PROPERTY(mDDLSupplementalLog) ) == IDE_SUCCESS);

    // BUG-41249 DISTINCT Elimination
    IDE_ASSERT( idp::read( "__OPTIMIZER_DISTINCT_ELIMINATION_ENABLE",
                           & QCU_PROPERTY(mOptimizerDistinctEliminationEnable) ) == IDE_SUCCESS );

    /* PROJ-2462 Result Cache */
    IDE_ASSERT( idp::read( "RESULT_CACHE_ENABLE",
                           &QCU_PROPERTY( mResultCacheEnable ) ) == IDE_SUCCESS);
    /* PROJ-2462 Result Cache */
    IDE_ASSERT( idp::read( "TOP_RESULT_CACHE_MODE",
                           &QCU_PROPERTY( mTopResultCacheMode ) ) == IDE_SUCCESS);
    /* PROJ-2462 Result Cache */
    IDE_ASSERT( idp::read( "RESULT_CACHE_MEMORY_MAXIMUM",
                           &QCU_PROPERTY( mResultCacheMemoryMax ) ) == IDE_SUCCESS);
    /* PROJ-2462 Result Cache */
    IDE_ASSERT( idp::read( "TRCLOG_DETAIL_RESULTCACHE",
                           &QCU_PROPERTY( mTraceLog_Detail_ResultCache ) ) == IDE_SUCCESS );
    /* PROJ-2492 Dynamic sample selection */
    IDE_ASSERT( idp::read( "OPTIMIZER_AUTO_STATS",
                           &QCU_PROPERTY( mOptimizerAutoStats ) ) == IDE_SUCCESS );
    // BUG-36438 LIST transformation
    IDE_ASSERT( idp::read( "__OPTIMIZER_LIST_TRANSFORMATION",
                           &QCU_PROPERTY(mOptimizerListTransformation) ) == IDE_SUCCESS );
    
    // BUG-41248 DBMS_SQL package
    IDE_ASSERT( idp::read( "PSM_CURSOR_OPEN_LIMIT",
                           & QCU_PROPERTY(mPSMCursorOpenLimit) ) == IDE_SUCCESS );

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    IDE_ASSERT( idp::read( "__OPTIMIZER_TRANSITIVITY_OLD_RULE",
                           &QCU_PROPERTY(mOptimizerTransitivityOldRule) ) == IDE_SUCCESS );
    
    // PROJ-2582 recursive with
    IDE_ASSERT( idp::read( "RECURSION_LEVEL_MAXIMUM",
                           &QCU_PROPERTY( mRecursionLevelMaximum ) ) == IDE_SUCCESS );
   
    // fix BUG-42752
    IDE_ASSERT( idp::read( "__OPTIMIZER_ESTIMATE_KEY_FILTER_SELECTIVITY",
                           &QCU_PROPERTY( mOptimizerEstimateKeyFilterSelectivity ) ) == IDE_SUCCESS );
 
    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_ASSERT( idp::read( "DDL_MEM_USAGE_THRESHOLD",
                           & QCU_PROPERTY( mDDLMemUsageThreshold ) ) == IDE_SUCCESS );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_ASSERT( idp::read( "DDL_TBS_USAGE_THRESHOLD",
                           & QCU_PROPERTY( mDDLTBSUsageThreshold ) ) == IDE_SUCCESS );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_ASSERT( idp::read( "ANALYZE_USAGE_MIN_ROWCOUNT",
                           & QCU_PROPERTY( mAnalyzeUsageMinRowCount ) ) == IDE_SUCCESS );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_ASSERT( idp::read( "MEM_MAX_DB_SIZE",
                           & QCU_PROPERTY( mMemMaxDBSize ) ) == IDE_SUCCESS );

    /* PROJ-2586 PSM Parameters and return without precision */
    IDE_ASSERT( idp::read( "PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE",
                           &QCU_PROPERTY( mPSMParamAndReturnWithoutPrecisionEnable ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "PSM_CHAR_DEFAULT_PRECISION",
                           &QCU_PROPERTY( mPSMCharDefaultPrecision ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "PSM_VARCHAR_DEFAULT_PRECISION",
                           &QCU_PROPERTY( mPSMVarcharDefaultPrecision ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "PSM_NCHAR_UTF16_DEFAULT_PRECISION",
                           &QCU_PROPERTY( mPSMNcharUTF16DefaultPrecision ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "PSM_NVARCHAR_UTF16_DEFAULT_PRECISION",
                           &QCU_PROPERTY( mPSMNvarcharUTF16DefaultPrecision ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "PSM_NCHAR_UTF8_DEFAULT_PRECISION",
                           &QCU_PROPERTY( mPSMNcharUTF8DefaultPrecision ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "PSM_NVARCHAR_UTF8_DEFAULT_PRECISION",
                           &QCU_PROPERTY( mPSMNvarcharUTF8DefaultPrecision ) )
                == IDE_SUCCESS );

    /* BUG-42639 Monitoring query */
    IDE_ASSERT( idp::read( "OPTIMIZER_PERFORMANCE_VIEW",
                           &QCU_PROPERTY( mOptimizerPerformanceView ) ) == IDE_SUCCESS );

    // BUG-42322
    IDE_ASSERT( idp::read( "__PSM_FORMAT_CALL_STACK_OID",
                           & QCU_PROPERTY( mPSMFormatCallStackOID ) ) == IDE_SUCCESS );

    // BUG-43039 inner join push down
    IDE_ASSERT( idp::read( "__OPTIMIZER_INNER_JOIN_PUSH_DOWN",
                           &QCU_PROPERTY( mOptimizerInnerJoinPushDown ) ) == IDE_SUCCESS );

    // BUG-43068 Indexable order by 개선
    IDE_ASSERT( idp::read( "__OPTIMIZER_ORDER_PUSH_DOWN",
                           &QCU_PROPERTY( mOptimizerOrderPushDown ) ) == IDE_SUCCESS );

    // BUG-43059 Target subquery unnest/removal disable
    IDE_ASSERT( idp::read( "__OPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE",
                           &QCU_PROPERTY( mOptimizerTargetSubqueryUnnestDisable ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__OPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE",
                           &QCU_PROPERTY( mOptimizerTargetSubqueryRemovalDisable ) ) == IDE_SUCCESS );

    /* BUG-43112 */
    IDE_ASSERT( idp::read( "__FORCE_AUTONOMOUS_TRANSACTION_PRAGMA",
                           & QCU_PROPERTY( mForceAutonomousTransactionPragma ) )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__AUTONOMOUS_TRANSACTION_PRAGMA_DISABLE",
                           & QCU_PROPERTY( mAutonomousTransactionPragmaDisable ) )
                == IDE_SUCCESS );

    // BUG-43258
    IDE_ASSERT( idp::read( "__OPTIMIZER_INDEX_CLUSTERING_FACTOR_ADJ",
                           &QCU_PROPERTY( mOptimizerIndexClusteringFactorAdj ) ) == IDE_SUCCESS );

    // BUG-43158 Enhance statement list caching in PSM
    IDE_ASSERT( idp::read( "__PSM_STATEMENT_LIST_COUNT",
                           &QCU_PROPERTY( mPSMStmtListCount ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__PSM_STATEMENT_POOL_COUNT",
                           &QCU_PROPERTY( mPSMStmtPoolCount ) ) == IDE_SUCCESS );

    QCU_PROPERTY( mPSMStmtPoolCount ) = idlOS::align( QCU_PROPERTY( mPSMStmtPoolCount ), 32 );

    /* BUG-42928 No Partition Lock */
    IDE_ASSERT( idp::read( "TABLE_LOCK_MODE",
                           & QCU_PROPERTY( mTableLockMode ) ) == IDE_SUCCESS );

    // BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
    IDE_ASSERT( idp::read( "__DISK_TEMP_SIZE_ESTIMATE",
                           &QCU_PROPERTY( mDiskTempSizeEstimate ) ) == IDE_SUCCESS );

    // BUG-43421
    IDE_ASSERT( idp::read( "__OPTIMIZER_SEMI_JOIN_TRANSITIVITY_DISABLE",
                           &QCU_PROPERTY(mOptimizerSemiJoinTransitivityDisable) ) == IDE_SUCCESS );

    // BUG-43493
    IDE_ASSERT( idp::read( "OPTIMIZER_DELAYED_EXECUTION",
                           &QCU_PROPERTY(mOptimizerDelayedExecution) ) == IDE_SUCCESS );

    /* BUG-43495 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE",
                           &QCU_PROPERTY(mOptimizerLikeIndexScanWithOuterColumnDisable) ) == IDE_SUCCESS );

    /* BUG-43737 */
    IDE_ASSERT( idp::read( "__FORCE_TABLESPACE_DEFAULT",
                           &QCU_PROPERTY(mForceTablespaceDefault) ) == IDE_SUCCESS );
    
    /* BUG-43769 */
    IDE_ASSERT( idp::read( "THREAD_CPU_AFFINITY",
                           &QCU_PROPERTY(mIsCPUAffinity) ) == IDE_SUCCESS );

    /* TASK-6744 */
    IDE_ASSERT( idp::read( "__PLAN_RANDOM_SEED",
                           &QCU_PROPERTY(mPlanRandomSeed) ) == IDE_SUCCESS );

    /* BUG-44499 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_BUCKET_COUNT_MIN",
                           &QCU_PROPERTY(mOptimizerBucketCountMin) ) == IDE_SUCCESS );

    /* BUG-48161 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_BUCKET_COUNT_MAX",
                           &QCU_PROPERTY(mOptimizerBucketCountMax) ) == IDE_SUCCESS );

    /* PROJ-2641 Hierarchy Query Index */
    IDE_ASSERT( idp::read( "__OPTIMIZER_HIERARCHY_TRANSFORMATION",
                           &QCU_PROPERTY(mOptimizerHierarchyTransformation) ) == IDE_SUCCESS );

    // BUG-44692
    IDE_ASSERT( idp::read( "BUG_44692",
                           &QCU_PROPERTY(mOptimizerBug44692) ) == IDE_SUCCESS );

    // BUG-44795
    IDE_ASSERT( idp::read( "__OPTIMIZER_DBMS_STAT_POLICY",
                           &QCU_PROPERTY(mOptimizerDBMSStatPolicy) ) == IDE_SUCCESS );

    /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
    IDE_ASSERT( idp::read( "__OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY",
                           &QCU_PROPERTY(mOptimizerIndexNLJoinAccessMethodPolicy) ) == IDE_SUCCESS);
    
    IDE_ASSERT( idp::read( "__OPTIMIZER_SEMI_JOIN_REMOVE",
                           &QCU_PROPERTY(mOptimizerSemiJoinRemove) ) == IDE_SUCCESS);
    
    IDE_ASSERT( idp::read( "__PRINT_OUT_ENABLE",
                           & QCU_PROPERTY(mPrintOutEnable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__KEY_PRESERVED_TABLE",
                           & QCU_PROPERTY(mKeyPreservedTable) ) == IDE_SUCCESS );

    /* BUG-46544 unnest hint */
    IDE_ASSERT( idp::read( "__OPTIMIZER_UNNEST_COMPATIBILITY",
                           &QCU_PROPERTY(mOptimizerUnnestCompatibility) ) == IDE_SUCCESS );

    /* PROJ-2632 */
    IDE_ASSERT( idp::read( "SERIAL_EXECUTE_MODE",
                           & QCU_PROPERTY( mSerialExecuteMode ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "TRCLOG_DETAIL_INFORMATION",
                           & QCU_PROPERTY( mTraceLogDetailInformation ) ) == IDE_SUCCESS );

    /* BUG-46932 */ 
    IDE_ASSERT( idp::read( "__OPTIMIZER_INVERSE_JOIN_ENABLE",
                           &QCU_PROPERTY(mOptimizerInverseJoinEnable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__REDUCE_PARTITION_PREPARE_MEMORY",
                           &QCU_PROPERTY(mReducePartPrepareMemory) ) == IDE_SUCCESS );

    /* BUG-47986 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_OR_VALUE_INDEX",
                           &QCU_PROPERTY(mOptimizerOrValueIndex) ) == IDE_SUCCESS );

    /* BUG-48132 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_PLAN_HASH_OR_SORT_METHOD",
                           &QCU_PROPERTY(mOptimizerPlanHashOrSortMethod) ) == IDE_SUCCESS );

    /* BUG-48135 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_INDEX_NL_JOIN_PENALTY",
                           &QCU_PROPERTY(mOptimizerIndexNlJoinPenalty) ) == IDE_SUCCESS );

    /* BUG-48120 */
    IDE_ASSERT( idp::read( "__OPTIMIZER_INDEX_COST_MODE",
                           &QCU_PROPERTY(mOptimizerIndexCostMode) ) == IDE_SUCCESS );

    /* BUG-48594 */
    IDE_ASSERT( idp::read( "__SQL_PLAN_CACHE_VALID_MODE",
                           &QCU_PROPERTY(mSqlPlanCacheValidMode) ) == IDE_SUCCESS );

    /* BUG-48776 */
    IDE_ASSERT( idp::read( "__SUBQUERY_MODE",
                           &QCU_PROPERTY(mSubqueryMode) ) == IDE_SUCCESS );

    /* PROJ-2750 */
    IDE_ASSERT( idp::read( "__LEFT_OUTER_SKIP_RIGHT_ENABLE",
                           &QCU_PROPERTY(mLeftOuterSkipRightEnable) ) == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_fatal_invalid_ttl_size);
    {
        IDE_SET(ideSetErrorCode( qpERR_FATAL_QCU_INVALID_TTL_SIZE_PROPERTY ) );
    }
    IDE_EXCEPTION(ERR_INVALID_PCTFREE_PCTUSED_VALUE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_PCTFREE_PCTUSED_VALUE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcuProperty::setupUpdateCallback()
{
/***********************************************************************
 *
 * Description :
 *    Writable Property의 경우 CallBack을 등록해야 함.
 *
 * Implementation :
 *
 ***********************************************************************/

    //--------------------------------------------------------
    // Trace Log 관련 property loading
    //    - Writable Property이므로 CallBack을 등록한다.
    //    - Atomic Operation이므로, BeforeCallBack은 필요 없다.
    //--------------------------------------------------------

    // TRCLOG_DML_SENTENCE 관련 설정
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "TRCLOG_DML_SENTENCE",
                  qcuProperty::changeTRCLOG_DML_SENTENCE )
              != IDE_SUCCESS );

    // TRCLOG_DETAIL_PREDICATE 관련 설정
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "TRCLOG_DETAIL_PREDICATE",
                  qcuProperty::changeTRCLOG_DETAIL_PREDICATE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  "TRCLOG_DETAIL_MTRNODE",
                  qcuProperty::changeTRCLOG_DETAIL_MTRNODE )
              != IDE_SUCCESS );

    // TRCLOG_EXPLAIN_GRAPH 관련 설정
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "TRCLOG_EXPLAIN_GRAPH",
                  qcuProperty::changeTRCLOG_EXPLAIN_GRAPH )
              != IDE_SUCCESS );

    // PROJ-2179
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "TRCLOG_RESULT_DESC",
                  qcuProperty::changeTRCLOG_RESULT_DESC )
              != IDE_SUCCESS );

    // BUG-38192
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "TRCLOG_DISPLAY_CHILDREN",
                  qcuProperty::changeTRCLOG_DISPLAY_CHILDREN )
              != IDE_SUCCESS );

    // UPDATE_IN_PLACE 관련 설정
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "UPDATE_IN_PLACE",
                  qcuProperty::changeUPDATE_IN_PLACE )
              != IDE_SUCCESS );

    //--------------------------------------------------------
    // Query Processor 관련 Property Loading
    //--------------------------------------------------------

    IDE_TEST( idp::setupAfterUpdateCallback(
                  "NORMALFORM_MAXIMUM",
                  qcuProperty::changeNORMALFORM_MAXIMUM )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  "EXEC_DDL_DISABLE",
                  qcuProperty::changeEXEC_DDL_DISABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  "OPTIMIZER_MODE",
                  qcuProperty::changeOPTIMIZER_MODE )
              != IDE_SUCCESS );

    // AUTO_REMOTE_EXEC
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "AUTO_REMOTE_EXEC",
                  qcuProperty::changeAUTO_REMOTE_EXEC )
              != IDE_SUCCESS );

    // PROJ-1358 Query Stack Size의 조정
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "QUERY_STACK_SIZE",
                  qcuProperty::changeQUERY_STACK_SIZE )
              != IDE_SUCCESS );

    // PR-13395 가상 통계 정보 구축
    // TPC-H Scale Factor
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__QP_FAKE_STAT_TPCH_SCALE_FACTOR",
                  qcuProperty::changeFAKE_STAT_TPCH_SCALE_FACTOR )
              != IDE_SUCCESS );

    // PR-13395 가상 통계 정보 구축
    // 가상 Buffer Size
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__QP_FAKE_STAT_BUFFER_SIZE",
                  qcuProperty::changeFAKE_STAT_BUFFER_SIZE )
              != IDE_SUCCESS );

    // BUG-13068 filehandle open limit 조정
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "PSM_FILE_OPEN_LIMIT",
                  qcuProperty::changePSM_FILE_OPEN_LIMIT )
              != IDE_SUCCESS );
    
    // BUG-40854
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "CONNECT_TYPE_OPEN_LIMIT",
                  qcuProperty::changeCONNECT_TYPE_OPEN_LIMIT )
              != IDE_SUCCESS );
    
    /* BUG-41307 User Lock 지원 */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "USER_LOCK_REQUEST_TIMEOUT",
                  qcuProperty::changeUSER_LOCK_REQUEST_TIMEOUT )
              != IDE_SUCCESS );

    /* BUG-41307 User Lock 지원 */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "USER_LOCK_REQUEST_CHECK_INTERVAL",
                  qcuProperty::changeUSER_LOCK_REQUEST_CHECK_INTERVAL )
              != IDE_SUCCESS );

    /* BUG-41307 User Lock 지원 */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "USER_LOCK_REQUEST_LIMIT",
                  qcuProperty::changeUSER_LOCK_REQUEST_LIMIT )
              != IDE_SUCCESS );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "HOST_OPTIMIZE_ENABLE",
                  qcuProperty::changeHOST_OPTIMIZE_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__OPTIMIZER_DEFAULT_TEMP_TBS_TYPE",
                  qcuProperty::changeOPTIMIZER_DEFAULT_TEMP_TBS_TYPE )
              != IDE_SUCCESS );

    // BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__OPTIMIZER_FIXED_GROUP_MEMORY_TEMP",
                  qcuProperty::changeOPTIMIZER_FIXED_GROUP_MEMORY_TEMP )
              != IDE_SUCCESS );
    // BUG-38339 Outer Join Elimination
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__OPTIMIZER_OUTERJOIN_ELIMINATION",
                  qcuProperty::changeOPTIMIZER_OUTERJOIN_ELIMINATION )
              != IDE_SUCCESS );

    // PROJ-1413 Simple View Merging
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE",
                  qcuProperty::changeOPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE )
              != IDE_SUCCESS );

    //PROJ-1583 large geometry
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "ST_OBJECT_BUFFER_SIZE",
                  qcuProperty::changeST_OBJECT_BUFFER_SIZE)
              != IDE_SUCCESS );

    // BUG-19089
    // FK가 있는 상태에서 CREATE REPLICATION 구문이 가능하도록 한다.
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "CHECK_FK_IN_CREATE_REPLICATION_DISABLE",
                  qcuProperty::changeCHECK_FK_IN_CREATE_REPLICATION_DISABLE)
              != IDE_SUCCESS );



    // PROJ-1436
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT",
                  qcuProperty::changeSQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT )
              != IDE_SUCCESS );

    // PROJ-2002 Column Security
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "SECURITY_MODULE_NAME",
                  qcuProperty::changeQCU_SECURITY_MODULE_NAME )
              != IDE_SUCCESS );

    // PROJ-2002 Column Security
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "SECURITY_MODULE_LIBRARY",
                  qcuProperty::changeQCU_SECURITY_MODULE_LIBRARY )
              != IDE_SUCCESS );

    // PROJ-2002 Column Security
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "SECURITY_ECC_POLICY_NAME",
                  qcuProperty::changeQCU_SECURITY_ECC_POLICY_NAME )
              != IDE_SUCCESS );

    // BUG-29209 : natc test를 위하여 Plan Display에서
    //             특정 정보( DISK_PAGE_COUNT, ITEM_SIZE)를 보여주지
    //             않게 하는 프로퍼티
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__DISPLAY_PLAN_FOR_NATC",
                  qcuProperty::changeDISPLAY_PLAN_FOR_NATC )
              != IDE_SUCCESS );

    // BUG-31040
    IDE_TEST(
        idp::setupAfterUpdateCallback("__MAX_SET_OP_RECURSION_DEPTH",
                                      qcuProperty::change__MAX_SET_OP_RECURSION_DEPTH)
        != IDE_SUCCESS );

    // BUG-32101
    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_DISK_INDEX_COST_ADJ",
                                      qcuProperty::changeOPTIMIZER_DISK_INDEX_COST_ADJ)
        != IDE_SUCCESS );

    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_MEMORY_INDEX_COST_ADJ",
                                      qcuProperty::changeOPTIMIZER_MEMORY_INDEX_COST_ADJ)
        != IDE_SUCCESS );

    // BUG-34441
    IDE_TEST(idp::setupAfterUpdateCallback("OPTIMIZER_HASH_JOIN_COST_ADJ",
                                           qcuProperty::changeOPTIMIZER_HASH_JOIN_COST_ADJ)
             != IDE_SUCCESS);

    // BUG-34235
    IDE_TEST(idp::setupAfterUpdateCallback("OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD",
                                           qcuProperty::changeOPTIMIZER_SUBQUERY_OPTIMIZE_METHOD)
             != IDE_SUCCESS);

    // fix BUG-33589
    IDE_TEST(
        idp::setupAfterUpdateCallback( "PLAN_REBUILD_ENABLE",
                                       qcuProperty::changePLAN_REBUILD_ENABLE )
        != IDE_SUCCESS );

    /* PROJ-2361 */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__OPTIMIZER_AVG_TRANSFORM_ENABLE",
                  qcuProperty::changeAVERAGE_TRANSFORM_ENABLE )
              != IDE_SUCCESS );

    /* PROJ-1071 Parallel Query */
    IDE_TEST(idp::setupAfterUpdateCallback("PARALLEL_QUERY_THREAD_MAX",
                                           qcuProperty::changePARALLEL_QUERY_THREAD_MAX)
             != IDE_SUCCESS );

    IDE_TEST(idp::setupAfterUpdateCallback("PARALLEL_QUERY_QUEUE_SLEEP_MAX",
                                           qcuProperty::changePARALLEL_QUERY_QUEUE_SLEEP_MAX)
             != IDE_SUCCESS );

    IDE_TEST(idp::setupAfterUpdateCallback("PARALLEL_QUERY_QUEUE_SIZE",
                                           qcuProperty::changePARALLEL_QUERY_QUEUE_SIZE)
             != IDE_SUCCESS );

    IDE_TEST(
        idp::setupAfterUpdateCallback("__FORCE_PARALLEL_DEGREE",
                                          qcuProperty::changeFORCE_PARALLEL_DEGREE)
            != IDE_SUCCESS );

    /* PROJ-2207 Password policy support */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  "__SYSDATE_FOR_NATC",
                  qcuProperty::change__SYSDATE_FOR_NATC )
              != IDE_SUCCESS );

    // BUG-34295
    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_ANSI_JOIN_ORDERING",
                                      qcuProperty::changeOPTIMIZER_ANSI_JOIN_ORDERING)
        != IDE_SUCCESS );
    // BUG-38402
    IDE_TEST(
        idp::setupAfterUpdateCallback("__OPTIMIZER_ANSI_INNER_JOIN_CONVERT",
                                      qcuProperty::changeOPTIMIZER_ANSI_INNER_JOIN_CONVERT)
        != IDE_SUCCESS );

    // BUG-34350
    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_REFINE_PREPARE_MEMORY",
                                      qcuProperty::changeOPTIMIZER_REFINE_PREPARE_MEMORY)
        != IDE_SUCCESS );

    // BUG-35155
    IDE_TEST(
            idp::setupAfterUpdateCallback("OPTIMIZER_PARTIAL_NORMALIZE",
                                          qcuProperty::changeOPTIMIZER_PARTIAL_NORMALIZE)
            != IDE_SUCCESS );

    // PROJ-2264 Dictionary table
    IDE_TEST(
        idp::setupAfterUpdateCallback("__FORCE_COMPRESSION_COLUMN",
                                          qcuProperty::changeFORCE_COMPRESSION_COLUMN)
            != IDE_SUCCESS );

    /* PROJ-1090 Function-based Index */
    IDE_TEST(
        idp::setupAfterUpdateCallback( "QUERY_REWRITE_ENABLE",
                                       qcuProperty::changeQUERY_REWRITE_ENABLE )
        != IDE_SUCCESS );

    // BUG-35713
    IDE_TEST(
        idp::setupAfterUpdateCallback( "PSM_IGNORE_NO_DATA_FOUND_ERROR",
                                       changePSM_IGNORE_NO_DATA_FOUND_ERROR )
        != IDE_SUCCESS );

    // PROJ-1718
    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_UNNEST_SUBQUERY",
                                      qcuProperty::changeOPTIMIZER_UNNEST_SUBQUERY)
        != IDE_SUCCESS );

    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_UNNEST_COMPLEX_SUBQUERY",
                                      qcuProperty::changeOPTIMIZER_UNNEST_COMPLEX_SUBQUERY)
        != IDE_SUCCESS );

    IDE_TEST(
        idp::setupAfterUpdateCallback("OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY",
                                      qcuProperty::changeOPTIMIZER_UNNEST_AGGREGATION_SUBQUERY)
        != IDE_SUCCESS );

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    IDE_TEST(
        idp::setupAfterUpdateCallback( "__INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT",
                                       qcuProperty::changeINTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT )
        != IDE_SUCCESS );

    /* PROJ-2242 CSE, CFS, Feature enable */
    IDE_TEST( idp::setupAfterUpdateCallback(
                "__OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION",
                qcuProperty::changeOPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                "__OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION",
                qcuProperty::changeOPTIMIZER_CONSTANT_FILTER_SUBSUMPTION )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  "OPTIMIZER_FEATURE_ENABLE",
                  qcuProperty::changeOPTIMIZER_FEATURE_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST(
        idp::setupAfterUpdateCallback( "__DML_WITHOUT_RETRY_ENABLE",
                                       qcuProperty::changeDML_WITHOUT_RETRY_ENABLE )
        != IDE_SUCCESS );

    // BUG-36203 PSM Optimize
    IDE_TEST(
        idp::setupAfterUpdateCallback( "PSM_TEMPLATE_CACHE_COUNT",
                                       changePSM_TEMPLATE_CACHE_COUNT )
        != IDE_SUCCESS );

    // fix BUG-36522
    IDE_TEST(
        idp::setupAfterUpdateCallback( "__PSM_SHOW_ERROR_STACK",
                                       changePSM_SHOW_ERROR_STACK )
        != IDE_SUCCESS );

    // fix BUG-36793
    IDE_TEST(
        idp::setupAfterUpdateCallback( "__BIND_PARAM_DEFAULT_PRECISION",
                                       changeBIND_PARAM_DEFAULT_PRECISION )
        != IDE_SUCCESS );

    // BUG-37247
    IDE_TEST(
        idp::setupAfterUpdateCallback( "SYS_CONNECT_BY_PATH_PRECISION",
                                       changeSYS_CONNECT_BY_PATH_PRECISION )
        != IDE_SUCCESS );

    // BUG-37302
    IDE_TEST(
        idp::setupAfterUpdateCallback( "SQL_ERROR_INFO_SIZE",
                                       changeSQL_ERROR_INFO_SIZE )
        != IDE_SUCCESS );

    // PROJ-2362
    IDE_TEST(
        idp::setupAfterUpdateCallback( "REDUCE_TEMP_MEMORY_ENABLE",
                                       changeREDUCE_TEMP_MEMORY_ENABLE )
        != IDE_SUCCESS );

    // BUG-38434
    IDE_TEST(
            idp::setupAfterUpdateCallback( "__OPTIMIZER_DNF_DISABLE",
                                           qcuProperty::changeOPTIMIZER_DNF_DISABLE )
            != IDE_SUCCESS );

    // fix BUG-39754
    IDE_TEST( idp::setupAfterUpdateCallback( "__FOREIGN_KEY_LOCK_ROW",
                                             qcuProperty::changeFOREIGN_KEY_LOCK_ROW )
              != IDE_SUCCESS );

    // BUG-40022
    IDE_TEST(
            idp::setupAfterUpdateCallback( "__OPTIMIZER_JOIN_DISABLE",
                                           qcuProperty::changeOPTIMIZER_JOIN_DISABLE )
            != IDE_SUCCESS );

    // PROJ-2469 Optimize View Materialization
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_VIEW_TARGET_ENABLE",
                                             qcuProperty::changeOPTIMIZER_VIEW_TARGET_ENABLE )
              != IDE_SUCCESS );

    /* PROJ-2451 Concurrent Execute Package */
    IDE_TEST(idp::setupAfterUpdateCallback("CONCURRENT_EXEC_DEGREE_DEFAULT",
                                           qcuProperty::changeCONCURRENT_EXEC_DEGREE_DEFAULT)
             != IDE_SUCCESS );

    /* PROJ-2451 Concurrent Execute Package */
    IDE_TEST(idp::setupAfterUpdateCallback("CONCURRENT_EXEC_WAIT_INTERVAL",
                                           qcuProperty::changeCONCURRENT_EXEC_WAIT_INTERVAL)
             != IDE_SUCCESS );

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    IDE_TEST( idp::setupAfterUpdateCallback( "__QUERY_EXECUTION_CACHE_MAX_COUNT",
                                             qcuProperty::changeQUERY_EXECUTION_CACHE_MAX_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__QUERY_EXECUTION_CACHE_MAX_SIZE",
                                             qcuProperty::changeQUERY_EXECUTION_CACHE_MAX_SIZE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__QUERY_EXECUTION_CACHE_BUCKET_COUNT",
                                             qcuProperty::changeQUERY_EXECUTION_CACHE_BUCKET_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__FORCE_FUNCTION_CACHE",
                                             qcuProperty::changeFORCE_FUNCTION_CACHE )
              != IDE_SUCCESS );

    /* PROJ-2441 flashback */
    IDE_TEST( idp::setupAfterUpdateCallback( "RECYCLEBIN_MEM_MAX_SIZE",
                                             qcuProperty::changeRECYCLEBIN_MEM_MAX_SIZE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "RECYCLEBIN_DISK_MAX_SIZE",
                                             qcuProperty::changeRECYCLEBIN_DISK_MAX_SIZE )
             != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "RECYCLEBIN_ENABLE",
                                             qcuProperty::changeRECYCLEBIN_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__RECYCLEBIN_FOR_NATC",
                                             qcuProperty::change__RECYCLEBIN_FOR_NATC )
              != IDE_SUCCESS );
    
    // PROJ-2553
    IDE_TEST( idp::setupAfterUpdateCallback( "HASH_JOIN_MEM_TEMP_PARTITIONING_DISABLE",
                                             qcuProperty::changeHASH_JOIN_MEM_TEMP_PARTITIONING_DISABLE )
              != IDE_SUCCESS );

    /* PROJ-2448 Subquery caching */
    IDE_TEST( idp::setupAfterUpdateCallback( "__FORCE_SUBQUERY_CACHE_DISABLE",
                                             qcuProperty::changeFORCE_SUBQUERY_CACHE_DISABLE )
              != IDE_SUCCESS );

    // PROJ-2551 simple query 최적화
    IDE_TEST( idp::setupAfterUpdateCallback( "EXECUTOR_FAST_SIMPLE_QUERY",
                                             qcuProperty::changeEXECUTOR_FAST_SIMPLE_QUERY )
              != IDE_SUCCESS );

    // BUG-41183 ORDER BY Elimination
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE",
                                              qcuProperty::changeOPTIMIZER_ORDER_BY_ELIMINATION_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "HASH_JOIN_MEM_TEMP_AUTO_BUCKET_COUNT_DISABLE",
                                             qcuProperty::changeHASH_JOIN_MEM_TEMP_AUTO_BUCKET_CNT_DISABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__HASH_JOIN_MEM_TEMP_TLB_ENTRY_COUNT",
                                             qcuProperty::changeHASH_JOIN_MEM_TEMP_TLB_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__FORCE_HASH_JOIN_MEM_TEMP_FANOUT_MODE",
                                             qcuProperty::changeHASH_JOIN_MEM_TEMP_FANOUT_MODE )
              != IDE_SUCCESS );

    // BUG-41398 use old sort
    IDE_TEST( idp::setupAfterUpdateCallback( "__USE_OLD_SORT",
                                             qcuProperty::changeUSE_OLD_SORT )
             != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("DDL_SUPPLEMENTAL_LOG_ENABLE",
                                            qcuProperty::changeDDL_SUPPLEMENTAL_LOG_ENABLE )
              != IDE_SUCCESS);

    // BUG-41249 DISTINCT Elimination
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_DISTINCT_ELIMINATION_ENABLE",
                                              qcuProperty::changeOPTIMIZER_DISTINCT_ELIMINATION_ENABLE )
              != IDE_SUCCESS );

    /* PROJ-2462 Result Cache */
    IDE_TEST( idp::setupAfterUpdateCallback( "RESULT_CACHE_ENABLE",
                                             qcuProperty::changeRESULT_CACHE_ENABLE )
              != IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback( "TOP_RESULT_CACHE_MODE",
                                             qcuProperty::changeTOP_RESULT_CACHE_MODE )
              != IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback( "RESULT_CACHE_MEMORY_MAXIMUM",
                                             qcuProperty::changeRESULT_CACHE_MEMORY_MAXIMUM )
              != IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback( "TRCLOG_DETAIL_ResultCache",
                                             qcuProperty::changeTRCLOG_DETAIL_RESULTCACHE )
              != IDE_SUCCESS );

    // BUG-36438 LIST transformation
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_LIST_TRANSFORMATION",
                                             qcuProperty::changeOPTIMIZER_LIST_TRANSFORMATION )
              != IDE_SUCCESS );

    /* PROJ-2492 Dynamic sample selection */
    IDE_TEST( idp::setupAfterUpdateCallback( "OPTIMIZER_AUTO_STATS",
                                             qcuProperty::changeOPTIMIZER_AUTO_STATS )
              != IDE_SUCCESS );

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_TRANSITIVITY_OLD_RULE",
                                             qcuProperty::changeOPTIMIZER_TRANSITIVITY_OLD_RULE )
              != IDE_SUCCESS );

    // PROJ-2582 recursive with
    IDE_TEST( idp::setupAfterUpdateCallback( "RECURSION_LEVEL_MAXIMUM",
                                             qcuProperty::changeRECURSION_LEVEL_MAXIMUM )
              != IDE_SUCCESS );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_TEST( idp::setupAfterUpdateCallback( "DDL_MEM_USAGE_THRESHOLD",
                                             qcuProperty::changeDDL_MEM_USAGE_THRESHOLD )
              != IDE_SUCCESS );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_TEST( idp::setupAfterUpdateCallback( "DDL_TBS_USAGE_THRESHOLD",
                                             qcuProperty::changeDDL_TBS_USAGE_THRESHOLD )
              != IDE_SUCCESS );

    /* PROJ-2465 Tablespace Alteration for Table */
    IDE_TEST( idp::setupAfterUpdateCallback( "ANALYZE_USAGE_MIN_ROWCOUNT",
                                             qcuProperty::changeANALYZE_USAGE_MIN_ROWCOUNT )
              != IDE_SUCCESS );

    /* BUG-42639 Monitoring query */
    IDE_TEST( idp::setupAfterUpdateCallback( "OPTIMIZER_PERFORMANCE_VIEW",
                                             qcuProperty::changeOPTIMIZER_PERFORMANCE_VIEW )
              != IDE_SUCCESS );

    // BUG-43039 inner join push down
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_INNER_JOIN_PUSH_DOWN",
                                             qcuProperty::changeOPTIMIZER_INNER_JOIN_PUSH_DOWN )
              != IDE_SUCCESS );

    // BUG-43068 Indexable order by 개선
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_ORDER_PUSH_DOWN",
                                             qcuProperty::changeOPTIMIZER_ORDER_PUSH_DOWN )
              != IDE_SUCCESS );

    // BUG-43059 Target subquery unnest/removal disable
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE",
                                             qcuProperty::changeOPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE",
                                             qcuProperty::changeOPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__FORCE_AUTONOMOUS_TRANSACTION_PRAGMA",
                                             qcuProperty::changeFORCE_AUTONOMOUS_TRANSACTION_PRAGMA )
              != IDE_SUCCESS );

    // BUG-43158 Enhance statement list caching in PSM
    IDE_TEST( idp::setupAfterUpdateCallback( "__PSM_STATEMENT_LIST_COUNT",
                                             qcuProperty::changePSM_STMT_LIST_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__PSM_STATEMENT_POOL_COUNT",
                                             qcuProperty::changePSM_STMT_POOL_COUNT )
              != IDE_SUCCESS );

    // BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
    IDE_TEST( idp::setupAfterUpdateCallback( "__DISK_TEMP_SIZE_ESTIMATE",
                                             qcuProperty::changeDISK_TEMP_SIZE_ESTIMATE)
              != IDE_SUCCESS );

    // BUG-43493
    IDE_TEST( idp::setupAfterUpdateCallback( "OPTIMIZER_DELAYED_EXECUTION",
                                             qcuProperty::changeOPTIMIZER_DELAYED_EXECUTION)
              != IDE_SUCCESS );

    /* BUG-43495 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE",
                                             qcuProperty::changeOPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE )
              != IDE_SUCCESS );

    /* TASK-6744 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__PLAN_RANDOM_SEED",
                                             qcuProperty::changePlanRandomSeed )
              != IDE_SUCCESS );

    /* BUG-44499 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_BUCKET_COUNT_MIN",
                                             qcuProperty::changeOPTIMIZER_BUCKET_COUNT_MIN )
              != IDE_SUCCESS );

    /* BUG-48161 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_BUCKET_COUNT_MAX",
                                             qcuProperty::changeOPTIMIZER_BUCKET_COUNT_MAX )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_HIERARCHY_TRANSFORMATION",
                                             qcuProperty::changeOPTIMIZER_HIERARCHY_TRANSFORMATION )
              != IDE_SUCCESS );

    // BUG-44692
    IDE_TEST( idp::setupAfterUpdateCallback( "BUG_44692",
                                             qcuProperty::changeOPTIMIZER_BUG_44692 )
              != IDE_SUCCESS );

    // BUG-44795
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_DBMS_STAT_POLICY",
                                             qcuProperty::changeOPTIMIZER_DBMS_STAT_POLICY )
              != IDE_SUCCESS );

    /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
    IDE_TEST( idp::setupAfterUpdateCallback("__OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY",
                                            qcuProperty::changeOPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("__OPTIMIZER_SEMI_JOIN_REMOVE",
                                            qcuProperty::changeOPTIMIZER_SEMI_JOIN_REMOVE)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__PRINT_OUT_ENABLE",
                                             qcuProperty::change__PRINT_OUT_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__KEY_PRESERVED_TABLE",
                                             qcuProperty::change__KEY_PRESERVED_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_UNNEST_COMPATIBILITY",
                                             qcuProperty::change__OPTIMIZER_UNNEST_COMPATIBILITY )
              != IDE_SUCCESS );

    /* PROJ-2632 */
    IDE_TEST( idp::setupBeforeUpdateCallback( "SERIAL_EXECUTE_MODE",
                                              qcuProperty::changeSERIAL_EXECUTE_MODE )
              != IDE_SUCCESS);

    /* BUG-46932 */ 
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_INVERSE_JOIN_ENABLE",
                                             qcuProperty::changeOPTIMIZER_INVERSE_JOIN_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__REDUCE_PARTITION_PREPARE_MEMORY",
                                             qcuProperty::changeREDUCE_PARTITION_PREPARE_MEMORY )
              != IDE_SUCCESS );

    /* BUG-47986 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_OR_VALUE_INDEX",
                                             qcuProperty::changeOPTIMIZER_OR_VALUE_INDEX )
              != IDE_SUCCESS );

    /* BUG-48132 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_PLAN_HASH_OR_SORT_METHOD",
                                             qcuProperty::changeOPTIMIZER_PLAN_HASH_OR_SORT_METHOD )
              != IDE_SUCCESS );

    /* BUG-48135 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_INDEX_NL_JOIN_PENALTY",
                                             qcuProperty::changeOPTIMIZER_INDEX_NL_JOIN_PENALTY )
              != IDE_SUCCESS );

    /* BUG-48120 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__OPTIMIZER_INDEX_COST_MODE",
                                             qcuProperty::changeOPTIMIZER_INDEX_COST_MODE )
              != IDE_SUCCESS );

    /* BUG-48776 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__SUBQUERY_MODE",
                                             qcuProperty::changeSUBQUERY_MODE )
              != IDE_SUCCESS );    

    /* PROJ-2750 */
    IDE_TEST( idp::setupAfterUpdateCallback( "__LEFT_OUTER_SKIP_RIGHT_ENABLE",
                                             qcuProperty::changeLEFT_OUTER_SKIP_RIGHT_ENABLE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcuProperty::changeTRCLOG_DML_SENTENCE( idvSQL* /* aStatistics */,
                                        SChar * /* aName */,
                                        void  * /* aOldValue */,
                                        void  * aNewValue,
                                        void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    TRCLOG_DML_SENTENCE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mTraceLog_DML_Sentence),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeTRCLOG_DETAIL_PREDICATE( idvSQL* /* aStatistics */,
                                            SChar * /* aName */,
                                            void  * /* aOldValue */,
                                            void  * aNewValue,
                                            void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    TRCLOG_DETAIL_PREDICATE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mTraceLog_Detail_Predicate),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeTRCLOG_DETAIL_MTRNODE( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    TRCLOG_DETAIL_MTRNODE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mTraceLog_Detail_MtrNode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeTRCLOG_EXPLAIN_GRAPH( idvSQL* /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    TRCLOG_EXPLAIN_GRAPH 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mTraceLog_Explain_Graph),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeTRCLOG_RESULT_DESC( idvSQL* /* aStatistics */,
                                       SChar * /* aName */,
                                       void  * /* aOldValue */,
                                       void  * aNewValue,
                                       void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    TRCLOG_RESULT_DESC 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mTraceLog_Result_Desc),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-38192
IDE_RC
qcuProperty::changeTRCLOG_DISPLAY_CHILDREN( idvSQL* /* aStatistics */,
                                            SChar * /* aName */,
                                            void  * /* aOldValue */,
                                            void  * aNewValue,
                                            void  * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY(mTraceLogDisplayChildren),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeUPDATE_IN_PLACE( idvSQL* /* aStatistics */,
                                    SChar * /* aName */,
                                    void  * /* aOldValue */,
                                    void  * aNewValue,
                                    void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    UPDATE_IN_PLACE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mUpdateInPlace),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeEXEC_DDL_DISABLE( idvSQL* /* aStatistics */,
                                     SChar * /* aName */,
                                     void  * /* aOldValue */,
                                     void  * aNewValue,
                                     void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    EXEC_DDL_DISABLE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mExecDDLDisable),
                   aNewValue,
                   ID_SIZEOF(SInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_MODE( idvSQL* /* aStatistics */,
                                   SChar * /* aName */,
                                   void  * /* aOldValue */,
                                   void  * aNewValue,
                                   void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
 *              관련프로퍼티를 참조하지 못하는 경우 있음.
 *    OPTIMIZER_MODE의 값을 설정함.
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mOptimizerMode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeAUTO_REMOTE_EXEC( idvSQL* /* aStatistics */,
                                     SChar * /* aName */,
                                     void  * /* aOldValue */,
                                     void  * aNewValue,
                                     void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
 *              관련프로퍼티를 참조하지 못하는 경우 있음.
 *    AUTO_REMOTE_EXEC의 값을 설정함.
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mAutoRemoteExec),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeQUERY_STACK_SIZE( idvSQL* /* aStatistics */,
                                     SChar * /* aName */,
                                     void  * /* aOldValue */,
                                     void  * aNewValue,
                                     void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1358
 *    QUERY_STACK_SIZE의 값을 설정함.
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mQueryStackSize),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeNORMALFORM_MAXIMUM( idvSQL* /* aStatistics */,
                                       SChar * /* aName */,
                                       void  * /* aOldValue */,
                                       void  * aNewValue,
                                       void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    PR-24056
 *    NORMALFORM_MAXIMUM의 값을 설정함.
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mNormalFormMaximum),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeFAKE_STAT_TPCH_SCALE_FACTOR( idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    PR-13395
 *    TPC-H Scale Factor 에 따른 가상 통계 정보 구축
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mFakeTpchScaleFactor),
                   aNewValue,
                   ID_SIZEOF(SInt) );

    return IDE_SUCCESS;
}


IDE_RC
qcuProperty::changeFAKE_STAT_BUFFER_SIZE( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    PR-13395
 *    가상 Buffer Size 설정
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mFakeBufferSize),
                   aNewValue,
                   ID_SIZEOF(SInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeHOST_OPTIMIZE_ENABLE( idvSQL* /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    HOST_OPTIMIZE_ENABLE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( &QCU_PROPERTY(mHostOptimizeEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changePSM_FILE_OPEN_LIMIT( idvSQL* /* aStatistics */,
                                        SChar * /* aName */,
                                        void  * /* aOldValue */,
                                        void  * aNewValue,
                                        void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    BUG-13068
 *    filehandle open limit 조정
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mFileOpenLimit),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-40854
IDE_RC qcuProperty::changeCONNECT_TYPE_OPEN_LIMIT( idvSQL* /* aStatistics */,
                                                   SChar * /* aName */,
                                                   void  * /* aOldValue */,
                                                   void  * aNewValue,
                                                   void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mSocketOpenLimit),
                   aNewValue,
                   ID_SIZEOF(UInt) );
    
    return IDE_SUCCESS;
}

/* BUG-41307 User Lock 지원 */
IDE_RC qcuProperty::changeUSER_LOCK_REQUEST_TIMEOUT( idvSQL * /* aStatistics */,
                                                     SChar  * /* aName */,
                                                     void   * /* aOldValue */,
                                                     void   * aNewValue,
                                                     void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mUserLockRequestTimeout),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-41307 User Lock 지원 */
IDE_RC qcuProperty::changeUSER_LOCK_REQUEST_CHECK_INTERVAL( idvSQL * /* aStatistics */,
                                                            SChar  * /* aName */,
                                                            void   * /* aOldValue */,
                                                            void   * aNewValue,
                                                            void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mUserLockRequestCheckInterval),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-41307 User Lock 지원 */
IDE_RC qcuProperty::changeUSER_LOCK_REQUEST_LIMIT( idvSQL * /* aStatistics */,
                                                   SChar  * /* aName */,
                                                   void   * /* aOldValue */,
                                                   void   * aNewValue,
                                                   void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mUserLockRequestLimit),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// 참조 : mmuProperty
void*
qcuProperty::callbackForGettingArgument( idvSQL              * /* aStatistics */,
                                         qcuPropertyArgument *aArg,
                                         idpArgumentID        aID)
{
    void *sValue;

    switch(aID)
    {
        case IDP_ARG_USERID :
            sValue = (void *)&(aArg->mUserID);
            break;
        case IDP_ARG_TRANSID:
            sValue = (void *)(aArg->mTrans);
            break;
        default:
            IDE_CALLBACK_FATAL("Can't Be Here!!");
            IDE_ASSERT(0);
    }
    return sValue;
}

//PROJ-1583 large geometry
IDE_RC
qcuProperty::changeST_OBJECT_BUFFER_SIZE( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */)
{
    idlOS::memcpy( & QCU_PROPERTY(mSTObjBufSize),
                   aNewValue,
                   ID_SIZEOF(SInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_DEFAULT_TEMP_TBS_TYPE( idvSQL* /* aStatistics */,
                                                    SChar * /* aName */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *
 *    BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
 *
 *    __OPTIMIZER_DEFAULT_TEMP_TBS_TYPE 을 위한 CallBack 함수
 *
 * Implementation :
 *     하나의 querySet 마다 검사하므로 동시성의 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( &QCU_PROPERTY(mOptimizerDefaultTempTbsType),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_FIXED_GROUP_MEMORY_TEMP( idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerFixedGroupMemoryTemp),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}
// BUG-38339 Outer Join Elimination
IDE_RC
qcuProperty::changeOPTIMIZER_OUTERJOIN_ELIMINATION( idvSQL* /* aStatistics */,
                                                    SChar * /* aName */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerOuterJoinElimination),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeCHECK_FK_IN_CREATE_REPLICATION_DISABLE( idvSQL* /* aStatistics */,
                                                                  SChar * /* aName */,
                                                                  void  * /* aOldValue */,
                                                                  void  * aNewValue,
                                                                  void  * /* aArg */)
{
/***********************************************************************
 *
 * Description :
 *    BUG-19089
 *    CHECK_FK_IN_CREATE_REPLICATION_DISABLE 을 위한 CallBack 함수
 *
 * Implementation :
 *    Atomic Operation이므로 동시성 제어에 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( &QCU_PROPERTY(mCheckFkInCreateReplicationDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     __OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE 을 위한 CallBack 함수
 *
 * Implementation :
 *     하나의 view merge시 마다 검사하므로 동시성의 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( &QCU_PROPERTY(mOptimizerSimpleViewMergingDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeSQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT( idvSQL* /* aStatistics */,
                                                                  SChar * /* aName */,
                                                                  void  * /* aOldValue */,
                                                                  void  * aNewValue,
                                                                  void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : PROJ-1436 SQL Plan Cache
 *     SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT를 위한 CallBack 함수
 *
 * Implementation :
 *     makePlanCacheInfo시 한번만 검사하므로 동시성의 문제가 없다.
 *
 ***********************************************************************/

    idlOS::memcpy( &QCU_PROPERTY(mSqlPlanCachePreparedExecutionContextCnt),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeQCU_SECURITY_MODULE_NAME( idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *     SECURITY_MODULE을 위한 CallBack 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar *  sNewValue;
    UInt     sLength;

    sNewValue = (SChar*) aNewValue;
    sLength = idlOS::strlen( sNewValue );

    IDE_TEST_RAISE( sLength > QCS_MODULE_NAME_SIZE,
                    ERR_TOO_LONG_MODULE_NAME );

    idlOS::strncpy( QCU_PROPERTY(mSecurityModuleName), sNewValue, sLength );
    QCU_PROPERTY(mSecurityModuleName)[sLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_MODULE_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCU_TOO_LONG_MODULE_NAME));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcuProperty::changeQCU_SECURITY_MODULE_LIBRARY( idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *     SECURITY_MODULE_LIBRARY을 위한 CallBack 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar *  sNewValue;
    UInt     sLength;

    sNewValue = (SChar*) aNewValue;
    sLength = idlOS::strlen( sNewValue );

    IDE_TEST_RAISE( sLength > QCS_MODULE_LIBRARY_SIZE,
                    ERR_TOO_LONG_LIBRARY_NAME );

    idlOS::strncpy( QCU_PROPERTY(mSecurityModuleLibrary), sNewValue, sLength );
    QCU_PROPERTY(mSecurityModuleLibrary)[sLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_LIBRARY_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCU_TOO_LONG_LIBRARY_NAME));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcuProperty::changeQCU_SECURITY_ECC_POLICY_NAME( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : PROJ-2002 Column Security
 *     SECURITY_ECC_POLICY_NAME을 위한 CallBack 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar *  sNewValue;
    UInt     sLength;

    sNewValue = (SChar*) aNewValue;
    sLength = idlOS::strlen( sNewValue );

    IDE_TEST_RAISE( sLength > QCS_POLICY_NAME_SIZE,
                    ERR_TOO_LONG_POLICY_NAME );

    idlOS::strncpy( QCU_PROPERTY(mSecurityEccPolicyName), sNewValue, sLength );
    QCU_PROPERTY(mSecurityEccPolicyName)[sLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_POLICY_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_TOO_LONG_POLICY_NAME, ""));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcuProperty::changeDISPLAY_PLAN_FOR_NATC( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : BUG-29209
 *     DISPLAY_PLAN_FOR_NATC 변경을 위한 CallBack 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mDisplayPlanForNATC),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeOPTIMIZER_VIEW_TARGET_ENABLE( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */)
{
/***********************************************************************
 *
 * Description : PROJ-2469 Optimize View Materialization
 *     __OPTIMIZER_VIEW_TARGET_ENABLE 변경을 위한 CallBack
 *
 * Implementation :
 *
 ***********************************************************************/

    idlOS::memcpy( & QCU_PROPERTY(mOptimizerViewTargetEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::change__MAX_SET_OP_RECURSION_DEPTH(idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mMaxSetOpRecursionDepth),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_DISK_INDEX_COST_ADJ(idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerDiskIndexCostAdj),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

// BUG-43736
IDE_RC
qcuProperty::changeOPTIMIZER_MEMORY_INDEX_COST_ADJ(idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerMemoryIndexCostAdj),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

// BUG-34441
IDE_RC
qcuProperty::changeOPTIMIZER_HASH_JOIN_COST_ADJ(idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerHashJoinCostAdj),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

// BUG-34235
IDE_RC
qcuProperty::changeOPTIMIZER_SUBQUERY_OPTIMIZE_METHOD(idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerSubqueryOptimizeMethod),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

// fix BUG-33589
IDE_RC
qcuProperty::changePLAN_REBUILD_ENABLE( idvSQL* /* aStatistics */,
                                        SChar * /* aName */,
                                        void  * /* aOldValue */,
                                        void  * aNewValue,
                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mPlanRebuildEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2361 */
IDE_RC
qcuProperty::changeAVERAGE_TRANSFORM_ENABLE( idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mAvgTransformEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/*
 * PROJ-1071 Parallel Query
 */
IDE_RC qcuProperty::changePARALLEL_QUERY_THREAD_MAX( idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mParallelQueryThreadMax),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changePARALLEL_QUERY_QUEUE_SLEEP_MAX( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mParallelQueryQueueSleepMax),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changePARALLEL_QUERY_QUEUE_SIZE( idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mParallelQueryQueueSize),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeFORCE_PARALLEL_DEGREE(idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mForceParallelDegree),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

/* PROJ-2207 Password policy support */
IDE_RC
qcuProperty::change__SYSDATE_FOR_NATC(idvSQL* /* aStatistics */,
                                      SChar * /* aName */,
                                      void  * /* aOldValue */,
                                      void  * aNewValue,
                                      void  * /* aArg */)
{
    SChar *  sNewValue;
    UInt     sLength;

    sNewValue = (SChar*) aNewValue;
    sLength = idlOS::strlen( sNewValue );

    if ( (sLength > 0) && (sLength <= QTC_SYSDATE_FOR_NATC_LEN) )
    {
        idlOS::strncpy( QCU_PROPERTY(mSysdateForNATC), sNewValue, sLength );
        QCU_PROPERTY(mSysdateForNATC)[sLength] = '\0';
    }
    else
    {
        QCU_PROPERTY(mSysdateForNATC)[0] = '\0';
    }

    return IDE_SUCCESS;
}

// BUG-34295
IDE_RC
qcuProperty::changeOPTIMIZER_ANSI_JOIN_ORDERING(idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerAnsiJoinOrdering),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}
// BUG-38402
IDE_RC
qcuProperty::changeOPTIMIZER_ANSI_INNER_JOIN_CONVERT(idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerAnsiInnerJoinConvert),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

// BUG-34350
IDE_RC
qcuProperty::changeOPTIMIZER_REFINE_PREPARE_MEMORY(idvSQL* /* aStatistics */,
                                                   SChar * /* aName */,
                                                   void  * /* aOldValue */,
                                                   void  * aNewValue,
                                                   void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerRefinePrepareMemory),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

// BUG-35155
IDE_RC
qcuProperty::changeOPTIMIZER_PARTIAL_NORMALIZE(idvSQL* /* aStatistics */,
                                               SChar * /* aName */,
                                               void  * /* aOldValue */,
                                               void  * aNewValue,
                                               void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerPartialNormalize),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

// PROJ-2264 Dictionary table
IDE_RC
qcuProperty::changeFORCE_COMPRESSION_COLUMN(idvSQL* /* aStatistics */,
                                            SChar * /* aName */,
                                            void  * /* aOldValue */,
                                            void  * aNewValue,
                                            void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mForceCompressionColumn),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

/* PROJ-1090 Function-based Index */
IDE_RC
qcuProperty::changeQUERY_REWRITE_ENABLE( idvSQL* /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mQueryRewriteEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-35713
IDE_RC
qcuProperty::changePSM_IGNORE_NO_DATA_FOUND_ERROR( idvSQL* /* aStatistics */,
                                                   SChar * /* aName */,
                                                   void  * /* aOldValue */,
                                                   void  * aNewValue,
                                                   void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mPSMIgnoreNoDataFoundError),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// PROJ-1718
IDE_RC
qcuProperty::changeOPTIMIZER_UNNEST_SUBQUERY(idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerUnnestSubquery),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_UNNEST_COMPLEX_SUBQUERY(idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerUnnestComplexSubquery),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_UNNEST_AGGREGATION_SUBQUERY(idvSQL* /* aStatistics */,
                                                         SChar * /* aName */,
                                                         void  * /* aOldValue */,
                                                         void  * aNewValue,
                                                         void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerUnnestAggregationSubquery),
                   aNewValue,
                   ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
IDE_RC
qcuProperty::changeINTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mIntermediateTupleLobObjectLimit),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2242 CSE, FS, Feature enable */
IDE_RC
qcuProperty::changeOPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION(
        idvSQL* /* aStatistics */,
        SChar * /* aName */,
        void  * /* aOldValue */,
        void  * aNewValue,
        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerEliminateCommonSubexpression),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_CONSTANT_FILTER_SUBSUMPTION(
        idvSQL* /* aStatistics */,
        SChar * /* aName */,
        void  * /* aOldValue */,
        void  * aNewValue,
        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerConstantFilterSubsumption),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
qcuProperty::changeOPTIMIZER_FEATURE_ENABLE( idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg  */ )
{
/******************************************************************************
 *
 * Description : PROJ-2242 OPTIMIZER_FEATURE_ENABLE
 *
 * Implementation :
 *
 *****************************************************************************/

    SChar *  sFeatureValue;
    UInt     sLength;

    sFeatureValue = (SChar*) aNewValue;
    sLength = idlOS::strlen( sFeatureValue );

    if ( sLength > 0 && sLength <= QCU_OPTIMIZER_FEATURE_VERSION_LEN )
    {
        // OPTIMIZER_FEATURE_ENABLE property value 변경
        idlOS::strncpy( QCU_PROPERTY(mOptimizerFeatureEnable), sFeatureValue, sLength );
        QCU_PROPERTY(mOptimizerFeatureEnable)[sLength] = '\0';
    }
    else
    {
        // 유효하지 않은 길이의 입력값
        IDE_RAISE( err_invalid_feature_enable_value );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_feature_enable_value)
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCU_INVALID_FEATURE_ENABLE_VALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qcuProperty::changeDML_WITHOUT_RETRY_ENABLE( idvSQL* /* aStatistics */,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /* aArg*/ )
{
/******************************************************************************
 *
 * Description : PROJ-1784 DML WITHOUT RETRY
 *
 * Implementation :
 *
 *****************************************************************************/

    idlOS::memcpy( &QCU_PROPERTY(mDmlWithoutRetryEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// fix BUG-36522
IDE_RC
qcuProperty::changePSM_SHOW_ERROR_STACK( idvSQL* /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mPSMShowErrorStack),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// fix BUG-36793
IDE_RC
qcuProperty::changeBIND_PARAM_DEFAULT_PRECISION( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mBindParamDefaultPrecision),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-36203 PSM Optimize
IDE_RC
qcuProperty::changePSM_TEMPLATE_CACHE_COUNT( idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mPSMTemplateCacheCount),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-37247
IDE_RC
qcuProperty::changeSYS_CONNECT_BY_PATH_PRECISION( idvSQL* /* aStatistics */,
                                                  SChar * /* aName */,
                                                  void  * /* aOldValue */,
                                                  void  * aNewValue,
                                                  void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mSysConnectByPathPrecision),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-37302
IDE_RC
qcuProperty::changeSQL_ERROR_INFO_SIZE( idvSQL* /* aStatistics */,
                                        SChar * /* aName */,
                                        void  * /* aOldValue */,
                                        void  * aNewValue,
                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mSQLErrorInfoSize),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// PROJ-2362
IDE_RC
qcuProperty::changeREDUCE_TEMP_MEMORY_ENABLE(
    idvSQL* /* aStatistics */,
    SChar * /* aName */,
    void  * /* aOldValue */,
    void  * aNewValue,
    void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mReduceTempMemoryEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-38434
IDE_RC
qcuProperty::changeOPTIMIZER_DNF_DISABLE( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerDnfDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// fix BUG-39754
IDE_RC
qcuProperty::changeFOREIGN_KEY_LOCK_ROW( idvSQL* /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mForeignKeyLockRow),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

// BUG-40022
IDE_RC
qcuProperty::changeOPTIMIZER_JOIN_DISABLE( idvSQL* /* aStatistics */,
                                           SChar * /* aName */,
                                           void  * /* aOldValue */,
                                           void  * aNewValue,
                                           void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerJoinDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC qcuProperty::changeCONCURRENT_EXEC_DEGREE_DEFAULT( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mConcExecDegreeDefault),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC qcuProperty::changeCONCURRENT_EXEC_WAIT_INTERVAL( idvSQL* /* aStatistics */,
                                                         SChar * /* aName */,
                                                         void  * /* aOldValue */,
                                                         void  * aNewValue,
                                                         void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mConcExecWaitInterval),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2452 Caching for DETERMINISTIC Function */
IDE_RC qcuProperty::changeQUERY_EXECUTION_CACHE_MAX_COUNT( idvSQL* /* aStatistics */,
                                                           SChar * /* aName */,
                                                           void  * /* aOldValue */,
                                                           void  * aNewValue,
                                                           void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mQueryExecutionCacheMaxCount),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeQUERY_EXECUTION_CACHE_MAX_SIZE( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mQueryExecutionCacheMaxSize),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeQUERY_EXECUTION_CACHE_BUCKET_COUNT( idvSQL* /* aStatistics */,
                                                              SChar * /* aName */,
                                                              void  * /* aOldValue */,
                                                              void  * aNewValue,
                                                              void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mQueryExecutionCacheBucketCount),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeFORCE_FUNCTION_CACHE( idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mForceFunctionCache),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2441 flashback */
IDE_RC qcuProperty::changeRECYCLEBIN_MEM_MAX_SIZE( idvSQL* /* aStatistics */,
                                                   SChar * /* aName */,
                                                   void  * /* aOldValue */,
                                                   void  * aNewValue,
                                                   void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mRecyclebinMemMaxSize),
                   aNewValue,
                   ID_SIZEOF(ULong) );
    
    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeRECYCLEBIN_DISK_MAX_SIZE( idvSQL* /* aStatistics */,
                                                    SChar * /* aName */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mRecyclebinDiskMaxSize),
                   aNewValue,
                   ID_SIZEOF(ULong) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeRECYCLEBIN_ENABLE( idvSQL* /* aStatistics */,
                                             SChar * /* aName */,
                                             void  * /* aOldValue */,
                                             void  * aNewValue,
                                             void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mRecyclebinEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::change__RECYCLEBIN_FOR_NATC( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mRecyclebinNATC),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2448 Subquery caching */
IDE_RC qcuProperty::changeFORCE_SUBQUERY_CACHE_DISABLE( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mForceSubqueryCacheDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeEXECUTOR_FAST_SIMPLE_QUERY( idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mExecutorFastSimpleQuery),
                   aNewValue,
                   ID_SIZEOF(UInt) );
    
    return IDE_SUCCESS;
}

/*
 * PROJ-2553 Cache-aware Memory Hash Temp Table
 */
IDE_RC qcuProperty::changeHASH_JOIN_MEM_TEMP_PARTITIONING_DISABLE( idvSQL* /* aStatistics */,
                                                                   SChar * /* aName */,
                                                                   void  * /* aOldValue */,
                                                                   void  * aNewValue,
                                                                   void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mHashJoinMemTempPartitioningDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeHASH_JOIN_MEM_TEMP_AUTO_BUCKET_CNT_DISABLE( idvSQL* /* aStatistics */,
                                                                      SChar * /* aName */,
                                                                      void  * /* aOldValue */,
                                                                      void  * aNewValue,
                                                                      void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mHashJoinMemTempAutoBucketCntDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeHASH_JOIN_MEM_TEMP_TLB_COUNT( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mHashJoinMemTempTLBCnt),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeHASH_JOIN_MEM_TEMP_FANOUT_MODE( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mForceHashJoinMemTempFanoutMode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-41183 ORDER BY Elimination
IDE_RC qcuProperty::changeOPTIMIZER_ORDER_BY_ELIMINATION_ENABLE( idvSQL * /* aStatistics */,
                                                                 SChar  * /* aName */,
                                                                 void   * /* aOldValue */,
                                                                 void   * aNewValue,
                                                                 void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mOptimizerOrderByEliminationEnable ),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

// BUG-41398 use old sort
IDE_RC qcuProperty::changeUSE_OLD_SORT( idvSQL* /* aStatistics */,
                                        SChar * /* aName */,
                                        void  * /* aOldValue */,
                                        void  * aNewValue,
                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mUseOldSort),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeDDL_SUPPLEMENTAL_LOG_ENABLE( idvSQL* /* aStatistics */,
                                                       SChar * /* aName */,
                                                       void  * /* aOldValue */,
                                                       void  * aNewValue,
                                                       void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mDDLSupplementalLog),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-41249 DISTINCT Elimination
IDE_RC qcuProperty::changeOPTIMIZER_DISTINCT_ELIMINATION_ENABLE( idvSQL * /* aStatistics */,
                                                                 SChar  * /* aName */,
                                                                 void   * /* aOldValue */,
                                                                 void   * aNewValue,
                                                                 void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mOptimizerDistinctEliminationEnable ),
                   aNewValue,
                   ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeRESULT_CACHE_ENABLE( idvSQL* /* aStatistics */,
                                               SChar *, /* aName */
                                               void  *, /* aOleValue */
                                               void  * aNewValue,
                                               void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mResultCacheEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeTOP_RESULT_CACHE_MODE( idvSQL* /* aStatistics */,
                                                 SChar *,/* aName */
                                                 void  *,/* aOleValue */
                                                 void  * aNewValue,
                                                 void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mTopResultCacheMode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeTRCLOG_DETAIL_RESULTCACHE( idvSQL* /* aStatistics */,
                                                     SChar *,/* aName */
                                                     void  *,/* aOleValue */
                                                     void  * aNewValue,
                                                     void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mTraceLog_Detail_ResultCache),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeRESULT_CACHE_MEMORY_MAXIMUM( idvSQL* /* aStatistics */,
                                                       SChar *,/* aName */
                                                       void  *,/* aOleValue */
                                                       void  * aNewValue,
                                                       void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mResultCacheMemoryMax),
                   aNewValue,
                   ID_SIZEOF(ULong) );

    return IDE_SUCCESS;
}

// BUG-36438 LIST transformation
IDE_RC qcuProperty::changeOPTIMIZER_LIST_TRANSFORMATION( idvSQL* /* aStatistics */,
                                                         SChar * /* aName */,
                                                         void  * /* aOldValue */,
                                                         void  * aNewValue,
                                                         void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerListTransformation),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// PROJ-2492 Dynamic sample selection
IDE_RC qcuProperty::changeOPTIMIZER_AUTO_STATS( idvSQL* /* aStatistics */,
                                                       SChar *,/* aName */
                                                       void  *,/* aOleValue */
                                                       void  * aNewValue,
                                                       void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerAutoStats),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
IDE_RC qcuProperty::changeOPTIMIZER_TRANSITIVITY_OLD_RULE( idvSQL * /* aStatistics */,
                                                           SChar  * /* aName */,
                                                           void   * /* aOldValue */,
                                                           void   * aNewValue,
                                                           void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerTransitivityOldRule),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// PROJ-2582 recursive with
IDE_RC qcuProperty::changeRECURSION_LEVEL_MAXIMUM( idvSQL* /* aStatistics */,
                                                   SChar *,/* aName */
                                                   void  *,/* aOleValue */
                                                   void  * aNewValue,
                                                   void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mRecursionLevelMaximum),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2465 Tablespace Alteration for Table */
IDE_RC qcuProperty::changeDDL_MEM_USAGE_THRESHOLD( idvSQL * /* aStatistics */,
                                                   SChar  * /* aName */,
                                                   void   * /* aOldValue */,
                                                   void   * aNewValue,
                                                   void   * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY( mDDLMemUsageThreshold ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* PROJ-2465 Tablespace Alteration for Table */
IDE_RC qcuProperty::changeDDL_TBS_USAGE_THRESHOLD( idvSQL * /* aStatistics */,
                                                   SChar  * /* aName */,
                                                   void   * /* aOldValue */,
                                                   void   * aNewValue,
                                                   void   * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY( mDDLTBSUsageThreshold ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* PROJ-2465 Tablespace Alteration for Table */
IDE_RC qcuProperty::changeANALYZE_USAGE_MIN_ROWCOUNT( idvSQL * /* aStatistics */,
                                                      SChar  * /* aName */,
                                                      void   * /* aOldValue */,
                                                      void   * aNewValue,
                                                      void   * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY( mAnalyzeUsageMinRowCount ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* BUG-42639 Monitoring query */
IDE_RC qcuProperty::changeOPTIMIZER_PERFORMANCE_VIEW( idvSQL * /* aStatistics */,
                                                      SChar  * /* aName */,
                                                      void   * /* aOldValue */,
                                                      void   * aNewValue,
                                                      void   * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY( mOptimizerPerformanceView ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

// BUG-43039 inner join push down
IDE_RC qcuProperty::changeOPTIMIZER_INNER_JOIN_PUSH_DOWN( idvSQL* /* aStatistics */,
                                                          SChar *,/* aName */
                                                          void  *,/* aOleValue */
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerInnerJoinPushDown),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-43068 Indexable order by 개선
IDE_RC qcuProperty::changeOPTIMIZER_ORDER_PUSH_DOWN( idvSQL* /* aStatistics */,
                                                          SChar *,/* aName */
                                                          void  *,/* aOleValue */
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerOrderPushDown),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-43059 Target subquery unnest/removal disable
IDE_RC qcuProperty::changeOPTIMIZER_TARGET_SUBQUERY_UNNEST_DISABLE( idvSQL* /* aStatistics */,
                                                                    SChar * /* aName */,
                                                                    void  * /* aOldValue */,
                                                                    void  * aNewValue,
                                                                    void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mOptimizerTargetSubqueryUnnestDisable ),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeOPTIMIZER_TARGET_SUBQUERY_REMOVAL_DISABLE( idvSQL* /* aStatistics */,
                                                                     SChar * /* aName */,
                                                                     void  * /* aOldValue */,
                                                                     void  * aNewValue,
                                                                     void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mOptimizerTargetSubqueryRemovalDisable ),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeFORCE_AUTONOMOUS_TRANSACTION_PRAGMA( idvSQL* /* aStatistics */,
                                                               SChar * /* aName */,
                                                               void  * /* aOldValue */,
                                                               void  * aNewValue,
                                                               void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mForceAutonomousTransactionPragma ),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-43158 Enhance statement list caching in PSM
IDE_RC qcuProperty::changePSM_STMT_LIST_COUNT( idvSQL* /* aStatistics */,
                                               SChar * /* aName */,
                                               void  * /* aOldValue */,
                                               void  * aNewValue,
                                               void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mPSMStmtListCount ),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changePSM_STMT_POOL_COUNT( idvSQL* /* aStatistics */,
                                               SChar * /* aName */,
                                               void  * /* aOldValue */,
                                               void  * aNewValue,
                                               void  * /* aArg */ )
{
    UInt sNewValue = idlOS::align(*(UInt*)aNewValue, 32);

    idlOS::memcpy( &QCU_PROPERTY( mPSMStmtPoolCount ),
                   &sNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
IDE_RC qcuProperty::changeDISK_TEMP_SIZE_ESTIMATE( idvSQL* /* aStatistics */,
                                                   SChar * /* aName */,
                                                   void  * /* aOldValue */,
                                                   void  * aNewValue,
                                                   void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mDiskTempSizeEstimate ),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-43493
IDE_RC qcuProperty::changeOPTIMIZER_DELAYED_EXECUTION( idvSQL* /* aStatistics */,
                                                       SChar * /* aName */,
                                                       void  * /* aOldValue */,
                                                       void  * aNewValue,
                                                       void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY( mOptimizerDelayedExecution ),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-43495 */
IDE_RC qcuProperty::changeOPTIMIZER_LIKE_INDEX_SCAN_WITH_OUTER_COLUMN_DISABLE( idvSQL* /* aStatistics */,
                                                                               SChar * /* aName */,
                                                                               void  * /* aOldValue */,
                                                                               void  * aNewValue,
                                                                               void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerLikeIndexScanWithOuterColumnDisable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* TASK-6744 */
IDE_RC qcuProperty::changePlanRandomSeed( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mPlanRandomSeed),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-44499 */
IDE_RC qcuProperty::changeOPTIMIZER_BUCKET_COUNT_MIN( idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerBucketCountMin),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-48161 */
IDE_RC qcuProperty::changeOPTIMIZER_BUCKET_COUNT_MAX( idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerBucketCountMax),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeOPTIMIZER_HIERARCHY_TRANSFORMATION( idvSQL* /* aStatistics */,
                                                              SChar * /* aName */,
                                                              void  * /* aOldValue */,
                                                              void  * aNewValue,
                                                              void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerHierarchyTransformation),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-44692
IDE_RC qcuProperty::changeOPTIMIZER_BUG_44692( idvSQL* /* aStatistics */,
                                               SChar * /* aName */,
                                               void  * /* aOldValue */,
                                               void  * aNewValue,
                                               void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerBug44692),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// BUG-44795
IDE_RC qcuProperty::changeOPTIMIZER_DBMS_STAT_POLICY( idvSQL* /* aStatistics */,
                                                            SChar * /* aName */,
                                                            void  * /* aOldValue */,
                                                            void  * aNewValue,
                                                            void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerDBMSStatPolicy),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
IDE_RC qcuProperty::changeOPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY(idvSQL* /* aStatistics */,
                                                                       SChar * /* aName */,
                                                                       void  * /* aOldValue */,
                                                                       void  * aNewValue,
                                                                       void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerIndexNLJoinAccessMethodPolicy),
                   aNewValue,
                   ID_SIZEOF(SInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::changeOPTIMIZER_SEMI_JOIN_REMOVE(idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */)
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerSemiJoinRemove),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::change__PRINT_OUT_ENABLE( idvSQL* /* aStatistics */,
                                              SChar * /* aName */,
                                              void  * /* aOldValue */,
                                              void  * aNewValue,
                                              void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mPrintOutEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::change__KEY_PRESERVED_TABLE( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mKeyPreservedTable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC qcuProperty::change__OPTIMIZER_UNNEST_COMPATIBILITY( idvSQL* /* aStatistics */,
                                                            SChar * /* aName */,
                                                            void  * /* aOldValue */,
                                                            void  * aNewValue,
                                                            void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerUnnestCompatibility),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2632 */
IDE_RC qcuProperty::changeSERIAL_EXECUTE_MODE( idvSQL * /* aStatistics */,
                                               SChar  * /* aName */,
                                               void   * /* aOldValue */,
                                               void   * aNewValue,
                                               void   * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY( mSerialExecuteMode ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* PROJ-2632 */
IDE_RC qcuProperty::changeTRCLOG_DETAIL_INFORMATION( idvSQL * /* aStatistics */,
                                                     SChar  * /* aName */,
                                                     void   * /* aOldValue */,
                                                     void   * aNewValue,
                                                     void   * /* aArg */ )
{
    idlOS::memcpy( & QCU_PROPERTY( mTraceLogDetailInformation ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* BUG-46932 */
IDE_RC qcuProperty::changeOPTIMIZER_INVERSE_JOIN_ENABLE( idvSQL* /* aStatistics */,
                                                         SChar * /* aName */,
                                                         void  * /* aOldValue */,
                                                         void  * aNewValue,
                                                         void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerInverseJoinEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
IDE_RC qcuProperty::changeREDUCE_PARTITION_PREPARE_MEMORY( idvSQL* /* aStatistics */,
                                                           SChar * /* aName */,
                                                           void  * /* aOldValue */,
                                                           void  * aNewValue,
                                                           void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mReducePartPrepareMemory),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-47986 */
IDE_RC qcuProperty::changeOPTIMIZER_OR_VALUE_INDEX( idvSQL * /* aStatistics */,
                                                    SChar  * /* aName */,
                                                    void   * /* aOldValue */,
                                                    void   * aNewValue,
                                                    void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerOrValueIndex),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-48132 */
IDE_RC qcuProperty::changeOPTIMIZER_PLAN_HASH_OR_SORT_METHOD( idvSQL * /* aStatistics */,
                                                              SChar  * /* aName */,
                                                              void   * /* aOldValue */,
                                                              void   * aNewValue,
                                                              void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerPlanHashOrSortMethod),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-48135 */
IDE_RC qcuProperty::changeOPTIMIZER_INDEX_NL_JOIN_PENALTY( idvSQL * /* aStatistics */,
                                                           SChar  * /* aName */,
                                                           void   * /* aOldValue */,
                                                           void   * aNewValue,
                                                           void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerIndexNlJoinPenalty),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-48120 */
IDE_RC qcuProperty::changeOPTIMIZER_INDEX_COST_MODE( idvSQL * /* aStatistics */,
                                                    SChar  * /* aName */,
                                                    void   * /* aOldValue */,
                                                    void   * aNewValue,
                                                    void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mOptimizerIndexCostMode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* BUG-48776 */
IDE_RC qcuProperty::changeSUBQUERY_MODE( idvSQL * /* aStatistics */,
                                         SChar  * /* aName */,
                                         void   * /* aOldValue */,
                                         void   * aNewValue,
                                         void   * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mSubqueryMode),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

/* PROJ-2750 */
IDE_RC qcuProperty::changeLEFT_OUTER_SKIP_RIGHT_ENABLE( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */ )
{
    idlOS::memcpy( &QCU_PROPERTY(mLeftOuterSkipRightEnable),
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

