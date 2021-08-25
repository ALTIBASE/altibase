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

#include <idsCrypt.h>
#include <idv.h>
#include <mtl.h>
#include <qcm.h>
#include <qcmUser.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcTask.h>
#include <mmtSessionManager.h>
#include <mmtServiceThread.h>
#include <smi.h>
#include <idl.h>
#include <mtz.h>
#include <mtuProperty.h>
#include <mmtAdminManager.h>
#include <mmtAuditManager.h>
#include <idvAudit.h>
#include <qci.h>

/* PROJ-2177 User Interface - Cancel */
/* BUG-38496 Notify users when their password expiry date is approaching */
static IDE_RC answerConnectResult(cmiProtocolContext *aProtocolContext, 
                                  mmcSession         *aSession)
{
    UInt               sRemainingDays   = 0;
    UInt               sErrorCode;
    ULong              sReserved        = 0;
    smSCN              sSCN;
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    mmcSessID    sSessionID = aSession->getSessionID();
    qciUserInfo *sUserInfo  = aSession->getUserInfo();

    SM_INIT_SCN(&sSCN);

    /* PROJ-2733-DistTxInfo */
    if ( aSession->isGCTx() == ID_TRUE )
    {
        aSession->getLastSystemSCN( aProtocolContext->mProtocol.mOpID, &sSCN );
    }

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] answerConnectResult, SCN : %"ID_UINT64_FMT,
                aSession->getSessionTypeString(),
                sSCN);
    #endif

    /* PROJ-2733-Protocol SCN은 ConnectEx의 Reserved 영역을 활용할 수 있지만
                          정보가 추가될 수 있으니 ConnectV3을 추가해 두자. */
    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_ConnectV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 21);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
            CMI_WOP(aProtocolContext, CMP_OP_DB_ConnectV3Result);
            CMI_WR4(aProtocolContext, &sSessionID);
            sRemainingDays = qci::getRemainingGracePeriod( sUserInfo );
            if( sRemainingDays > 0 )
            {
                sErrorCode = mmERR_IGNORE_PASSWORD_GRACE_PERIOD;
            }
            else
            {
                sErrorCode = mmERR_IGNORE_NO_ERROR;
            }
            CMI_WR4(aProtocolContext, &sErrorCode);
            CMI_WR4(aProtocolContext, &sRemainingDays);
            CMI_WR8(aProtocolContext, &sSCN);
            break;

        case CMP_OP_DB_ConnectEx:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 21);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
            CMI_WOP(aProtocolContext, CMP_OP_DB_ConnectExResult);
            CMI_WR4(aProtocolContext, &sSessionID);
            sRemainingDays = qci::getRemainingGracePeriod( sUserInfo );
            if( sRemainingDays > 0 )
            {
                sErrorCode = mmERR_IGNORE_PASSWORD_GRACE_PERIOD;
            }
            else
            {
                sErrorCode = mmERR_IGNORE_NO_ERROR;
            }
            CMI_WR4(aProtocolContext, &sErrorCode);
            CMI_WR4(aProtocolContext, &sRemainingDays);
            /* reserved */
            CMI_WR8(aProtocolContext, &sReserved);
            break;

        case CMP_OP_DB_Connect:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 5);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
            CMI_WOP(aProtocolContext, CMP_OP_DB_ConnectResult);
            CMI_WR4(aProtocolContext, &sSessionID);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerDisconnectResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_DisconnectResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerPropertyGetResult(cmiProtocolContext *aProtocolContext,
                                      mmcSession         *aSession,
                                      UShort              aPropertyID,
                                      idBool             *aIsUnsupportedProperty)
{
    mmcSessionInfo    *sInfo            = aSession->getInfo();
    SChar             *sDBCharSet       = NULL;
    SChar             *sNationalCharSet = NULL;
    UInt               sLen;
    UShort             sOrgCursor       = aProtocolContext->mWriteBlock->mCursor;
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    const SChar       *sNlsTerritory    = NULL;
    const SChar       *sNlsISOCurrency  = NULL;
    SChar             *sNlsCurrency     = NULL;
    SChar             *sNlsNumChar      = NULL;

    UChar              sOpID = CMP_OP_DB_PropertyGetResult;  /* PROJ-2733-Protocol */

    switch (aPropertyID)
    {
        case CMP_DB_PROPERTY_NLS:
            sLen = idlOS::strlen(sInfo->mNlsUse);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sInfo->mNlsUse, sLen);
            break;

        case CMP_DB_PROPERTY_AUTOCOMMIT:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 4);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR1(aProtocolContext, (sInfo->mCommitMode == MMC_COMMITMODE_AUTOCOMMIT) ? 1 : 0);
            break;

        case CMP_DB_PROPERTY_EXPLAIN_PLAN:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 4);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR1(aProtocolContext, sInfo->mExplainPlan);
            break;

        case CMP_DB_PROPERTY_ISOLATION_LEVEL: /* BUG-39817 */

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mIsolationLevel);
            break;

        case CMP_DB_PROPERTY_OPTIMIZER_MODE:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mOptimizerMode);
            break;

        case CMP_DB_PROPERTY_HEADER_DISPLAY_MODE:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mHeaderDisplayMode);
            break;

        case CMP_DB_PROPERTY_STACK_SIZE:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mStackSize);
            break;

        case CMP_DB_PROPERTY_IDLE_TIMEOUT:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mIdleTimeout);
            break;

        case CMP_DB_PROPERTY_QUERY_TIMEOUT:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mQueryTimeout);
            break;

        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case CMP_DB_PROPERTY_DDL_TIMEOUT:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mDdlTimeout);
            break;

        case CMP_DB_PROPERTY_FETCH_TIMEOUT:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mFetchTimeout);
            break;

        case CMP_DB_PROPERTY_UTRANS_TIMEOUT:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mUTransTimeout);
            break;

        case CMP_DB_PROPERTY_DATE_FORMAT:
            sLen = idlOS::strlen(sInfo->mDateFormat);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sInfo->mDateFormat, sLen);
            break;

        // fix BUG-18971
        case CMP_DB_PROPERTY_SERVER_PACKAGE_VERSION:
            sLen = idlOS::strlen(iduVersionString);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, iduVersionString, sLen);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_NCHAR_LITERAL_REPLACE:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mNlsNcharLiteralReplace);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_CHARACTERSET:
            sDBCharSet = smiGetDBCharSet();

            sLen = idlOS::strlen(sDBCharSet);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sDBCharSet, sLen);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_NCHAR_CHARACTERSET:
            sNationalCharSet = smiGetNationalCharSet();

            sLen = idlOS::strlen(sNationalCharSet);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sNationalCharSet, sLen);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_ENDIAN:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 4);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR1(aProtocolContext, (iduBigEndian == ID_TRUE) ? 1 : 0);
            break;

        /* PROJ-2047 Strengthening LOB - LOBCACHE */
        case CMP_DB_PROPERTY_LOB_CACHE_THRESHOLD:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mLobCacheThreshold);

            /* BUG-42012 */
            IDU_FIT_POINT_RAISE( "answerPropertyGetResult::LobCacheThreshold",
                                 UnsupportedProperty );
            break;

        case CMP_DB_PROPERTY_REMOVE_REDUNDANT_TRANSMISSION:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sInfo->mRemoveRedundantTransmission);
            break;

            /* PROJ-2436 ADO.NET MSDTC */
        case CMP_DB_PROPERTY_TRANS_ESCALATION:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 4);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR1(aProtocolContext, 0); /* dummy value */
            break;

        case CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK( aProtocolContext, 4 );
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP( aProtocolContext, sOpID );
            CMI_WR2( aProtocolContext, &aPropertyID );
            CMI_WR1( aProtocolContext, sInfo->mGlobalTransactionLevel );
            break;

        case CMP_DB_PROPERTY_TRANSACTIONAL_DDL:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_PropertyGetResult);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, (UInt*)&sInfo->mTransactionalDDL);
            break;

        case CMP_DB_PROPERTY_GLOBAL_DDL:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_PropertyGetResult);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, (UInt*)&sInfo->mGlobalDDL);
            break;

            // PROJ-2727 add connect attr
            // INT
        case CMP_DB_PROPERTY_COMMIT_WRITE_WAIT_MODE:
        case CMP_DB_PROPERTY_ST_OBJECT_BUFFER_SIZE:            
        case CMP_DB_PROPERTY_PARALLEL_DML_MODE:
        case CMP_DB_PROPERTY_NLS_NCHAR_CONV_EXCP:            
        case CMP_DB_PROPERTY_AUTO_REMOTE_EXEC:            
        case CMP_DB_PROPERTY_TRCLOG_DETAIL_PREDICATE:            
        case CMP_DB_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:            
        case CMP_DB_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
        case CMP_DB_PROPERTY_QUERY_REWRITE_ENABLE:
        case CMP_DB_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
        case CMP_DB_PROPERTY_RECYCLEBIN_ENABLE:
        case CMP_DB_PROPERTY___USE_OLD_SORT:
        case CMP_DB_PROPERTY_ARITHMETIC_OPERATION_MODE:
        case CMP_DB_PROPERTY_RESULT_CACHE_ENABLE:
        case CMP_DB_PROPERTY_TOP_RESULT_CACHE_MODE:
        case CMP_DB_PROPERTY_OPTIMIZER_AUTO_STATS:
        case CMP_DB_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
        case CMP_DB_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
        case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC:
        case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
        case CMP_DB_PROPERTY___PRINT_OUT_ENABLE:
        case CMP_DB_PROPERTY_TRCLOG_DETAIL_SHARD:
        case CMP_DB_PROPERTY_SERIAL_EXECUTE_MODE:
        case CMP_DB_PROPERTY_TRCLOG_DETAIL_INFORMATION:
        case CMP_DB_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
        case CMP_DB_PROPERTY_NORMALFORM_MAXIMUM:
        case CMP_DB_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
        case CMP_DB_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD: /* BUG-48132 */
        case CMP_DB_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX:         /* BUG-48161 */
        case CMP_DB_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION: /* BUG-48348 */ 
            
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);

            switch (aPropertyID)
            {
                case CMP_DB_PROPERTY_COMMIT_WRITE_WAIT_MODE:
                    CMI_WR4(aProtocolContext, (UInt*)&sInfo->mCommitWriteWaitMode);
                    break;
                    
                case CMP_DB_PROPERTY_ST_OBJECT_BUFFER_SIZE:
                    CMI_WR4(aProtocolContext, &sInfo->mSTObjBufSize);
                    break;

                case CMP_DB_PROPERTY_PARALLEL_DML_MODE:
                    CMI_WR4(aProtocolContext, (UInt*)&sInfo->mParallelDmlMode);
                    break;
                                
                case CMP_DB_PROPERTY_NLS_NCHAR_CONV_EXCP:
                    CMI_WR4(aProtocolContext, &sInfo->mNlsNcharConvExcp);
                    break;
                    
                case CMP_DB_PROPERTY_AUTO_REMOTE_EXEC:
                    CMI_WR4(aProtocolContext, &sInfo->mAutoRemoteExec);
                    break;
                    
                case CMP_DB_PROPERTY_TRCLOG_DETAIL_PREDICATE:
                    CMI_WR4(aProtocolContext, &sInfo->mTrclogDetailPredicate);
                    break;
                    
                case CMP_DB_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:
                    CMI_WR4(aProtocolContext, (UInt*)&sInfo->mOptimizerDiskIndexCostAdj);
                    break;
                    
                case CMP_DB_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mOptimizerMemoryIndexCostAdj);
                    break;
                    
                case CMP_DB_PROPERTY_QUERY_REWRITE_ENABLE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mQueryRewriteEnable);
                    break;
                                
                case CMP_DB_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mDblinkRemoteStatementAutoCommit);
                    break;
            
                case CMP_DB_PROPERTY_RECYCLEBIN_ENABLE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mRecyclebinEnable);
                    break;
            
                case CMP_DB_PROPERTY___USE_OLD_SORT:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mUseOldSort);
                    break;
            
                case CMP_DB_PROPERTY_ARITHMETIC_OPERATION_MODE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mArithmeticOpMode);
                    break;
            
                case CMP_DB_PROPERTY_RESULT_CACHE_ENABLE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mResultCacheEnable);
                    break;
            
                case CMP_DB_PROPERTY_TOP_RESULT_CACHE_MODE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mTopResultCacheMode);
                    break;
            
                case CMP_DB_PROPERTY_OPTIMIZER_AUTO_STATS:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mOptimizerAutoStats);
                    break;
            
                case CMP_DB_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mOptimizerTransitivityOldRule);
                    break;
            
                case CMP_DB_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mOptimizerPerformanceView);
                    break;
            
                case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mReplicationDDLSync);
                    break;
            
                case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mReplicationDDLSyncTimeout);
                    break;
            
                case CMP_DB_PROPERTY___PRINT_OUT_ENABLE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mPrintOutEnable);
                    break;
            
                case CMP_DB_PROPERTY_TRCLOG_DETAIL_SHARD:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mTrclogDetailShard);
                    break;
            
                case CMP_DB_PROPERTY_SERIAL_EXECUTE_MODE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mSerialExecuteMode);
                    break;
            
                case CMP_DB_PROPERTY_TRCLOG_DETAIL_INFORMATION:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mTrcLogDetailInformation);
                    break;

                case CMP_DB_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mOptimizerDefaultTempTbsType);
                    break;
                    
                case CMP_DB_PROPERTY_NORMALFORM_MAXIMUM:  
                    CMI_WR4(aProtocolContext, &sInfo->mNormalFormMaximum);
                    break;
            
                case CMP_DB_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mReducePartPrepareMemory);
                    break;
                case CMP_DB_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD: /* BUG-48132 */
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mPlanHashOrSortMethod);
                    break; 
                case CMP_DB_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX: /* BUG-48161 */
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mBucketCountMax);
                    break; 
                case CMP_DB_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION: /* BUG-48348 */
                    CMI_WR4(aProtocolContext, (UInt*) &sInfo->mEliminateCommonSubexpression);
                    break;
                default:
                    break;
            }
            
            break;            

            // ULONG 
        case CMP_DB_PROPERTY_TRX_UPDATE_MAX_LOGSIZE:

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR8(aProtocolContext, &sInfo->mUpdateMaxLogSize);
            break;           

            // STRING
        case CMP_DB_PROPERTY_NLS_TERRITORY:

            sNlsTerritory = aSession->getNlsTerritory();
            
            sLen = idlOS::strlen(sNlsTerritory);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sNlsTerritory, sLen);
            break;
            
        case CMP_DB_PROPERTY_NLS_ISO_CURRENCY:
            
            sNlsISOCurrency = aSession->getNlsISOCurrency();
            
            sLen = idlOS::strlen(sNlsISOCurrency);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sNlsISOCurrency, sLen);
            break;

        case CMP_DB_PROPERTY_NLS_CURRENCY:
            
            sNlsCurrency = aSession->getNlsCurrency();
            
            sLen = idlOS::strlen(sNlsCurrency);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sNlsCurrency, sLen);
            break;
            
        case CMP_DB_PROPERTY_NLS_NUMERIC_CHARACTERS:
            
            sNlsNumChar = aSession->getNlsNumChar();
            
            sLen = idlOS::strlen(sNlsNumChar);

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 7 + sLen);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR4(aProtocolContext, &sLen);
            CMI_WCP(aProtocolContext, sNlsNumChar, sLen);
            break;

        case CMP_DB_PROPERTY_SHARD_STATEMENT_RETRY:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK( aProtocolContext, 4 );
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP( aProtocolContext, sOpID );
            CMI_WR2( aProtocolContext, &aPropertyID );
            CMI_WR1( aProtocolContext, aSession->getShardStatementRetry() );
            break;
            
        case CMP_DB_PROPERTY_INDOUBT_FETCH_TIMEOUT:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK( aProtocolContext, 7 );
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP( aProtocolContext, sOpID );
            CMI_WR2( aProtocolContext, &aPropertyID );
            CMI_WR4( aProtocolContext, &sInfo->mIndoubtFetchTimeout);
            break;
 
        case CMP_DB_PROPERTY_INDOUBT_FETCH_METHOD:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK( aProtocolContext, 4 );
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP( aProtocolContext, sOpID );
            CMI_WR2( aProtocolContext, &aPropertyID );
            CMI_WR1( aProtocolContext, sInfo->mIndoubtFetchMethod );
            break;

        case CMP_DB_PROPERTY_DDL_LOCK_TIMEOUT:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK( aProtocolContext, 7 );
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP( aProtocolContext, sOpID );
            CMI_WR2( aProtocolContext, &aPropertyID );
            CMI_WR4( aProtocolContext, &sInfo->mDDLLockTimeout);
            break;
        /* 서버-클라이언트(Session) 프로퍼티는 FIT 테스트를 추가하자. since 2015.07.09 */

        default:
            /* BUG-36256 Improve property's communication */
            IDE_RAISE(UnsupportedProperty);
            break;
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnsupportedProperty)
    {
        *aIsUnsupportedProperty = ID_TRUE;
    }

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    aProtocolContext->mWriteBlock->mCursor = sOrgCursor;

    return IDE_FAILURE;
}

static IDE_RC answerPropertySetResult(cmiProtocolContext *aProtocolContext,
                                      mmcSession         *aSession,
                                      UShort              aPropertyID)
{
    UChar sOpID = 0;
    UChar sValue1 = ID_UCHAR_MAX;
    smSCN sSCN;

    SM_INIT_SCN(&sSCN);  /* PROJ-2733-DistTxInfo */

    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_PropertySetV3:
            sOpID = CMP_OP_DB_PropertySetV3Result;
            break;

        case CMP_OP_DB_PropertySetV2:
        case CMP_OP_DB_PropertySet:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_PropertySetResult);
            IDE_CONT(NO_NEED_WORK);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2733-Protocol CMP_OP_DB_PropertySetV3부터는 프로퍼티 응답에 추가정보를 보낼 수 있다. */
    switch (aPropertyID)
    {
        case CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
            if ( aSession->isGCTx() == ID_TRUE )
            {
                aSession->getLastSystemSCN( aProtocolContext->mProtocol.mOpID, &sSCN );
            }
            sValue1 = aSession->getGlobalTransactionLevel();

            #if defined(DEBUG)
            ideLog::log(IDE_SD_18, "= [%s] answerPropertySetResult, SCN : %"ID_UINT64_FMT
                                   ", GTxLevel : %"ID_UINT32_FMT,
                        aSession->getSessionTypeString(),
                        sSCN,
                        sValue1);
            #endif

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 12);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            CMI_WR1(aProtocolContext, sValue1);
            CMI_WR8(aProtocolContext, &sSCN);
            break;

        default:
            IDE_TEST(aPropertyID >= CMP_DB_PROPERTY_MAX);  /* Non-reachable */

            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 3);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, sOpID);
            CMI_WR2(aProtocolContext, &aPropertyID);
            break;
    }

    IDE_EXCEPTION_CONT(NO_NEED_WORK);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

IDE_RC connectProtocolCore(cmiProtocolContext *aProtocolContext,
                           qciUserInfo        *aUserInfo,
                           void               *aSessionOwner )
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmcSession       *sSession;
    SChar             sDbmsName[IDP_MAX_PROP_DBNAME_LEN + 1] = {'\0', };
    SChar             sUserName[QC_MAX_OBJECT_NAME_LEN + 1] = {'\0', };
    SChar             sPassword[QC_MAX_NAME_LEN + 1] = {'\0', };
    UShort            sDbmsNameLen;
    UShort            sUserNameLen;
    UShort            sPasswordLen;
    UShort            sMode;
    UShort            sOrgCursor = aProtocolContext->mReadBlock->mCursor;

    /* TASK-5894 Permit sysdba via IPC */
    idBool            sHasSysdbaViaIPC = ID_FALSE;
    idBool            sIsSysdba        = ID_FALSE;
    SChar             sErrMsg[MAX_ERROR_MSG_LEN] = {'\0', };
    cmiLink          *sLink            = NULL;

    /* PROJ-2446 */
    idvSQL           *sStatistics = NULL;

    IDE_CLEAR();

    IDE_TEST_RAISE(sTask == NULL, NoTask);

    idlOS::memset(aUserInfo, 0, ID_SIZEOF(qciUserInfo));

    /* PROJ-2733-Protocol */
    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_ConnectV3:
        case CMP_OP_DB_ConnectEx:
        case CMP_OP_DB_Connect:
            CMI_RD2(aProtocolContext, &sDbmsNameLen);
            IDE_TEST_RAISE(sDbmsNameLen > IDP_MAX_PROP_DBNAME_LEN, DbmsNameTooLong);
            CMI_RCP(aProtocolContext, sDbmsName, sDbmsNameLen);
            CMI_RD2(aProtocolContext, &sUserNameLen);
            IDE_TEST_RAISE(sUserNameLen > QC_MAX_OBJECT_NAME_LEN, UserNameTooLong);
            CMI_RCP(aProtocolContext, sUserName, sUserNameLen);
            CMI_RD2(aProtocolContext, &sPasswordLen);
            IDE_TEST_RAISE(sPasswordLen > QC_MAX_NAME_LEN, PasswordTooLong)  /* BUG-39382 */
            CMI_RCP(aProtocolContext, sPassword, sPasswordLen);
            CMI_RD2(aProtocolContext, &sMode);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    if (sDbmsNameLen > 0)
    {
        // BUGBUG (2013-01-23) catalog를 지원하지 않으므로 property에 설정된 db name과 단순 비교한다.
        // 나중에 catalog를 지원하게 되면, user처럼 확인해야 할 것이다.
        IDE_TEST_RAISE(idlOS::strcasecmp(sDbmsName, mmuProperty::getDbName())
                       != 0, DbmsNotFound);
    }
    else
    {
        // use default dbms
        // BUGBUG (2013-01-23) connect 인자로 db name을 받지 않는 CLI를 위해, 비어있으면 기본 dbms로 연결해준다.
        // 나중에 catalog를 지원하게 되면, 기본으로 사용자가 속한 catalog를 쓰도록 해야 할 것이다.
    }

    /* TASK-5894 Permit sysdba via IPC */
    sIsSysdba = (sMode == CMP_DB_CONNECT_MODE_SYSDBA) ? ID_TRUE : ID_FALSE;
    /* BUG-43654 */
    sHasSysdbaViaIPC = mmtAdminManager::isConnectedViaIPC();

    IDE_TEST_RAISE(cmiPermitConnection(sTask->getLink(),
                                       sHasSysdbaViaIPC,
                                       sIsSysdba)
                   != IDE_SUCCESS, ConnectionNotPermitted);

    // To Fix BUG-17430
    // 무조건 대문자로 변경하면 안됨.
    sUserName[sUserNameLen] = '\0';
    mtl::makeNameInSQL( aUserInfo->loginID, sUserName, sUserNameLen );

    // To fix BUG-21137
    // password에도 double quotation이 올 수 있다.
    // 이를 makeNameInSQL함수로 제거한다.
    sPassword[sPasswordLen] = '\0';
    mtl::makePasswordInSQL( aUserInfo->loginPassword, sPassword, sPasswordLen );

    // PROJ-2002 Column Security
    // login IP(session login IP)는 모든 레코드의 컬럼마다
    // 호출하여 호출 횟수가 많아 IP를 별도로 저장한다.
    if( cmiGetLinkInfo( sTask->getLink(),
                        aUserInfo->loginIP,
                        QCI_MAX_IP_LEN,
                        CMI_LINK_INFO_REMOTE_IP_ADDRESS )
        == IDE_SUCCESS )
    {
        aUserInfo->loginIP[QCI_MAX_IP_LEN] = '\0';
    }
    else
    {
        idlOS::snprintf( aUserInfo->loginIP,
                         QCI_MAX_IP_LEN,
                         "127.0.0.1" );
        aUserInfo->loginIP[QCI_MAX_IP_LEN] = '\0';
    }

    aUserInfo->mUsrDN         = NULL;
    aUserInfo->mSvrDN         = NULL;
    aUserInfo->mCheckPassword = ID_TRUE;

    aUserInfo->mIsSysdba = sIsSysdba;

    /* BUG-40505 TCP user connect fail in windows 2008 */
    aUserInfo->mDisableTCP = QCI_DISABLE_TCP_FALSE;

    if (mmm::getCurrentPhase() == MMM_STARTUP_SERVICE)
    {
        /* >> PROJ-2446 */
        sSession = sTask->getSession();

        if (sSession != NULL)
        {
            sStatistics = sSession->getStatSQL();
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( mmtServiceThread::getUserInfoFromDB( sStatistics, 
                                                       aUserInfo ) 
                  != IDE_SUCCESS );
        /* << PROJ-2446 */
    }
    else
    {
        IDE_TEST_RAISE(aUserInfo->mIsSysdba != ID_TRUE, OnlyAdminAcceptable);

        IDE_TEST(mmtServiceThread::getUserInfoFromFile(aUserInfo) != IDE_SUCCESS);
    }

    /* PROJ-2474 SSL/TLS */
    IDE_TEST(cmiGetLinkForProtocolContext(aProtocolContext, &sLink));
    aUserInfo->mConnectType = (qciConnectType)sLink->mImpl;

    IDE_TEST(sTask->authenticate(aUserInfo) != IDE_SUCCESS);

    /*
     * Session 생성
     */

    IDE_TEST(mmtSessionManager::allocSession(sTask, aUserInfo->mIsSysdba) != IDE_SUCCESS);

    /*
     * Session 상태 확인
     */

    sSession = sTask->getSession();

    // proj_2160 cm_type removal
    // set sessionID at packet header.
    // this value in the header is currently not used.
    aProtocolContext->mWriteHeader.mA7.mSessionID =
        (UShort) sSession->getSessionID();

    IDE_TEST_RAISE(sSession->getSessionState() >= MMC_SESSION_STATE_AUTH, AlreadyConnectedError);

    /*
     * Session에 로그인정보 저장
     */

    sTask->getSession()->setUserInfo(aUserInfo);

    sSession->setSessionState(MMC_SESSION_STATE_AUTH);

    IDV_SESS_ADD(sTask->getSession()->getStatistics(), IDV_STAT_INDEX_LOGON_CUMUL, 1);
    IDV_SESS_ADD(sTask->getSession()->getStatistics(), IDV_STAT_INDEX_LOGON_CURR, 1);

    /* BUGBUG 여기서는 세션타입을 알 수 없다. */
    if ( ( sdi::isShardEnable() == ID_TRUE ) &&
         ( sSession->isShardUserSession() == ID_TRUE ) )
    {
        /* Make Shard PIN
         * SHARD_ENABLE Property 가 설정된 노드(META)만 신규 Shard PIN을 생성한다.
         * DATA 노드는 PropertySet 프로토콜을 통해 Shard PIN을 받는다.
         * */
        sSession->setNewSessionShardPin();

        /* BUG-46090 Meta Node SMN 전파 */
        sSession->setShardMetaNumber( sdi::getSMNForDataNode() );
        sSession->setLastShardMetaNumber( sSession->getShardMetaNumber() );
        sSession->setReceivedShardMetaNumber( sSession->getShardMetaNumber() );
    }
    else
    {
        /* Nothing to do. */
    }

    /* PROJ-2733 서버가 GCTx 상태에서 GCTx 미지원 클라이언트가 접속된 경우
                 GCTx 동작을 막기 위해 Multi-node transaction(Default)으로 변경한다. */
    if (aProtocolContext->mProtocol.mClientLastOpID < CMP_OP_DB_ConnectV3)
    {
        sSession->setGCTxPermit(ID_FALSE);  /* GCTx 미지원 클라이언트는 허용안함 */

        if (sSession->isGCTx() == ID_TRUE)
        {
            /* DKT_ADLP_SIMPLE_TRANSACTION_COMMIT */
            IDE_TEST(sSession->setGlobalTransactionLevel(1) != IDE_SUCCESS);
            IDE_TEST(sSession->setSessionPropertyInfo(CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL, 1)
                     != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoTask);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SESSION_NOT_SPECIFIED));
    }
    IDE_EXCEPTION(DbmsNameTooLong);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_LONG_IDENTIFIER_NAME));
    }
    IDE_EXCEPTION(DbmsNotFound);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DBMS_NOT_FOUND, sDbmsName));
    }
    IDE_EXCEPTION(UserNameTooLong);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_LONG_IDENTIFIER_NAME));
    }
    IDE_EXCEPTION(PasswordTooLong);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_LONG_IDENTIFIER_NAME));
    }
    IDE_EXCEPTION(OnlyAdminAcceptable);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADMIN_MODE_ONLY));
    }
    IDE_EXCEPTION(AlreadyConnectedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ALREADY_CONNECT_ERROR));
    }
    IDE_EXCEPTION( ConnectionNotPermitted )
    {
        idlOS::snprintf( sErrMsg, ID_SIZEOF(sErrMsg), ideGetErrorMsg() );
        IDE_SET( ideSetErrorCode(mmERR_ABORT_FAIL_TO_ESTABLISH_CONNECTION, sErrMsg) );
    }
    IDE_EXCEPTION_END;

    /* PROJ-2160 CM 타입제거
       프로토콜이 읽는 도중에 실패해도 모두 읽어야 한다. */
    aProtocolContext->mReadBlock->mCursor = sOrgCursor;

    CMI_RD2(aProtocolContext, &sDbmsNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sDbmsNameLen);
    CMI_RD2(aProtocolContext, &sUserNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sUserNameLen);
    CMI_RD2(aProtocolContext, &sPasswordLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sPasswordLen);
    CMI_RD2(aProtocolContext, &sMode);

    IDE_DASSERT( ideIsIgnore() != IDE_SUCCESS ) /*BUG-40961*/

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::connectProtocol(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    IDE_RC            sRet;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession = NULL;

    /* BUG-41986 */
    idvAuditTrail     sAuditTrail;
    qciUserInfo       sUserInfo;
    UInt              sResultCode = 0;

    sRet = connectProtocolCore( aProtocolContext, 
                                &sUserInfo,
                                aSessionOwner );

    if( sRet == IDE_SUCCESS && sThread->mErrorFlag != ID_TRUE )
    {
        sSession = ((mmcTask *)aSessionOwner)->getSession();

        /* PROJ-2177 User Interface - Cancel
         * 중복되지 않는 StmtCID 생성을 위해 SessionID 참조 */
        sRet = answerConnectResult(aProtocolContext, sSession);

        /* Set the error code for auditing */
        sResultCode = (sRet == IDE_SUCCESS) ? 0 : E_ERROR_CODE(ideGetErrorCode());
    }
    else
    {
        /* Set the error code for auditing first 
           before a cm error happends and overwrites the error. */
        sResultCode = E_ERROR_CODE(ideGetErrorCode());

        /* In case the connectProtocolCore() has failed. */
        sRet = sThread->answerErrorResult( aProtocolContext, aProtocol->mOpID, 0 );  /* PROJ-2733-Protocol */
        if ( sRet == IDE_SUCCESS )
        {
            sThread->setErrorFlag( ID_TRUE );
        }
        else
        {
            /* No error has been set in this thread
             * since the server couldn't send the error result to the client. */
        }
    }

    /* BUG-41986 */
    IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    mmtAuditManager::initAuditConnInfo( ((mmcTask *)aSessionOwner)->getSession(),
                                        &sAuditTrail, 
                                        &sUserInfo,
                                        sResultCode,
                                        QCI_AUDIT_OPER_CONNECT );

    mmtAuditManager::auditConnectInfo( &sAuditTrail );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );

    return sRet;
}

IDE_RC mmtServiceThread::disconnectProtocol(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        * /*aProtocol*/,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    mmcTask          *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession = NULL;
    IDE_RC            sRet = IDE_FAILURE;

    /* BUG-41986 */
    idvAuditTrail     sAuditTrail;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_SKIP_READ_BLOCK(aProtocolContext, 1);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_AUTH) != IDE_SUCCESS);

    aProtocolContext->mIsDisconnect = ID_TRUE;

    /* BUG-41986 
     * Auditing for disconnection must be conducted 
     * before releasing the resources related to the session to get the session information
     */
    IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    mmtAuditManager::initAuditConnInfo( sSession, 
                                        &sAuditTrail, 
                                        sSession->getUserInfo(),
                                        0,  /* success */
                                        QCI_AUDIT_OPER_DISCONNECT );

    mmtAuditManager::auditConnectInfo( &sAuditTrail );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );

    /* sharding의 모든 connection은 disconnect시 rollback한다. */
    if (sSession->isShardTrans() == ID_TRUE )
    {
        /* freeSession에서 rollback한다. */
        sSession->setSessionState(MMC_SESSION_STATE_ROLLBACK);
    }
    else
    {
        sSession->setSessionState(MMC_SESSION_STATE_END);
    }

    sSession->preFinalizeShardSession();

    /* BUG-38585 IDE_ASSERT remove */
    IDU_FIT_POINT("mmtServiceThread::disconnectProtocol::EndSession");
    IDE_TEST(sSession->endSession() != IDE_SUCCESS);

    IDE_TEST(mmtSessionManager::freeSession(sTask) != IDE_SUCCESS);

    sRet = answerDisconnectResult(aProtocolContext);

    return sRet;

    IDE_EXCEPTION_END;

    /* BUG-41986 */
    IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED_FOR_ERR_RESULT );

    mmtAuditManager::initAuditConnInfo( sSession, 
                                        &sAuditTrail, 
                                        sSession->getUserInfo(), 
                                        E_ERROR_CODE(ideGetErrorCode()),
                                        QCI_AUDIT_OPER_DISCONNECT );

    mmtAuditManager::auditConnectInfo( &sAuditTrail );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED_FOR_ERR_RESULT );

    sRet = sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Disconnect),
                                      0);
    
    return sRet;
}

IDE_RC mmtServiceThread::propertyGetProtocol(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    UShort               sPropertyID;
    idBool               sIsUnsupportedProperty = ID_FALSE;
    UInt                 sErrorIndex = 0;

    CMI_RD2(aProtocolContext, &sPropertyID);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_AUTH) != IDE_SUCCESS);

    IDE_TEST(answerPropertyGetResult(aProtocolContext,
                                     sSession,
                                     sPropertyID,
                                     &sIsUnsupportedProperty) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sIsUnsupportedProperty == ID_TRUE)
    {
        /*
         * BUG-36256 Improve property's communication
         *
         * ulnCallbackDBPropertySetResult 함수를 이용할 수 없기에 Get()도
         * 통일성을 위해 answerErrorResult를 이용해 에러를 발생시키지 않고
         * Client에게 응답을 준다.
         */
        ideLog::log(IDE_MM_0,
                    MM_TRC_GET_UNSUPPORTED_PROPERTY,
                    sPropertyID);

        IDE_SET(ideSetErrorCode(mmERR_IGNORE_UNSUPPORTED_PROPERTY, sPropertyID));

        sErrorIndex = sPropertyID;
    }
    else
    {
        /* Nothing */
    }

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, PropertyGet),
                                      sErrorIndex);
}

IDE_RC mmtServiceThread::propertySetProtocol(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcSessionInfo      *sInfo;
    UInt                 sLen;
    UInt                 sMaxLen;
    UChar                sBool;
    UInt                 sValue4; /* BUG-39817 */
    ULong                sValue8;
    UShort               sPropertyID;
    UShort               sOrgCursor;
    UInt                 sValueLen    = 0;
    UInt                 sErrorIndex  = 0;
    IDE_RC               sRC          = IDE_FAILURE;
    idBool               sHasValueLen = ID_TRUE;
    UChar               *sSessionTypeStrBefore = NULL;
    SChar                sNlsTerritory[MMC_NLSUSE_MAX_LEN + 1] = { 0, };
    SChar                sNlsISOCurrency[MMC_NLSUSE_MAX_LEN + 1] = { 0, };
    SChar                sInvokeUserName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };
    qciUserInfo         *sUserInfo;
    UInt                 sInvokeUserID;

    CMI_RD2(aProtocolContext, &sPropertyID);

    ACP_UNUSED( sSessionTypeStrBefore );

    /* BUG-41793 Keep a compatibility among tags */
    switch (aProtocol->mOpID)
    {
        /* CMP_OP_DB_PropertySetV2(1) | PropetyID(2) | ValueLen(4) | Value(?) */
        case CMP_OP_DB_PropertySetV3:
        case CMP_OP_DB_PropertySetV2:
            CMI_RD4(aProtocolContext, &sValueLen);
            break;

        /* CMP_OP_DB_PropertySet  (1) | PropetyID(2) | Value(?) */
        case CMP_OP_DB_PropertySet:
            sHasValueLen = ID_FALSE;  /* PROJ-2733-Protocol */
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    sOrgCursor = aProtocolContext->mReadBlock->mCursor;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_AUTH) != IDE_SUCCESS);

    sInfo = sSession->getInfo();

    switch (sPropertyID)
    {
        case CMP_DB_PROPERTY_CLIENT_PACKAGE_VERSION:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mClientPackageVersion) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mClientPackageVersion, sLen);
            sInfo->mClientPackageVersion[sLen] = 0;

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_CLIENT_PROTOCOL_VERSION:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            CMI_RD8(aProtocolContext, &sValue8);

            idlOS::snprintf(sInfo->mClientProtocolVersion,
                            ID_SIZEOF(sInfo->mClientProtocolVersion),
                            "%" ID_UINT32_FMT ".%" ID_UINT32_FMT ".%" ID_UINT32_FMT,
                            CM_GET_MAJOR_VERSION(sValue8),
                            CM_GET_MINOR_VERSION(sValue8),
                            CM_GET_PATCH_VERSION(sValue8));

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_CLIENT_PID:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            CMI_RD8(aProtocolContext, &(sInfo->mClientPID));

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_CLIENT_TYPE:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mClientType) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mClientType, sLen);
            sInfo->mClientType[sLen] = 0;

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_APP_INFO:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mClientAppInfo) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mClientAppInfo, sLen);
            sInfo->mClientAppInfo[sLen] = 0;

            /* PROJ-2626 Snapshot Export
             * iloader 인지 아닌지를 구분해야 될 경우 매번 string compare를 하기
             * 보다 미리 값을 정해 놓는다.
             */
            if ( ( sLen == 7 ) &&
                 ( idlOS::strncmp( sInfo->mClientAppInfo, "iloader", sLen ) == 0 ) )
            {
                sInfo->mClientAppInfoType = MMC_CLIENT_APP_INFO_TYPE_ILOADER;
            }
            else
            {
                /* Nothing to do */
            }
            break;

        case CMP_DB_PROPERTY_NLS:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mNlsUse) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mNlsUse, sLen);
            sInfo->mNlsUse[sLen] = 0;

            IDE_TEST(sSession->findLanguage() != IDE_SUCCESS);

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_AUTOCOMMIT:
            CMI_RD1(aProtocolContext, sBool);

            /* BUG-21230 */
            IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                           DCLNotAllowedError);

            IDE_TEST(sSession->setCommitMode(sBool ?
                                             MMC_COMMITMODE_AUTOCOMMIT :
                                             MMC_COMMITMODE_NONAUTOCOMMIT) != IDE_SUCCESS);

            break;

        case CMP_DB_PROPERTY_EXPLAIN_PLAN:
            CMI_RD1(aProtocolContext, sBool);

            sSession->setExplainPlan(sBool);
            break;

        case CMP_DB_PROPERTY_ISOLATION_LEVEL: /* BUG-39817 */
            CMI_RD4(aProtocolContext, &sValue4);

            IDE_TEST(sSession->setTX(SMI_ISOLATION_MASK, sValue4, ID_TRUE));
            
            break;

        case CMP_DB_PROPERTY_OPTIMIZER_MODE:
            CMI_RD4(aProtocolContext, &(sInfo->mOptimizerMode));
            
            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_OPTIMIZER_MODE,
                                                        sInfo->mOptimizerMode )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_HEADER_DISPLAY_MODE:
            CMI_RD4(aProtocolContext, &(sInfo->mHeaderDisplayMode));

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_HEADER_DISPLAY_MODE,
                                                        sInfo->mHeaderDisplayMode )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_STACK_SIZE:
            CMI_RD4(aProtocolContext, &(sInfo->mStackSize));
            break;
            
        case CMP_DB_PROPERTY_IDLE_TIMEOUT:
            CMI_RD4(aProtocolContext, &(sInfo->mIdleTimeout));

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_IDLE_TIMEOUT,
                                                        sInfo->mIdleTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_QUERY_TIMEOUT:
            CMI_RD4(aProtocolContext, &(sInfo->mQueryTimeout));

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_QUERY_TIMEOUT,
                                                        sInfo->mQueryTimeout )
                      != IDE_SUCCESS );
            break;

            /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case CMP_DB_PROPERTY_DDL_TIMEOUT:
            CMI_RD4(aProtocolContext, &(sInfo->mDdlTimeout));

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_DDL_TIMEOUT,
                                                        sInfo->mDdlTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_FETCH_TIMEOUT:
            CMI_RD4(aProtocolContext, &(sInfo->mFetchTimeout));

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_FETCH_TIMEOUT,
                                                        sInfo->mFetchTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_UTRANS_TIMEOUT:
            CMI_RD4(aProtocolContext, &(sInfo->mUTransTimeout));
            
            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_UTRANS_TIMEOUT,
                                                        sInfo->mUTransTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_DATE_FORMAT:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mDateFormat) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mDateFormat, sLen);
            sInfo->mDateFormat[sLen] = 0;

            // PROJ-2727
            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_DATE_FORMAT,
                                                        sInfo->mDateFormat,
                                                        idlOS::strlen( sInfo->mDateFormat ) )
                      != IDE_SUCCESS );
            break;
            
            /* PROJ-2209 DBTIMEZONE */
        case CMP_DB_PROPERTY_TIME_ZONE:

            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mTimezoneString) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );

            CMI_RCP(aProtocolContext, sInfo->mTimezoneString, sLen);
            sInfo->mTimezoneString[sLen] = 0;

            if ( idlOS::strCaselessMatch( sInfo->mTimezoneString, "DB_TZ" ) == 0 )
            {
                idlOS::memcpy( sInfo->mTimezoneString, MTU_DB_TIMEZONE_STRING, MTU_DB_TIMEZONE_STRING_LEN );
                sInfo->mTimezoneString[MTU_DB_TIMEZONE_STRING_LEN] = 0;
                sInfo->mTimezoneSecond = MTU_DB_TIMEZONE_SECOND;
            }
            else
            {
                IDE_TEST ( mtz::getTimezoneSecondAndString( sInfo->mTimezoneString,
                                                            &sInfo->mTimezoneSecond,
                                                            sInfo->mTimezoneString )
                           != IDE_SUCCESS );
            }

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TIME_ZONE,
                                                        sInfo->mTimezoneString,
                                                        idlOS::strlen( sInfo->mTimezoneString ) )
                      != IDE_SUCCESS );
            
            break;

            // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_NCHAR_LITERAL_REPLACE:
            CMI_RD4(aProtocolContext, &sInfo->mNlsNcharLiteralReplace);

            break;

            /* BUG-31144 */
        case CMP_DB_PROPERTY_MAX_STATEMENTS_PER_SESSION:
            CMI_RD4(aProtocolContext, &sLen);
            IDE_TEST_RAISE(sLen < sSession->getNumberOfStatementsInSession(), StatementNumberExceedsInputValue);

            sInfo->mMaxStatementsPerSession = sLen;

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_MAX_STATEMENTS_PER_SESSION,
                                                        sInfo->mMaxStatementsPerSession )
                      != IDE_SUCCESS );
            break;
            /* BUG-31390 Failover info for v$session */
        case CMP_DB_PROPERTY_FAILOVER_SOURCE:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mFailOverSource) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mFailOverSource, sLen);
            sInfo->mFailOverSource[sLen] = 0;
            break;

            /* PROJ-2047 Strengthening LOB - LOBCACHE */
        case CMP_DB_PROPERTY_LOB_CACHE_THRESHOLD:
            CMI_RD4(aProtocolContext, &sInfo->mLobCacheThreshold);

            /* BUG-42012 */
            IDU_FIT_POINT_RAISE( "mmtServiceThread::propertySetProtocol::LobCacheThreshold",
                                 UnsupportedProperty );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_LOB_CACHE_THRESHOLD,
                                                        sInfo->mLobCacheThreshold )
                      != IDE_SUCCESS );
            break;

            // PROJ-2331
        case CMP_DB_PROPERTY_REMOVE_REDUNDANT_TRANSMISSION:
            if (idlOS::strncmp(sInfo->mClientType, "NEW_JDBC", 8) == 0)
            {
                CMI_RD4(aProtocolContext, &sInfo->mRemoveRedundantTransmission );
            }
            else
            {
                CMI_SKIP_READ_BLOCK(aProtocolContext, 4);
            }
            break;

            /* PROJ-2436 ADO.NET MSDTC */
        case CMP_DB_PROPERTY_TRANS_ESCALATION:
            CMI_RD1(aProtocolContext, sBool);

            sSession->setTransEscalation(sBool ?
                                         MMC_TRANS_ESCALATE :
                                         MMC_TRANS_DO_NOT_ESCALATE);

            break;

            /* PROJ-2638 shard native linker */
        case CMP_DB_PROPERTY_SHARD_LINKER_TYPE:
            /* Nothing to do */
            break;

            /* PROJ-2638 shard native linker */
        case CMP_DB_PROPERTY_SHARD_NODE_NAME:
            CMI_RD4( aProtocolContext, &sLen );
            sMaxLen = ID_SIZEOF(sInfo->mShardNodeName) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP( aProtocolContext, sInfo->mShardNodeName, sLen );
            sInfo->mShardNodeName[sLen] = '\0';
            break;

            /* PROJ-2660 hybrid sharding */
        case CMP_DB_PROPERTY_SHARD_PIN:
            CMI_RD8( aProtocolContext, &sInfo->mShardPin );
            break;

            /* BUG-46090 Meta Node SMN 전파 */
        case CMP_DB_PROPERTY_SHARD_META_NUMBER:
            CMI_RD8( aProtocolContext, &sValue8 );

            IDU_FIT_POINT( "mmtServiceThread::propertySetProtocol::PROPERTY_SHARD_META_NUMBER" );

            /* SessionSMN = SMN, LastSessionSMN = SMN */
            if ( sSession->getShardMetaNumber() != sValue8 )
            {
                IDE_TEST( sSession->rebuildShardSession( sValue8,
                                                         NULL )
                          != IDE_SUCCESS );
            }

            sSession->cleanupShardRebuildSession();

            sSession->setReceivedShardMetaNumber( sValue8 );

            break;

        case CMP_DB_PROPERTY_REBUILD_SHARD_META_NUMBER:
            CMI_RD8( aProtocolContext, &sValue8 );

            IDU_FIT_POINT( "mmtServiceThread::propertySetProtocol::PROPERTY_REBUILD_SHARD_META_NUMBER" );

            IDE_DASSERT( sSession->getReceivedShardMetaNumber() <= sValue8 );

            /* sValues8 값이
             * ReceivedShardMetaNumber(마지막으로 수신한 SHARD_META_NUMBER, REBUILD_SHARD_META_NUMBER) 값과
             * 비교하여 동일하거나 작은 경우
             * Rebuild loop 가 발생한 경우이다.
             */
            IDU_FIT_POINT_RAISE( "mmtServiceThread::propertySetProtocol::NO_SHARD_META_CHANGE_TO_REBUILD",
                                 ERROR_NO_SHARD_META_CHANGE_TO_REBUILD );

            IDE_TEST_RAISE( sSession->getReceivedShardMetaNumber() > sValue8,
                            ERROR_NO_SHARD_META_CHANGE_TO_REBUILD );

            /* LastSessionSMN = SessionSMN, SessionSMN = RebuildSMN */
            IDE_TEST( sSession->rebuildShardSession( sValue8,
                                                     NULL )
                      != IDE_SUCCESS );

            IDE_TEST( sSession->propagateRebuildShardMetaNumber()
                      != IDE_SUCCESS );

            sSession->setReceivedShardMetaNumber( sValue8 );
            break;

            /* BUG-45707 */
        case CMP_DB_PROPERTY_SHARD_CLIENT:
            sSessionTypeStrBefore = sSession->getSessionTypeString();

            CMI_RD1( aProtocolContext, sBool );
            sSession->setShardClient( (sdiShardClient)sBool );

            #if defined(DEBUG)
            ideLog::log(IDE_SD_18, "= [%s] Changed SessionType From %s",
                        sSession->getSessionTypeString(),
                        sSessionTypeStrBefore);
            #endif
            break;

        case CMP_DB_PROPERTY_SHARD_SESSION_TYPE:
            sSessionTypeStrBefore = sSession->getSessionTypeString();

            CMI_RD1( aProtocolContext, sBool );
            sSession->setShardSessionType( (sdiSessionType)sBool );

            sSession->setIsNeedBlockCommit();

            #if defined(DEBUG)
            ideLog::log(IDE_SD_18, "= [%s] Changed SessionType From %s",
                        sSession->getSessionTypeString(),
                        sSessionTypeStrBefore);
            #endif
            break;

        case CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL:
            CMI_RD1( aProtocolContext, sBool );

            /* BUG-45844 (Server-Side) (Autocommit Mode) Multi-Transaction을 지원해야 합니다. */
            IDE_TEST( sSession->setGlobalTransactionLevel( (UInt)sBool ) != IDE_SUCCESS );

            sSession->setIsNeedBlockCommit();

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_GLOBAL_TRANSACTION_LEVEL,
                                                        sInfo->mGlobalTransactionLevel )
                      != IDE_SUCCESS );
            break;

            /* BUG-46092 */
        case CMP_DB_PROPERTY_SHARD_CLIENT_CONNECTION_REPORT:
            IDU_FIT_POINT_RAISE( "answerPropertyGetResult::ShardConnectionReport",
                                 UnsupportedProperty );

            CMI_RD4( aProtocolContext, &sValue4 );
            CMI_RD1( aProtocolContext, sBool );
            IDE_TEST( sSession->shardNodeConnectionReport( sValue4, sBool ) != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_MESSAGE_CALLBACK:  /* BUG-46019 */
            IDU_FIT_POINT_RAISE( "mmtServiceThread::propertySetProtocol::MessageCallback",
                                 UnsupportedProperty );

            CMI_RD1(aProtocolContext, sBool);
            sSession->setMessageCallback((mmcMessageCallback)sBool);
            break;
            
            // PROJ-2727 add connect attr
        case CMP_DB_PROPERTY_COMMIT_WRITE_WAIT_MODE:
            CMI_RD4(aProtocolContext,  (UInt*)&(sInfo->mCommitWriteWaitMode));

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_COMMIT_WRITE_WAIT_MODE,
                                                        sInfo->mCommitWriteWaitMode )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_SHARD_INTERNAL_LOCAL_OPERATION:
            CMI_RD1( aProtocolContext, sBool );
            sSession->setShardInternalLocalOperation( (sdiInternalOperation)sBool );
            break;

        case CMP_DB_PROPERTY_ST_OBJECT_BUFFER_SIZE:
            CMI_RD4(aProtocolContext,  &sInfo->mSTObjBufSize);
 
            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_ST_OBJECT_BUFFER_SIZE,
                                                        sInfo->mSTObjBufSize )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_TRX_UPDATE_MAX_LOGSIZE:
            CMI_RD8( aProtocolContext, &sInfo->mUpdateMaxLogSize );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TRX_UPDATE_MAX_LOGSIZE,
                                                        sInfo->mUpdateMaxLogSize )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_PARALLEL_DML_MODE:
            CMI_RD4(aProtocolContext, (UInt*)&sInfo->mParallelDmlMode);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_PARALLEL_DML_MODE,
                                                        sInfo->mParallelDmlMode )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_NLS_NCHAR_CONV_EXCP:
            CMI_RD4(aProtocolContext, &sInfo->mNlsNcharConvExcp);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_NLS_NCHAR_CONV_EXCP,
                                                        sInfo->mNlsNcharConvExcp )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_AUTO_REMOTE_EXEC:
            CMI_RD4(aProtocolContext, &sInfo->mAutoRemoteExec);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_AUTO_REMOTE_EXEC,
                                                        sInfo->mAutoRemoteExec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_TRCLOG_DETAIL_PREDICATE:
            CMI_RD4(aProtocolContext, &sInfo->mTrclogDetailPredicate);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TRCLOG_DETAIL_PREDICATE,
                                                        sInfo->mTrclogDetailPredicate )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:
            CMI_RD4(aProtocolContext, (UInt*)&sInfo->mOptimizerDiskIndexCostAdj);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ,
                                                        sInfo->mOptimizerDiskIndexCostAdj )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
            CMI_RD4(aProtocolContext, (UInt*)&sInfo->mOptimizerMemoryIndexCostAdj);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ,
                                                        sInfo->mOptimizerMemoryIndexCostAdj )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_NLS_TERRITORY:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sNlsTerritory) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sNlsTerritory, sLen);
            sNlsTerritory[sLen] = 0;
            
            IDE_TEST( sSession->setNlsTerritory((SChar *)sNlsTerritory,
                                                idlOS::strlen(sNlsTerritory))
                      != IDE_SUCCESS );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_NLS_TERRITORY,
                                                        sNlsTerritory,
                                                        idlOS::strlen( sNlsTerritory ) )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_NLS_ISO_CURRENCY:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sNlsISOCurrency) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sNlsISOCurrency, sLen);
            sNlsISOCurrency[sLen] = 0;

            // code 값을 국가 이름으로 변경
            
            IDE_TEST( sSession->setNlsISOCurrency((SChar *)sNlsISOCurrency,
                                                  idlOS::strlen(sNlsISOCurrency))
                      != IDE_SUCCESS );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_NLS_ISO_CURRENCY,
                                                        sNlsISOCurrency,
                                                        idlOS::strlen( sNlsISOCurrency ) )    
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_NLS_CURRENCY:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mNlsCurrency) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mNlsCurrency, sLen);
            sInfo->mNlsCurrency[sLen] = 0;

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_NLS_CURRENCY,
                                                        sInfo->mNlsCurrency,
                                                        idlOS::strlen( sInfo->mNlsCurrency ) )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_NLS_NUMERIC_CHARACTERS:
            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInfo->mNlsNumChar) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );
            CMI_RCP(aProtocolContext, sInfo->mNlsNumChar, sLen);
            sInfo->mNlsNumChar[sLen] = 0;

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_NLS_NUMERIC_CHARACTERS,
                                                        sInfo->mNlsNumChar,
                                                        idlOS::strlen( sInfo->mNlsNumChar ) )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_QUERY_REWRITE_ENABLE:
            CMI_RD4(aProtocolContext,  &sInfo->mQueryRewriteEnable);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_QUERY_REWRITE_ENABLE,
                                                        sInfo->mQueryRewriteEnable )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
            CMI_RD4(aProtocolContext,  &sInfo->mDblinkRemoteStatementAutoCommit);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT,
                                                        sInfo->mDblinkRemoteStatementAutoCommit )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_RECYCLEBIN_ENABLE:
            CMI_RD4(aProtocolContext,  &sInfo->mRecyclebinEnable);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_RECYCLEBIN_ENABLE,
                                                        sInfo->mRecyclebinEnable )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY___USE_OLD_SORT:
            CMI_RD4(aProtocolContext,  &sInfo->mUseOldSort);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___USE_OLD_SORT,
                                                        sInfo->mUseOldSort )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_ARITHMETIC_OPERATION_MODE:
            CMI_RD4(aProtocolContext,  &sInfo->mArithmeticOpMode);
            
            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_ARITHMETIC_OPERATION_MODE,
                                                        sInfo->mArithmeticOpMode )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_RESULT_CACHE_ENABLE:
            CMI_RD4(aProtocolContext,  &sInfo->mResultCacheEnable);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_RESULT_CACHE_ENABLE,
                                                        sInfo->mResultCacheEnable )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_TOP_RESULT_CACHE_MODE:
            CMI_RD4(aProtocolContext,  &sInfo->mTopResultCacheMode);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TOP_RESULT_CACHE_MODE,
                                                        sInfo->mTopResultCacheMode )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_OPTIMIZER_AUTO_STATS:
            CMI_RD4(aProtocolContext,  &sInfo->mOptimizerAutoStats);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_OPTIMIZER_AUTO_STATS,
                                                        sInfo->mOptimizerAutoStats )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
            CMI_RD4(aProtocolContext,  &sInfo->mOptimizerTransitivityOldRule);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE,
                                                        sInfo->mOptimizerTransitivityOldRule )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
            CMI_RD4(aProtocolContext,  &sInfo->mOptimizerPerformanceView);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW,
                                                        sInfo->mOptimizerPerformanceView )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC:
            CMI_RD4(aProtocolContext,  &sInfo->mReplicationDDLSync);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_REPLICATION_DDL_SYNC,
                                                        sInfo->mReplicationDDLSync )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
            CMI_RD4(aProtocolContext,  &sInfo->mReplicationDDLSyncTimeout);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT,
                                                        sInfo->mReplicationDDLSyncTimeout )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY___PRINT_OUT_ENABLE:
            CMI_RD4(aProtocolContext,  &sInfo->mPrintOutEnable);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___PRINT_OUT_ENABLE,
                                                        sInfo->mPrintOutEnable )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_TRCLOG_DETAIL_SHARD:
            CMI_RD4(aProtocolContext,  &sInfo->mTrclogDetailShard);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TRCLOG_DETAIL_SHARD,
                                                        sInfo->mTrclogDetailShard )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_SERIAL_EXECUTE_MODE:
            CMI_RD4(aProtocolContext,  &sInfo->mSerialExecuteMode);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_SERIAL_EXECUTE_MODE,
                                                        sInfo->mSerialExecuteMode )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY_TRCLOG_DETAIL_INFORMATION:
            CMI_RD4(aProtocolContext,  &sInfo->mTrcLogDetailInformation);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TRCLOG_DETAIL_INFORMATION,
                                                        sInfo->mTrcLogDetailInformation )
                      != IDE_SUCCESS );
            break;
            
        case CMP_DB_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
            CMI_RD4(aProtocolContext,  &sInfo->mOptimizerDefaultTempTbsType);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE,
                                                        sInfo->mOptimizerDefaultTempTbsType )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_NORMALFORM_MAXIMUM:
            CMI_RD4(aProtocolContext, &sInfo->mNormalFormMaximum);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_NORMALFORM_MAXIMUM,
                                                        sInfo->mNormalFormMaximum )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
            CMI_RD4(aProtocolContext,  &sInfo->mReducePartPrepareMemory);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY,
                                                        sInfo->mReducePartPrepareMemory )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_SHARD_STATEMENT_RETRY:
            CMI_RD1( aProtocolContext, sBool );

            sSession->setShardStatementRetry( (UInt)sBool );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_SHARD_STATEMENT_RETRY,
                                                        sSession->getShardStatementRetry() )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_INDOUBT_FETCH_TIMEOUT:
            CMI_RD4( aProtocolContext, &sInfo->mIndoubtFetchTimeout );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_INDOUBT_FETCH_TIMEOUT,
                                                        sInfo->mIndoubtFetchTimeout )
                      != IDE_SUCCESS );
            
            /* BUG-48250 */
            if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) &&
                 ( mmcTrans::getSmiTrans( sSession->getTransPtr() ) != NULL ) )
            {
                mmcTrans::getSmiTrans( sSession->getTransPtr() )->setIndoubtFetchTimeout( sInfo->mIndoubtFetchTimeout );
            }

            break;

        case CMP_DB_PROPERTY_INDOUBT_FETCH_METHOD:
            CMI_RD1( aProtocolContext, sBool );

            sSession->setIndoubtFetchMethod( (UInt)sBool );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_INDOUBT_FETCH_METHOD,
                                                        sInfo->mIndoubtFetchMethod )
                      != IDE_SUCCESS );

            /* BUG-48250 */
            if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) &&
                 ( mmcTrans::getSmiTrans( sSession->getTransPtr() ) != NULL ) )
            {
                mmcTrans::getSmiTrans( sSession->getTransPtr() )->setIndoubtFetchMethod( sInfo->mIndoubtFetchMethod );
            }

            break;

            /* 서버-클라이언트(Session) 프로퍼티는 FIT 테스트를 추가하자. since 2015.07.09 */

        case CMP_DB_PROPERTY_TRANSACTIONAL_DDL:
            CMI_RD4(aProtocolContext, &sValue4 );

            IDE_TEST( sSession->setTransactionalDDL( (idBool)sValue4 ) != IDE_SUCCESS );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_TRANSACTIONAL_DDL,
                                                        sInfo->mTransactionalDDL )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_PROPERTY_INVOKE_USER:
            sUserInfo = sSession->getUserInfo();

            CMI_RD4(aProtocolContext, &sLen);
            sMaxLen = ID_SIZEOF(sInvokeUserName) - 1;
            IDE_TEST_RAISE( sLen > sMaxLen, InvalidLengthError );

            // 1. Invoke user name을 받는다. 
            CMI_RCP(aProtocolContext, sInvokeUserName, sLen);
            sInvokeUserName[sLen] = '\0';

            // 2. TODO
            if ( sSession->isShardCoordinatorSession() == ID_TRUE )
            {
                // 3. invokeUserName을 user id로 변환
                //      - User name에 해당하는 user가 없으면
                //        "user not found" ERROR가 올라온다.
                IDE_TEST( qciMisc::getUserIdByName( sInvokeUserName,
                                                    &sInvokeUserID )
                          != IDE_SUCCESS );

                idlOS::strncpy( sUserInfo->invokeUserName, sInvokeUserName, sLen );
                sUserInfo->invokeUserName[sLen] = '\0';

                // 4. userId에 invokeUserName에 해당하는 user id를 설정
                sUserInfo->userID = sInvokeUserID;
                // 5. invokeUserNamePtr이 invokeUserName을 pointing
                sUserInfo->invokeUserNamePtr = (SChar*)&(sUserInfo->invokeUserName);

                IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_INVOKE_USER,
                                                            sInvokeUserName,
                                                            sLen )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_RAISE(UnsupportedProperty);
            }
            break;

        case CMP_DB_PROPERTY_DDL_LOCK_TIMEOUT:
            CMI_RD4( aProtocolContext, &sInfo->mDDLLockTimeout );

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_DDL_LOCK_TIMEOUT,
                                                        sInfo->mDDLLockTimeout )
                      != IDE_SUCCESS );
            break;
        case CMP_DB_PROPERTY_GLOBAL_DDL:
            CMI_RD4(aProtocolContext, &sValue4 );

            IDE_TEST( sSession->setGlobalDDL( (idBool)sValue4 ) != IDE_SUCCESS );

            /* PROJ-2757 Advanced Global DDL */
            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY_GLOBAL_DDL,
                                                        sInfo->mGlobalDDL )
                      != IDE_SUCCESS );
            break;
        /* BUG-48132 */
        case CMP_DB_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD:
            CMI_RD4(aProtocolContext,  &sInfo->mPlanHashOrSortMethod);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD,
                                                        sInfo->mPlanHashOrSortMethod )
                      != IDE_SUCCESS );
            break;
        /* BUG-48161 */
        case CMP_DB_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX:
            CMI_RD4(aProtocolContext,  &sInfo->mBucketCountMax);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX,
                                                        sInfo->mBucketCountMax )
                      != IDE_SUCCESS );
            break;
        /* BUG-48348 */
        case CMP_DB_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION:
            CMI_RD4(aProtocolContext,  &sInfo->mEliminateCommonSubexpression);

            IDE_TEST( sSession->setSessionPropertyInfo( CMP_DB_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION,
                                                        sInfo->mEliminateCommonSubexpression )
                      != IDE_SUCCESS );
            break;
        default:
            /* BUG-36256 Improve property's communication */
            IDE_RAISE(UnsupportedProperty);
            break;
    }

    return answerPropertySetResult(aProtocolContext, sSession, sPropertyID);

    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(InvalidSessionState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_STATE));
    }
    IDE_EXCEPTION(InvalidLengthError)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_VALUE_LENGTH_EXCEED,
                                cmpGetDbPropertyName(sPropertyID), sMaxLen));
    }

    /* BUG-31144 */
    IDE_EXCEPTION(StatementNumberExceedsInputValue);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_STATEMENT_NUMBER_EXCEEDS_INPUT_VALUE));
    }

    IDE_EXCEPTION(UnsupportedProperty)
    {
        /*
         * BUG-36256 Improve property's communication
         *
         * CMP_OP_DB_ProprtySetResult는 OP(2)만 보내기 때문에 하위 호환성을
         * 유지하기 위해서는 ulnCallbackDBPropertySetResult 함수를 이용할 수 없다.
         * answerErrorResult를 이용해 에러를 발생시키지 않고 Client에게 응답을 준다.
         * 초기 설계가 아쉬운 부분이다.
         */
        ideLog::log(IDE_MM_0,
                    MM_TRC_SET_UNSUPPORTED_PROPERTY,
                    sPropertyID);

        IDE_SET(ideSetErrorCode(mmERR_IGNORE_UNSUPPORTED_PROPERTY, sPropertyID));

        sErrorIndex = sPropertyID;
    }
    IDE_EXCEPTION( ERROR_NO_SHARD_META_CHANGE_TO_REBUILD )
    {
        SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];

        idlOS::snprintf( sMsgBuf,
                         SMI_MAX_ERR_MSG_LEN,
                         "SESSION-SMN:%"ID_UINT64_FMT" RECV-SMN:%"ID_UINT64_FMT" DATA-SMN:%"ID_UINT64_FMT" REBUILD-SMN:%"ID_UINT64_FMT"",
                         sSession->getShardMetaNumber(),
                         sSession->getReceivedShardMetaNumber(),
                         sdi::getSMNForDataNode(),
                         sValue8 );

        IDE_SET( ideSetErrorCode( mmERR_ABORT_NO_SHARD_META_CHANGE_TO_REBUILD,
                                  sMsgBuf ) );
    }

    IDE_EXCEPTION_END;

    /* PROJ-2160 CM 타입제거
       프로토콜이 읽는 도중에 실패해도 모두 읽어야 한다. */
    aProtocolContext->mReadBlock->mCursor = sOrgCursor;

    /* BUG-41793 Keep a compatibility among tags */
    if (sHasValueLen == ID_TRUE) /* PROJ-2733-Protocol */
    {
        CMI_SKIP_READ_BLOCK(aProtocolContext, sValueLen);
    }
    else
    {
        /*
         * 6.3.1과의 하위호환성을 위해 둔다.
         * 프로퍼티가 추가되어도 아래에 추가할 필요가 없다. since CM 7.1.3
         */
        switch (sPropertyID)
        {
            case CMP_DB_PROPERTY_AUTOCOMMIT:
            case CMP_DB_PROPERTY_EXPLAIN_PLAN:
            case CMP_DB_PROPERTY_TRANS_ESCALATION: /* PROJ-2436 ADO.NET MSDTC */
                CMI_SKIP_READ_BLOCK(aProtocolContext, 1);
                break;

            case CMP_DB_PROPERTY_ISOLATION_LEVEL: /* BUG-39817 */
            case CMP_DB_PROPERTY_OPTIMIZER_MODE:
            case CMP_DB_PROPERTY_HEADER_DISPLAY_MODE:
            case CMP_DB_PROPERTY_STACK_SIZE:
            case CMP_DB_PROPERTY_IDLE_TIMEOUT:
            case CMP_DB_PROPERTY_QUERY_TIMEOUT:
            case CMP_DB_PROPERTY_FETCH_TIMEOUT:
            case CMP_DB_PROPERTY_UTRANS_TIMEOUT:
            case CMP_DB_PROPERTY_NLS_NCHAR_LITERAL_REPLACE:
            case CMP_DB_PROPERTY_MAX_STATEMENTS_PER_SESSION:
            case CMP_DB_PROPERTY_DDL_TIMEOUT:
            case CMP_DB_PROPERTY_LOB_CACHE_THRESHOLD:  /* BUG-36966 */
            case CMP_DB_PROPERTY_REMOVE_REDUNDANT_TRANSMISSION:
                CMI_SKIP_READ_BLOCK(aProtocolContext, 4);
                break;

            case CMP_DB_PROPERTY_CLIENT_PACKAGE_VERSION:
            case CMP_DB_PROPERTY_CLIENT_TYPE:
            case CMP_DB_PROPERTY_APP_INFO:
            case CMP_DB_PROPERTY_NLS:
            case CMP_DB_PROPERTY_DATE_FORMAT:
            case CMP_DB_PROPERTY_TIME_ZONE:
            case CMP_DB_PROPERTY_FAILOVER_SOURCE:
                CMI_RD4(aProtocolContext, &sLen);
                CMI_SKIP_READ_BLOCK(aProtocolContext, sLen);
                break;

            case CMP_DB_PROPERTY_CLIENT_PROTOCOL_VERSION:
            case CMP_DB_PROPERTY_CLIENT_PID:
                CMI_SKIP_READ_BLOCK(aProtocolContext, 8);
                break;

            default:
                break;
        }
    }

    /* PROJ-2733-Protocol 하위 호환성으로 인해 아래 코드가 추가된다.
                          CMP_OP_DB_PropertySetV?가 추가되어도 여기는 신경쓸 필요가 없다. */
    if ((aProtocol->mOpID == CMP_OP_DB_PropertySetV2) ||
        (aProtocol->mOpID == CMP_OP_DB_PropertySet))
    {
        sRC = sThread->answerErrorResult(aProtocolContext,
                                         CMI_PROTOCOL_OPERATION(DB, PropertySet),
                                         sErrorIndex);
    }
    else
    {
        sRC = sThread->answerErrorResult(aProtocolContext,
                                         aProtocol->mOpID,
                                         sErrorIndex);
    }

    return sRC;
}

/*********************************************************/
/* proj_2160 cm_type removal
 * new callback for DB Handshake protocol added.
 * old version: BASE handshake */
IDE_RC mmtServiceThread::handshakeProtocol(cmiProtocolContext *aCtx,
                                           cmiProtocol        *aProtocol,
                                           void               * /*aSessionOwner*/,
                                           void               *aUserContext)
{
    UChar                    sModuleID;      /* 1: DB, 2: RP */
    UChar                    sMajorVersion;  /* CM major ver of client */
    UChar                    sMinorVersion;  /* CM minor ver of client */
    UChar                    sPatchVersion;  /* CM patch ver of client */
    UChar                    sLastOpID;      /* PROJ-2733-Protocol */

    cmpArgBASEHandshake      sArgA5;
    cmpArgBASEHandshake*     sResultA5;
    cmiProtocol              sProtocol;

    mmtServiceThread*        sThread  = (mmtServiceThread *)aUserContext;

    IDE_CLEAR();

    // client is A7 or higher.
    if (cmiGetPacketType(aCtx) != CMP_PACKET_TYPE_A5)
    {
        /* PROJ-2733-Protocol-BUGBUG Handshake에 더 많은 정보를 넣을려면 프로토콜을 추가해야 한다. */
        CMI_RD1(aCtx, sModuleID);
        CMI_RD1(aCtx, sMajorVersion);
        CMI_RD1(aCtx, sMinorVersion);
        CMI_RD1(aCtx, sPatchVersion);
        CMI_RD1(aCtx, sLastOpID);

        /* PROJ-2733-Protocol Reserve 영역을 이용하자. */
        if (sLastOpID != 0)
        {
            aProtocol->mClientLastOpID = sLastOpID;
            sLastOpID = CMP_OP_DB_MAX - 1;
        }

        IDE_TEST_RAISE(sModuleID != aCtx->mModule->mModuleID, InvalidModule);
        sModuleID     = CMP_MODULE_DB;
        sMajorVersion = CM_MAJOR_VERSION;
        sMinorVersion = CM_MINOR_VERSION;
        sPatchVersion = CM_PATCH_VERSION;

        CMI_WOP(aCtx, CMP_OP_DB_HandshakeResult);
        CMI_WR1(aCtx, sModuleID);
        CMI_WR1(aCtx, sMajorVersion);
        CMI_WR1(aCtx, sMinorVersion);
        CMI_WR1(aCtx, sPatchVersion);
        CMI_WR1(aCtx, sLastOpID);

        if (cmiGetLinkImpl(aCtx) == CMI_LINK_IMPL_IPCDA)
        {
            /* PROJ-2616 */
            MMT_IPCDA_INCREASE_DATA_COUNT(aCtx);
        }
        else
        {
            IDE_TEST(cmiSend(aCtx, ID_TRUE) != IDE_SUCCESS);
        }
    }
    // client is A5. so let's send a A5 packet through A5 CM.
    else
    {
        CMI_RD1(aCtx, sArgA5.mBaseVersion);
        CMI_RD1(aCtx, sArgA5.mModuleID);
        CMI_RD1(aCtx, sArgA5.mModuleVersion);

        IDE_TEST_RAISE(sArgA5.mModuleID != aCtx->mModule->mModuleID, InvalidModule);

        // initialize writeBlock's header using recved header
        aCtx->mWriteHeader.mA5.mHeaderSign      = CMP_HEADER_SIGN_A5;
        aCtx->mWriteHeader.mA5.mModuleID        = aCtx->mReadHeader.mA5.mModuleID;
        aCtx->mWriteHeader.mA5.mModuleVersion   = aCtx->mReadHeader.mA5.mModuleVersion;
        aCtx->mWriteHeader.mA5.mSourceSessionID = aCtx->mReadHeader.mA5.mTargetSessionID;
        aCtx->mWriteHeader.mA5.mTargetSessionID = aCtx->mReadHeader.mA5.mSourceSessionID;

        // mModule has to be set properly for cmiWriteProtocol
        aCtx->mModule = gCmpModule[CMP_MODULE_BASE];
        CMI_PROTOCOL_INITIALIZE(sProtocol, sResultA5, BASE, Handshake);

        sResultA5->mBaseVersion   = CMP_VER_BASE_MAX - 1; // 1
        sResultA5->mModuleID      = CMP_MODULE_DB;        // 1
        sResultA5->mModuleVersion = CM_PATCH_VERSION;

        IDE_TEST(cmiWriteProtocol(aCtx, &sProtocol) != IDE_SUCCESS);

        IDE_TEST(cmiFlushProtocol(aCtx, ID_TRUE) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidModule);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_MODULE));
    }
    IDE_EXCEPTION_END;

    if (cmiGetPacketType(aCtx) != CMP_PACKET_TYPE_A5)
    {
        return sThread->answerErrorResult(aCtx,
                                          CMI_PROTOCOL_OPERATION(DB, Handshake),
                                          0);
    }
    else
    {
        return IDE_FAILURE;
    }
}
