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
 

#include <cm.h>
#include <qci.h>
#include <qcg.h>
#include <qsxEnv.h>
#include <qsxCursor.h>
#include <qsxExecutor.h>
#include <qcpManager.h>
#include <qsvEnv.h>
#include <qdtAlter.h>
#include <qdc.h>
#include <qmgDef.h>
#include <qcmUser.h>
#include <qcmProc.h>
#include <qcmDatabase.h>
#include <qcmDNUser.h>
#include <mtuProperty.h>
#include <qmmParseTree.h>
#include <qriParseTree.h>
#include <qsv.h>
#include <qsx.h>
#include <qsxUtil.h>
#include <qcgPlan.h>
#include <qcuTemporaryObj.h>
#include <idx.h>
#include <qdkParseTree.h>
#include <qdk.h>
#include <qmr.h>
#include <qdcAudit.h>
#include <qcuSessionPkg.h>
#include <qdcJob.h>
#include <qdpRole.h>
#include <qsc.h>
#include <qmxResultCache.h>
#include <qsxArray.h>
#include <sdi.h>
#include <sdiZookeeper.h>

qciStartupPhase                 qci::mStartupPhase = QCI_STARTUP_INIT;
qciSessionCallback              qci::mSessionCallback;
qciOutBindLobCallback           qci::mOutBindLobCallback;
qciCloseOutBindLobCallback      qci::mCloseOutBindLobCallback;
qciInternalSQLCallback          qci::mInternalSQLCallback;
qciSetParamData4RebuildCallback qci::mSetParamData4RebuildCallback;

/* PROJ-2240 */
qciValidateReplicationCallback  qci::mValidateReplicationCallback;
qciExecuteReplicationCallback   qci::mExecuteReplicationCallback;
qciCatalogReplicationCallback   qci::mCatalogReplicationCallback;
qciManageReplicationCallback    qci::mManageReplicationCallback;

/* PROJ-2223 */
qciAuditManagerCallback         qci::mAuditManagerCallback;

/* PROJ-2626 Snapshot Export */
qciSnapshotCallback             qci::mSnapshotCallback;
idBool                          qci::mInplaceUpdateDisable = ID_FALSE;

IDE_RC
qci::initializeStmtInfo( qciStmtInfo * aStmtInfo,
                         void        * aMmStatement )
{    
    aStmtInfo->mSmiStmtForExecute = NULL;
    aStmtInfo->mMmStatement = aMmStatement;
    aStmtInfo->mIsQpAlloc   = ID_FALSE;

    return IDE_SUCCESS;
}

// BUG-43158 Enhance statement list caching in PSM
IDE_RC qci::initializeStmtListInfo( qcStmtListInfo * aStmtListInfo )
{
    UInt sStage = 0;
    UInt sPoolSize = 0;
    UInt i;
    UInt j;

    aStmtListInfo->mStmtListCount      = QCU_PSM_STMT_LIST_COUNT;
    aStmtListInfo->mStmtListCursor     = 0;
    aStmtListInfo->mStmtListFreedCount = 0;
    aStmtListInfo->mStmtPoolCount      = QCU_PSM_STMT_POOL_COUNT;
    aStmtListInfo->mStmtList           = NULL;
    aStmtListInfo->mStmtList2          = NULL;
    aStmtListInfo->mUsePtr             = ID_FALSE;

    if ( aStmtListInfo->mStmtListCount > 0 )
    {
        if ( aStmtListInfo->mStmtPoolCount > QC_PSM_POOL_DEFAULT )
        {
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                         ID_SIZEOF( qsxStmtList ) * aStmtListInfo->mStmtListCount,
                                         (void**)&aStmtListInfo->mStmtList )
                      != IDE_SUCCESS );
            sStage = 2;
            sPoolSize = idlOS::align8( ID_SIZEOF( void * ) * aStmtListInfo->mStmtPoolCount );
            aStmtListInfo->mUsePtr = ID_TRUE;

            /* PoolCount의 최소값이 32이다 */
            aStmtListInfo->mStmtPoolStatusSize = 
                idlOS::align8( ( ID_SIZEOF( UChar ) * aStmtListInfo->mStmtPoolCount ) / QC_UCHAR_BIT );

            for ( i = 0; i < aStmtListInfo->mStmtListCount; i++ )
            {
                aStmtListInfo->mStmtList[i].mParseTree      = NULL;
                aStmtListInfo->mStmtList[i].mStmtPool       = NULL;
                aStmtListInfo->mStmtList[i].mStmtPoolStatus = NULL;
                aStmtListInfo->mStmtList[i].mNext           = NULL;

                IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                             sPoolSize,
                                             (void**)&aStmtListInfo->mStmtList[i].mStmtPool )
                          != IDE_SUCCESS );

                IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                             aStmtListInfo->mStmtPoolStatusSize,
                                             (void**)&aStmtListInfo->mStmtList[i].mStmtPoolStatus )
                          != IDE_SUCCESS );

                idlOS::memset( aStmtListInfo->mStmtList[i].mStmtPoolStatus,
                               0,
                               aStmtListInfo->mStmtPoolStatusSize );
            }
        }
        else
        {
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                         ID_SIZEOF( qsxStmtList2 ) * aStmtListInfo->mStmtListCount,
                                         (void**)&aStmtListInfo->mStmtList2 )
                      != IDE_SUCCESS );
            sStage = 1;

            /* PoolCount의 최소값이 32이다 */
            aStmtListInfo->mStmtPoolStatusSize = 
                idlOS::align8( ( ID_SIZEOF( UChar ) * aStmtListInfo->mStmtPoolCount ) / QC_UCHAR_BIT );

            for ( i = 0; i < aStmtListInfo->mStmtListCount; i++ )
            {
                aStmtListInfo->mStmtList2[i].mParseTree      = NULL;
                aStmtListInfo->mStmtList2[i].mNext           = NULL;

                idlOS::memset( aStmtListInfo->mStmtList2[i].mStmtPoolStatus,
                               0,
                               aStmtListInfo->mStmtPoolStatusSize );
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            {
                for ( j = 0; j <= i; j++ )
                {
                    if ( aStmtListInfo->mStmtList[j].mStmtPool != NULL )
                    {
                        (void)iduMemMgr::free( aStmtListInfo->mStmtList[j].mStmtPool );
                        aStmtListInfo->mStmtList[j].mStmtPool = NULL;
                    }

                    if ( aStmtListInfo->mStmtList[j].mStmtPoolStatus != NULL )
                    {
                        (void)iduMemMgr::free( aStmtListInfo->mStmtList[j].mStmtPoolStatus );
                        aStmtListInfo->mStmtList[j].mStmtPoolStatus = NULL;
                    }
                }

                (void)iduMemMgr::free( aStmtListInfo->mStmtList );
                aStmtListInfo->mStmtList = NULL;
                break;
            }
        case 1:
            {
                (void)iduMemMgr::free( aStmtListInfo->mStmtList2 );
                aStmtListInfo->mStmtList2 = NULL;
                break;
            }
        case 0:
            // Nothing to do.
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

// BUG-43158 Enhance statement list caching in PSM
IDE_RC qci::finalizeStmtListInfo( qcStmtListInfo * aStmtListInfo )
{
    IDE_RC  sRet;
    UInt    i;
    UInt    sErrorCode;
    SChar * sErrorMsg;

    IDE_TEST_CONT( aStmtListInfo->mStmtListCount <= 0, normal_exit );

    if ( aStmtListInfo->mStmtList != NULL )
    {
        for ( i = 0; i < aStmtListInfo->mStmtListCount; i++ )
        {
            if ( aStmtListInfo->mStmtList[i].mStmtPool != NULL )
            {
                sRet = iduMemMgr::free( aStmtListInfo->mStmtList[i].mStmtPool );
                aStmtListInfo->mStmtList[i].mStmtPool = NULL;

                if ( sRet != IDE_SUCCESS )
                {
                    sErrorCode = ideGetErrorCode();
                    sErrorMsg  = ideGetErrorMsg(sErrorCode);

                    if ( sErrorMsg != NULL )
                    {
                        ideLog::log( IDE_QP_0, "Finalize Session Warning [ERR-%u] : %s",
                                     E_ERROR_CODE(sErrorCode), sErrorMsg );

                        IDE_DASSERT(0);
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    IDE_CLEAR();
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( aStmtListInfo->mStmtList[i].mStmtPoolStatus != NULL )
            {
                sRet = iduMemMgr::free( aStmtListInfo->mStmtList[i].mStmtPoolStatus );
                aStmtListInfo->mStmtList[i].mStmtPoolStatus = NULL;

                if ( sRet != IDE_SUCCESS )
                {
                    sErrorCode = ideGetErrorCode();
                    sErrorMsg  = ideGetErrorMsg(sErrorCode);

                    if ( sErrorMsg != NULL )
                    {
                        ideLog::log( IDE_QP_0, "Finalize Session Warning [ERR-%u] : %s",
                                     E_ERROR_CODE(sErrorCode), sErrorMsg );

                        IDE_DASSERT(0);
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    IDE_CLEAR();
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }

        sRet = iduMemMgr::free( aStmtListInfo->mStmtList );
        aStmtListInfo->mStmtList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aStmtListInfo->mStmtList2 != NULL )
    {
        sRet = iduMemMgr::free( aStmtListInfo->mStmtList2 );
        aStmtListInfo->mStmtList2 = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sRet != IDE_SUCCESS )
    {
        sErrorCode = ideGetErrorCode();
        sErrorMsg  = ideGetErrorMsg(sErrorCode);

        if ( sErrorMsg != NULL )
        {
            ideLog::log( IDE_QP_0, "Finalize Session Warning [ERR-%u] : %s",
                         E_ERROR_CODE(sErrorCode), sErrorMsg );

            IDE_DASSERT(0);
        }
        else
        {
            // Nothing to do.
        }

        IDE_CLEAR();
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;
}


IDE_RC qci::initializeSession( qciSession * aSession,
                               void       * aMmSession )
{
/***********************************************************************
 *
 * Description : qciSession 객체를 받아서 QP에 필요한 정보를 초기화한다.
 *
 *      MM에서 세션이 시작될 때 이 함수를 불러야 한다.
 *
 * Implementation :
 *
 *   QP에서 필요한 각 정보에 대한 메모리 할당 및 초기화 수행.
 *
 *   1. session 종속적인 object 초기화
 *   2. sequence 초기화
 *
 ***********************************************************************/

    qcSessionSpecific * sQPSpecific;
    UInt                sStage = 0;
    UInt i = 0;

    IDE_DASSERT( aSession != NULL );

    sQPSpecific = &aSession->mQPSpecific;

    sQPSpecific->mFlag = 0;

    sQPSpecific->mPropertyFlag = 0;
    
    aSession->mMmSession = aMmSession;

    // PROJ-2638
    sQPSpecific->mClientInfo = NULL;

    sQPSpecific->mIsGTx  = ID_FALSE;
    sQPSpecific->mIsGCTx = ID_FALSE;

    IDE_TEST( qcg::initSessionObjInfo( &(sQPSpecific->mSessionObj) )
              != IDE_SUCCESS );
    sStage = 1;

    sQPSpecific->mCache.mSequences_ = NULL;

    // PROJ-1407 Temporary Table
    // session temporary table object 초기화
    qcuTemporaryObj::initTemporaryObj( &(sQPSpecific->mTemporaryObj) );

    // PROJ-1073 Package
    sQPSpecific->mSessionPkg = NULL;

    // BUG-38129
    sQPSpecific->mLastRowGRID = SC_NULL_GRID;

    sQPSpecific->mThrMgr  = NULL;
    sQPSpecific->mConcMgr = NULL;

    /* PROJ-2451 Concurrent Execute Package */
    IDE_TEST( qsc::initialize( &(sQPSpecific->mThrMgr),
                               &(sQPSpecific->mConcMgr) )
              != IDE_SUCCESS );
    sStage = 2;

    IDU_FIT_POINT("qci::initializeSession::malloc::iduMemory",
                  idERR_ABORT_InsufficientMemory);
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSN,
                                 ID_SIZEOF( qsxArrayMemMgr ),
                                 (void**)&sQPSpecific->mArrMemMgr )
              != IDE_SUCCESS );
    sStage = 3;

    // PROJ-1904 Extend UDT
    IDE_TEST( sQPSpecific->mArrMemMgr->mMemMgr.initialize( IDU_MEM_QSN,
                                                           0,
                                                           (SChar*)"PSM_ARRAY_HEADER",
                                                           idlOS::align8(ID_SIZEOF(qsxArrayInfo)),
                                                           QSX_MEM_EXTEND_COUNT,
                                                           QSX_MEM_FREE_COUNT,
                                                           ID_FALSE )
              != IDE_SUCCESS );
    sStage = 4;

    sQPSpecific->mArrMemMgr->mNodeSize = 0;
    sQPSpecific->mArrMemMgr->mRefCount = 1;
    sQPSpecific->mArrMemMgr->mNext     = NULL;

    sQPSpecific->mArrInfo = NULL;

    // BUG-41248 DBMS_SQL package
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                 ID_SIZEOF( qcOpenCursorInfo ) * QCU_PSM_CURSOR_OPEN_LIMIT,
                                 (void **)&sQPSpecific->mOpenCursorInfo )
             != IDE_SUCCESS );
    sStage = 5;
               
    for ( i = 0; i < QCU_PSM_CURSOR_OPEN_LIMIT; i++ )
    {
        sQPSpecific->mOpenCursorInfo[i].mMmStatement = NULL;
        sQPSpecific->mOpenCursorInfo[i].mMemory = NULL;
        sQPSpecific->mOpenCursorInfo[i].mBindInfo = NULL;
        sQPSpecific->mOpenCursorInfo[i].mRecordExist = ID_FALSE;
        sQPSpecific->mOpenCursorInfo[i].mFetchCount = 0;
    }
    // BUG-44856
    sQPSpecific->mLastCursorId = ID_UINT_MAX;

    /* BUG-43605 [mt] random함수의 seed 설정을 session 단위로 변경해야 합니다. */
    qciMisc::initRandomInfo( & sQPSpecific->mRandomInfo );

    IDE_TEST( initializeStmtListInfo( &sQPSpecific->mStmtListInfo )
              != IDE_SUCCESS );
    sStage = 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 6:
            {
                (void)finalizeStmtListInfo( &sQPSpecific->mStmtListInfo );
            }
            /* fall through */
        case 5:
            {
                (void)iduMemMgr::free( sQPSpecific->mOpenCursorInfo );
            }
            /* fall through */
        case 4:
            {
                (void)sQPSpecific->mArrMemMgr->mMemMgr.destroy(ID_FALSE);
            }
            /* fall through */
        case 3:
            {
                (void)iduMemMgr::free( sQPSpecific->mArrMemMgr );
            }
            /* fall through */
        case 2:
            {
                (void)qsc::finalize( &(sQPSpecific->mThrMgr),
                                     &(sQPSpecific->mConcMgr) );
            }
            /* fall through */
        case 1:
            {
                (void)qcg::finalizeSessionObjInfo( &(sQPSpecific->mSessionObj) );
            }
            /* fall through */
        case 0:
            // Nothing to do.
            break;
        default:
            break;
    }
    
    return IDE_FAILURE;
}

IDE_RC qci::finalizeSession( qciSession * aSession,
                             void       * aMmSession )
{
/***********************************************************************
 *
 * Description : qciSession 객체를 받아서 QP에서 사용한 정보를
 *               해제한다.
 *
 *      MM에서 세션이 종료될 때 이 함수를 불러야 한다.
 *
 * Implementation :
 *
 *   QP에서 필요한 각 정보에 대한 메모리 해제 및 정리
 *
 *   1. session 종속적인 object에 대한 메모리 해제
 *   2. sequence 메모리 해제
 *
 ***********************************************************************/

    IDE_RC              sRet;
    UInt                sErrorCode;
    SChar             * sErrorMsg;
    qcSessionSpecific * sQPSpecific;
    UInt                sSessionID;
    idvSQL            * sStatistics;
    UInt                i = 0;

    sSessionID = qci::mSessionCallback.mGetSessionID( aMmSession );
    sStatistics = qci::mSessionCallback.mGetStatistics( aMmSession );

    sQPSpecific = &aSession->mQPSpecific;

    sQPSpecific->mFlag = 0;

    aSession->mMmSession = NULL;

    sQPSpecific->mIsGTx  = ID_FALSE;
    sQPSpecific->mIsGCTx = ID_FALSE;

    // sequence 정보에 대한 메모리 해제.
    sQPSpecific->mCache.clearSequence();

    // session종속적인 PSM file object 삭제
    (void)qcg::finalizeSessionObjInfo( &(sQPSpecific->mSessionObj) );

    // PROJ-1073 Package
    (void)qcuSessionPkg::finalizeSessionPkgInfoList( &(sQPSpecific->mSessionPkg) );

    sQPSpecific->mSessionPkg = NULL;

    // PROJ-1407 Temporary Table
    // session temporary table object 삭제
    qcuTemporaryObj::finalizeTemporaryObj( sStatistics,
                                           & sQPSpecific->mTemporaryObj );

    // PROJ-1685 Delete agent process
    idxProc::destroyAgentProcess( sSessionID );

    /* PROJ-2451 Concurrent Execute Package */
    if ( sQPSpecific->mThrMgr != NULL )
    {
        IDE_DASSERT(sQPSpecific->mConcMgr != NULL );

        sRet = qsc::finalize( &sQPSpecific->mThrMgr,
                              &sQPSpecific->mConcMgr );

        if ( sRet != IDE_SUCCESS )
        {
            sErrorCode = ideGetErrorCode();
            sErrorMsg  = ideGetErrorMsg(sErrorCode);

            if( sErrorMsg != NULL )
            {
                ideLog::log( IDE_QP_0, "Finalize Session Warning [ERR-%u] : %s",
                             E_ERROR_CODE(sErrorCode), sErrorMsg );
                //IDE_DASSERT(0);
            }
            else
            {
                // Nothing to do.
            }

            IDE_CLEAR();
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1904 Extend UDT
    sRet = qsxUtil::destroyArrayMemMgr( sQPSpecific );
           
    if ( sRet != IDE_SUCCESS )
    {
        sErrorCode = ideGetErrorCode();
        sErrorMsg  = ideGetErrorMsg(sErrorCode);

        if ( sErrorMsg != NULL )
        {
            ideLog::log( IDE_QP_0, "Finalize Session Warning [ERR-%u] : %s",
                         E_ERROR_CODE(sErrorCode), sErrorMsg );

            IDE_DASSERT(0);
        }
        else
        {
            // Nothing to do.
        }

        IDE_CLEAR();
    }
    else
    {
        // Nothing to do.
    }

    sQPSpecific->mArrMemMgr = NULL;
    sQPSpecific->mArrInfo   = NULL;

    // BUG-41248 DBMS_SQL package
    for ( i = 0; i < QCU_PSM_CURSOR_OPEN_LIMIT; i++ )
    {
        // mmStatement is already freed before calling qci::finalizeSession

        if ( sQPSpecific->mOpenCursorInfo[i].mMemory != NULL )
        {
            (void)sQPSpecific->mOpenCursorInfo[i].mMemory->destroy();
            sQPSpecific->mOpenCursorInfo[i].mMemory = NULL;
        }
        else
        {
            // Nothing to do.
        }
    }

    sRet = iduMemMgr::free( sQPSpecific->mOpenCursorInfo );

    if ( sRet != IDE_SUCCESS )
    {
        sErrorCode = ideGetErrorCode();
        sErrorMsg  = ideGetErrorMsg(sErrorCode);

        if ( sErrorMsg != NULL )
        {
            ideLog::log( IDE_QP_0, "Finalize Session Warning [ERR-%u] : %s",
                         E_ERROR_CODE(sErrorCode), sErrorMsg );

            IDE_DASSERT(0);
        }
        else
        {
            // Nothing to do.
        }

        IDE_CLEAR();
    }
    else
    {
        // Nothing to do.
    }

    sQPSpecific->mOpenCursorInfo = NULL;

    (void)finalizeStmtListInfo( &sQPSpecific->mStmtListInfo );

    // BUG-47765
    sdi::unSetSessionPropertyFlag( aSession );
    
    // PROJ-2638
    sdi::finalizeSession( aSession );

    return IDE_SUCCESS;
}

void qci::endTransForSession( qciSession * aSession,
                              idBool       aCommit,
                              smSCN      * aCommitSCN,
                              ULong        aNewSMN,
                              sdiZKPendingJobFunc * aOutPendingJobFunc )
{
    idvSQL            * sStatistics;
    ULong               sBeforeSMN;
    idBool              sNeedUnsetSessionFlag = ID_FALSE;

    ACP_UNUSED(aCommitSCN);

    sStatistics = qci::mSessionCallback.mGetStatistics( aSession->mMmSession );
    sBeforeSMN  = qci::mSessionCallback.mGetShardMetaNumber( aSession->mMmSession );

    // PROJ-1407 Temporary Table
    // session에서 end-transaction시 수행할 작업
    // transaction temporary table 삭제
    qcuTemporaryObj::dropAllTempTables( sStatistics,
                                        & aSession->mQPSpecific.mTemporaryObj,
                                        QCM_TEMPORARY_ON_COMMIT_DELETE_ROWS );

    if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
         QC_SESSION_SHARD_DDL_TRUE ) //shard ddl
    {
        if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
             QC_SESSION_SHARD_META_TOUCH_TRUE )
        {
            IDE_DASSERT( ( aSession->mQPSpecific.mFlag & QC_SESSION_HANDOVER_SHARD_DDL_MASK ) ==
                         QC_SESSION_HANDOVER_SHARD_DDL_FALSE );

            if ( aCommit == ID_TRUE )
            {
                if (sdiZookeeper::isExistPendingJob() == ID_TRUE)
                {
                    *aOutPendingJobFunc = sdiZookeeper::callAfterCommitFunc;
                }
                else
                {
                    sdiZookeeper::callAfterCommitFunc( aNewSMN, sBeforeSMN );
                }

                ideLog::log( IDE_SD_17, "[SHARD_META] COMMIT" );
            }
            else
            {
                if (sdiZookeeper::isExistPendingJob() == ID_TRUE)
                {
                    *aOutPendingJobFunc = sdiZookeeper::callAfterRollbackFunc;
                }
                else
                {
                    sdiZookeeper::callAfterRollbackFunc();
                }

                ideLog::log( IDE_SD_17, "[SHARD_META] ROLLBACK" );
            }
            sNeedUnsetSessionFlag = ID_TRUE;
        }
    }
    else if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_GLOBAL_DDL_MASK ) ==
               QC_SESSION_GLOBAL_DDL_TRUE )
    {
        /*
         * PROJ-2757 Advanced Global DDL
         */
        if ( aCommit == ID_TRUE )
        {
            if (sdiZookeeper::isExistPendingJob() == ID_TRUE)
            {
                *aOutPendingJobFunc = sdiZookeeper::callAfterCommitFunc;
            }
            else
            {
                sdiZookeeper::callAfterCommitFunc( aNewSMN, sBeforeSMN );
            }
            ideLog::log( IDE_SD_17, "[sdiGlobalDDL] COMMIT" );
        }
        else
        {
            if (sdiZookeeper::isExistPendingJob() == ID_TRUE)
            {
                *aOutPendingJobFunc = sdiZookeeper::callAfterRollbackFunc;
            }
            else
            {
                sdiZookeeper::callAfterRollbackFunc();
            }
            ideLog::log( IDE_SD_17, "[sdiGlobalDDL] ROLLBACK" );
        }
        sNeedUnsetSessionFlag = ID_TRUE;
    }
    else // shard meta procedure only
    {
        if ( ( aSession->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
             QC_SESSION_SHARD_META_TOUCH_TRUE )
        {
            if ( aCommit == ID_TRUE )
            {
                if( aNewSMN != SDI_NULL_SMN )
                {
                    IDE_DASSERT( ( aSession->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) 
                                 == QC_SESSION_SHARD_META_TOUCH_TRUE );
                    if ( (*aCommitSCN) != SM_SCN_INIT )
                    {
                        /* shard meta update procedure of user session */
                        if( sdiZookeeper::updateSMN(aNewSMN) != IDE_SUCCESS )
                        {
                            ideLog::log( IDE_SD_4, "[ZOOKEEPER_ERROR] fail. shard procedure after commit for smn: SMN(%"ID_UINT64_FMT")", aNewSMN );
                        }
                        ideLog::log( IDE_SD_20, "[ZOOKEEPER] success. shard procedure zookeeper after commit for smn: SMN(%"ID_UINT64_FMT")", aNewSMN );
                    }
                }
                else
                {
                    /* self coordinator session of shard meta update procedure : do nothing, already update smn by user session */
                }

                ideLog::log( IDE_SD_17, "[SHARD_META] COMMIT" );
            }
            else
            {
                ideLog::log( IDE_SD_17, "[SHARD_META] ROLLBACK" );
            }
            //shard meta touch release shard meta lock
            sdiZookeeper::releaseShardMetaLock();

            sNeedUnsetSessionFlag = ID_TRUE;
        }
    }

    if ( sNeedUnsetSessionFlag == ID_TRUE )
    {
        sdi::unsetInternalTableSwap( aSession );
        // BUG-46884
        sdi::unsetShardMetaTouched( aSession );
        /* initialize session flag shard ddl */
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_DDL_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_SHARD_DDL_FALSE;
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_HANDOVER_SHARD_DDL_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_HANDOVER_SHARD_DDL_FALSE;
        // BUG-48616
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_SAHRD_ADD_CLONE_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_SAHRD_ADD_CLONE_FALSE;
        /* initialize session flag global ddl */
        aSession->mQPSpecific.mFlag &= ~QC_SESSION_GLOBAL_DDL_MASK;
        aSession->mQPSpecific.mFlag |= QC_SESSION_GLOBAL_DDL_FALSE;
    }
    else
    {
        /* Nothing to do */
    }
}

void qci::endSession( qciSession * aSession )
{
    idvSQL            * sStatistics;

    sStatistics = qci::mSessionCallback.mGetStatistics( aSession->mMmSession );

    // PROJ-1407 Temporary Table
    // session에서 end-session시 수행할 작업
    // transaction temporary table 삭제
    qcuTemporaryObj::dropAllTempTables( sStatistics,
                                        & aSession->mQPSpecific.mTemporaryObj,
                                        QCM_TEMPORARY_ON_COMMIT_PRESERVE_ROWS);
}

IDE_RC qci::initializeStatement( qciStatement * aStatement,
                                 qciSession   * aSession,
                                 qciStmtInfo  * aStmtInfo,
                                 idvSQL       * aStatistics )
{
    IDE_TEST( checkExecuteFuncAndSetEnv( aStatement, EXEC_INITIALIZE )
              != IDE_SUCCESS );

    IDE_TEST( qcg::allocStatement( &aStatement->statement,
                                   aSession,
                                   aStmtInfo,
                                   aStatistics ) != IDE_SUCCESS );

    qsxEnv::initialize( aStatement->statement.spxEnv,
                        (qcSession*)aSession );

    IDE_TEST( changeStmtState( aStatement, EXEC_INITIALIZE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qci::finalizeStatement( qciStatement *aStatement )
{
    IDE_TEST( checkExecuteFuncAndSetEnv( aStatement, EXEC_FINALIZE )
              != IDE_SUCCESS );

    IDE_TEST( qcg::freeStatement( &aStatement->statement ) != IDE_SUCCESS );

    IDE_TEST( changeStmtState( aStatement, EXEC_FINALIZE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void qci::clearShardDataInfo( qciStatement * aStatement )
{
    qcStatement * sStatement = & aStatement->statement;

    if ( sStatement->allocFlag == ID_TRUE )
    {
        if ( QC_PRIVATE_TMPLATE( sStatement ) != NULL )
        {
            sdi::clearDataInfo( sStatement,
                                & ( QC_PRIVATE_TMPLATE( sStatement )->shardExecData ) );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

void qci::clearShardDataInfoForRebuild( qciStatement * aStatement )
{
    qcStatement * sStatement = & aStatement->statement;

    if ( sStatement->allocFlag == ID_TRUE )
    {
        if ( QC_PRIVATE_TMPLATE( sStatement ) != NULL )
        {
            sdi::clearShardDataInfoForRebuild( sStatement,
                                               & ( QC_PRIVATE_TMPLATE( sStatement )->shardExecData ) );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

IDE_RC qci::checkExecuteFuncAndSetEnv( qciStatement * aStatement,
                                       qciExecFunc    aExecFunc )
{   
    //----------------------------------------
    // aExecFunc이
    // 현재 statement의 state로
    // 호출할 수 있는 함수인지에 대해 판단하고,
    // 호출할 수 있는 함수로 판단되면,
    // 함수수행에 맞도록 psm의 상태와 qcStatement의 상태를 정리한다.
    //
    // PROJ-2163
    // 호스트 변수의 타입 바인딩이 hard prepare 로 이동하면서
    // QCI_STMT_STATE_PREPARED와 QCI_STMT_STATE_PARAM_INFO_BOUND 순서 변경
    //---------------------------------------

    static qciStateInfo
        sCheckExecuteFuncMatrix[QCI_STMT_STATE_MAX][EXEC_FUNC_MAX] =
        {
            // QCI_STMT_STATE_ALLOCED
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK  },
                /* EXEC_PARSE           */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_ERR },
                /* EXEC_PREPARE         */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_ERR },
                /* EXEC_EXECUTE         */
                { STATE_ERR },
                /* EXEC_FINALIZE        */
                { STATE_ERR },
                /* EXEC_REBUILD         */
                { STATE_ERR },
                /* EXEC_RETRY           */
                { STATE_ERR }
            },

            // QCI_STMT_STATE_INITIALIZED
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK | STATE_CLEAR },
                /* EXEC_PARSE           */
                { STATE_OK  },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_ERR },
                /* EXEC_PREPARE         */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_ERR },
                /* EXEC_EXECUTE         */
                { STATE_ERR },
                /* EXEC_FINALIZE        */
                { STATE_OK  },
                /* EXEC_REBUILD         */
                { STATE_ERR },
                /* EXEC_RETRY           */
                { STATE_ERR }
            },

            // QCI_STMT_STATE_PARSED
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK | STATE_CLEAR },
                /* EXEC_PARSE           */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_OK  },
                /* EXEC_PREPARE         */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_ERR },
                /* EXEC_EXECUTE         */
                { STATE_ERR },
                /* EXEC_FINALIZE        */
                { STATE_OK  },
                /* EXEC_REBUILD         */
                { STATE_ERR },
                /* EXEC_RETRY           */
                { STATE_ERR }
            },

            // QCI_STMT_STATE_PARAM_INFO_BOUND
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK | STATE_CLEAR },
                /* EXEC_PARSE           */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_ERR },
                /* EXEC_PREPARE         */
                { STATE_OK  },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_ERR },
                /* EXEC_EXECUTE         */
                { STATE_ERR },
                /* EXEC_FINALIZE        */
                { STATE_OK  },
                /* EXEC_REBUILD         */
                /* BUG-40170: ERR -> OK */
                { STATE_OK  },
                /* EXEC_RETRY           */
                { STATE_ERR }
            },

            // QCI_STMT_STATE_PREPARED
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK | STATE_CLEAR },
                /* EXEC_PARSE           */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_ERR },
                /* EXEC_PREPARE         */
                { STATE_OK | STATE_PARAM_DATA_CLEAR },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_OK  },
                /* EXEC_EXECUTE         */
                { STATE_ERR },
                /* EXEC_FINALIZE        */
                { STATE_OK  },
                /* EXEC_REBUILD         */
                { STATE_OK },
                /* EXEC_RETRY           */
                { STATE_ERR }
            },

            // QCI_STMT_STATE_PARAM_DATA_BOUND
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK | STATE_CLEAR | STATE_PSM_UNLATCH },
                /* EXEC_PARSE           */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_ERR },
                /* EXEC_PREPARE         */
                /* mm에서 하나의 sql문장이 끝나고 나면,
                   다음 sql 수행을 대기하거나, statement 종료를 위해
                   QCI_STMT_STATE_PARAM_DATA_BOUND로 상태를 돌려 놓는다.
                   prepare된 sql문이 이후의 처리를 위해서 대기하는 경우,
                   bindParamInfo의 정보가 변경될때
                   이를 처리하기 위해 prepared 상태로 되돌릴수 있어야 한다.
                   예) insert into t1 values ( ?, ? )  */
                { STATE_OK | STATE_PARAM_DATA_CLEAR },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_ERR },
                /* EXEC_EXECUTE         */
                { STATE_OK | STATE_PSM_LATCH },
                /* EXEC_FINALIZE        */
                { STATE_OK | STATE_PSM_UNLATCH },
                /* EXEC_REBUILD         */
                { STATE_OK | STATE_PSM_UNLATCH },
                /* EXEC_RETRY           */
                { STATE_ERR }
            },

            // QCI_STMT_STATE_EXECUTED
            {
                /* EXEC_INITIALIZE      */
                { STATE_OK | STATE_CLEAR | STATE_PSM_UNLATCH },
                /* EXEC_PARSE           */
                { STATE_ERR },
                /* EXEC_BIND_PARAM_INFO */
                { STATE_ERR },
                /* EXEC_PREPARE         */
                /* execute 상태에서 prepare protocol이 들어오는 경우는,
                   MM에서 에러 처리됨.  */
                { STATE_OK | STATE_CLOSE | STATE_PSM_UNLATCH | STATE_PARAM_DATA_CLEAR },
                /* EXEC_BIND_PARAM_DATA */
                { STATE_OK | STATE_CLOSE | STATE_PSM_UNLATCH },
                /* EXEC_EXECUTE         */
                { STATE_ERR },
                /* EXEC_FINALIZE        */
                { STATE_OK | STATE_PSM_UNLATCH },
                /* EXEC_REBUILD         */
                { STATE_OK | STATE_PSM_UNLATCH },
                /* EXEC_RETRY           */
                { STATE_OK | STATE_CLOSE }
            }
        };

    UShort          i;
    UInt            sState = aStatement->state;
    qciStateInfo  * sStateInfo;
    qcStatement   * sStatement;

    sStateInfo = & sCheckExecuteFuncMatrix[ sState ][ aExecFunc ];
    sStatement = & aStatement->statement;

    if ( sStateInfo->StateBit == STATE_OK )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        IDE_TEST_RAISE( sStateInfo->StateBit == STATE_ERR,
                        invalid_statement_state );
    }

    // fix BUG-16482
    if ( ( sStateInfo->StateBit & STATE_PARAM_DATA_CLEAR )
        == STATE_PARAM_DATA_CLEAR )
    {
        if( sStatement->pBindParamCount > 0 )
        {
            for( i = 0; i < sStatement->pBindParamCount; i++ )
            {
                IDE_ASSERT( sStatement->pBindParam != NULL );
                sStatement->pBindParam[i].isParamDataBound = ID_FALSE;
            }
        }
        else
        {
            // parameter가 존재하지 않는 경우.
            // Nothing To Do
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( ( sStateInfo->StateBit & STATE_CLOSE ) == STATE_CLOSE )
    {
        // 만일 execute중에 rebuild가 발생했다면, 그리고 rebuild가
        // 실패했다면 closeStatement를 호출해서는 안된다.
        // rebuild전에 사용했던 cursor나 temp table등은 rebuild전
        // clearStatement로 모두 정리되었다.
        if ( (aStatement->flag & QCI_STMT_REBUILD_EXEC_MASK)
             != QCI_STMT_REBUILD_EXEC_FAILURE )
        {
            IDE_TEST( qcg::closeStatement( sStatement )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( ( sStateInfo->StateBit & STATE_PSM_LATCH ) == STATE_PSM_LATCH )
    {
        if ( ( sStatement->spvEnv->latched == ID_FALSE ) &&
             ( sStatement->spvEnv->procPlanList != NULL ) )
        {
            IDE_TEST_RAISE( qsxRelatedProc::latchObjects(
                                sStatement->spvEnv->procPlanList )
                            != IDE_SUCCESS,
                            invalid_procedure );
            sStatement->spvEnv->latched = ID_TRUE;

            IDE_TEST_RAISE( qsxRelatedProc::checkObjects(
                                sStatement->spvEnv->procPlanList )
                            != IDE_SUCCESS,
                            invalid_procedure );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( ( sStateInfo->StateBit & STATE_PSM_UNLATCH ) == STATE_PSM_UNLATCH )
    {
        if ( ( sStatement->spvEnv->latched == ID_TRUE ) &&
             ( sStatement->spvEnv->procPlanList != NULL ) )
        {
            IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
                      != IDE_SUCCESS );
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( ( sStateInfo->StateBit & STATE_CLEAR ) == STATE_CLEAR )
    {
        IDE_TEST(qcg::clearStatement( sStatement,
                                      ID_FALSE ) /* aRebuild = ID_FALSE */
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_statement_state );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION( invalid_procedure );
    {
        IDE_SET(ideSetErrorCode(qpERR_REBUILD_QCI_PROC_INVALID));
    }
    IDE_EXCEPTION_END;

    if ( sStatement->spvEnv->latched == ID_TRUE )
    {
        if ( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}


IDE_RC qci::changeStmtState( qciStatement * aStatement,
                             qciExecFunc    aExecFunc )
{
    qciStmtState     sStmtState;

    switch( aExecFunc )
    {
        case EXEC_INITIALIZE :
            {
                sStmtState = QCI_STMT_STATE_INITIALIZED;
                break;
            }
        case EXEC_PARSE :
            {
                sStmtState = QCI_STMT_STATE_PARSED;
                break;
            }
        // PROJ-2163 
        // 순서변경
        // BIND_PARAM_INFO <-> PREPARE
        case EXEC_BIND_PARAM_INFO :
            {
                sStmtState = QCI_STMT_STATE_PARAM_INFO_BOUND;
                break;
            }
        case EXEC_PREPARE :
            {
                sStmtState = QCI_STMT_STATE_PREPARED;
                break;
            }
        case EXEC_BIND_PARAM_DATA :
            {
                sStmtState = QCI_STMT_STATE_PARAM_DATA_BOUND;
                break;
            }
        case EXEC_EXECUTE :
            {
                sStmtState = QCI_STMT_STATE_EXECUTED;
                break;
            }
        case EXEC_RETRY :
            {
                sStmtState = QCI_STMT_STATE_PARAM_DATA_BOUND;
                break;
            }
        case EXEC_FINALIZE :
            {
                sStmtState = QCI_STMT_STATE_ALLOCED;
                break;
            }
        default :
            IDE_RAISE( invalid_statement_state );
            break;
    }

    aStatement->state = sStmtState;

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_statement_state );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC qci::clearStatement( qciStatement *aStatement,
                            smiStatement *aSmiStmt,
                            qciStmtState  aTargetState )
{
/***********************************************************************
 *
 * Description : 한번 사용한 statement를
 *               initialized, prepared 상태로 전환.
 *               ( mm에서 사용 )
 *
 * Implementation :
 *
 *     aTargetState로 아래의 상태만 올 수 있으며,
 *     상태전이만 수행한다.
 *     1. QCI_STMT_STATE_INITIALIZED
 *     2. QCI_STMT_STATE_PREPARED
 *        ( client에서 cursor close를 호출하는 경우 )
 *
 ***********************************************************************/

    qcStatement   * sStatement;
    smiStatement  * sSmiStmtOrg;

    sStatement = &aStatement->statement;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    switch( aTargetState )
    {
        case QCI_STMT_STATE_INITIALIZED :
            {
                qcg::setPlanTreeState( sStatement, ID_FALSE );

                IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                                     EXEC_INITIALIZE )
                          != IDE_SUCCESS );
                IDE_TEST( changeStmtState( aStatement,
                                           EXEC_INITIALIZE )
                          != IDE_SUCCESS );
                break;
            }
        // PROJ-2163 
        // 순서 변경
        case QCI_STMT_STATE_PARAM_INFO_BOUND :
            {
                qcg::setPlanTreeState( sStatement, ID_FALSE );

                IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                                     EXEC_BIND_PARAM_INFO )
                          != IDE_SUCCESS );
                IDE_TEST( changeStmtState( aStatement,
                                           EXEC_BIND_PARAM_INFO )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_STATE_PREPARED :
            {
                IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                                     EXEC_PREPARE )
                          != IDE_SUCCESS );
                IDE_TEST( changeStmtState( aStatement,
                                           EXEC_PREPARE )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_STATE_PARAM_DATA_BOUND :
            {
                IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                                     EXEC_BIND_PARAM_DATA )
                          != IDE_SUCCESS );
                IDE_TEST( changeStmtState( aStatement,
                                           EXEC_BIND_PARAM_DATA )
                          != IDE_SUCCESS );
                break;
            }
        default :
            IDE_RAISE( invalid_statement_state );
            break;
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_statement_state );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION_END;

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}


IDE_RC qci::getCurrentState( qciStatement *aStatement,
                             qciStmtState *aState )
{
    *aState = aStatement->state;

    return IDE_SUCCESS;
}

IDE_RC qci::parse( qciStatement *aStatement,
                   SChar        *aQueryString,
                   UInt          aQueryLen )
{
    IDE_FT_TRACE("\"%.*s\"", aQueryLen, aQueryString);

    IDE_FT_ROOT_BEGIN();

    IDE_FT_BEGIN();

    IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                         EXEC_PARSE )
              != IDE_SUCCESS );

    // PROJ-2163
    IDE_TEST( qcg::setStmtText( &aStatement->statement,
                                aQueryString,
                                aQueryLen ) != IDE_SUCCESS );

    IDV_SQL_OPTIME_BEGIN(
        aStatement->statement.mStatistics,
        IDV_OPTM_INDEX_QUERY_PARSE );

    // BUG-41944 초기화
    QC_SHARED_TMPLATE(&aStatement->statement)->tmplate.arithmeticOpMode =
        QCG_GET_SESSION_ARITHMETIC_OP_MODE( &aStatement->statement );

    IDE_TEST_RAISE( qcpManager::parseIt( &aStatement->statement )
                    != IDE_SUCCESS,
                    err_parse );

    // BUG-35471
    // 사용자가 직접 실행하는 구문만 DDL disable 여부를 검사한다.
    if ( ( aStatement->statement.myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
         == QCI_STMT_MASK_DDL )
    {
        IDE_TEST_RAISE( qdc::checkExecDDLdisableProperty() != IDE_SUCCESS,
                        err_parse );
    }

    IDV_SQL_OPTIME_END( aStatement->statement.mStatistics, IDV_OPTM_INDEX_QUERY_PARSE );

    IDE_TEST( changeStmtState( aStatement, EXEC_PARSE ) != IDE_SUCCESS );

    IDE_FT_END();

    IDE_FT_ROOT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_parse )
    {
        IDV_SQL_OPTIME_END( aStatement->statement.mStatistics, IDV_OPTM_INDEX_QUERY_PARSE );
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    IDE_FT_EXCEPTION_END();

    IDE_FT_ROOT_END();

    return IDE_FAILURE;
}

//PROJ-1436 SQL-Plan Cache.
IDE_RC qci::hardPrepare( qciStatement           * aStatement,
                         smiStatement           * aParentSmiStmt,
                         qciSQLPlanCacheContext * aPlanCacheContext,
                         smiDistTxInfo          * aDistTxInfo) /* Default : NULL */
{
    smiStatement   sBuildSmiStmt;
    smiStatement * sSmiStmtOrg;
    qcStatement  * sStatement;
    qciStmtType    sStmtType;
    volatile SInt  sStage;
    volatile SInt  sOpState;
    volatile SInt  sTestState;

    //---------------------------------------------
    // QCI_STMT_STATE_PARSED 상태에서만
    // 이 함수를 호출할 수 있다.
    // 이미 수행중인 statement를 QCI_STMT_STATE_PREPARE 상태로
    // 되돌리고자 할때는  qci::clearStatement()를 호출한다.
    //---------------------------------------------

    sStatement = &aStatement->statement;
    sStage     = 0; /* BUG-45994 - 컴파일러 최적화 회피 */
    sOpState   = 0; /* BUG-45994 */
    sTestState = 0; /* BUG-45994 */

    IDE_TEST( checkExecuteFuncAndSetEnv(
                  aStatement,
                  EXEC_PREPARE ) != IDE_SUCCESS );

    IDE_TEST( getStmtType( aStatement, &sStmtType ) != IDE_SUCCESS );

    IDU_FIT_POINT( "qci::hardPrepare::validate" );
        
    // BUG-36657
    // DML, SP 를 제외한 모든 구문에서 host 변수는 존재할 수 없다.
    if( ( qciMisc::isStmtDML( sStmtType ) == ID_FALSE ) &&
        ( qciMisc::isStmtSP( sStmtType )  == ID_FALSE ) &&
        ( sStatement->pBindParamCount > 0 ) )
    {
        IDE_RAISE(ERR_NOT_ALLOWED_HOST_VAR);
    }
    else
    {
        // Nothing to do.
    }

    if( qciMisc::isStmtDCL( sStmtType ) != ID_TRUE )
    {
        IDE_TEST( sBuildSmiStmt.begin( aParentSmiStmt->mStatistics,
                                       aParentSmiStmt,
                                       SMI_STATEMENT_UNTOUCHABLE |
                                       SMI_STATEMENT_ALL_CURSOR,
                                       SM_SCN_INIT,
                                       aDistTxInfo )
                  != IDE_SUCCESS );

        qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
        qcg::setSmiStmt( sStatement, &sBuildSmiStmt );

        sStage = 1;

        /* TASK-6744 */
        if ( (QCU_PLAN_RANDOM_SEED != 0) &&
             (smiGetStartupPhase() == SMI_STARTUP_SERVICE) )
        {
            IDU_FIT_POINT( "qci::hardPrepare::alloc", idERR_ABORT_InsufficientMemory );
            IDE_TEST( QC_QME_MEM( sStatement )->alloc( ID_SIZEOF(qmgRandomPlanInfo),
                                                       (void**)&(sStatement->mRandomPlanInfo))
                      != IDE_SUCCESS );
            sTestState = 1;

            (void)qmg::initializeRandomPlanInfo( sStatement->mRandomPlanInfo );

            qcgPlan::registerPlanProperty( sStatement,
                                           PLAN_PROPERTY_OPTIMIZER_PLAN_RANDOM_SEED );

        }
        else
        {
            // Nothing to do.
            /* release 또는 test 상황이 아닐 경우 필요 없다. */
        }

        /* PROJ-2701 Sharding online data rebuild */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_META_NUMBER_FOR_DATA );
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_META_NUMBER_FOR_SESSION );
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_IS_USER_SESSION );
        /* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_CALL_BY_SHARD_ANALYZE_PROTOCOL );

        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_IN_PSM_ENABLE);

        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_STATUS);

        /* TASK-7219 Non-shard DML */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_PARTIAL_EXEC_TYPE );

        /* TASK-7307 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_INTERNAL_LOCAL_OPERATION);

        /* BUG-49093 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_GLOBAL_TRANSACTION_LEVEL );

        //-----------------------------------------
        // VALIDATE
        //-----------------------------------------

        IDV_SQL_OPTIME_BEGIN(
            aStatement->statement.mStatistics,
            IDV_OPTM_INDEX_QUERY_VALIDATE );
        sOpState = 1;

        IDE_FT_ROOT_BEGIN();

        IDE_FT_BEGIN();

        IDE_TEST( sStatement->myPlan->parseTree->parse( sStatement )
                  != IDE_SUCCESS );
        sStage = 2;

        IDE_FT_END();

        IDE_FT_ROOT_END();

        IDE_TEST( qcg::fixAfterParsing( sStatement ) != IDE_SUCCESS);

        IDE_FT_ROOT_BEGIN();

        IDE_FT_BEGIN();

        IDE_TEST( sStatement->myPlan->parseTree->validate( sStatement )
                  != IDE_SUCCESS );
            
        IDE_FT_END();

        IDE_FT_ROOT_END();

        if( ( sStmtType == QCI_STMT_SELECT ) ||
            ( sStmtType == QCI_STMT_SELECT_FOR_UPDATE ) ||
            ( sStmtType == QCI_STMT_DEQUEUE ) )
        {
            // base table info를 생성한다.
            qcg::setBaseTableInfo( sStatement );
        }
        else
        {
            // Nothing To Do
        }

        IDE_TEST( qcg::fixAfterValidation( sStatement ) != IDE_SUCCESS );

        sOpState = 0;
        IDV_SQL_OPTIME_END(
            aStatement->statement.mStatistics,
            IDV_OPTM_INDEX_QUERY_VALIDATE );

        //-----------------------------------------
        // OPTIMIZE
        //-----------------------------------------

        IDV_SQL_OPTIME_BEGIN(
            aStatement->statement.mStatistics,
            IDV_OPTM_INDEX_QUERY_OPTIMIZE );
        sOpState = 2;

        IDE_FT_ROOT_BEGIN();

        IDE_FT_BEGIN();

        /* PROJ-2750 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_LEFT_OUTER_SKIP_RIGHT_ENABLE );

        if ( QCU_LEFT_OUTER_SKIP_RIGHT_ENABLE == 1 )
        {
            QC_SHARED_TMPLATE( sStatement )->flag &= ~QC_TMP_LEFT_OUTER_SKIP_RIGHT_MASK;
            QC_SHARED_TMPLATE( sStatement )->flag |=  QC_TMP_LEFT_OUTER_SKIP_RIGHT_TRUE;
        }

        IDE_TEST( sStatement->myPlan->parseTree->optimize( sStatement )
                  != IDE_SUCCESS );

        IDE_FT_END();

        IDE_FT_ROOT_END();

        IDE_TEST( qcg::fixAfterOptimization( sStatement ) != IDE_SUCCESS );

        // BUG-47971
        // Package global variable의 분산 실행을 제한합니다.
        if ( (SDU_SHARD_ENABLE == 1) &&
             (QCG_GET_SESSION_SHARD_SESSION_TYPE( sStatement ) != SDI_SESSION_TYPE_USER) )
        {
            IDE_TEST_RAISE ( (sStatement->spvEnv->flag & QSV_ENV_PKG_VAR_EXIST_MASK)
                             == QSV_ENV_PKG_VAR_EXIST_TRUE,
                             ERR_PKG_VAR_EXIST );
        }

        /* TASK-6744 */
        if ( (QCU_PLAN_RANDOM_SEED != 0) &&
             (smiGetStartupPhase() == SMI_STARTUP_SERVICE) )
        {
            sTestState = 0;
            IDE_TEST( QC_QME_MEM( sStatement )->free( sStatement->mRandomPlanInfo )
                      != IDE_SUCCESS );
            sStatement->mRandomPlanInfo = NULL;
        }
        else
        {
            // Nothing to do.
            /* release 또는 test 상황이 아닐 경우 필요 없다. */
        }

        /* BUG-42639 Monitoring query */
        if ( ( ( QC_SHARED_TMPLATE(sStatement)->flag & QC_TMP_ALL_FIXED_TABLE_MASK )
               == QC_TMP_ALL_FIXED_TABLE_TRUE ) &&
             ( sStmtType == QCI_STMT_SELECT ) )
        {
            QC_PARSETREE(sStatement)->stmtKind = QCI_STMT_SELECT_FOR_FIXED_TABLE;
        }
        else
        {
            /* Nothing to do */
        }

        IDV_SQL_SET( aStatement->statement.mStatistics,
                     mOptimizer,
                     qcg::getOptimizeMode( sStatement ) );
        IDV_SQL_SET( aStatement->statement.mStatistics,
                     mCost,
                     qcg::getTotalCost( sStatement ) );

        IDE_TEST( qcg::getSmiStatementCursorFlag(
                      QC_SHARED_TMPLATE(sStatement),
                      &(aPlanCacheContext->mSmiStmtCursorFlag) )
                  != IDE_SUCCESS );

        // PROJ-2551 simple query 최적화
        // simple query인 경우 fast execute를 수행한다.
        qciMisc::setSimpleFlag( sStatement );

        if( ( sStmtType == QCI_STMT_SELECT ) ||
            ( sStmtType == QCI_STMT_SELECT_FOR_UPDATE ) ||
            ( sStmtType == QCI_STMT_DEQUEUE ) )
        {
            // bindColumn를 구축한다.
            IDE_TEST( makeBindColumnArray( aStatement )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        // environment의 기록
        if ( QC_SHARED_TMPLATE(sStatement)->tmplate.dateFormatRef == ID_TRUE )
        {
            qcgPlan::registerPlanProperty( sStatement,
                                           PLAN_PROPERTY_DEFAULT_DATE_FORMAT );
        }
        else
        {
            // Nothing to do.
        }

        if ( QC_SHARED_TMPLATE(sStatement)->tmplate.nlsCurrencyRef == ID_TRUE )
        {
            qcgPlan::registerPlanProperty( sStatement,
                                           PLAN_PROPERTY_NLS_CURRENCY_FORMAT );
        }
        else
        {
            /* Nothing to do */
        }

        if ( QC_SHARED_TMPLATE(sStatement)->tmplate.groupConcatPrecisionRef == ID_TRUE )
        {
            qcgPlan::registerPlanProperty( sStatement,
                                           PLAN_PROPERTY_GROUP_CONCAT_PRECISION );
        }
        else
        {
            /* Nothing to do */
        }

        // BUG-41944
        if ( QC_SHARED_TMPLATE(sStatement)->tmplate.arithmeticOpModeRef == ID_TRUE )
        {
            qcgPlan::registerPlanProperty( sStatement,
                                           PLAN_PROPERTY_ARITHMETIC_OP_MODE );
        }
        else
        {
            // Nothing to do.
        }

        /* PROJ-2632 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SERIAL_EXECUTE_MODE );

        // 사용빈도가 낮거나 항상 참조하여 무조건 기록한다.
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_STACK_SIZE );
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_FAKE_TPCH_SCALE_FACTOR );
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_FAKE_BUFFER_SIZE );
        // BUG-34295
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_ANSI_JOIN_ORDERING );
        // BUG-34350
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_REFINE_PREPARE_MEMORY );
        // PROJ-2469 Optimize View Materialization
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_VIEW_TARGET_ENABLE );
        /* BUG-42639 Monitoring query */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW );

        // BUG-44795
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_DBMS_STAT_POLICY );

        // PROJ-2687
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_AGGREGATION_TRANSFORM_ENABLE );

        /* TASK-7219 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_SHARD_TRANSFORM_MODE );
        /* BUG-47986 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_OR_VALUE_INDEX );

        /* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_INDEX_NL_JOIN_PENALTY );

        /* BUG-48120 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_INDEX_COST_MODE);

        /* BUG-48161 */
        qcgPlan::registerPlanProperty( sStatement,
                                       PLAN_PROPERTY_OPTIMIZER_BUCKET_COUNT_MAX );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanProc(
                      sStatement,
                      sStatement->spvEnv->procPlanList )
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanProcSynonym(
                      sStatement,
                      sStatement->spvEnv->objectSynonymList )
                  != IDE_SUCCESS );

        // PROJ-1073 Package
        IDE_TEST( qcgPlan::registerPlanPkg(
                      sStatement,
                      sStatement->spvEnv->procPlanList )
                  != IDE_SUCCESS );

        if ( aPlanCacheContext->mPlanCacheInMode == QCI_SQL_PLAN_CACHE_IN_OFF )
        {
            // plan cache 비대상인 경우 원본 template을
            // private template으로 사용한다.
            IDE_TEST( setPrivateTemplate( aStatement,
                                          NULL,
                                          QCI_SQL_PLAN_CACHE_IN_NORMAL )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        sOpState = 0;
        IDV_SQL_OPTIME_END(
            aStatement->statement.mStatistics,
            IDV_OPTM_INDEX_QUERY_OPTIMIZE );

        //-----------------------------------------
        // MISC
        //-----------------------------------------

        // PROJ-2223 audit
        // auditing위하여 sql에서 참조하고 있는 objects를 기록한다.
        if ( ( aStatement->flag & QCI_STMT_AUDIT_MASK ) == QCI_STMT_AUDIT_TRUE )
        {
            if ( ( qciMisc::isStmtDML( sStmtType ) == ID_TRUE ) ||
                 ( qciMisc::isStmtSP( sStmtType ) == ID_TRUE ) )
            {
                IDE_TEST( qdcAudit::setAllRefObjects( sStatement )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            qdcAudit::setOperation( sStatement );
        }
        else
        {
            // Nothing to do.
        }

        /* PROJ-2701 Sharding online data rebuild
         * Rebuild coordinator가 bind정보 없이 생성한 plan은
         * Plan cache에 등록하지 않는다.
         */
        if ( ( sStatement->mFlag & QC_STMT_SHARD_REBUILD_FORCE_MASK ) ==
             QC_STMT_SHARD_REBUILD_FORCE_TRUE )
        {
            aPlanCacheContext->mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
        }
        else
        {
            // Nothing to do.
        }

        if ( ( aPlanCacheContext->mPlanCacheInMode ==
               QCI_SQL_PLAN_CACHE_IN_ON ) &&
             ( (QC_SHARED_TMPLATE(sStatement)->flag & QC_TMP_PLAN_CACHE_IN_MASK) ==
               QC_TMP_PLAN_CACHE_IN_ON ) &&
             ( (sStatement->session->mQPSpecific.mFlag & QC_SESSION_PLAN_CACHE_MASK) ==
               QC_SESSION_PLAN_CACHE_ENABLE ) )
        {
            // PROJ-2163
            // reprepare 를 통한 호출인지를 검사
            // reprepare 일 때는 pBindParam 은 NULL 이 아님
            if( ( qcg::getBindCount( sStatement ) > 0 ) &&
                ( sStatement->pBindParam == NULL ) &&
                ( QCG_GET_CALL_BY_SHARD_ANALYZE_PROTOCOL(sStatement) == ID_FALSE ) )
            {
                aPlanCacheContext->mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
            }
            else
            {
                // plan cache의 대상이고 cache가 가능한 쿼리라고 판단되는 경우

                // DDL은 plan cache의 대상이 아니다.
                IDE_DASSERT( qciMisc::isStmtDDL( sStmtType ) != ID_TRUE );

                // plan cache 정보를 생성한다.
                IDE_TEST( makePlanCacheInfo( aStatement,
                                             aPlanCacheContext )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // plan cache 대상이 아니거나 cache가 불가능한 쿼리인 경우

            aPlanCacheContext->mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
        }

        if( sStatement->spvEnv->latched == ID_TRUE )
        {
            IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
                      != IDE_SUCCESS );
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing To Do
        }

        //ideLog::log(IDE_QP_2, "[INFO] Query Preparation Memory Size : %llu KB",
        //            QC_QMP_MEM(sStatement)->getAllocSize()/1024);

        sStage = 3;
        qcg::setSmiStmt( sStatement, sSmiStmtOrg );

        IDE_TEST( sBuildSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
                  != IDE_SUCCESS );
    }
    else
    {
        // PROJ-2223 audit
        // auditing위하여 sql의 종류를 기록한다.
        if ( ( aStatement->flag & QCI_STMT_AUDIT_MASK ) == QCI_STMT_AUDIT_TRUE )
        {
            qdcAudit::setOperation( sStatement );
        }
        else
        {
            // Nothing to do.
        }

        // DCL, DB는 항상 plan cache 비대상이다.
        IDE_DASSERT( aPlanCacheContext->mPlanCacheInMode ==
                     QCI_SQL_PLAN_CACHE_IN_OFF );

        IDE_TEST( qcg::setPrivateArea( sStatement )
                  != IDE_SUCCESS );

        /* BUG-40133 insure++ READ_UNINIT_MEM */
        aPlanCacheContext->mSmiStmtCursorFlag = SMI_STATEMENT_CURSOR_NONE;
    }

    IDE_TEST( changeStmtState( aStatement, EXEC_PREPARE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    // BUG-36657
    IDE_EXCEPTION(ERR_NOT_ALLOWED_HOST_VAR);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_HOSTVAR));
    }
    IDE_EXCEPTION( ERR_PKG_VAR_EXIST )
    {
        /* TASK-7219 Shard Transformer Refactoring */
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Package global variable exists.",
                                  "" ) );
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    if( sStage >= 1 )
    {
        if ( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            IDE_ERRLOG( IDE_QP_1 );
        }
    }
    else
    {
        // Nothing To Do
    }

    if( ( sStage == 1 ) || ( sStage == 2 ) )
    {
        qcg::setSmiStmt( sStatement, sSmiStmtOrg );

        if( sBuildSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE )
            != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_QP_1 );
        }
    }
    else
    {
        // Nothing To Do
    }

    /* TASK-6744 */
    if ( sTestState == 1 )
    {
        sTestState = 0;
        (void)QC_QME_MEM( sStatement )->free( sStatement->mRandomPlanInfo );
        sStatement->mRandomPlanInfo = NULL;
    }
    else
    {
        // Nothing to do.
        /* test 상황에서만 sTestState가 변경된다.
           release 또는 test 상황이 아닐 경우 필요 없다. */
    }

    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     *  qci::makePlanCacheInfo()에서 만든 QMP Memory와 Shared Template을 제거한다.
     */
    if ( aPlanCacheContext->mPlanCacheInMode == QCI_SQL_PLAN_CACHE_IN_ON )
    {
        if ( aPlanCacheContext->mSharedPlanMemory != NULL )
        {
            (void)qci::freeSharedPlan( aPlanCacheContext->mSharedPlanMemory );
            aPlanCacheContext->mSharedPlanMemory = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aPlanCacheContext->mPrepPrivateTemplate != NULL )
        {
            (void)qci::freePrepPrivateTemplate( aPlanCacheContext->mPrepPrivateTemplate );
            aPlanCacheContext->mPrepPrivateTemplate = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    switch ( sOpState )
    {
        case 2:
            IDV_SQL_OPTIME_END(
                aStatement->statement.mStatistics,
                IDV_OPTM_INDEX_QUERY_OPTIMIZE );
            break;
        case 1:
            IDV_SQL_OPTIME_END(
                aStatement->statement.mStatistics,
                IDV_OPTM_INDEX_QUERY_VALIDATE );
            break;
        default:
            break;
    }

    IDE_FT_EXCEPTION_END();
    
    IDE_FT_ROOT_END();
    
    return IDE_FAILURE;
}

IDE_RC qci::bindParamInfo( qciStatement           * aStatement,
                           qciSQLPlanCacheContext * aPlanCacheContext )
{
    //------------------------------------
    // QCI_STMT_STATE_PREPARED, QCI_STATE_PREPARED_DIRECT
    //   : parse->prepare->bindParamInfo 순으로 순차적으로 진행되는 경우,
    // QCI_STMT_STATE_EXECUTED
    //   : prepare이상의 상태에서 bindParamInfo를 호출하는 경우.
    //     예) execute 후,
    //         fetch 진행 중에 bind param info 정보가 바뀌는 경우 등..
    //-------------------------------------

    IDE_TEST( checkExecuteFuncAndSetEnv(
                  aStatement,
                  EXEC_BIND_PARAM_INFO ) != IDE_SUCCESS );

    //-------------------------------------------------------------------------
    // PROJ-2163
    //     Plan cache 에 호스트 변수의 type 정보가 추가되면서
    //     plan environment 에도 type 정보를 추가해야 한다.
    //     호스트 변수의 type 정보를 다루는 qci::bindParamInfo 가
    //     hard prepare 전 단계로 이동되어서 plan environment 의
    //     초기화 함수도 qci::bindParamInfo 에서 수행한다.
    //-------------------------------------------------------------------------
    if ( ( aPlanCacheContext->mPlanCacheInMode == QCI_SQL_PLAN_CACHE_IN_ON ) ||
         ( ( aStatement->flag & QCI_STMT_AUDIT_MASK ) == QCI_STMT_AUDIT_TRUE ) )
    {
        // plan cache 대상인 경우 plan environment를 생성한다.
        IDE_TEST( qcgPlan::allocAndInitPlanEnv( & aStatement->statement )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2163
    // bindParamInfo 의 호출위치 변경으로 sBindParam 사용이 불가능해졌다.
    // 대체 함수를 호출하도록 변경한다.
    if ( qcg::getBindCount( & aStatement->statement ) > 0 )
    {
        // Type binding 전에 바인드 정보를 담는 sBindParam 을 생성한다.
        IDE_TEST( makeBindParamArray( aStatement ) != IDE_SUCCESS );

        IDE_TEST( buildBindParamInfo( aStatement ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( changeStmtState( aStatement, EXEC_BIND_PARAM_INFO )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::setParamDataState( qciStatement * aStatement )
{
    //------------------------------------
    // QCI_STMT_STATE_PARAM_INFO_BOUND
    //   : parse->prepare->bindParamInfo->bindParamData 또는
    //     execute->bindParamInfo->bindParamData 순으로
    //     순차적으로 진행되는 경우이거나,
    // QCI_STMT_STATE_EXECUTED
    //   : execute 상태에서 bindParamData를 호출하는 경우.
    //     예) execute 후,
    //         fetch 진행 중에 bind param Data 정보가 바뀌는 경우 등..
    //-------------------------------------

    UShort         i;
    qciBindParam * sBindParam;
    qcStatement  * sStatement;

    sStatement = & aStatement->statement;

    for ( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        sBindParam = & sStatement->pBindParam[i].param;

        // PROJ-2163
        IDE_TEST_RAISE( sStatement->pBindParam[i].isParamInfoBound == ID_FALSE,
                        err_bind_column_count_mismatch );

        // in, inout 타입 data.
        if ( (sBindParam->inoutType == CMP_DB_PARAM_INPUT) ||
             (sBindParam->inoutType == CMP_DB_PARAM_INPUT_OUTPUT) )
        {
            IDE_TEST_RAISE( sStatement->pBindParam[i].isParamDataBound == ID_FALSE,
                            err_invalid_binding );
        }
    }

    // PROJ-2163
    IDE_TEST( checkExecuteFuncAndSetEnv( aStatement, EXEC_BIND_PARAM_DATA )
              != IDE_SUCCESS );

    IDE_TEST( changeStmtState( aStatement, EXEC_BIND_PARAM_DATA )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_bind_column_count_mismatch );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_BIND_COLUMN_COUNT_MISMATCH));
    }
    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::execute( qciStatement * aStatement,
                     smiStatement * aSmiStmt )
{
    qcStatement     * sStatement;
    smiStatement    * sSmiStmtOrg;
    qmmInsParseTree * sInsParseTree;
    //PROJ-1677 DEQUEUE
    qmsParseTree    * sParseTree;
    qciArgEnqueue     sArgEnqueue;
    qciArgDequeue     sArgDequeue;
    qciStmtType       sStmtType;
    UInt              sRowCount;
    idBool            sOrgPSMFlag;

    /* TASK-7219 Non-shard DML */
    idBool            sStmtExecSeqIncreased  = ID_FALSE;
    UInt              sStmtExecSeqForShardTx = 0;

    IDE_FT_ROOT_BEGIN();

    IDE_NOFT_BEGIN();

    IDU_FIT_POINT_FATAL( "qci::execute::__NOFT__" );

    sStatement  = &aStatement->statement;
    sOrgPSMFlag = sStatement->calledByPSMFlag;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    //fix BUG-17553
    IDV_SQL_SET( sStatement->mStatistics, mMemoryTableAccessCount, 0 );

    //PROJ-1677 DEQUEUE
    IDE_TEST( getStmtType( aStatement, &sStmtType ) != IDE_SUCCESS );

    // BUG-41030 Set called by PSM flag
    if ( (sStmtType & QCI_STMT_MASK_SP) == QCI_STMT_MASK_SP )
    {
        sStatement->calledByPSMFlag = ID_TRUE;
    }

    IDE_TEST( checkExecuteFuncAndSetEnv(
                  aStatement,
                  EXEC_EXECUTE ) != IDE_SUCCESS );

    /* PROJ-2626 Snapshot Export
     * begin snapshot 시에는 inplace update를 금지하도록 한다.
     */
    sStatement->mInplaceUpdateDisableFlag = getInplaceUpdateDisable();

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    qciMisc::setSimpleBindFlag( sStatement );

    /* TASK-7219 Non-shard DML */
    if ( QCG_GET_SESSION_IS_AUTOCOMMIT( sStatement ) == ID_FALSE )
    {
        if ( sdi::isShardCoordinator( sStatement ) == ID_TRUE )
        {
            QCG_INCREASE_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX( sStatement, &sStmtExecSeqIncreased );

            if ( sStmtExecSeqIncreased == ID_TRUE )
            {
                sStmtExecSeqForShardTx = QCG_GET_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX( sStatement );

                IDE_TEST_RAISE( sStmtExecSeqForShardTx > SDI_STMT_EXEC_SEQ_MAX, ERR_SHARD_STMT_EXEC_SEQ_OVERFLOW );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        // Nothing to do.
    }
    else
    {
        // BUG-15928
        IDE_TEST( qcm::validateAndLockAllObjects( sStatement )
                  != IDE_SUCCESS );

        /* PROJ-2701 Sharding online data rebuild */
        IDE_TEST( checkShardPlanRebuild( (qcStatement*)aStatement )
                != IDE_SUCCESS );

        /* PROJ-2462 Result Cache */
        if ( ( sStmtType == QCI_STMT_SELECT ) ||
             ( sStmtType == QCI_STMT_SELECT_FOR_UPDATE ) )
        {
            qmxResultCache::setUpResultCache( sStatement );
        }
        else
        {
            /* Nothing to do */
        }
    }

    //PROJ-1677 DEQUEUE
    if(sStmtType ==  QCI_STMT_DEQUEUE)
    {
        sParseTree = (qmsParseTree *)(sStatement->myPlan->parseTree);
        sArgDequeue.mMmSession = sStatement->session->mMmSession;
        sArgDequeue.mTableID = sParseTree->queue->tableID;;
        sArgDequeue.mViewSCN = aSmiStmt->getSCN();
                            
        // execute 하기전에 timestamp를 저장한다.
        IDE_TEST( qcg::mSetQueueStampFuncPtr( (void *)&sArgDequeue)
                  != IDE_SUCCESS );
    }

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        IDE_TEST( qmxSimple::fastExecute( aSmiStmt->getTrans(),
                                          sStatement,
                                          NULL,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          & sRowCount )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sStatement->myPlan->parseTree->execute( sStatement )
                  != IDE_SUCCESS );
    }
    
    //PROJ-1677 DEQUEUE
    if( sStmtType == QCI_STMT_ENQUEUE)
    {
        sInsParseTree = (qmmInsParseTree *)(sStatement->myPlan->parseTree);
        sArgEnqueue.mMmSession = sStatement->session->mMmSession;
        sArgEnqueue.mTableID = sInsParseTree->tableRef->tableInfo->tableID;
        IDE_TEST( qcg::mEnqueueQueueFuncPtr( (void *)&sArgEnqueue )
                  != IDE_SUCCESS );
    }

    // set success
    QC_PRIVATE_TMPLATE(sStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(sStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );
    // BUG-41030 Unset called by PSM flag
    sStatement->calledByPSMFlag = sOrgPSMFlag;

    IDE_NOFT_END();

    IDE_FT_ROOT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_STMT_EXEC_SEQ_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "qci::execute",
                                  "The statement execution sequence for shard transaction overflow" ) );
    }
    IDE_EXCEPTION_END;

    IDE_NOFT_EXCEPTION_BEGIN();

    if( sStatement->spvEnv->latched == ID_TRUE )
    {
        if ( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7219 Non-shard DML */
    if ( sStmtExecSeqIncreased == ID_TRUE )
    {
        /* Statement 수행 에러 반복 시 sequence의 소진을 방지하기 위해 sequence를 다시 감소시킨다. */
        QCG_DECREASE_SESSION_STMT_EXEC_SEQ_FOR_SHARD_TX( sStatement );
    }
    else
    {
        /* Nothing to do. */
    }

    changeStmtState( aStatement, EXEC_EXECUTE );
    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    // BUG-41030 Unset called by PSM flag
    sStatement->calledByPSMFlag = sOrgPSMFlag;

    IDE_NOFT_EXCEPTION_END();

    IDE_FT_ROOT_END();

    return IDE_FAILURE;
}

IDE_RC qci::setBindColumnInfo( qciStatement  * /* aStatement */,
                               qciBindColumn * /* aBindColumn */ )
{
    // BUG-20652
    // server에서 setBindColumnInfo는 허용하지 않는다.
    IDE_RAISE( err_invalid_binding );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::getBindColumnInfo( qciStatement  * aStatement,
                               qciBindColumn * aBindColumn )
{
    qcStatement    * sStatement;
    qciStmtState     sState;
    qciBindColumn  * sCurrBindColumn;

    sStatement = & aStatement->statement;

    IDE_TEST( getCurrentState( aStatement, & sState ) != IDE_SUCCESS );

    // BUG-44763
    IDE_TEST_RAISE( sState < QCI_STMT_STATE_PARAM_INFO_BOUND, err_invalid_binding );

    IDE_TEST_RAISE( aBindColumn->mId >= sStatement->myPlan->sBindColumnCount,
                    err_invalid_binding );

    sCurrBindColumn = & sStatement->myPlan->sBindColumn[aBindColumn->mId].column;

    aBindColumn->mType        = sCurrBindColumn->mType;
    aBindColumn->mLanguage    = sCurrBindColumn->mLanguage;
    aBindColumn->mArguments   = sCurrBindColumn->mArguments;
    aBindColumn->mPrecision   = sCurrBindColumn->mPrecision;
    aBindColumn->mScale       = sCurrBindColumn->mScale;
    aBindColumn->mMaxByteSize = sCurrBindColumn->mMaxByteSize;  // PROJ-2256
    aBindColumn->mFlag        = sCurrBindColumn->mFlag;

    /* PROJ-1789 Updatable Scrollable Cursor */

    aBindColumn->mDisplayName        = sCurrBindColumn->mDisplayName;
    aBindColumn->mDisplayNameSize    = sCurrBindColumn->mDisplayNameSize;
    aBindColumn->mColumnName         = sCurrBindColumn->mColumnName;
    aBindColumn->mColumnNameSize     = sCurrBindColumn->mColumnNameSize;
    aBindColumn->mBaseColumnName     = sCurrBindColumn->mBaseColumnName;
    aBindColumn->mBaseColumnNameSize = sCurrBindColumn->mBaseColumnNameSize;
    aBindColumn->mTableName          = sCurrBindColumn->mTableName;
    aBindColumn->mTableNameSize      = sCurrBindColumn->mTableNameSize;
    aBindColumn->mBaseTableName      = sCurrBindColumn->mBaseTableName;
    aBindColumn->mBaseTableNameSize  = sCurrBindColumn->mBaseTableNameSize;
    aBindColumn->mSchemaName         = sCurrBindColumn->mSchemaName;
    aBindColumn->mSchemaNameSize     = sCurrBindColumn->mSchemaNameSize;
    aBindColumn->mCatalogName        = NULL;
    aBindColumn->mCatalogNameSize    = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qci::fetchColumn( qciStatement           * aStatement,
                         UShort                   aBindId,
                         qciFetchColumnCallback   aFetchColumnCallback,
                         void                   * aFetchColumnContext )
{
    qcStatement     * sStatement;
    qcTemplate      * sTemplate;
    qciStmtState      sState;
    qciBindColumn   * sBindColumn;
    mtcColumn       * sTargetColumn;
    void            * sTargetValue;
    UInt            * sLocatorInfo = NULL;
    qcSimpleResult  * sResult;
    qmncPROJ        * sPROJ;

    sStatement = & aStatement->statement;
    sTemplate = QC_PRIVATE_TMPLATE(sStatement);

    IDE_TEST( getCurrentState( aStatement, & sState ) != IDE_SUCCESS );

    // execute 전이면 에러
    IDE_TEST_RAISE( sState < QCI_STMT_STATE_EXECUTED, err_invalid_binding );

    IDE_TEST_RAISE( aBindId >= sStatement->myPlan->sBindColumnCount,
                    err_invalid_binding );

    sBindColumn = & sStatement->myPlan->sBindColumn[aBindId].column;
    
    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        sResult = sStatement->simpleInfo.results;
        sPROJ = (qmncPROJ*)sStatement->myPlan->plan;
        
        sTargetColumn = &(sPROJ->simpleValues[aBindId].column);
        sTargetValue  = sResult->result +
            sResult->idx * sPROJ->simpleResultSize +
            sPROJ->simpleResultOffsets[aBindId];
    }
    else
    {
        // DB에 저장된 datatype과 value
        sTargetColumn = sTemplate->tmplate.stack[aBindId].column;
        sTargetValue = sTemplate->tmplate.stack[aBindId].value;
    }

    IDE_DASSERT( sTargetColumn->type.dataTypeId == sBindColumn->mType );

    // BUG-40427
    // 더 이상 내부적으로 사용하는 LOB Cursor가 아님을 표시한다.
    if( ( sTargetColumn->type.dataTypeId == MTD_BLOB_LOCATOR_ID ) ||
        ( sTargetColumn->type.dataTypeId == MTD_CLOB_LOCATOR_ID ) )
    {
        if ( *( smLobLocator * )sTargetValue != MTD_LOCATOR_NULL )
        {
            IDE_TEST( smiLob::getInfoPtr( *(smLobLocator*) sTargetValue,
                                          & sLocatorInfo )
                      != IDE_SUCCESS );

            *sLocatorInfo &= ~MTC_LOB_LOCATOR_CLIENT_MASK; 
            *sLocatorInfo |=  MTC_LOB_LOCATOR_CLIENT_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // Nothing to do.
    }

    // callback
    IDE_TEST( aFetchColumnCallback( aStatement->statement.mStatistics,
                                    sBindColumn,
                                    sTargetValue,
                                    aFetchColumnContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::fetchColumn( iduMemory     * aMemory,
                         qciStatement  * aStatement,
                         UShort          aBindId,
                         mtcColumn     * aColumn,
                         void          * aData )
{
    qcStatement     * sStatement;
    qcTemplate      * sTemplate;
    qciStmtState      sState;
    mtcColumn       * sTargetColumn;
    void            * sTargetValue;
    UInt            * sLocatorInfo = NULL;
    qcSimpleResult  * sResult;
    qmncPROJ        * sPROJ;

    sStatement = & aStatement->statement;
    sTemplate = QC_PRIVATE_TMPLATE(sStatement);

    IDE_TEST( getCurrentState( aStatement, & sState ) != IDE_SUCCESS );

    // execute 전이면 에러
    IDE_TEST_RAISE( sState < QCI_STMT_STATE_EXECUTED, err_invalid_binding );

    IDE_TEST_RAISE( aBindId >= sStatement->myPlan->sBindColumnCount,
                    err_invalid_binding );

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        sResult = sStatement->simpleInfo.results;
        sPROJ = (qmncPROJ*)sStatement->myPlan->plan;
        
        sTargetColumn = &(sPROJ->simpleValues[aBindId].column);
        sTargetValue  = sResult->result +
            sResult->idx * sPROJ->simpleResultSize +
            sPROJ->simpleResultOffsets[aBindId];
    }
    else
    {
        // DB에 저장된 datatype과 value
        sTargetColumn = sTemplate->tmplate.stack[aBindId].column;
        sTargetValue = sTemplate->tmplate.stack[aBindId].value;
    }

    // BUG-40427
    // 더 이상 내부적으로 사용하는 LOB Cursor가 아님을 표시한다.
    if( ( sTargetColumn->type.dataTypeId == MTD_BLOB_LOCATOR_ID ) ||
        ( sTargetColumn->type.dataTypeId == MTD_CLOB_LOCATOR_ID ) )
    {
        if ( *( smLobLocator * )sTargetValue != MTD_LOCATOR_NULL )
        {
            IDE_TEST( smiLob::getInfoPtr( *(smLobLocator*) sTargetValue,
                                          & sLocatorInfo )
                      != IDE_SUCCESS );

            *sLocatorInfo &= ~MTC_LOB_LOCATOR_CLIENT_MASK; 
            *sLocatorInfo |=  MTC_LOB_LOCATOR_CLIENT_TRUE; 
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qsxUtil::assignPrimitiveValue( aMemory,
                                             sTargetColumn,
                                             sTargetValue,
                                             aColumn,
                                             aData,
                                             & sTemplate->tmplate )  // dest template이 와야하나
                                                                     // session이 같으므로 상관없다.
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::makeBindColumnArray( qciStatement * aStatement )
{
    qcStatement       * sStatement;
    qcTemplate        * sTemplate;
    qciStmtState        sState;
    qmnPlan           * sPlan;
    qmncPROJ          * sPROJ;
    qmsTarget         * sTopTarget;
    qmsTarget         * sTarget;
    qciBindColumnInfo * sBindColumnInfo = NULL;
    qciBindColumn     * sBindColumn;
    mtcColumn         * sColumn;
    UShort              sBindColumnCount = 0;
    UShort              i;
    mtcNode           * sNode;
    UInt                sMaxByteSize;

    sStatement = & aStatement->statement;

    IDE_TEST( getCurrentState( aStatement, & sState )
              != IDE_SUCCESS );

    sTemplate = QC_SHARED_TMPLATE(sStatement);
    sPlan = sStatement->myPlan->plan;

    IDE_TEST( qmnPROJ::getCodeTargetPtr( sPlan,
                                         & sTopTarget )
              != IDE_SUCCESS );

    for( sTarget = sTopTarget;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        sBindColumnCount++;
    }

    if( sBindColumnCount > 0 )
    {
        // bindColumnInfo를 만든다.
        // qmpMem을 사용한다.
        IDE_TEST( QC_QMP_MEM(sStatement)->alloc(
                      sBindColumnCount * ID_SIZEOF(qciBindColumnInfo),
                      (void**) & sBindColumnInfo )
                  != IDE_SUCCESS);

        for( i = 0, sTarget = sTopTarget;
             sTarget != NULL;
             i++, sTarget = sTarget->next )
        {
            //fix BUG-17713
            sNode = mtf::convertedNode( &sTarget->targetColumn->node,
                                        &sTemplate->tmplate );

            sColumn = QTC_TMPL_COLUMN(sTemplate, (qtcNode*)sNode);
            
            // PROJ-2616 align을 고려한 컬럼크기
            if ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
                 == QC_STMT_FAST_EXEC_TRUE )
            {
                sPROJ = (qmncPROJ*)sPlan;
                sMaxByteSize = sPROJ->simpleValueSizes[i];
            }
            else
            {
                sMaxByteSize = sColumn->column.size; // PROJ-2256
            }
            
            // target column 정보 저장
            sBindColumn = & sBindColumnInfo[i].column;

            sBindColumn->mId                 = i;
            sBindColumn->mType               = sColumn->type.dataTypeId;
            sBindColumn->mLanguage           = sColumn->type.languageId;
            sBindColumn->mArguments          = sColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;
            sBindColumn->mPrecision          = sColumn->precision;
            sBindColumn->mScale              = sColumn->scale;
            sBindColumn->mMaxByteSize        = sMaxByteSize;
            sBindColumn->mFlag               = sTarget->flag;

            /* PROJ-1789 Updatable Scrollable Cursor */

            sBindColumn->mDisplayName        = sTarget->displayName.name;
            sBindColumn->mDisplayNameSize    = sTarget->displayName.size;

            if( sTarget->aliasColumnName.size == QC_POS_EMPTY_SIZE )
            {
                sBindColumn->mColumnName         = NULL;
                sBindColumn->mColumnNameSize     = 0;
            }
            else
            {
                sBindColumn->mColumnName         = sTarget->aliasColumnName.name;
                sBindColumn->mColumnNameSize     = sTarget->aliasColumnName.size;
            }

            if( sTarget->columnName.size == QC_POS_EMPTY_SIZE )
            {
                sBindColumn->mBaseColumnName     = NULL;
                sBindColumn->mBaseColumnNameSize = 0;
            }
            else
            {
                sBindColumn->mBaseColumnName     = sTarget->columnName.name;
                sBindColumn->mBaseColumnNameSize = sTarget->columnName.size;
            }

            if( sTarget->aliasTableName.size == QC_POS_EMPTY_SIZE )
            {
                sBindColumn->mTableName          = NULL;
                sBindColumn->mTableNameSize      = 0;
            }
            else
            {
                sBindColumn->mTableName          = sTarget->aliasTableName.name;
                sBindColumn->mTableNameSize      = sTarget->aliasTableName.size;
            }

            if( sTarget->tableName.size == QC_POS_EMPTY_SIZE )
            {
                sBindColumn->mBaseTableName      = NULL;
                sBindColumn->mBaseTableNameSize  = 0;
            }
            else
            {
                sBindColumn->mBaseTableName      = sTarget->tableName.name;
                sBindColumn->mBaseTableNameSize  = sTarget->tableName.size;
            }

            if( sTarget->userName.size == QC_POS_EMPTY_SIZE )
            {
                sBindColumn->mSchemaName         = NULL;
                sBindColumn->mSchemaNameSize     = 0;
            }
            else
            {
                sBindColumn->mSchemaName         = sTarget->userName.name;
                sBindColumn->mSchemaNameSize     = sTarget->userName.size;
            }

            sBindColumn->mCatalogName        = NULL;
            sBindColumn->mCatalogNameSize    = 0;

            // target 정보 저장
            sBindColumnInfo[i].target = sTarget;
        }
    }
    else
    {
        // Nothing to do.
    }

    // 저장한다.
    sStatement->myPlan->sBindColumn      = sBindColumnInfo;
    sStatement->myPlan->sBindColumnCount = sBindColumnCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::makeBindParamArray( qciStatement  * aStatement )
{
    qcStatement      * sStatement;
    qciBindParam     * sBindParam;
    qciBindParamInfo * sBindParamInfo = NULL;
    UShort             sBindParamCount = 0;
    UShort             i;

    sStatement = & aStatement->statement;

    sBindParamCount = qcg::getBindCount( sStatement );

    if( sBindParamCount > 0 )
    {
        // bindParamInfo를 만든다.
        IDE_TEST( QC_QMP_MEM(sStatement)->alloc(
                      sBindParamCount * ID_SIZEOF(qciBindParamInfo),
                      (void**) & sBindParamInfo )
                  != IDE_SUCCESS);

        for( i = 0; i < sBindParamCount; i++ )
        {
            sBindParam = & sBindParamInfo[i].param;

            // set default type
            sBindParam->id            = i;
            sBindParam->type          = MTD_VARCHAR_ID;
            sBindParam->language      = MTL_DEFAULT;
            sBindParam->arguments     = 1;
            sBindParam->precision     = QCU_BIND_PARAM_DEFAULT_PRECISION; // fix BUG-36793
            sBindParam->scale         = 0;
            sBindParam->inoutType     = CMP_DB_PARAM_INPUT;
            sBindParam->data          = NULL;
            sBindParam->dataSize      = 0; // PROJ-2163

            // 아직 bind되지 않았음.
            sBindParamInfo[i].isParamInfoBound = ID_FALSE;
            sBindParamInfo[i].isParamDataBound = ID_FALSE; // fix BUG-16482

            // convert 정보 초기화
            sBindParamInfo[i].convert = NULL;

            // canonize buffer 초기화
            sBindParamInfo[i].canonBuf = NULL;
        }
    }
    else
    {
        // Nothing to do.
    }

    // 저장한다.
    sStatement->myPlan->sBindParam      = sBindParamInfo;
    sStatement->myPlan->sBindParamCount = sBindParamCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// Param Info protocl
IDE_RC qci::setBindParamInfo( qciStatement  * aStatement,
                              qciBindParam  * aBindParam )
{
    qcStatement    * sStatement;
    qciBindParam     sBindParam;

    UShort           sParamCount;
    UInt             i;

    sStatement = & aStatement->statement;

    sParamCount = getParameterCount(aStatement);

    IDE_TEST_RAISE( aBindParam->id >= sStatement->pBindParamCount,
                    err_invalid_binding );

    // PROJ-2002 Column Security
    // 암호 타입으로는 바인드할 수 없음
    IDE_TEST_RAISE( ( (aBindParam->type == MTD_ECHAR_ID) ||
                      (aBindParam->type == MTD_EVARCHAR_ID) ),
                    err_invalid_binding );

    // INPUT or INPUT_OUTPUT type의 마지막 ParamInfo의 Id를 기록
    switch (aBindParam->inoutType)
    {
        case CMP_DB_PARAM_INPUT_OUTPUT:
        {
            if( sStatement->calledByPSMFlag == ID_TRUE )
            {
                IDE_TEST_RAISE( ( (aBindParam->type == MTD_BLOB_LOCATOR_ID) ||
                                  (aBindParam->type == MTD_CLOB_LOCATOR_ID) ),
                                err_invalid_binding );
            }
            else
            {
                // lob value, lob locator 모두 불가능
                IDE_TEST_RAISE( ( (aBindParam->type == MTD_BLOB_ID) ||
                                  (aBindParam->type == MTD_CLOB_ID) ||
                                  (aBindParam->type == MTD_BLOB_LOCATOR_ID) ||
                                  (aBindParam->type == MTD_CLOB_LOCATOR_ID) ),
                                err_invalid_binding );
            }
            /* fall through */
        }
        case CMP_DB_PARAM_INPUT:
        {
            sStatement->bindParamDataInLastId = aBindParam->id;
            break;
        }
        case CMP_DB_PARAM_OUTPUT:
        {
            break;
        }
        default:
        {
            IDE_DASSERT( 0 );
            break;
        }

    }

    // bind param 정보를 변경한다.

    sBindParam.id            = aBindParam->id;
    sBindParam.type          = aBindParam->type;
    sBindParam.language      = aBindParam->language;
    sBindParam.arguments     = aBindParam->arguments;
    sBindParam.precision     = aBindParam->precision;
    sBindParam.scale         = aBindParam->scale;
    sBindParam.inoutType     = aBindParam->inoutType;
    // BUG-43636 qcg::initBindParamData에서 해당 메모리를 free 할 수 있도록
    // NULL이 아닌 이전 메모리를 저장합니다.
    sBindParam.data          = sStatement->pBindParam[aBindParam->id].param.data;
    sBindParam.dataSize      = 0; // PROJ-2163
    sBindParam.ctype         = aBindParam->ctype; // PROJ-2616
    sBindParam.sqlctype      = aBindParam->sqlctype; // PROJ-2616

    sStatement->pBindParam[aBindParam->id].param = sBindParam;

    // 바인드 되었음.
    sStatement->pBindParam[aBindParam->id].isParamInfoBound = ID_TRUE;

    // convert 정보를 초기화한다.
    // memory 낭비가 있지만 많지 않다.
    sStatement->pBindParam[aBindParam->id].convert = NULL;

    // canonize buffer를 초기화한다.
    // memory 낭비가 있지만 많지 않다.
    sStatement->pBindParam[aBindParam->id].canonBuf = NULL;

    // PROJ-2163
    // pBindParam 의 값이 바뀌었다.
    sStatement->pBindParamChangedFlag = ID_TRUE;

    // PROJ-2163
    if( aBindParam->id == (sParamCount - 1) )
    {
        for( i = 0; i < sStatement->pBindParamCount; i++ )
        {
            // BUG-15878
            IDE_TEST_RAISE( sStatement->pBindParam[i].isParamInfoBound == ID_FALSE,
                            err_bind_column_count_mismatch );
        }

        // 호스트 변수의 값을 저장할 데이터 영역을 초기화 한다.
        // 이 영역은 variable tuple 의 row 로도 사용된다.
        IDE_TEST_RAISE( qcg::initBindParamData( sStatement ) != IDE_SUCCESS,
                        err_invalid_binding_init_data );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION( err_bind_column_count_mismatch );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_BIND_COLUMN_COUNT_MISMATCH));
    }
    IDE_EXCEPTION( err_invalid_binding_init_data );
    {
    }
    IDE_EXCEPTION_END;

    // BUG-37405
    for( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        sStatement->pBindParam[i].isParamInfoBound = ID_FALSE;
    }
    
    // PROJ-2163
    //sStatement->pBindParam[aBindParam->id].isParamInfoBound = ID_FALSE;

    return IDE_FAILURE;
}


// Param Info List protocol
IDE_RC qci::setBindParamInfoSet( qciStatement               * aStatement,
                                 void                       * aBindContext,
                                 qciReadBindParamInfoCallback aReadBindParamInfoCallback)
{
    qcStatement    * sStatement;
    qciBindParam     sBindParam;
    UShort           sParamCount;
    UShort           sBindId;
    UInt             i;

    sStatement = & aStatement->statement;

    sParamCount = getParameterCount(aStatement);

    for( sBindId = 0; sBindId < sParamCount; sBindId++ )
    {
        // bind param 정보를 변경한다.

        IDE_TEST_RAISE( aReadBindParamInfoCallback( aBindContext,
                                                    &sBindParam )
                        != IDE_SUCCESS, err_bind_column_count_mismatch );


        // INPUT or INPUT_OUTPUT type의 마지막 ParamInfo의 Id를 기록
        switch (sBindParam.inoutType)
        {
            case CMP_DB_PARAM_INPUT_OUTPUT:
                // lob value, lob locator 모두 불가능
                IDE_TEST_RAISE( ( (sBindParam.type == MTD_BLOB_ID) ||
                                  (sBindParam.type == MTD_CLOB_ID) ||
                                  (sBindParam.type == MTD_BLOB_LOCATOR_ID) ||
                                  (sBindParam.type == MTD_CLOB_LOCATOR_ID) ),
                                err_invalid_binding );
                /* fall through */

            case CMP_DB_PARAM_INPUT:
                sStatement->bindParamDataInLastId = sBindParam.id;
                break;

            case CMP_DB_PARAM_OUTPUT:
                break;

            default:
                IDE_DASSERT( 0 );
                    break;
        }

        IDE_ASSERT( sStatement->pBindParam != NULL);

        // BUG-44492 qcg::initBindParamData에서 해당 메모리를 free 할 수 있도록
        // NULL이 아닌 이전 메모리를 저장합니다.
        sBindParam.data = sStatement->pBindParam[sBindId].param.data;
        sStatement->pBindParam[sBindId].param = sBindParam;

        // 바인드 되었음.
        sStatement->pBindParam[sBindId].isParamInfoBound = ID_TRUE;

        // convert 정보를 초기화한다.
        // memory 낭비가 있지만 많지 않다.
        sStatement->pBindParam[sBindId].convert = NULL;

        // canonize buffer를 초기화한다.
        // memory 낭비가 있지만 많지 않다.
        sStatement->pBindParam[sBindId].canonBuf = NULL;

        // PROJ-2163
        // pBindParam 의 값이 바뀌었다.
        sStatement->pBindParamChangedFlag = ID_TRUE;
    }

    // PROJ-2163
    for( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        // BUG-15878
        IDE_TEST_RAISE( sStatement->pBindParam[i].isParamInfoBound == ID_FALSE,
                        err_bind_column_count_mismatch );
    }

    // 호스트 변수의 값을 저장할 데이터 영역을 초기화 한다.
    // 이 영역은 variable tuple 의 row 로도 사용된다.
    IDE_TEST( qcg::initBindParamData( sStatement ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_bind_column_count_mismatch );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_BIND_COLUMN_COUNT_MISMATCH));
    }
    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    // PROJ-2163
    for( sBindId = 0; sBindId < sParamCount; sBindId++ )
    {
        sStatement->pBindParam[sBindId].isParamInfoBound = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC qci::getBindParamInfo( qciStatement  * aStatement,
                              qciBindParam  * aBindParam )
{
    qcStatement    * sStatement;
    qciStmtState     sState;
    qciBindParam   * sBindParam;

    sStatement = & aStatement->statement;

    IDE_TEST( getCurrentState( aStatement, & sState )
              != IDE_SUCCESS );

    // prepare 전이면 에러
    IDE_TEST_RAISE( sState < QCI_STMT_STATE_PREPARED,
                    err_invalid_binding );

    IDE_TEST_RAISE( aBindParam->id >= sStatement->pBindParamCount,
                    err_invalid_binding );

    sBindParam = & sStatement->pBindParam[aBindParam->id].param;

    aBindParam->type          = sBindParam->type;
    aBindParam->language      = sBindParam->language;
    aBindParam->arguments     = sBindParam->arguments;
    aBindParam->precision     = sBindParam->precision;
    aBindParam->scale         = sBindParam->scale;
    aBindParam->inoutType     = sBindParam->inoutType;
    aBindParam->data          = sBindParam->data;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// setBindParamDataInList protocol(A7)
IDE_RC qci::setBindParamDataSet( qciStatement                 *aStatement,
                                 UChar                        *aParamData,
                                 UInt                         *aOffset,
                                 qciCopyBindParamDataCallback  aCopyBindParamDataCallback )
{
    qcStatement    *sStatement   = &(aStatement->statement);
    qciBindParam   *sBindParam;
    UShort          sParamCount;
    UShort          sBindId;

    UChar          *sSource;
    UInt            sOffset     = 0;
    UInt            sStructSize = 0;

    IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                         EXEC_BIND_PARAM_DATA )
                                         != IDE_SUCCESS );

    sParamCount = getParameterCount(aStatement);

    for( sBindId = 0; sBindId < sParamCount; sBindId++ )
    {
        // 바인드되지 않았다면 에러
        IDE_TEST_RAISE( sStatement->pBindParam[sBindId].isParamInfoBound == ID_FALSE,
                err_invalid_binding );

        sBindParam  = &(sStatement->pBindParam[sBindId].param);

        sSource = aParamData + sOffset;

        // in, inout 타입 data.
        if( sBindParam->inoutType != CMP_DB_PARAM_OUTPUT )
        {
            IDE_TEST(aCopyBindParamDataCallback(sBindParam, sSource, &sStructSize) != IDE_SUCCESS);
            sOffset += sStructSize;
        }
        else
        {
            continue;
        }

        // BUG-22196 
        sStatement->pBindParam[sBindId].isParamDataBound = ID_TRUE;
    }

    // BUG-34995
    // QCI_STMT_STATE 를 DATA_BOUND 로 변경한다.
    IDE_TEST( qci::setParamDataState( aStatement ) != IDE_SUCCESS);

    *aOffset += sOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// PROJ-1697: BindParamDataSet List Protocol(A5)
IDE_RC qci::setBindParamDataSetOld( qciStatement                * aStatement,
                                    void                        * aBindContext,
                                    qciSetParamDataCallback       aSetParamDataCallback,
                                    qciReadBindParamDataCallback  aReadBindParamDataCallback )
{
    qcStatement      * sStatement;
    qciBindParam     * sBindParam;
    UInt               sParamState = 0;
    UShort             sParamCount = 0;
    UShort             sBindId = 0;

    sStatement = & aStatement->statement;

    sParamCount = getParameterCount(aStatement);

    IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                                         EXEC_BIND_PARAM_DATA )
                                         != IDE_SUCCESS );

    for( ; sBindId < sParamCount; sBindId++ )
    {
        sParamState = 0;

        IDE_ASSERT( sStatement->pBindParam != NULL );

        // 바인드되지 않았다면 에러
        IDE_TEST_RAISE( sStatement->pBindParam[sBindId].isParamInfoBound == ID_FALSE,
                err_invalid_binding );

        sBindParam = & sStatement->pBindParam[sBindId].param;

        // in, inout 타입 data.
        if( ( sBindParam->inoutType == CMP_DB_PARAM_INPUT ) ||
            ( sBindParam->inoutType == CMP_DB_PARAM_INPUT_OUTPUT ) )
        {
            // read a cmtAny from a collection
            IDE_TEST( aReadBindParamDataCallback( aBindContext )
                      != IDE_SUCCESS );
            sParamState = 1;
        }
        else
        {
            continue;
        }

        // PROJ-2163
        // copy data to pBindParam->param.data (variableTuple's row)
        IDE_TEST( qcg::setBindData( sStatement,
                                    sBindParam->id,
                                    sBindParam->inoutType,
                                    sBindParam,
                                    aBindContext,
                                    (void*) aSetParamDataCallback )
                  != IDE_SUCCESS );

        // BUG-22196
        sStatement->pBindParam[sBindId].isParamDataBound = ID_TRUE;
    }

    // BUG-34995
    // QCI_STMT_STATE 를 DATA_BOUND 로 변경한다.
    IDE_TEST( qci::setParamDataState( aStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    /* 통신 버퍼를 비운다 */
    for( sBindId += sParamState; sBindId < sParamCount; sBindId++ )
    {
        sBindParam = & sStatement->pBindParam[sBindId].param;
        if( ( sBindParam->inoutType == CMP_DB_PARAM_INPUT ) ||
            ( sBindParam->inoutType == CMP_DB_PARAM_INPUT_OUTPUT ) )
        {
            if( sStatement->pBindParam[sBindId].isParamInfoBound == ID_TRUE )
            {
                (void)aReadBindParamDataCallback( aBindContext );
            }
        }
    }

    return IDE_FAILURE;
}

// setBindParamDataIn protocol(A7 after)
IDE_RC qci::setBindParamData( qciStatement                 *aStatement,
                              UShort                        aBindId,
                              UChar                        *aParamData,
                              qciCopyBindParamDataCallback  aCopyBindParamDataCallback )
{
    qcStatement      *sStatement   = &(aStatement->statement);
    qciBindParam     *sBindParam;
    UShort            sBindId      = aBindId;
    UChar            *sSource;
    UInt              sStructSize;

    if( aBindId == 0 )
    {
        IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                    EXEC_BIND_PARAM_DATA )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    IDE_TEST_RAISE( aBindId >= sStatement->pBindParamCount,
                    err_invalid_binding );

    // 바인드되지 않았다면 에러
    IDE_TEST_RAISE( sStatement->pBindParam[sBindId].isParamInfoBound == ID_FALSE,
            err_invalid_binding );

    sBindParam  = &(sStatement->pBindParam[sBindId].param);
    sSource     = aParamData;

    // in, inout 타입만 가능하다.
    IDE_TEST_RAISE( sBindParam->inoutType == CMP_DB_PARAM_OUTPUT,
                    err_invalid_binding );

    IDE_TEST(aCopyBindParamDataCallback(sBindParam, sSource, &sStructSize) != IDE_SUCCESS);

    // fix BUG-16482
    sStatement->pBindParam[aBindId].isParamDataBound = ID_TRUE;

    if( isLastParamData( aStatement, sBindParam->id ) )
    {
        // BUG-34995
        // QCI_STMT_STATE 를 DATA_BOUND 로 변경한다.
        IDE_TEST( qci::setParamDataState( aStatement ) != IDE_SUCCESS);

    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// setBindParamDataIn protocol(A5)
IDE_RC qci::setBindParamDataOld(
        qciStatement                * aStatement,
        UShort                        aBindId,
        qciSetParamDataCallback       aSetParamDataCallback,
        void                        * aBindContext )
{
    qcStatement      * sStatement;
    qciBindParam     * sBindParam;

    sStatement = & aStatement->statement;

    if( aBindId == 0 )
    {
        IDE_TEST( checkExecuteFuncAndSetEnv( aStatement,
                    EXEC_BIND_PARAM_DATA )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    IDE_TEST_RAISE( aBindId >= sStatement->pBindParamCount,
                    err_invalid_binding );

    // 바인드되지 않았다면 에러
    IDE_TEST_RAISE( sStatement->pBindParam[aBindId].isParamInfoBound == ID_FALSE,
            err_invalid_binding );

    sBindParam = & sStatement->pBindParam[aBindId].param;

    // in, inout 타입만 가능하다.
    IDE_TEST_RAISE( sBindParam->inoutType == CMP_DB_PARAM_OUTPUT,
                    err_invalid_binding );

    // PROJ-2163
    // copy data to pBindParam->param.data (variableTuple's row)
    IDE_TEST( qcg::setBindData( sStatement,
                                sBindParam->id,
                                sBindParam->inoutType,
                                sBindParam,
                                aBindContext,
                                (void*) aSetParamDataCallback )
            != IDE_SUCCESS );

    // fix BUG-16482
    sStatement->pBindParam[aBindId].isParamDataBound = ID_TRUE;

    if( isLastParamData( aStatement, sBindParam->id ) == ID_TRUE )
    {
        // BUG-34995
        // QCI_STMT_STATE 를 DATA_BOUND 로 변경한다.
        IDE_TEST( qci::setParamDataState( aStatement ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//mmtInternalSql
IDE_RC qci::setBindParamData( qciStatement  * aStatement,
                              UShort          aBindId,
                              void          * aData,
                              UInt            aDataSize )
{
    qcStatement      * sStatement;
    qciBindParam     * sBindParam;

    sStatement = & aStatement->statement;

    if( aBindId == 0 )
    {
        IDE_TEST( checkExecuteFuncAndSetEnv( aStatement, EXEC_BIND_PARAM_DATA )
                != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    IDE_TEST_RAISE( aBindId >= sStatement->pBindParamCount,
                    err_invalid_binding );

    // 바인드되지 않았다면 에러
    IDE_TEST_RAISE( sStatement->pBindParam[aBindId].isParamInfoBound == ID_FALSE,
                    err_invalid_binding );

    sBindParam = & sStatement->pBindParam[aBindId].param;

    // PROJ-2163 
    // copy data to pBindParam->param.data (variableTuple's row)
    if ( sBindParam->inoutType != CMP_DB_PARAM_OUTPUT )
    {
        IDE_TEST( qcg::setBindData( sStatement,
                                    aBindId,
                                    sBindParam->inoutType,
                                    aDataSize,
                                    aData )
                  != IDE_SUCCESS );

        // fix BUG-16482
        sStatement->pBindParam[aBindId].isParamDataBound = ID_TRUE;
    }

    if( isLastParamData ( aStatement, aBindId ) == ID_TRUE )
    {
        // BUG-34995
        // QCI_STMT_STATE 를 DATA_BOUND 로 변경한다.
        IDE_TEST( qci::setParamDataState( aStatement ) != IDE_SUCCESS);

    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qci::getBindParamData( qciStatement                * aStatement,
                              UShort                        aBindId,
                              qciGetBindParamDataCallback   aGetBindParamDataCallback,
                              void                        * aGetBindParamContext )
{
    qcStatement     * sStatement;
    qcTemplate      * sTemplate;
    qciStmtState      sState;
    qciBindParam    * sBindParam;
    UShort            sBindTuple;
    mtcColumn       * sSrcColumn;
    mtcColumn         sDestColumn;
    void            * sSrcValue;

    sStatement = & aStatement->statement;
    sTemplate = QC_PRIVATE_TMPLATE(sStatement);

    IDE_TEST( getCurrentState( aStatement, & sState )
              != IDE_SUCCESS );

    // execute 전이면 에러
    IDE_TEST_RAISE( sState < QCI_STMT_STATE_PARAM_DATA_BOUND,
                    err_invalid_binding );

    IDE_TEST_RAISE( aBindId >= sStatement->pBindParamCount,
                    err_invalid_binding );

    sBindParam = & sStatement->pBindParam[aBindId].param;

    // out, inout 타입만 가능하다.
    IDE_TEST_RAISE( sBindParam->inoutType == CMP_DB_PARAM_INPUT,
                    err_invalid_binding );

    // DB에 저장된 datatype과 value
    sBindTuple = sTemplate->tmplate.variableRow;

    IDE_TEST_RAISE( sBindTuple == ID_USHORT_MAX, err_invalid_binding );

    sSrcColumn = & sTemplate->tmplate.rows[sBindTuple].columns[aBindId];
    sSrcValue = (SChar*) sTemplate->tmplate.rows[sBindTuple].row
        + sTemplate->tmplate.rows[sBindTuple].columns[aBindId].column.offset;

    // 사용자가 바인드한 datatype
    IDE_TEST( mtc::initializeColumn( & sDestColumn,
                                     sBindParam->type,
                                     sBindParam->arguments,
                                     sBindParam->precision,
                                     sBindParam->scale )
              != IDE_SUCCESS );

    // BUG-35195
    // bind data는 항상 사용자가 bind한 type,precision으로 template에 저장되어있다.
    IDE_TEST_RAISE( sSrcColumn->type.dataTypeId != sBindParam->type,
                    ERR_ABORT_INVALID_COLUMN_TYPE );
    IDE_TEST_RAISE( sSrcColumn->column.size != sDestColumn.column.size,
                    ERR_ABORT_INVALID_COLUMN_PRECISION );
    
    // callback
    IDE_TEST( aGetBindParamDataCallback( aStatement->statement.mStatistics,
                                         sBindParam,
                                         sSrcValue,
                                         aGetBindParamContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION( ERR_ABORT_INVALID_COLUMN_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::getBindParamData",
                                  "mismatch column type" ));
    }
    IDE_EXCEPTION( ERR_ABORT_INVALID_COLUMN_PRECISION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::getBindParamData",
                                  "mismatch column precision" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::getBindParamData( qciStatement  * aStatement,
                              UShort          aBindId,
                              void          * aData )
{
    qcStatement     * sStatement;
    qcTemplate      * sTemplate;
    qciStmtState      sState;
    qciBindParam    * sBindParam;
    UShort            sBindTuple;
    mtcColumn       * sSrcColumn;
    mtcColumn         sDestColumn;
    void            * sSrcValue;
    UInt              sActualSize;

    sStatement = & aStatement->statement;
    sTemplate = QC_PRIVATE_TMPLATE(sStatement);

    IDE_TEST( getCurrentState( aStatement, & sState )
              != IDE_SUCCESS );

    // execute 전이면 에러
    IDE_TEST_RAISE( sState < QCI_STMT_STATE_PARAM_DATA_BOUND,
                    err_invalid_binding );

    IDE_TEST_RAISE( aBindId >= sStatement->pBindParamCount,
                    err_invalid_binding );

    sBindParam = & sStatement->pBindParam[aBindId].param;

    // out, inout 타입만 가능하다.
    IDE_TEST_RAISE( sBindParam->inoutType == CMP_DB_PARAM_INPUT,
                    err_invalid_binding );

    // DB에 저장된 datatype과 value
    sBindTuple = sTemplate->tmplate.variableRow;

    IDE_TEST_RAISE( sBindTuple == ID_USHORT_MAX, err_invalid_binding );

    sSrcColumn = & sTemplate->tmplate.rows[sBindTuple].columns[aBindId];
    sSrcValue = (SChar*) sTemplate->tmplate.rows[sBindTuple].row
        + sTemplate->tmplate.rows[sBindTuple].columns[aBindId].column.offset;

    // 사용자가 바인드한 datatype
    IDE_TEST( mtc::initializeColumn( & sDestColumn,
                                     sBindParam->type,
                                     sBindParam->arguments,
                                     sBindParam->precision,
                                     sBindParam->scale )
              != IDE_SUCCESS );

    // BUG-35195
    // bind data는 항상 사용자가 bind한 type,precision으로 template에 저장되어있다.
    IDE_TEST_RAISE( sSrcColumn->type.dataTypeId != sBindParam->type,
                    ERR_ABORT_INVALID_COLUMN_TYPE );
    IDE_TEST_RAISE( sSrcColumn->column.size != sDestColumn.column.size,
                    ERR_ABORT_INVALID_COLUMN_PRECISION );
    
    sActualSize = sSrcColumn->module->actualSize( sSrcColumn,
                                                  sSrcValue );

    idlOS::memcpy( aData,
                   sSrcValue,
                   sActualSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION( ERR_ABORT_INVALID_COLUMN_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::getBindParamData",
                                  "mismatch column type" ));
    }
    IDE_EXCEPTION( ERR_ABORT_INVALID_COLUMN_PRECISION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::getBindParamData",
                                  "mismatch column precision" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::buildBindParamInfo( qciStatement  * aStatement )
{
    qcStatement      * sStatement;
    qciBindParam     * sBindParam;
    UShort             i;
    qciBindParam       sUndefBindParam;
    UShort             sBindParamCount;

    sStatement = & aStatement->statement;

    // PROJ-2163
    if( sStatement->pBindParam == NULL )
    {
        // undef 타입 parameter 초기화
        sUndefBindParam.type      = MTD_UNDEF_ID;
        sUndefBindParam.language  = MTL_DEFAULT;
        sUndefBindParam.arguments = 0;
        sUndefBindParam.precision = 0;
        sUndefBindParam.scale     = 0;
        sUndefBindParam.inoutType = CMP_DB_PARAM_INPUT;
        sUndefBindParam.data      = NULL;
        sUndefBindParam.dataSize  = 0;
        sUndefBindParam.ctype     = 0;
        sUndefBindParam.sqlctype  = 0;

        sBindParam = & sUndefBindParam;

        // undef 타입 binding
        sBindParamCount = qcg::getBindCount( sStatement );

        for( i = 0; i < sBindParamCount ; i++ )
        {
            sUndefBindParam.id = i;

            IDE_TEST( qcg::setBindColumn( sStatement,
                                          sBindParam->id,
                                          sBindParam->type,
                                          sBindParam->arguments,
                                          sBindParam->precision,
                                          sBindParam->scale )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // user type binding
        sBindParamCount = sStatement->pBindParamCount;

        for( i = 0; i < sBindParamCount ; i++ )
        {
            sBindParam = & sStatement->pBindParam[i].param;

            IDE_TEST( qcg::setBindColumn( sStatement,
                                          sBindParam->id,
                                          sBindParam->type,
                                          sBindParam->arguments,
                                          sBindParam->precision,
                                          sBindParam->scale )
                      != IDE_SUCCESS );
        }

        if( sStatement->myPlan->planEnv != NULL )
        {
            qcgPlan::registerPlanBindInfo( sStatement );
        }
    }

    IDE_TEST( qtc::setVariableTupleRowSize( QC_SHARED_TMPLATE(sStatement) )
              != IDE_SUCCESS );

    // pBindParam 으로 type binding 해서 바인드 타입이 반영되었다.
    sStatement->pBindParamChangedFlag = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::moveNextRecord( qciStatement * aStatement,
                            smiStatement * aSmiStmt,
                            idBool       * aRecordExist )
{
    qcStatement  * sStatement;
    smiStatement * sSmiStmtOrg;
    qmsParseTree * sParseTree;
    qmcRowFlag     sRowFlag = QMC_ROW_INITIALIZE;
    qciStmtType    sStmtType;
    qciArgDequeue  sArgDequeue;

    IDE_FT_ROOT_BEGIN();

    IDE_NOFT_BEGIN();

    IDU_FIT_POINT( "qci::moveNextRecord::__NOFT__" );

    sStatement = & aStatement->statement;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        IDE_TEST( qmxSimple::fastMoveNextResult( sStatement,
                                                 aRecordExist )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE(&aStatement->statement),
                                 aStatement->statement.myPlan->plan,
                                 & sRowFlag )
                  != IDE_SUCCESS );

        if ( ( sRowFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            *aRecordExist = ID_TRUE;
        }
        else
        {
            *aRecordExist = ID_FALSE;
        }
    }

    //PROJ-1677 DEQUEUE
    IDE_TEST( getStmtType( aStatement, &sStmtType ) != IDE_SUCCESS );
    if( sStmtType == QCI_STMT_DEQUEUE )
    {
        sParseTree = (qmsParseTree *)(sStatement->myPlan->parseTree);

        sArgDequeue.mMmSession = sStatement->session->mMmSession;
        sArgDequeue.mTableID = sParseTree->queue->tableID;
        sArgDequeue.mWaitSec = sParseTree->queue->waitSec;

        IDE_TEST( qcg::mDequeueQueueFuncPtr( (void *)&sArgDequeue ,
                                             *aRecordExist)
                  != IDE_SUCCESS );
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    IDE_NOFT_END();

    IDE_FT_ROOT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_NOFT_EXCEPTION_BEGIN();

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    IDE_NOFT_EXCEPTION_END();

    IDE_FT_ROOT_END();

    return IDE_FAILURE;
}

//PROJ-1436 SQL-Plan Cache
IDE_RC qci::hardRebuild( qciStatement            * aStatement,
                         smiStatement            * aSmiStmt,
                         smiStatement            * aParentSmiStmt,
                         qciSQLPlanCacheContext  * aPlanCacheContext,
                         SChar                   * aQueryString,
                         UInt                      aQueryLen,
                         smiDistTxInfo           * aDistTxInfo ) /* Default : NULL */
{
    qcStatement  * sStatement;
    smiStatement * sSmiStmtOrg;

    // PROJ-2617 PVO안정성
    // 아래의 함수에 이미 FT / NOFT 적용 되어 있어 현재의 함수에서
    // 추가 하지 않는다.
    sStatement = & aStatement->statement;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    IDE_TEST( parse( aStatement,
                     aQueryString,
                     aQueryLen )
              != IDE_SUCCESS );

    //-------------------------------------------------------------------------
    // PROJ-2163
    // Type Binding 추가
    // QP 쪽 핸들링으로 type binding 이후 MM 의 statement 상태는 변경되지 않는다
    // hardRebuild 완료 후 data 정보를 복구하고 setBindState(MMC_STMT_BIND_DATA)가
    //  수행된다. 따라서 MMC_STMT_BIND_INFO 상태를 skip 하는 셈이다.
    // 결과적으로 qci::hardRebuild 이후의 진입에 문제는 없으나 상태의 흐름을
    // 일관성 있게 적용하려면 MM 에서 parse, bindparamInfo, hardPrepare 를
    // 각각 호출하는 구조로 바꿔야 한다.
    //-------------------------------------------------------------------------
    IDE_TEST( bindParamInfo( aStatement,
                             aPlanCacheContext ) != IDE_SUCCESS );

    IDE_TEST( hardPrepare( aStatement,
                           aParentSmiStmt,
                           aPlanCacheContext,
                           aDistTxInfo )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( ideIsRebuild() == IDE_SUCCESS )
    {
        // rebuild중 다시 rebuild가 발생한 경우는 statement만
        // clear하고 다시 rebuild를 시도할 수 있도록 한다.
        (void) qcg::clearStatement( &(aStatement->statement),
                             ID_TRUE ); // rebuild = TRUE

        (void) changeStmtState( aStatement, EXEC_INITIALIZE );
    }
    else
    {
        // parse, hard prepare가 실패하는 경우 private template을
        // 지정하여 bind parameter 정보를 기록한다.
        if( QC_PRIVATE_TMPLATE(sStatement) == NULL )
        {
            (void) qcg::setPrivateArea( sStatement );
        }
        else
        {
            // Nothing to do.
        }

        qcg::setSmiStmt( sStatement, sSmiStmtOrg );

        (void) changeStmtState( aStatement, EXEC_EXECUTE );

        aStatement->flag &= ~QCI_STMT_REBUILD_EXEC_MASK;
        aStatement->flag |= QCI_STMT_REBUILD_EXEC_FAILURE;
    }

    return IDE_FAILURE;
}

IDE_RC qci::retry( qciStatement * aStatement,
                   smiStatement * aSmiStmt )
{
    qcStatement    * sStatement;
    smiStatement   * sSmiStmtOrg;

    sStatement = &aStatement->statement;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    IDE_TEST( checkExecuteFuncAndSetEnv(
                  aStatement,
                  EXEC_RETRY ) != IDE_SUCCESS );

    IDE_TEST( changeStmtState( aStatement, EXEC_RETRY )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
                           
}

IDE_RC qci::getStmtType( qciStatement  *aStatement,
                         qciStmtType   *aType )
{
    if( aStatement->statement.myPlan->parseTree != NULL )
    {
        *aType = (qciStmtType)aStatement->statement.myPlan->parseTree->stmtKind;
    }
    else
    {
        IDE_RAISE(ERR_UNINITIALIZED_PARSE_TREE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNINITIALIZED_PARSE_TREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::getStmtType",
                                  "statement might have not been parsed" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::checkInternalSqlType( qciStmtType aType )
{
    if( ( aType == QCI_STMT_SET_AUTOCOMMIT_TRUE ) ||
        ( aType == QCI_STMT_SET_AUTOCOMMIT_FALSE ) )
    {
        IDE_RAISE(ERR_NO_SUPPORT_SYNTAX);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SUPPORT_SYNTAX );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::checkInternalProcCall( qciStatement * aStatement )
{
/***********************************************************************
 *
 *  Description : PROJ-1386
 *                result-set procedure는 dynamic sql로 호출불가
 *
 *  Implementation : prepared상태에서만 호출가능
 *                   proc call이고 resultset 있는경우 에러
 *
 ***********************************************************************/

    UShort sResultSetCount;

    sResultSetCount = 0;

    IDE_ASSERT( aStatement->state == QCI_STMT_STATE_PREPARED );

    if( (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_EXEC_PROC) ||
        (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_EXEC_FUNC) )
    {
        sResultSetCount = qsv::getResultSetCount( &aStatement->statement );

        IDE_TEST_RAISE( sResultSetCount > 0,
                        ERR_NO_SUPPORT_SYNTAX );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SUPPORT_SYNTAX );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qci::hasFixedTableView( qciStatement *aStatement,
                               idBool       *aHas )
{
    IDE_TEST( qcg::detectFTnPV( &aStatement->statement ) != IDE_SUCCESS );

    *aHas = qcg::isFTnPV( &aStatement->statement );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::executeDCL( qciStatement * aStatement,
                        smiStatement * aSmiStmt,
                        smiTrans     * aSmiTrans )
{
    void          * sMmSession;
    qciStmtType     sType;
    qcStatement   * sStatement;
    smiStatement  * sSmiStmtOrg;
    qcParseTree   * sParseTree;

    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    UInt             i = 0;
    
    IDE_FT_ROOT_BEGIN();

    IDE_NOFT_BEGIN();

    IDU_FIT_POINT( "qci::executeDCL::__NOFT__" );

    sStatement = &aStatement->statement;
    sMmSession = sStatement->session->mMmSession;
    sParseTree = sStatement->myPlan->parseTree;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    IDE_TEST_RAISE( aStatement->state != QCI_STMT_STATE_PARAM_DATA_BOUND,
                    invalid_statement_state );

    IDE_TEST( getStmtType( aStatement, &sType ) != IDE_SUCCESS );

    // PROJ-2727
    IDE_TEST( sdi::checkDCLStmt( sStatement ) != IDE_SUCCESS );
                        
    switch( sType )
    {
        case QCI_STMT_COMMIT:
            {
                /* BUG-45678 */
                IDE_TEST( qci::mSessionCallback.mCommit(
                              sMmSession,
                              (sStatement->spxEnv->mCallDepth > 0) ? ID_TRUE:ID_FALSE)
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ROLLBACK:
        case QCI_STMT_ROLLBACK_TO_SAVEPOINT:  /* BUG-48216 */
            {
                qdTransParseTree * sTree = (qdTransParseTree *) sParseTree;
                IDE_TEST_RAISE(sTree->savepointName.size > QC_MAX_OBJECT_NAME_LEN,
                               ERR_TOO_LONG_NAME);

                SChar * sSavePoint;
                SChar   sName[QC_MAX_OBJECT_NAME_LEN + 1];

                if( sTree->savepointName.offset == QC_POS_EMPTY_OFFSET )
                {
                    sSavePoint = NULL;
                }
                else
                {
                    QC_STR_COPY( sName, sTree->savepointName );

                    sSavePoint = sName;
                }

                /* BUG-45678 */
                IDE_TEST( qci::mSessionCallback.mRollback(
                              sMmSession,
                              sSavePoint,
                              (sStatement->spxEnv->mCallDepth > 0) ? ID_TRUE:ID_FALSE)
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_SAVEPOINT:
            {
                SChar sName[QC_MAX_OBJECT_NAME_LEN + 1];
                qdTransParseTree * sTree = (qdTransParseTree *) sParseTree;
                IDE_TEST_RAISE(sTree->savepointName.size > QC_MAX_OBJECT_NAME_LEN,
                               ERR_TOO_LONG_NAME);

                QC_STR_COPY( sName, sTree->savepointName );

                IDE_TEST( qci::mSessionCallback.mSavepoint( sMmSession,
                                                            sName,
                                                            ID_FALSE )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_SET_REPLICATION_MODE: //PROJ-1541
            {
                qriSessionParseTree * sTree = (qriSessionParseTree *) sParseTree;
                IDE_TEST( qci::mSessionCallback.mSetReplicationMode(
                              sMmSession,
                              sTree->replModeFlag) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_SET_TX:
            {
                qdTransParseTree * sTree = (qdTransParseTree *) sParseTree;
                IDE_TEST( qci::mSessionCallback.mSetTX(
                              sMmSession,
                              sTree->maskType,
                              sTree->maskValue,
                              sTree->isSession ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_SET_STACK:
            {
                qdStackParseTree * sTree = (qdStackParseTree *) sParseTree;
                IDE_TEST( qci::mSessionCallback.mSetStackSize(
                              sMmSession,
                              sTree->stackSize ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_SET:
            {
                qdSetParseTree * sTree = (qdSetParseTree *) sParseTree;
                SChar sName[QC_MAX_OBJECT_NAME_LEN + 1];
                SChar sValue[QC_MAX_OBJECT_NAME_LEN + 1];

                // fix BUG-42682
                IDE_TEST_RAISE( sTree->variableName.size > QC_MAX_OBJECT_NAME_LEN,
                                ERR_TOO_LONG_NAME );

                IDE_TEST_RAISE( sTree->charValue.size > QC_MAX_OBJECT_NAME_LEN,
                                ERR_TOO_LONG_NAME );

                idlOS::strncpy(sName,
                               sTree->variableName.stmtText + sTree->variableName.offset,
                               sTree->variableName.size);
                sName[sTree->variableName.size] = '\0';
                idlOS::strncpy(sValue,
                               sTree->charValue.stmtText + sTree->charValue.offset,
                               sTree->charValue.size);
                sValue[sTree->charValue.size] = '\0';
                IDE_TEST( qci::mSessionCallback.mSet(
                              sMmSession,
                              sName,
                              sValue ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_MEMORY_COMPACT:
            {
                qci::mSessionCallback.mMemoryCompact();
                break;
            }
        case QCI_STMT_ALT_SYS_REORGANIZE:
            {
                break;
            }
        case QCI_STMT_SET_SYSTEM_PROPERTY:
        {
            qdSystemSetParseTree * sSystemParseTree =
                (qdSystemSetParseTree *)(sStatement->myPlan->parseTree);
            qcuPropertyArgument sArgSet;
            sdiLocalMetaInfo    sLocalMetaInfo;
            SChar               sPropName[IDP_MAX_VALUE_LEN + 1];
            UInt                sPropertyAttribute = 0;

            // system property 전파 여부 설정
            idlOS::memcpy( sPropName,
                           sSystemParseTree->name.stmtText +
                           sSystemParseTree->name.offset,
                           sSystemParseTree->name.size );
            sPropName[sSystemParseTree->name.size] = '\0';
                
            IDE_TEST( idp::getPropertyAttribute( sPropName, &sPropertyAttribute )
                      != IDE_SUCCESS );

            QCG_SET_SESSION_PROPERTY_ATTRIBUTE( sStatement, sPropertyAttribute );

            // PROJ-2727 
            if (( QCG_GET_SESSION_PROPERTY_ATTRIBUTE( sStatement ) == IDP_ATTR_SHARD_ALL ) &&
                ( sStatement->myPlan->parseTree->stmtShard != QC_STMT_SHARD_META ))
            {
                if (( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( sStatement ) != ID_TRUE  ) &&
                    ( sdi::isShardCoordinator( sStatement ) == ID_TRUE ))
                {
                    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
                            
                    // shard linker 검사 & 초기화
                    IDE_TEST( sdi::checkShardLinker( sStatement ) != IDE_SUCCESS );

                    sClientInfo = sStatement->session->mQPSpecific.mClientInfo;

                    // current node set system property
                    IDE_TEST( sdi::systemPropertyForShard( sStatement,
                                                           sLocalMetaInfo.mNodeName,
                                                           (SChar*)sStatement->myPlan->stmtText )
                              != IDE_SUCCESS );
                            
                    // remote node set system property
                    if ( sClientInfo != NULL )
                    {
                        sConnectInfo = sClientInfo->mConnectInfo;
            
                        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
                        {
                            if ( idlOS::strMatch( sLocalMetaInfo.mNodeName,
                                                  idlOS::strlen( sLocalMetaInfo.mNodeName ),
                                                  sConnectInfo->mNodeName,
                                                  idlOS::strlen( sConnectInfo->mNodeName )) != 0 )
                            {
                                IDE_TEST( sdi::systemPropertyForShard( sStatement,
                                                                       sConnectInfo->mNodeName,
                                                                       (SChar*)sStatement->myPlan->stmtText )
                                          != IDE_SUCCESS );
                            }
                            else
                            {
                                //nothing to do
                            }
                        }
                    }
                }
                else
                {
                    // 전노드에 전파되는 프로퍼티
                    // new session, trans 생성하여 수행하는 경우
                    // non sharding 경우 
                    IDE_ASSERT( (void *)&sArgSet == (void *)&(sArgSet.mArg));

                    sArgSet.mArg.getArgValue = (void*(*)(idvSQL *,idpArgument*,idpArgumentID))
                        (qcuProperty::callbackForGettingArgument);
                    sArgSet.mUserID = QCG_GET_SESSION_USER_ID(sStatement);
                    sArgSet.mTrans  = aSmiTrans;

                    IDE_TEST( qdc::setSystemProperty(
                                  sStatement,
                                  (idpArgument *)&sArgSet ) != IDE_SUCCESS );
                }

            }
            else
            {
                /* ------------------------------------------------
                 *  setup Argument Sets
                 * ----------------------------------------------*/

                // 전노드 전파 하지 않는 프로퍼티
                // new session ,transaction으로 수행 하지 않음
                // noh sharding 경우
                IDE_ASSERT( (void *)&sArgSet == (void *)&(sArgSet.mArg));

                sArgSet.mArg.getArgValue = (void*(*)(idvSQL *,idpArgument*,idpArgumentID))
                    (qcuProperty::callbackForGettingArgument);
                sArgSet.mUserID = QCG_GET_SESSION_USER_ID(sStatement);
                sArgSet.mTrans  = aSmiTrans;

                IDE_TEST( qdc::setSystemProperty(
                              sStatement,
                              (idpArgument *)&sArgSet ) != IDE_SUCCESS );
            }
            
            break;
        }
        case QCI_STMT_SET_SESSION_PROPERTY:
            {
                qdSystemSetParseTree * sSessionParseTree =
                    (qdSystemSetParseTree *)(sStatement->myPlan->parseTree);
                SChar  sPropName[IDP_MAX_VALUE_LEN + 1];
                SChar  sPropValueStr[IDP_MAX_VALUE_LEN + 1];
                UInt   sPropNameSize;
                UInt   sPropValueSize;

                // fix BUG-42682
                IDE_TEST_RAISE( sSessionParseTree->name.size > IDP_MAX_VALUE_LEN,
                                ERR_TOO_LONG_NAME );

                IDE_TEST_RAISE( sSessionParseTree->value.size > IDP_MAX_VALUE_LEN,
                                ERR_TOO_LONG_NAME );

                // BUG-19498
                // property name 뒤에 있는 필요없는 값이 쓰이지 않도록
                // 해줌
                idlOS::memcpy( sPropName,
                               sSessionParseTree->name.stmtText +
                               sSessionParseTree->name.offset,
                               sSessionParseTree->name.size );
                sPropName[sSessionParseTree->name.size] = '\0';
                sPropNameSize = sSessionParseTree->name.size;

                // BUG-19498
                // property value 뒤에 있는 필요없는 값이 쓰이지 않도록
                // 해줌
                idlOS::memcpy( sPropValueStr,
                               sSessionParseTree->value.stmtText +
                               sSessionParseTree->value.offset,
                               sSessionParseTree->value.size );
                sPropValueStr[sSessionParseTree->value.size] = '\0';
                sPropValueSize = sSessionParseTree->value.size;

                // PROJ-2727
                if ( sStatement->myPlan->parseTree->stmtShard == QC_STMT_SHARD_META )
                {
                    sStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_ATTR_SHARD_META_MASK;
                    sStatement->session->mQPSpecific.mFlag |= QC_SESSION_ATTR_SHARD_META_TRUE;
                }
                else
                {
                    // nothing to do
                }                

                IDE_TEST( qci::mSessionCallback.mSetProperty(
                              sMmSession,
                              sPropName,
                              sPropNameSize,
                              sPropValueStr,
                              sPropValueSize ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_CHECK:
            {
                /* implemented in mmtCmsExecute::doDirectExecute() */
                break;
            }
        case QCI_STMT_ALT_TABLESPACE_CHKPT_PATH :
            {
                IDE_TEST( qdtAlter::executeAlterMemoryTBSChkptPath( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_TABLESPACE_DISCARD :
            {
                IDE_TEST( qdtAlter::executeAlterTBSDiscard( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_DATAFILE_ONOFF :
            {
                IDE_TEST( qdtAlter::executeModifyFileOnOffLine( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_RENAME_DATAFILE :
            {
                IDE_TEST( qdtAlter::executeRenameFile( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_TABLESPACE_BACKUP :
            {
                IDE_TEST( qdtAlter::executeTBSBackup( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_CHKPT :
            {
                IDE_TEST( qdc::checkpoint( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_SHRINK_MEMPOOL :
            {
                IDE_TEST( qdc::shrinkMemPool( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_DUMP_CALLSTACKS:
            {
                IDE_TEST( qdc::dumpAllCallstacks( sStatement )
                          != IDE_SUCCESS );
                break;
            }

        case QCI_STMT_ALT_SYS_VERIFY :
            {
                IDE_TEST( qdc::verify( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_ARCHIVELOG :
            {
                IDE_TEST( qdc::archivelog( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_SWITCH_LOGFILE :
            {
                IDE_TEST( qdc::switchLogFile( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_FLUSH_BUFFER_POOL :
            {
                IDE_TEST( qdc::flushBufferPool( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_FLUSHER_ONOFF :
            {
                qdSystemParseTree *sTree = (qdSystemParseTree *) sParseTree;
                switch( sTree->startOption )
                {
                    case QDP_OPTION_START:
                        IDE_TEST( qdc::flusherOnOff( sStatement,
                                                     sTree->flusherID,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                        break;

                    case QDP_OPTION_STOP:
                        IDE_TEST( qdc::flusherOnOff( sStatement,
                                                     sTree->flusherID,
                                                     ID_FALSE )
                                  != IDE_SUCCESS );
                        break;

                    default:
                        break;
                }
                break;
            }
        case QCI_STMT_ALT_SYS_COMPACT_PLAN_CACHE:
            {
                IDE_TEST( qdc::compactPlanCache( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_RESET_PLAN_CACHE:
            {
                IDE_TEST( qdc::resetPlanCache( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_REBUILD_MIN_VIEWSCN :
            {
                IDE_TEST( qdc::rebuildMinViewSCN(sStatement)
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_SYS_SECURITY:
            {
                qdSystemParseTree *sTree = (qdSystemParseTree *) sParseTree;
                switch( sTree->startOption )
                {
                    case QDP_OPTION_START:
                        IDE_TEST( qdc::startSecurity( sStatement )
                                  != IDE_SUCCESS );
                        break;
                        
                    case QDP_OPTION_STOP:
                        IDE_TEST( qdc::stopSecurity( sStatement )
                                  != IDE_SUCCESS );
                        break;

                    default:
                        break;
                }
                break;
            }
        case QCI_STMT_ALT_SYS_AUDIT:
            {
                qdSystemParseTree *sTree = (qdSystemParseTree *) sParseTree;
                switch( sTree->startOption )
                {
                    case QDP_OPTION_START:
                        IDE_TEST( qdc::startAudit( sStatement )
                                  != IDE_SUCCESS );
                        break;

                    case QDP_OPTION_STOP:
                        IDE_TEST( qdc::stopAudit( sStatement )
                                  != IDE_SUCCESS );
                        break;

                    case QDP_OPTION_RELOAD:
                        IDE_TEST( qdc::reloadAudit( sStatement )
                                  != IDE_SUCCESS );
                        break;

                    default:
                        break;
                }
                break;
            }
        case QCI_STMT_AUDIT_OPTION:
            {
                IDE_TEST( qdc::auditOption( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_NOAUDIT_OPTION:
            {
                IDE_TEST( qdc::noAuditOption( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_DELAUDIT_OPTION:   /* BUG-39074 */
            {
                IDE_TEST( qdc::delAuditOption( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_STOP : /* BUG-42852 STOP과 FLUSH를 DCL로 변환합니다. */
            {
                IDE_TEST( qrc::executeStop( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_FLUSH : /* BUG-42852 STOP과 FLUSH를 DCL로 변환합니다. */
            {
                IDE_TEST( qrc::executeFlush( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_START:
            {
                IDE_TEST( qrc::executeStart( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_QUICKSTART:
            {
                IDE_TEST( qrc::executeQuickStart( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_SYNC:
            {
                IDE_TEST( qrc::executeSync( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_SYNC_CONDITION:
            {
                IDE_TEST( qrc::executeSyncCondition( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_TEMP_SYNC:
            {
                IDE_TEST( qrc::executeTempSync( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_FAILBACK:
            {
                IDE_TEST( qrc::executeFailback( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_DELETE_ITEM_REPLACE_HISTORY:
            {
                IDE_TEST( qrc::executeDeleteItemReplaceHistory( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ALT_REPLICATION_FAILOVER:
            {
                IDE_TEST( qrc::executeFailover( sStatement ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_COMMIT_FORCE:
            {
                qdSystemSetParseTree * sSessionParseTree =
                    (qdSystemSetParseTree *)(sStatement->myPlan->parseTree);
                SChar  sPropValueStr[IDP_MAX_VALUE_LEN + 1];
                UInt   sPropValueSize;

                // fix BUG-42682
                IDE_TEST_RAISE( sSessionParseTree->value.size > IDP_MAX_VALUE_LEN,
                                ERR_TOO_LONG_NAME );

                idlOS::memcpy( sPropValueStr,
                               sSessionParseTree->value.stmtText +
                               sSessionParseTree->value.offset,
                               sSessionParseTree->value.size );
                sPropValueStr[sSessionParseTree->value.size] = '\0';
                sPropValueSize = sSessionParseTree->value.size;

                IDE_TEST( qci::mSessionCallback.mCommitForce(
                              sMmSession,
                              sPropValueStr,
                              sPropValueSize ) != IDE_SUCCESS );
                break;
            }
        case QCI_STMT_ROLLBACK_FORCE:
            {
                qdSystemSetParseTree * sSessionParseTree =
                    (qdSystemSetParseTree *)(sStatement->myPlan->parseTree);
                SChar  sPropValueStr[IDP_MAX_VALUE_LEN + 1];
                UInt   sPropValueSize;

                // fix BUG-42682
                IDE_TEST_RAISE( sSessionParseTree->value.size > IDP_MAX_VALUE_LEN,
                                ERR_TOO_LONG_NAME );

                idlOS::memcpy( sPropValueStr,
                               sSessionParseTree->value.stmtText +
                               sSessionParseTree->value.offset,
                               sSessionParseTree->value.size );
                sPropValueStr[sSessionParseTree->value.size] = '\0';
                sPropValueSize = sSessionParseTree->value.size;

                IDE_TEST( qci::mSessionCallback.mRollbackForce(
                              sMmSession,
                              sPropValueStr,
                              sPropValueSize ) != IDE_SUCCESS );
                break;
            }
            
        /*
         * PROJ-1832 New database link
         */
            
        case QCI_STMT_CONTROL_DATABASE_LINKER:
            {
                IDE_TEST( qdkControlDatabaseLinker(
                              sStatement,
                              (qdkDatabaseLinkAlterParseTree *)sParseTree )
                          != IDE_SUCCESS );
                break;
            }

        case QCI_STMT_CLOSE_DATABASE_LINK:
            {
                IDE_TEST( qdkCloseDatabaseLink(
                              sStatement,
                              QCG_GET_DATABASE_LINK_SESSION( sStatement ),
                              (qdkDatabaseLinkCloseParseTree *)sParseTree )
                          != IDE_SUCCESS );
                break;
            }

        case QCI_STMT_COMMIT_FORCE_DATABASE_LINK:
            {
                IDE_TEST( qci::mSessionCallback.mCommitForceDatabaseLink(
                              sMmSession,
                              ID_FALSE )
                          != IDE_SUCCESS );
                break;
            }
            
        case QCI_STMT_ROLLBACK_FORCE_DATABASE_LINK:
            {
                IDE_TEST( qci::mSessionCallback.mRollbackForceDatabaseLink(
                              sMmSession,
                              ID_FALSE )
                          != IDE_SUCCESS );
                break;
            }   
        /* BUG-42639 Monitoring query */
        case QCI_STMT_SELECT_FOR_FIXED_TABLE:
            {
                aSmiStmt->mTrans = NULL;
                IDE_TEST( qci::execute( aStatement, aSmiStmt )
                          != IDE_SUCCESS );
                break;
            }
        /* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
        case QCI_STMT_RELOAD_ACCESS_LIST:
            {
                IDE_TEST( qdc::reloadAccessList( sStatement )
                          != IDE_SUCCESS );
                break;
            }
        // PROJ-2638
        case QCI_STMT_RELOAD_SHARD_META_NUMBER:
            {
                IDE_TEST( qci::mSessionCallback.mReloadShardMetaNumber(
                              sMmSession,
                              ID_FALSE ) // LOCAL keyword does not exist
                          != IDE_SUCCESS );
                break;
            }
        // PROJ-2701
        case QCI_STMT_RELOAD_SHARD_META_NUMBER_LOCAL:
            {
                IDE_TEST( qci::mSessionCallback.mReloadShardMetaNumber(
                              sMmSession,
                              ID_TRUE ) // LOCAL keyword exists
                          != IDE_SUCCESS );
                break;
            }
        default :
            IDE_DASSERT( 0 );
            break;
    }

    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    IDE_NOFT_END();

    IDE_FT_ROOT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_statement_state );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(ERR_TOO_LONG_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_TOO_LONG_IDENTIFIER_NAME));
    }
    IDE_EXCEPTION_END;

    IDE_NOFT_EXCEPTION_BEGIN();

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    IDE_NOFT_EXCEPTION_END();

    IDE_FT_ROOT_END();

    return IDE_FAILURE;
}

IDE_RC qci::getUserInfo( qciStatement  *aStatement,
                         smiStatement  *aSmiStmt,
                         qciUserInfo   *aResult )
{
/***********************************************************************
 *
 * Description : userInfo 정보 구성
 *
 * Implementation :
 *
 *     connect protocol로 처리되는 곳에서 호출됨.
 *     ( isql상에서의 connect문 처리시. )
 *     mm에서 loginID, loginPassword, mIsSysdba를
 *     qciUserInfo에 세팅해서 넘김.
 *
 ***********************************************************************/

    qcStatement     * sStatement;
    smiStatement    * sSmiStmtOrg;
    qdUserParseTree * sParseTree;
    qcNamePosition    sUserName;
    SChar             sOwnerDN[2048+1];
    SChar             sUserNameStr[40+1];

    sStatement = &(aStatement->statement);

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    /*
     * 보안DB일 경우, 즉, OwnerDN이 NULL이 아닐경우,
     * 반드시, SYS_DN_USERS_ 테이블을 통해 권한을 확인한다.
     */
    sOwnerDN[0] = '\0';

    IDE_TEST( qcmDatabase::checkDatabase( aSmiStmt,
                                          sOwnerDN,
                                          ID_SIZEOF(sOwnerDN) )
              != IDE_SUCCESS );

    if( sOwnerDN[0] != '\0' )
    {
        if( aResult->mUsrDN == NULL )
        {
            idBool sIsPermitted = ID_FALSE;
            IDE_TEST( qcmDNUser::isPermittedUsername( sStatement, aSmiStmt, aResult->loginID, &sIsPermitted )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE ( sIsPermitted != ID_TRUE,
                             QCI_ERR_NotPermittedUser );
        }
        else
        {
            /*
             * 보안 DB에,
             * 인증서를 제출하여 접속할 경우,
             * mUsrDN이 OwnerDN과 동일하면 "SYS"로,
             * 그 외의 경우는 SYS_DN_USERS_ 테이블에 등록된
             * database user로 지정된다.
             */
            if( idlOS::memcmp(aResult->mUsrDN,
                              sOwnerDN,
                              idlOS::strlen(sOwnerDN)) == 0 )
            {
                idlOS::snprintf(aResult->loginID,
                                ID_SIZEOF(aResult->loginID),
                                "%s",
                                "SYS");
            }
            else
            {
                idlOS::snprintf(aResult->loginID,
                                ID_SIZEOF(aResult->loginID),
                                "%s",
                                "SYS");

                IDE_TEST( qcmDNUser::getUsername( sStatement,
                                                  aSmiStmt,
                                                  aResult->mUsrDN,
                                                  sUserNameStr,
                                                  ID_SIZEOF(sUserNameStr) )
                      != IDE_SUCCESS );
            }
        }
    }

    if( aStatement->state == QCI_STMT_STATE_PARAM_DATA_BOUND )
    {
        sParseTree = (qdUserParseTree *)sStatement->myPlan->parseTree;
        IDE_ASSERT( sParseTree != NULL );   // BUG-27037 QP Code Sonar

        idlOS::memset( aResult, 0x00, ID_SIZEOF( qciUserInfo ) );

        QC_STR_COPY( aResult->loginID, sParseTree->userName );

        QC_STR_COPY( aResult->loginPassword, sParseTree->password );

        if( sParseTree->isSysdba == ID_TRUE )
        {
            aResult->mIsSysdba = ID_TRUE;
        }
        else
        {
            aResult->mIsSysdba = ID_FALSE;
        }

        sUserName = sParseTree->userName;
    }
    else
    {
        sUserName.stmtText = aResult->loginID;
        sUserName.offset   = 0;
        sUserName.size     = idlOS::strlen( aResult->loginID );

        // PROJ-2163
        sStatement->myPlan->stmtText = aResult->loginID;
        sStatement->myPlan->stmtTextLen = sUserName.size;
    }

    IDE_TEST( qcmUser::getUserID( sStatement,
                                  sUserName,
                                  &aResult->userID )
              != IDE_SUCCESS );

    aResult->loginUserID = aResult->userID;   /* BUG-41561 */

    /* PROJ-1812 ROLE */
    IDE_TEST( qcmUser::getUserPasswordAndDefaultTBS( sStatement,
                                                     aResult->userID,
                                                     aResult->userPassword,
                                                     &aResult->tablespaceID,
                                                     &aResult->tempTablespaceID,
                                                     &aResult->mDisableTCP )
              != IDE_SUCCESS );

    IDE_TEST( qdpRole::getRoleListByUserID( sStatement,
                                            aResult->userID,
                                            aResult->mRoleList )
              != IDE_SUCCESS );
    
    // check grant
    IDE_TEST( qdpRole::checkDDLCreateSessionPriv( sStatement, aResult->userID )
              != IDE_SUCCESS );

    /* PROJ-2207 Password policy support */
    IDE_TEST( qcmUser::getCurrentDate ( sStatement,
                                        aResult )
            != IDE_SUCCESS );
    
    IDE_TEST( qcmUser::getPasswPolicyInfo( sStatement,
                                           aResult->userID,
                                           aResult )
              != IDE_SUCCESS );
    
    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION( QCI_ERR_NotPermittedUser );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_NotPermittedUser));
    }
    IDE_EXCEPTION_END;

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}


IDE_RC qci::setDatabaseCallback(
    qciDatabaseCallback        aCreatedbFuncPtr,
    qciDatabaseCallback        aDropdbFuncPtr,
    qciDatabaseCallback        aCloseSession,
    qciDatabaseCallback        aCommitDTX,
    qciDatabaseCallback        aRollbackDTX,
    qciDatabaseCallback        aStartupFuncPtr,
    qciDatabaseCallback        aShutdownFuncPtr )
{
    qcg::setDatabaseCallback( aCreatedbFuncPtr,
                              aDropdbFuncPtr,
                              aCloseSession,
                              aCommitDTX,
                              aRollbackDTX,
                              aStartupFuncPtr,
                              aShutdownFuncPtr );

    return IDE_SUCCESS;
}

IDE_RC qci::setSessionCallback( qciSessionCallback *aCallback )
{
    mSessionCallback = *aCallback;

    return IDE_SUCCESS;
}

IDE_RC qci::setQueueCallback( qciQueueCallback   aQueueCreateFuncPtr,
                              qciQueueCallback   aQueueDropFuncPtr,
                              qciQueueCallback   aQueueEnqueueFuncPtr,
                              qciDeQueueCallback aQueueDequeueFuncPtr,
                              qciQueueCallback   aSetQueueStampFuncPtr)
{
    qcg::setQueueCallback( aQueueCreateFuncPtr,
                           aQueueDropFuncPtr,
                           aQueueEnqueueFuncPtr,
                           aQueueDequeueFuncPtr,
                            aSetQueueStampFuncPtr);

    return IDE_SUCCESS;
}

IDE_RC qci::setParamData4RebuildCallback( qciSetParamData4RebuildCallback    aCallback )
{
    mSetParamData4RebuildCallback = aCallback;

    return IDE_SUCCESS;
}

IDE_RC qci::setReplicationCallback( qciValidateReplicationCallback  aValidateCallback,
                                    qciExecuteReplicationCallback   aExecuteCallback,
                                    qciCatalogReplicationCallback   aCatalogCallback,
                                    qciManageReplicationCallback    aManageCallaback )
{
    mValidateReplicationCallback = aValidateCallback;
    mExecuteReplicationCallback = aExecuteCallback;
    mCatalogReplicationCallback = aCatalogCallback;
    mManageReplicationCallback = aManageCallaback;

    return IDE_SUCCESS;
}

IDE_RC qci::setOutBindLobCallback(
    qciOutBindLobCallback      aOutBindLobCallback,
    qciCloseOutBindLobCallback aCloseOutBindLobCallback )
{
    mOutBindLobCallback      = aOutBindLobCallback;
    mCloseOutBindLobCallback = aCloseOutBindLobCallback;

    return IDE_SUCCESS;
}

IDE_RC qci::setInternalSQLCallback( qciInternalSQLCallback * aCallback )
{
    mInternalSQLCallback = *aCallback;

    return IDE_SUCCESS;
}

IDE_RC qci::setAuditManagerCallback( qciAuditManagerCallback *aCallback )
{
    mAuditManagerCallback = *aCallback;

    return IDE_SUCCESS;
}

/* PROJ-2626 Snapshot Export */
IDE_RC qci::setSnapshotCallback( qciSnapshotCallback * aCallback )
{
    mSnapshotCallback = *aCallback;

    return IDE_SUCCESS;
}

IDE_RC qci::startup( idvSQL          * aStatistics,
                     qciStartupPhase   aStartupPhase )
{
    switch (aStartupPhase)
    {
        case QCI_STARTUP_PRE_PROCESS:
            // fixed table 등록
            IDE_TEST(qcg::startupPreProcess( aStatistics ) != IDE_SUCCESS);
            break;
        case QCI_STARTUP_PROCESS:
            // fixed table 등록
            IDE_TEST(qcg::startupProcess() != IDE_SUCCESS);
            break;
        case QCI_STARTUP_CONTROL:
            // query 수행 권한 설정, DDL,DML 금지
            IDE_TEST(qcg::startupControl() != IDE_SUCCESS);
            break;
        case QCI_STARTUP_META:
            // open meta
            IDE_TEST(qcg::startupMeta() != IDE_SUCCESS);
            break;
        case QCI_STARTUP_SERVICE:
            // cache meta
            // PSM, NSP 초기화
            IDE_TEST(qcg::startupService( aStatistics ) != IDE_SUCCESS);
            if ( SDU_SHARD_ENABLE == 1 )
            {
                // PROJ-2638
                sdi::initOdbcLibrary();
            }
            else
            {
                // Nothing to do.
            }
            break;
        case QCI_STARTUP_SHUTDOWN:
            if ( SDU_SHARD_ENABLE == 1 )
            {
                // PROJ-2638
                sdi::finiOdbcLibrary();
            }
            else
            {
                // Nothing to do.
            }
            // cache meta
            // PSM, NSP 초기화
            IDE_TEST(qcg::startupShutdown( aStatistics ) != IDE_SUCCESS);
            break;
        case QCI_META_DOWNGRADE:
            // PROJ-2689 Downgrade meta
            IDE_TEST(qcg::startupDowngrade( aStatistics ) != IDE_SUCCESS);
            break;
        default:
            IDE_CALLBACK_FATAL("Can't Happen");
    }
    mStartupPhase = aStartupPhase;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}


qciStartupPhase qci::getStartupPhase()
{
    return mStartupPhase;
}

idBool
qci::isSysdba( qciStatement * aStatement )
{
    return qcg::getSessionIsSysdbaUser( & aStatement->statement );
}

// BUG-43566 hint 구문이 오류가 났을때 위치를 알수 있으면 좋겠습니다.
SInt qci::getLineNo( SChar * aStmtText, SInt aOffset )
{
    SInt i = 0;
    SInt sCount = 1;

    while( i < aOffset )
    {
        if( aStmtText[i] == '\n' )
        {
            sCount++;
        }
        else
        {
            // Nothing to do.
        }
        i++;
    }

    return sCount;
}

IDE_RC
qci::makePlanTreeText( qciStatement * aStatement,
                       iduVarString * aString,
                       idBool         aIsCodeOnly,
                       idBool         aIsNoGraph )
{
    //------------------------------------------------
    // aPlanBuffer : 버퍼포인터
    // aBufLen     : 버퍼포인터의 길이
    // mm에서 버퍼도 memset으로 초기화하고,
    // 이 함수 수행 후,
    // strlen를 구해서 길이가 0 인지아닌지 체크하고 있으므로,
    // qp에서는 이에 대해 신경쓰지 않아도 된다.
    //------------------------------------------------

    qcStatement       * sStatement;
    qcTemplate        * sTemplate;
    qciStmtType         sStmtType;
    qmnDisplay          sDisplay;

    sStatement = & aStatement->statement;
    sTemplate = QC_PRIVATE_TMPLATE(sStatement);

    if( aString == NULL )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-15637
    if( ( aStatement->state == QCI_STMT_STATE_EXECUTED ) &&
        ( aIsCodeOnly == ID_FALSE ) )
    {
        sDisplay = QMN_DISPLAY_ALL;
    }
    else
    {
        sDisplay = QMN_DISPLAY_CODE;
    }

    IDE_TEST( getStmtType( aStatement, &sStmtType )
              != IDE_SUCCESS );

    switch( sStmtType )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
        case QCI_STMT_SELECT_FOR_FIXED_TABLE:
        case QCI_STMT_MERGE:
        case QCI_STMT_INSERT:
        case QCI_STMT_UPDATE:
        case QCI_STMT_DELETE:
        case QCI_STMT_MOVE:
        case QCI_STMT_DEQUEUE:
        case QCI_STMT_ENQUEUE:         
            IDE_TEST( printPlanTreeText( sStatement,
                                         sTemplate,
                                         sStatement->myPlan->graph,
                                         sStatement->myPlan->plan,
                                         sDisplay,
                                         aString,
                                         aIsNoGraph )
                      != IDE_SUCCESS );
            break;

        default:
            break;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::printPlanTreeText( qcStatement  * aStatement,
                        qcTemplate   * aTemplate,
                        qmgGraph     * aGraph,
                        qmnPlan      * aPlan,
                        qmnDisplay     aDisplay,
                        iduVarString * aString,
                        idBool         aIsNoGraph )
{
    SChar       * sHintText;
    SChar       * sTemp;
    qcOffset    * sOffset;
    SInt          sLine;

    /**
     * BUG-48224
     * v$plantext 에서 plan 출력시 graph 정보는 출력하면 않된다.
     * graph는 optimize과정을 거져야하지만 v$plantext  은 plan
     * 의 text만 출력해야하는 데 의도치 않게 graph가 같이 출력됨.
     */
    if ( ( QCU_TRCLOG_EXPLAIN_GRAPH == 1 ) &&
         ( aIsNoGraph == ID_FALSE ) )
    {
        IDE_TEST( aGraph->printGraph( aStatement,
                                      aGraph,
                                      0,
                                      aString )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // 시작 line 출력
    iduVarStringAppend( aString,
                        "------------------------------------------------------------\n" );

    IDE_TEST( aPlan->printPlan( aTemplate,
                                aPlan,
                                0,
                                aString,
                                aDisplay )
              != IDE_SUCCESS );

    // 종료 line 출력
    iduVarStringAppend( aString,
                        "------------------------------------------------------------\n" );

    // PROJ-2551 simple query 최적화
    if ( ( aStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
         == QC_STMT_FAST_EXEC_TRUE )
    {
        iduVarStringAppend( aString, "* SIMPLE QUERY PLAN\n" );

        if ( ( aStatement->mFlag & QC_STMT_FAST_BIND_MASK )
             == QC_STMT_FAST_BIND_TRUE )
        {
            iduVarStringAppend( aString, "* FAST EXECUTED\n" );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2492 Dynamic sample selection
    // PLAN 정보에 출력을 해준다.
    if ( (QCU_DISPLAY_PLAN_FOR_NATC == 0) &&
         (aStatement->myPlan->planEnv != NULL) )
    {
        if ( aStatement->myPlan->planEnv->planProperty.mOptimizerAutoStatsRef == ID_TRUE )
        {
            iduVarStringAppendFormat( aString,
                                    "* AUTO STATISTICS USED: %"ID_UINT32_FMT"\n",
                                    (aStatement->myPlan->planEnv->planProperty.mOptimizerAutoStats) );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( QCU_DISPLAY_PLAN_FOR_NATC == 1 )
    {
        // BUG-43524 hint 구문이 오류가 났을때 알수 있으면 좋겠습니다.
        if ( ( aStatement->myPlan->mPlanFlag & QC_PLAN_HINT_PARSE_MASK )
            == QC_PLAN_HINT_PARSE_FAIL )
        {
            // BUG-43566 hint 구문이 오류가 났을때 위치를 알수 있으면 좋겠습니다.
            iduVarStringAppend( aString, "* WARNING : Failed to parse hints.\n" );


            for ( sOffset  = aStatement->myPlan->mHintOffset;
                  sOffset != NULL;
                  sOffset  = sOffset->mNext )
            {
                sHintText = aStatement->myPlan->stmtText + sOffset->mOffset;
                sLine     = getLineNo( aStatement->myPlan->stmtText,
                                       sOffset->mOffset );

                iduVarStringAppend( aString, "at \"" );

                if ( sOffset->mLen != QC_POS_EMPTY_SIZE )
                {
                    iduVarStringAppendLength( aString,
                                              sHintText,
                                              sOffset->mLen );

                    iduVarStringAppendFormat( aString, "\", line %"ID_INT32_FMT"\n", sLine );
                }
                else
                {
                    // 구문오류가 있는 경우에는 모든 hint를 출력한다.
                    // ex : ( SELECT /*+ full */ * FROM T1 )
                    sTemp = idlOS::strstr( sHintText, "*/" );

                    if ( sTemp != NULL )
                    {
                        iduVarStringAppendLength( aString,
                                                  sHintText,
                                                  sTemp - sHintText );
                    }
                    else
                    {
                        // Nothing to do
                    }

                    iduVarStringAppendFormat( aString, "\", line %"ID_INT32_FMT"\n", sLine );
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    sdi::printGCTxPlan( aTemplate, aString );

    /* BUG-45899 */
    sdi::printAnalysisInfo( aStatement, aString );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::getPlanTreeText( qciStatement * aStatement,
                      iduVarString * aString,
                      idBool         aIsCodeOnly )
{
    UInt           sStage = 0;
    qcStatement  * sStatement = &(aStatement->statement);

    qcg::lock( sStatement );

    sStage = 1;

    if( qcg::getPlanTreeState( sStatement ) == ID_TRUE )
    {
        IDE_TEST( qci::makePlanTreeText( aStatement,
                                         aString,
                                         aIsCodeOnly,
                                         ID_FALSE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    sStage = 0;

    qcg::unlock( sStatement );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            qcg::unlock( sStatement );
    }
    
    return IDE_FAILURE;
}

idBool
qci::isLastParamData( qciStatement * aStatement,
                      UShort         aBindParamId )
{
    return ( aBindParamId ==
             aStatement->statement.bindParamDataInLastId )
             ? ID_TRUE : ID_FALSE;
}

/* PROJ-2160 CM 타입제거
   파라미터의 타입을 리턴한다. */
UInt qci::getBindParamType( qciStatement * aStatement,
                            UInt           aBindID )
{
    return aStatement->statement.pBindParam[aBindID].param.type;
}

UInt
qci::getBindParamInOutType( qciStatement * aStatement,
                            UInt           aBindID )
{
    return aStatement->statement.pBindParam[aBindID].param.inoutType;
}

IDE_RC
qci::closeCursor( qciStatement * aStatement,
                  smiStatement * aSmiStmt )
{
    qcStatement    * sStatement;
    smiStatement   * sSmiStmtOrg;
    UInt             sStage = 0;

    sStatement = &aStatement->statement;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        // Nothing to do.
    }
    else
    {
        sStage = 3;
        IDE_TEST( qcg::joinThread( sStatement ) != IDE_SUCCESS );

        sStage = 2;
        IDE_TEST( qcg::closeAllCursor( sStatement ) != IDE_SUCCESS );

        sStage = 1;
        IDE_TEST( qcg::finishAndReleaseThread( sStatement ) != IDE_SUCCESS );

        sStage = 0;
        //fix BUG-17534
        if( sStatement->spvEnv->latched == ID_TRUE )
        {
            IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
                      != IDE_SUCCESS );
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 3:
            (void)qcg::closeAllCursor(sStatement);
            /* fall through */
        case 2:
            (void)qcg::finishAndReleaseThread(sStatement);
            /* fall through */
        case 1:
            if (sStatement->spvEnv->latched == ID_TRUE)
            {
                (void)qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList );
                sStatement->spvEnv->latched = ID_FALSE;
            }
            else
            {
                /* nothing to do */
            }
            break;
        default:
            break;
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}

IDE_RC
qci::refineStackSize( qciStatement * aStatement )
{
    IDE_TEST_RAISE( aStatement->state != QCI_STMT_STATE_INITIALIZED,
                    invalid_statement_state );

    IDE_TEST ( qcg::refineStackSize(
                   &(aStatement->statement),
                   QCG_GET_SESSION_STACK_SIZE( &(aStatement->statement) ) )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_statement_state );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::checkRebuild( qciStatement * aStatement )
{
    //---------------------------------------------
    // execute하려는 statement가 이전에
    // rebuild 수행중 에러난 statement인지 체크.
    // 예) prepare : insert into t1 values ( ? )
    //     drop table t1;
    //     execute : insert into t1 values ( ? )
    //               <== rebuild시 에러
    //     execute : insert into t1 values ( ? )
    //               <== rebuild가 수행되어야 함.
    //---------------------------------------------

    if( ( aStatement->flag & QCI_STMT_REBUILD_EXEC_MASK )
        == QCI_STMT_REBUILD_EXEC_FAILURE )
    {
        IDE_RAISE( exec_rebuild );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( exec_rebuild );
    {
        IDE_SET(ideSetErrorCode(qpERR_REBUILD_QCI_EXEC));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

UShort
qci::getResultSetCount( qciStatement * aStatement )
{
    if( (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_EXEC_PROC) ||
        (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_EXEC_FUNC) )
    {
        // 전체 Result set의 개수를 얻어온다.
        if( aStatement->state == QCI_STMT_STATE_PREPARED )
        {
            return qsv::getResultSetCount( &aStatement->statement );
        }
        else
        {
            return qsx::getResultSetCount( &aStatement->statement );
        }
    }
    else
    {
        //  select, dequeue 이면 무조건 1이다.
        if( (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ||
            (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_FIXED_TABLE) ||
            (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE) ||
            (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE) )
        {
            return 1;
        }
    }

    return 0;
}

IDE_RC
qci::getResultSet( qciStatement * aStatement,
                   UShort         aResultSetID,
                   void        ** aResultSetStmt,
                   idBool       * aInterResultSet,
                   idBool       * aRecordExist )
{
    if( (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_EXEC_PROC) ||
        (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_EXEC_FUNC) )
    {
        IDE_TEST( qsx::getResultSetInfo( &aStatement->statement,
                                         aResultSetID,
                                         aResultSetStmt,
                                         aRecordExist )
                  != IDE_SUCCESS );

        *aInterResultSet = ID_TRUE;
    }
    else
    {
        if( (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ||
            (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_FIXED_TABLE) ||
            (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE) ||
            (aStatement->statement.myPlan->parseTree->stmtKind == QCI_STMT_DEQUEUE) )
        {
            *aResultSetStmt = aStatement->statement.stmtInfo->mMmStatement;
            *aInterResultSet = ID_FALSE;
            *aRecordExist = ID_FALSE;
        }
        else
        {
            // 여기로 들어오면 절대로 안됨.
            IDE_ASSERT(0);
            *aResultSetStmt = NULL;
            *aInterResultSet = ID_FALSE;
            *aRecordExist = ID_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2160 CM 타입제거
   row 의 실제 사이즈를 구한다. */
ULong qci::getRowActualSize( qciStatement * aStatement )
{
    qcStatement     * sStatement = NULL;
    qcSimpleResult  * sResult;
    qmncPROJ        * sPROJ;
    qmnValueInfo    * sValueInfo;
    ULong             sSize;
    UChar           * sValue;
    UInt              i;

    sStatement = &(aStatement->statement);

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        sResult = sStatement->simpleInfo.results;
        sPROJ   = (qmncPROJ*)sStatement->myPlan->plan;
        sValue  = (UChar*)sResult->result +
            sResult->idx * sPROJ->simpleResultSize;
        sSize   = 0;
        
        for ( i = 0; i < sPROJ->targetCount; i++ )
        {
            sValueInfo = &(sPROJ->simpleValues[i]);

            sSize += sValueInfo->column.module->actualSize(
                &(sValueInfo->column),
                sValue + sPROJ->simpleResultOffsets[i] );
        }
    }
    else
    {
        sSize = qmnPROJ::getActualSize(
            QC_PRIVATE_TMPLATE(&aStatement->statement),
            aStatement->statement.myPlan->plan);
    }

    return sSize;
}

// fix BUG-17715
IDE_RC
qci::getRowSize( qciStatement * aStatement,
                 UInt         * aSize )
{

    IDE_TEST( qmnPROJ::getRowSize( QC_PRIVATE_TMPLATE(&aStatement->statement),
                                   aStatement->statement.myPlan->plan,
                                   aSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::getBaseTableInfo( qciStatement * aStatement,
                       SChar        * aTableOwnerName,
                       SChar        * aTableName,
                       idBool       * aIsUpdatable )
{
    qcStatement    * sStatement;
    qciStmtState     sState;
    qciStmtType      sStmtType;
    qmsParseTree   * sParseTree;

    IDE_ASSERT( aTableOwnerName != NULL );
    IDE_ASSERT( aTableName      != NULL );
    IDE_ASSERT( aIsUpdatable    != NULL );

    sStatement = & aStatement->statement;

    IDE_TEST( getCurrentState( aStatement, & sState ) != IDE_SUCCESS );

    if( sState >= QCI_STMT_STATE_PREPARED )
    {
        IDE_TEST( getStmtType( aStatement, &sStmtType )
                  != IDE_SUCCESS );

        if( ( sStmtType == QCI_STMT_SELECT ) ||
            ( sStmtType == QCI_STMT_SELECT_FOR_UPDATE ) ||
            ( sStmtType == QCI_STMT_DEQUEUE ) )
        {
            sParseTree = (qmsParseTree *)(sStatement->myPlan->parseTree);

            idlOS::strncpy( aTableOwnerName,
                            sParseTree->baseTableInfo.tableOwnerName,
                            QC_MAX_OBJECT_NAME_LEN + 1 );
            aTableOwnerName[QC_MAX_OBJECT_NAME_LEN] = '\0';

            idlOS::strncpy( aTableName,
                            sParseTree->baseTableInfo.tableName,
                            QC_MAX_OBJECT_NAME_LEN + 1 );
            aTableName[QC_MAX_OBJECT_NAME_LEN] = '\0';

            *aIsUpdatable = sParseTree->baseTableInfo.isUpdatable;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::checkBindParamCount( qciStatement * aStatement,
                          UShort         aBindParamCount )
{
    IDE_TEST_RAISE( aStatement->statement.pBindParamCount !=
                    aBindParamCount,
                    err_bind_column_count_mismatch );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_bind_column_count_mismatch );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_BIND_COLUMN_COUNT_MISMATCH));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qci::checkBindColumnCount( qciStatement * aStatement,
                           UShort         aBindColumnCount )
{
    IDE_TEST_RAISE( aStatement->statement.myPlan->sBindColumnCount !=
                    aBindColumnCount,
                    err_bind_column_count_mismatch );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_bind_column_count_mismatch );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_BIND_COLUMN_COUNT_MISMATCH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#define SQL_NO_TOTAL                    (-4)

IDE_RC
qci::getOutBindParamSize( qciStatement *aStatement,
                          UInt         *aOutBindParamSize,
                          UInt         *aOutBindParamCount)
{
    qcStatement   *sStatement;
    UShort         sBindParamCount = 0;
    UInt           sOutBindParamSize = 0;
    UInt           sOutBindParamCount = 0;
    qciBindParam  *sBindParam;
    UInt           i;

    sStatement = &aStatement->statement;

    sBindParamCount = qcg::getBindCount( sStatement );

    for( i = 0; i < sBindParamCount; i++ )
    {
        sBindParam = &(sStatement->pBindParam[i].param);

        if( (sBindParam->inoutType == CMP_DB_PARAM_OUTPUT) ||
            (sBindParam->inoutType == CMP_DB_PARAM_INPUT_OUTPUT) )
        {
            // proj_2160 cm_type removal
            // lob param인 경우 SQL_NO_TOTAL(-4)가 들어온다
            // 그러면, cm lob locator size를 세팅한다.
            if( ( sBindParam->type == MTD_CLOB_LOCATOR_ID ) ||
                ( sBindParam->type == MTD_BLOB_LOCATOR_ID ) )
            {
                if( sBindParam->precision == SQL_NO_TOTAL )
                {
                    /* BUG-45962 */
                    sOutBindParamSize += ID_SIZEOF(mtdBlobLocatorType) + ID_SIZEOF(ULong) + ID_SIZEOF(UChar);
                }
                else
                {
                    /* BUG-45962 */
                    sOutBindParamSize += (sBindParam->precision) + ID_SIZEOF(ULong) + ID_SIZEOF(UChar);
                }
            }
            else
            {
                // BUG-34230
                // NCHAR, NVARCHAR 는 한글자당 3byte 까지 차지하기 때문에
                // precision 의 3배로 size 를 계산해야 한다.
                if( ( sBindParam->type == MTD_NCHAR_ID ) ||
                    ( sBindParam->type == MTD_NVARCHAR_ID ) )
                {
                    sOutBindParamSize += (sBindParam->precision * 3) + ID_SIZEOF(UInt);
                }
                else
                {
                    /* BUG-45962 */
                    sOutBindParamSize += (sBindParam->precision) + ID_SIZEOF(ULong);
                }
            }
            sOutBindParamCount++;
        }
    }

    *aOutBindParamSize  = sOutBindParamSize;
    *aOutBindParamCount = sOutBindParamCount;

    return IDE_SUCCESS;
}

//PROJ-1436 SQL Plan Cache
// aEnv is PCO's aEnv.
IDE_RC qci::isMatchedEnvironment( qciStatement    * aStatement,
                                  qciPlanProperty * aPlanProperty,
                                  qciPlanBindInfo * aPlanBindInfo, // PROJ-2163
                                  idBool          * aIsMatched )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *   Plan 이 검색 조건에 맞는지 검사한다.
 *
 * Implementation :
 *   다음 2가지 조건을 검사한 후 결과 반환
 *
 *   1. Plan property 들의 일치 여부
 *   2. Plan 에 쓰인 호스트 변수의 수, 타입, precision 등의 bind info
 *
 ***********************************************************************/

    qcStatement * sStatement = NULL;
    idBool        sIsMatched = ID_TRUE;

    sStatement = & aStatement->statement;

    // aStatement->statement->stmtEnv 와 PCO(Plan Cache Object)의
    // environment 의 plan property 가 match 되는지 검사한다.
    IDE_TEST( qcgPlan::isMatchedPlanProperty( sStatement,
                                              aPlanProperty,
                                              & sIsMatched )
              != IDE_SUCCESS );

    // aStatement->statement->pBindParam 과 PCO(Plan Cache Object)의 environment 의
    // 바인드 정보가 match 되는지 검사한다.
    // (단, pBindParam 이 NULL 이면 검사하지 않고 일치한다고 반환된다.)
    if( sIsMatched == ID_TRUE )
    {
        IDE_TEST( qcgPlan::isMatchedPlanBindInfo( sStatement,
                                                  aPlanBindInfo,
                                                  & sIsMatched )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    // 검사 결과를 반환
    *aIsMatched = sIsMatched;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::rebuildEnvironment( qciStatement    * aStatement,
                                qciPlanProperty * aEnv )
{
    qcStatement * sStatement = NULL;

    sStatement = & aStatement->statement;

    IDE_TEST( qcgPlan::rebuildPlanProperty( sStatement,
                                            aEnv )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//PROJ-1436 SQL Plan Cache
// soft-prepare과정에서 plan이 valid한지를 검사한다.
// aEnv        -- PCO에 이는 plan생성시참조하였던 environment
// aSharedPlan -- qcSharedPlan
IDE_RC qci::validatePlanOrg(
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext,
    qciStatement                       * aStatement,
    void                               * aSharedPlan,
    idBool                             * aIsValidPlan )
{
    qcStatement       * sStatement = & aStatement->statement;
    smiStatement      * sSmiStmtOrg;
    qcgPlanObject     * sObject;
    qcgEnvProcList    * sProc;
    qcgEnvPkgList     * sPkg;
    idBool              sIsValid;
    UInt                sStage = 0;
    // BUG-46085
    UInt                sStage2 = 0;
    iduVarMemListStatus sPosition;
    qsvEnvInfo          sOriEnvInfo;

    /* BUG-45893 */
    UInt             sUserID = QCG_GET_SESSION_USER_ID( sStatement );

    sObject     = & ((qcSharedPlan*) aSharedPlan)->planEnv->planObject;

    *aIsValidPlan = ID_TRUE;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );

    if( sObject->tableList != NULL )
    {
        IDE_TEST( qcgPlan::validatePlanTable( sObject->tableList,
                                              sUserID,            /* BUG-45893 */
                                              & sIsValid )
                  != IDE_SUCCESS );

        if( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sObject->sequenceList != NULL )
    {
        IDE_TEST( qcgPlan::validatePlanSequence( sObject->sequenceList,
                                                 & sIsValid )
                  != IDE_SUCCESS );

        if( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sObject->procList != NULL )
    {
        // PROJ-2446
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        IDE_TEST( qcgPlan::validatePlanProc( sObject->procList,
                                             & sIsValid )
                  != IDE_SUCCESS );

        if( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }

        // plan이 참조하는 proc에 latch가 획득된 상태이다.
        sStage = 1;
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1073 Package
    if( sObject->pkgList != NULL )
    {
        // PROJ-2446
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        IDE_TEST( qcgPlan::validatePlanPkg( sObject->pkgList,
                                            & sIsValid )
                  != IDE_SUCCESS );

        if( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }

        // plan이 참조하는 pkg에 latch가 획득된 상태이다.
        sStage = 2;
    }
    else
    {
        // Nothing to do.
    }

    if( sObject->synonymList != NULL )
    {
        // BUG-46085
        IDE_TEST( sStatement->myPlan->qmpMem->getStatus( &sPosition ) != IDE_SUCCESS );
        sStage2 = 2;

        idlOS::memcpy( &sOriEnvInfo, sStatement->spvEnv, ID_SIZEOF(qsvEnvInfo) );
        sStage2 = 3;

        // synonym 객체의 검사는 meta table을 사용하므로
        // smiStmt가 필요하다.
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        IDE_TEST( qcgPlan::validatePlanSynonym( sStatement,
                                                sObject->synonymList,
                                                & sIsValid )
                  != IDE_SUCCESS );

        // BUG-46085
        sStage2 = 2;
        idlOS::memcpy( sStatement->spvEnv, &sOriEnvInfo, ID_SIZEOF(qsvEnvInfo) );
        sStage2 = 1;
        IDE_TEST( sStatement->myPlan->qmpMem->setStatus( &sPosition ) != IDE_SUCCESS );
        sStage2 = 0;
        IDE_TEST( sStatement->myPlan->qmpMem->free( sPosition.mCursor ) != IDE_SUCCESS );

        if( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT( INVALID_PLAN );

    // BUG-24206
    // INVALID_PLAN으로 점프한 경우 IDE_FAILURE로 리턴하는 것과
    // 동일하게 에러를 처리해야 한다.
    if( *aIsValidPlan == ID_FALSE )
    {
        switch ( sStage )
        {
            case 2:
                if( sObject->pkgList != NULL )
                {
                    // unlatch pkg
                    for( sPkg = sObject->pkgList;
                         sPkg != NULL;
                         sPkg = sPkg->next )
                    {
                        (void) qsxPkg::unlatch( sPkg->pkgID );
                    }
                }
                else
                {
                    // Nothing to do.
                }
            case 1:
                if( sObject->procList != NULL )
                {
                    // unlatch proc
                    for( sProc = sObject->procList;
                         sProc != NULL;
                         sProc = sProc->next )
                    {
                        (void) qsxProc::unlatch( sProc->procID );
                    }
                }
                else
                {
                    // Nothing to do.
                }
                break;
            default:
                // Nothing to do.
                break;
        }
    }
    else
    {
        // Nothing to do.
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            if( sObject->pkgList != NULL )
            {
                // unlatch pkg
                for( sPkg = sObject->pkgList;
                     sPkg != NULL;
                     sPkg = sPkg->next )
                {
                    (void) qsxPkg::unlatch( sPkg->pkgID );
                }
            }
            else
            {
                // Nothing to do.
            }
        case 1:
            if( sObject->procList != NULL )
            {
                // unlatch proc
                for( sProc = sObject->procList;
                     sProc != NULL;
                     sProc = sProc->next )
                {
                    (void) qsxProc::unlatch( sProc->procID );
                }
            }
            else
            {
                // Nothing to do.
            }
            break;
        default:
            // Nothing to do.
            break;
    }

    // BUG-46085
    switch( sStage2 )
    {
        case 3:
            idlOS::memcpy( sStatement->spvEnv, &sOriEnvInfo, ID_SIZEOF(qsvEnvInfo) );
        case 2:
            (void)sStatement->myPlan->qmpMem->setStatus( &sPosition );
        case 1:
            (void)sStatement->myPlan->qmpMem->free( sPosition.mCursor );
            break;
        default:
            break;
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );
    *aIsValidPlan = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qci::validatePlanMode( qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
                              void                               * aGetSmiStmt4PrepareContext,
                              qciStatement                       * aStatement,
                              void                               * aSharedPlan,
                              idBool                             * aIsValidPlan )
{
    qcStatement       * sStatement = & aStatement->statement;
    smiStatement      * sSmiStmtOrg;
    qcgPlanObject     * sObject;
    qcgEnvProcList    * sProc;
    qcgEnvPkgList     * sPkg;
    idBool              sIsValid;
    UInt                sStage = 0;
    ULong               sMemSize1 = 0;
    ULong               sMemSize2 = 0;

    /* BUG-45893 */
    UInt             sUserID = QCG_GET_SESSION_USER_ID( sStatement );

    sMemSize1 = sStatement->myPlan->qmpMem->getAllocSize();

    sObject = & ((qcSharedPlan*) aSharedPlan)->planEnv->planObject;

    *aIsValidPlan = ID_TRUE;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );

    if ( sObject->tableList != NULL )
    {
        IDE_TEST( qcgPlan::validatePlanTable( sObject->tableList,
                                              sUserID,            /* BUG-45893 */
                                              & sIsValid )
                  != IDE_SUCCESS );

        if ( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( sObject->sequenceList != NULL )
    {
        IDE_TEST( qcgPlan::validatePlanSequence( sObject->sequenceList,
                                                 & sIsValid )
                  != IDE_SUCCESS );

        if ( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( sObject->procList != NULL )
    {
        // PROJ-2446
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        IDE_TEST( qcgPlan::validatePlanProc( sObject->procList,
                                             & sIsValid )
                  != IDE_SUCCESS );

        if ( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }

        // plan이 참조하는 proc에 latch가 획득된 상태이다.
        sStage = 1;
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1073 Package
    if ( sObject->pkgList != NULL )
    {
        // PROJ-2446
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        IDE_TEST( qcgPlan::validatePlanPkg( sObject->pkgList,
                                            & sIsValid )
                  != IDE_SUCCESS );

        if ( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }

        // plan이 참조하는 pkg에 latch가 획득된 상태이다.
        sStage = 2;
    }
    else
    {
        // Nothing to do.
    }

    if ( sObject->synonymList != NULL )
    {
        // synonym 객체의 검사는 meta table을 사용하므로
        // smiStmt가 필요하다.
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        IDE_TEST( qcgPlan::validatePlanSynonym( sStatement,
                                                sObject->synonymList,
                                                & sIsValid )
                  != IDE_SUCCESS );

        if ( sIsValid == ID_FALSE )
        {
            *aIsValidPlan = ID_FALSE;

            IDE_CONT( INVALID_PLAN );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT( INVALID_PLAN );

    // BUG-24206
    // INVALID_PLAN으로 점프한 경우 IDE_FAILURE로 리턴하는 것과
    // 동일하게 에러를 처리해야 한다.
    if ( *aIsValidPlan == ID_FALSE )
    {
        switch ( sStage )
        {
            case 2:
                if ( sObject->pkgList != NULL )
                {
                    // unlatch pkg
                    for ( sPkg = sObject->pkgList;
                          sPkg != NULL;
                          sPkg = sPkg->next )
                    {
                        (void) qsxPkg::unlatch( sPkg->pkgID );
                    }
                }
                else
                {
                    // Nothing to do.
                }
            case 1:
                if ( sObject->procList != NULL )
                {
                    // unlatch proc
                    for ( sProc = sObject->procList;
                          sProc != NULL;
                          sProc = sProc->next )
                    {
                        (void) qsxProc::unlatch( sProc->procID );
                    }
                }
                else
                {
                    // Nothing to do.
                }
                break;
            default:
                // Nothing to do.
                break;
        }
    }
    else
    {
        /* BUG-48594 혹시 prepare Memory를 할당한 경우 rebuild를 시킨다.*/
        sMemSize2 = sStatement->myPlan->qmpMem->getAllocSize();
        if ( sMemSize1 != sMemSize2 )
        {
            *aIsValidPlan = ID_FALSE;
            ideLog::log( IDE_QP_0, "PLAN CACHE VALIDATE PREPARE MEMORY ERROR[%lu]- [%lu]", sMemSize1, sMemSize2 );
        }
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            if ( sObject->pkgList != NULL )
            {
                // unlatch pkg
                for ( sPkg = sObject->pkgList;
                      sPkg != NULL;
                      sPkg = sPkg->next )
                {
                    (void) qsxPkg::unlatch( sPkg->pkgID );
                }
            }
            else
            {
                // Nothing to do.
            }
        case 1:
            if ( sObject->procList != NULL )
            {
                // unlatch proc
                for ( sProc = sObject->procList;
                      sProc != NULL;
                      sProc = sProc->next )
                {
                    (void) qsxProc::unlatch( sProc->procID );
                }
            }
            else
            {
                // Nothing to do.
            }
            break;
        default:
            // Nothing to do.
            break;
    }

    *aIsValidPlan = ID_FALSE;
    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}

IDE_RC qci::validatePlan(
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext,
    qciStatement                       * aStatement,
    void                               * aSharedPlan,
    idBool                             * aIsValidPlan )
{

    if ( QCU_SQL_PLAN_CACHE_VALID_MODE == 1 )
    {
        IDE_TEST( qci::validatePlanMode( aGetSmiStmt4PrepareCallback,
                                         aGetSmiStmt4PrepareContext,
                                         aStatement,
                                         aSharedPlan,
                                         aIsValidPlan )
                  != IDE_SUCCESS )
    }
    else
    {
        IDE_TEST( qci::validatePlanOrg( aGetSmiStmt4PrepareCallback,
                                        aGetSmiStmt4PrepareContext,
                                        aStatement,
                                        aSharedPlan,
                                        aIsValidPlan )
                  != IDE_SUCCESS )
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
//PROJ-1436 SQL Plan Cache
// meta 검색때문에 smiStatement가 필요하다.
// aEnv -> PCO의 environment
IDE_RC qci::checkPrivilege(
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext,
    qciStatement                       * aStatement,
    void                               * aSharedPlan )
{
    qcStatement      * sStatement;
    smiStatement     * sSmiStmtOrg;
    qcSharedPlan     * sSharedPlan;
    qcgPlanObject    * sObject;
    qcgPlanPrivilege * sPrivilege;
    qcgEnvProcList   * sProc;
    UInt               sStage = 1;
    // BUG-37251
    qcgEnvPkgList    * sPkg;
    UInt               sPkgStage = 1;

    sStatement  = & aStatement->statement;
    sSharedPlan = (qcSharedPlan*) aSharedPlan;
    sObject     = & sSharedPlan->planEnv->planObject;
    sPrivilege  = & sSharedPlan->planEnv->planPrivilege;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );

    if( sPrivilege->tableList != NULL )
    {                                              
        IDE_TEST( qcgPlan::checkPlanPrivTable( sStatement,
                                               sPrivilege->tableList,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( sPrivilege->sequenceList != NULL )
    {
        IDE_TEST( qcgPlan::checkPlanPrivSequence( sStatement,
                                                  sPrivilege->sequenceList,
                                                  aGetSmiStmt4PrepareCallback,
                                                  aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( sObject->procList != NULL )
    {
        // PROJ-2446
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        // 이미 latch가 획득된 상태이다.

        IDE_TEST( qcgPlan::checkPlanPrivProc( sStatement,
                                              sObject->procList,
                                              aGetSmiStmt4PrepareCallback,
                                              aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        // unlatch proc
        sStage = 0;
        for( sProc = sObject->procList;
             sProc != NULL;
             sProc = sProc->next )
        {
            (void) qsxProc::unlatch( sProc->procID );
        }
    }
    else
    {
        // Nothing to do.
    }

    // BUG-37251 check privilege for package
    if( sObject->pkgList != NULL )
    {
        // PROJ-2446
        IDE_TEST( qcgPlan::setSmiStmtCallback( sStatement,
                                               aGetSmiStmt4PrepareCallback,
                                               aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        // 이미 latch가 획득된 상태잎.

        IDE_TEST( qcgPlan::checkPlanPrivPkg( sStatement,
                                             sObject->pkgList,
                                             aGetSmiStmt4PrepareCallback,
                                             aGetSmiStmt4PrepareContext )
                  != IDE_SUCCESS );

        // unlatch pkg
        sPkgStage = 0;
        for( sPkg = sObject->pkgList;
             sPkg != NULL;
             sPkg = sPkg->next )
        {
            (void) qsxPkg::unlatch( sPkg->pkgID );
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( changeStmtState( aStatement, EXEC_PREPARE )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            // unlatch proc
            if( sObject->procList != NULL )
            {
                for( sProc = sObject->procList;
                     sProc != NULL;
                     sProc = sProc->next )
                {
                    (void) qsxProc::unlatch( sProc->procID );
                }
            }
            else
            {
                // Nothing to do.
            }
            break;
        default :
            break;
    }

    // BUG-37251 check privilege for package
    switch ( sPkgStage )
    {
        case 1:
            // unlatch pkg
            if( sObject->pkgList != NULL )
            {
                for( sPkg = sObject->pkgList;
                     sPkg != NULL;
                     sPkg = sPkg->next )
                {
                    (void) qsxPkg::unlatch( sPkg->pkgID );
                }
            }
            else
            {
                // Nothing to do.
            }
            break;
        default :
            break;
    }

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}

//PROJ-1436 SQL Plan Cache
// aSharedPlan->PCO의 sharedplan.
IDE_RC qci::clonePrivateTemplate( qciStatement            * aStatement,
                                  void                    * aSharedPlan,
                                  void                    * aPrepPrivateTemplate,
                                  qciSQLPlanCacheContext  * aPlanCacheContext )
{
/***********************************************************************
 *
 * Description : PROJ-1436 SQL Plan Cache
 *
 * Implementation :
 *     qcStatement는 plan cache 입장에서 크게 세가지 형태로 구분된다.
 *
 *     1. cache 비대상이고 자신이 생성한 plan을 그대로 사용하는 경우
 *        - prepare와 execute 시점 모두 myPlan = &privatePlan
 *        - 원본 template을 그대로 private template으로 사용한다.
 *     2. cache 대상이고 자신이 생성한 plan을 공유하여 사용하는 경우
 *        - prepare 시에는 myPlan = &privatePlan
 *        - execute 시에는 myPlan = &sharedPlan
 *        - 원본 template을 복사하여 private template을 사용한다.
 *     3. cache 대상이고 다른 stmt에 의해 생성된 plan을 공유하여 사용하는 경우
 *        - prepare가 없다.
 *        - execute 시에는 myPlan = &sharedPlan
 *        - 원본 template을 복사하여 private template을 사용한다.
 *
 *     2, 3번인 경우, 여기로 들어온다.
 *
 ***********************************************************************/

    qcStatement           * sStatement;
    qcSharedPlan          * sSharedPlan;
    qcPrepTemplateHeader  * sPrepTemplateHeader = NULL;
    qcPrepTemplate        * sPrepTemplate       = NULL;
    iduListNode           * sPrepListNode;
    idBool                  sUsePrepTemplate;

    sStatement = & aStatement->statement;
    sSharedPlan = (qcSharedPlan *) aSharedPlan;
    sPrepTemplateHeader = (qcPrepTemplateHeader*) aPrepPrivateTemplate;

    //------------------------------------------
    // shared plan의 assign
    //------------------------------------------

    // shared plan을 statement의 plan으로 복사한다. PSM의 수행에서
    // statement change를 수행하므로 shared plan을 복사해서 사용한다.
    idlOS::memcpy( (void*) & sStatement->sharedPlan,
                   (void*) sSharedPlan,
                   ID_SIZEOF(qcSharedPlan) );

    sStatement->myPlan = & sStatement->sharedPlan;

    IDE_DASSERT( QC_PRIVATE_TMPLATE(sStatement) == NULL );
    IDE_DASSERT( QC_SHARED_TMPLATE(sStatement) != NULL );

    // get smiStatment cursor flag
    IDE_TEST( qcg::getSmiStatementCursorFlag( QC_SHARED_TMPLATE(sStatement),
                                              &(aPlanCacheContext->mSmiStmtCursorFlag) )
              != IDE_SUCCESS );

    aPlanCacheContext->mStmtType = (qciStmtType) sSharedPlan->parseTree->stmtKind;

    //------------------------------------------
    // procPlanList 생성
    //------------------------------------------

    // execute중 변경되지 않으므로 assign으로 대신한다.
    sStatement->spvEnv->procPlanList = sSharedPlan->procPlanList;

    sStatement->spvEnv->latched = ID_FALSE;

    //------------------------------------------
    // prepared private template 사용 혹은 private template 생성
    //------------------------------------------

    if( sPrepTemplateHeader != NULL )
    {
        IDE_ASSERT( sPrepTemplateHeader->prepMutex.lock(NULL /* idvSQL* */)
                    == IDE_SUCCESS );
        /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
           Dynamic SQL 환경에서는 개선이 필요하다.
           template 할당시  다음과 같이  free list에서 할당한다.
        */
        if(IDU_LIST_IS_EMPTY(&(sPrepTemplateHeader->freeList)))
        {
            /* 비어 있는 경우이어서, 새로 할당받아야 한다 */
            sUsePrepTemplate = ID_FALSE;
        }
        else
        {
            /* free list에서 하나를 띄어내어 할당받고,
             used list으로 move 한다.*/
            sPrepListNode = IDU_LIST_GET_FIRST(&(sPrepTemplateHeader->freeList));
            IDU_LIST_REMOVE(sPrepListNode);
            IDU_LIST_ADD_LAST(&(sPrepTemplateHeader->usedList),sPrepListNode);
            sPrepTemplate = (qcPrepTemplate*)sPrepListNode->mObj;
            sPrepTemplateHeader->freeCount--;
            sUsePrepTemplate = ID_TRUE;

        }
        IDE_ASSERT( sPrepTemplateHeader->prepMutex.unlock()
                    == IDE_SUCCESS );

        if( sUsePrepTemplate == ID_FALSE )
        {
            // prepared private template을 모두 사용중이어서
            // 추가 생성이 필요한 경우

            IDE_TEST( allocPrepTemplate( sStatement,
                                         & sPrepTemplate )
                      != IDE_SUCCESS );

            IDE_ASSERT( sPrepTemplateHeader->prepMutex.lock(NULL /* idvSQL* */)
                        == IDE_SUCCESS );

            // 연결한다.
            IDU_LIST_ADD_LAST(&(sPrepTemplateHeader->usedList),&(sPrepTemplate->prepListNode));
            IDE_ASSERT( sPrepTemplateHeader->prepMutex.unlock()
                        == IDE_SUCCESS );

            sUsePrepTemplate = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        //fix BUG-30081 qci::clonePrivateTemplate에서 UMR
        sUsePrepTemplate = ID_FALSE;
    }

    // PROJ-2163
    if( sUsePrepTemplate == ID_TRUE )
    {
        // private template 할당
        QC_PRIVATE_TMPLATE(sStatement) = sPrepTemplate->tmplate;

        // BUG-44710
        if ( QC_SHARED_TMPLATE(sStatement)->shardExecData.planCount > 0 )
        {
            QC_PRIVATE_TMPLATE(sStatement)->shardExecData =
                QC_SHARED_TMPLATE(sStatement)->shardExecData;
            IDE_TEST( sdi::allocDataInfo(
                          &(QC_PRIVATE_TMPLATE(sStatement)->shardExecData),
                          QC_QME_MEM(sStatement) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // bind parameter 할당
        if( sSharedPlan->sBindParamCount > 0 )
        {
            if( sStatement->pBindParam == NULL )
            {
                IDE_TEST( QC_QMB_MEM(sStatement)->alloc(
                                ID_SIZEOF(qciBindParamInfo)
                                    * sSharedPlan->sBindParamCount,
                                (void**) & sStatement->pBindParam )
                          != IDE_SUCCESS );

                idlOS::memcpy( (void*) sStatement->pBindParam,
                               (void*) sSharedPlan->sBindParam,
                               ID_SIZEOF(qciBindParamInfo)
                                   * sSharedPlan->sBindParamCount );

                sStatement->pBindParamCount = sSharedPlan->sBindParamCount;

                // pBindParam 의 값이 바뀌었다.
                sStatement->pBindParamChangedFlag = ID_TRUE;
            }
            else
            {
                // 사용자가 바인드한 정보는 덮어쓰지 않는다.
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        // prepared private template 설정
        sStatement->prepTemplateHeader = sPrepTemplateHeader;
        sStatement->prepTemplate = sPrepTemplate;
    }
    else
    {
        // private template 생성
        IDE_TEST( qcg::allocPrivateTemplate( sStatement,
                                             QC_QME_MEM(sStatement) )
                  != IDE_SUCCESS );

        IDE_TEST( qcg::cloneTemplate( QC_QME_MEM(sStatement),
                                      QC_SHARED_TMPLATE(sStatement),
                                      QC_PRIVATE_TMPLATE(sStatement) )
                  != IDE_SUCCESS );

        /* PROJ-2462 Result Cache */
        qmxResultCache::allocResultCacheData( QC_PRIVATE_TMPLATE( sStatement ),
                                              QC_QME_MEM(sStatement) );

        // BUG-44710
        if ( QC_SHARED_TMPLATE(sStatement)->shardExecData.planCount > 0 )
        {
            QC_PRIVATE_TMPLATE(sStatement)->shardExecData =
                QC_SHARED_TMPLATE(sStatement)->shardExecData;
            IDE_TEST( sdi::allocDataInfo(
                          &(QC_PRIVATE_TMPLATE(sStatement)->shardExecData),
                          QC_QME_MEM(sStatement) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // bind parameter 생성
        if( sSharedPlan->sBindParamCount > 0 )
        {
            if( sStatement->pBindParam == NULL )
            {
                IDE_TEST( QC_QMB_MEM(sStatement)->alloc(
                              ID_SIZEOF(qciBindParamInfo)
                                  * sSharedPlan->sBindParamCount,
                              (void**) & sStatement->pBindParam )
                          != IDE_SUCCESS );

                idlOS::memcpy( (void*) sStatement->pBindParam,
                               (void*) sSharedPlan->sBindParam,
                               ID_SIZEOF(qciBindParamInfo)
                                   * sSharedPlan->sBindParamCount );

                sStatement->pBindParamCount = sSharedPlan->sBindParamCount;

                // pBindParam 의 값이 바뀌었다.
                sStatement->pBindParamChangedFlag = ID_TRUE;
            }
            else
            {
                // 사용자가 바인드한 정보는 덮어쓰지 않는다.
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        // prepared private template 설정
        sStatement->prepTemplateHeader = NULL;
        sStatement->prepTemplate = NULL;
    }

    //------------------------------------------
    // execution을 위한 초기화
    //------------------------------------------

    IDU_FIT_POINT( "qci::clonePrivateTemplate::BEFORE::qcg::stepAfterPVO" );
    IDE_TEST( qcg::stepAfterPVO( sStatement ) != IDE_SUCCESS );

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    qciMisc::setSimpleFlag( sStatement );
    
    /* BUG-31570
     * DDL이 빈번한 환경에서 plan text를 안전하게 보여주는 방법이 필요하다.
     */
    qcg::setPlanTreeState( sStatement, ID_TRUE );

    // BUG-42512 plan 의 bind 정보와 statement 의 bind 정보를 검증합니다.
    IDU_FIT_POINT( "qci::clonePrivateTemplate::BEFORE::qci::checkBindInfo" );
    IDE_TEST( checkBindInfo( aStatement ) != IDE_SUCCESS );

    // 공유된 plan을 사용하고 있음을 표시한다.
    sStatement->sharedFlag = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* sStatement->sharedPlan은 Plan Cache를 복제한 것이므로, 사용하면 안 된다. */
    sStatement->myPlan = & sStatement->privatePlan;

    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     *  Plan Cache에서 Plan이 제거되면, Private Template도 같이 해제된다.
     *  따라서, Private Template을 미리 반납하여, Plan Cache에서 해제한 메모리를 사용하는 문제를 방지한다.
     */
    if ( sPrepTemplate != NULL )
    {
        qcg::freePrepTemplate( sStatement, ID_FALSE );
    }
    else
    {
        /* Nothing to do */
    }
    QC_PRIVATE_TMPLATE(sStatement) = NULL;

    return IDE_FAILURE;
}

IDE_RC qci::setPrivateTemplate( qciStatement            * aStatement,
                                void                    * aSharedPlan,
                                qciSqlPlanCacheInResult   aInResult )
{
    qcStatement   * sStatement = & aStatement->statement;
    qcSharedPlan  * sSharedPlan = NULL;
    iduVarMemList * sQmpMem     = NULL;

    if( aInResult == QCI_SQL_PLAN_CACHE_IN_FAILURE )
    {
        // check-in이 실패하여 호출된 경우
        IDE_DASSERT( aSharedPlan != NULL );
        sSharedPlan = (qcSharedPlan *)aSharedPlan;

        /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
         *  qci::makePlanCacheInfo()에서 만든 QMP Memory와 QMU Memory를 제거한다.
         */
        sQmpMem             = sSharedPlan->qmpMem;
        sSharedPlan->qmpMem = QC_QMP_MEM(sStatement);
        (void)freeSharedPlan( (void *)sSharedPlan );
        sSharedPlan->qmpMem = sQmpMem;
        sSharedPlan->qmuMem = NULL;

        idlOS::memcpy( (void *)sStatement->myPlan,
                       (void *)sSharedPlan,
                       ID_SIZEOF(qcSharedPlan) );

        sStatement->qmpStackPosition = sStatement->mQmpStackPositionForCheckInFailure;

        // 삭제했던 원본 template의 tuple row를 재생성한다.
        IDE_TEST( qcgPlan::allocUnusedTupleRow( sStatement->myPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        //ideLog::log(IDE_QP_2, "[INFO] Query Preparation Memory Size : %llu KB",
        //            QC_QMP_MEM(sStatement)->getAllocSize()/1024);
    }

    IDE_TEST( qcg::setPrivateArea( sStatement ) != IDE_SUCCESS );

    IDE_TEST( qcg::stepAfterPVO( sStatement ) != IDE_SUCCESS );

    qcg::setPlanTreeState( sStatement, ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::makePlanCacheInfo( qciStatement           * aStatement,
                        qciSQLPlanCacheContext * aPlanCacheContext )
{
    qcStatement           * sStatement;
    qcSharedPlan          * sSharedPlan = NULL;
    qcPrepTemplateHeader  * sPrepTemplateHeader = NULL;
    qcPrepTemplate        * sPrepTemplate;
    qsxProcPlanList       * sProcPlan;
    qsxProcPlanList       * sProc;
    void                  * sAllocPtr = NULL;
    void                  * sQmpAllocPtr = NULL;
    iduVarMemList         * sPmhMem = NULL;
    UInt                    sMutexState = 0;
    UInt                    sPrepContextCnt;
    iduListNode           * sIterator;
    iduListNode           * sNodeNext;
    UInt                    i;
    idBool                  sIsQmuInited = ID_FALSE;
    idBool                  sIsPmhInited = ID_FALSE;
    idBool                  sIsQmpInited = ID_FALSE;

    sStatement = & aStatement->statement;

    // BUG-46137
    if ( QC_SHARED_TMPLATE(sStatement) != NULL )
    {
        if ( ( QC_SHARED_TMPLATE(sStatement)->flag & QC_TMP_PLAN_CACHE_KEEP_MASK )
                == QC_TMP_PLAN_CACHE_KEEP_TRUE ) 
        {
            aPlanCacheContext->mPlanCacheKeep = ID_TRUE;
        }
        else
        {
            aPlanCacheContext->mPlanCacheKeep = ID_FALSE;
        }
    }
    else
    {
        aPlanCacheContext->mPlanCacheKeep = ID_FALSE;
    }

    aPlanCacheContext->mSharedPlanSize = 0;
    aPlanCacheContext->mPrepPrivateTemplateSize = 0;

    IDE_DASSERT( sStatement->myPlan->planEnv != NULL );

    //------------------------------------------
    // plan environment 설정
    //------------------------------------------

    // plan을 생성한 user id를 설정한다.
    sStatement->myPlan->planEnv->planUserID =
        QCG_GET_SESSION_USER_ID( sStatement );

    //------------------------------------------
    // myPlan 복사
    //------------------------------------------

    IDE_TEST( QC_QMP_MEM(sStatement)->alloc( ID_SIZEOF(qcSharedPlan),
                                             (void**) & sSharedPlan )
              != IDE_SUCCESS );

    idlOS::memcpy( (void*)sSharedPlan,
                   (void*)sStatement->myPlan,
                   ID_SIZEOF(qcSharedPlan) );

    /* Plan Cache를 만들기 위해, Shared Plan을 잠시 사용한다. */
    sStatement->myPlan = sSharedPlan;

    // statement와의 연결을 끊는다.
    IDE_ERROR(QC_SHARED_TMPLATE(sStatement)->stmt == sStatement);
    QC_SHARED_TMPLATE(sStatement)->stmt = NULL;

    //------------------------------------------
    // procPlanList 복사
    //------------------------------------------

    for( sProcPlan = sStatement->spvEnv->procPlanList;
         sProcPlan != NULL;
         sProcPlan = sProcPlan->next )
    {
        IDE_TEST( QC_QMP_MEM(sStatement)->alloc( ID_SIZEOF(qsxProcPlanList),
                                                 (void**) & sProc )
                  != IDE_SUCCESS );

        idlOS::memcpy( (void*)sProc,
                       (void*)sProcPlan,
                       ID_SIZEOF(qsxProcPlanList) );

        sProc->next = sSharedPlan->procPlanList;

        // 연결한다.
        sSharedPlan->procPlanList = sProc;
    }

    //------------------------------------------
    // shared plan 크기 줄이기 tip
    //------------------------------------------

    // variable/intermediate/table tuple의 row는 원본 template으로써
    // rowMaxinum 정보만이 필요할뿐 row 자체는 사용되지 않으므로
    // free가 가능하다.
    IDE_TEST( qcgPlan::freeUnusedTupleRow( sSharedPlan )
              != IDE_SUCCESS );

    //------------------------------------------
    // plan memory 백업
    //------------------------------------------

    if ( QCU_EXECUTION_PLAN_MEMORY_CHECK == 1 )
    {
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_TEST( qcg::allocIduVarMemList(&sAllocPtr) != IDE_SUCCESS );
        
        sSharedPlan->qmuMem = new (sAllocPtr) iduVarMemList;
        
        /* BUG-33688 iduVarMemList의 init 이 실패할수 있습니다. */
        IDE_TEST( sSharedPlan->qmuMem->init(IDU_MEM_QMP) != IDE_SUCCESS);
        sIsQmuInited = ID_TRUE;

        // qmpMem을 qmuMem으로 복제한다.
        IDE_TEST( sSharedPlan->qmpMem->clone( sSharedPlan->qmuMem )
                  != IDE_SUCCESS );
        
        IDE_DASSERT( sSharedPlan->qmpMem->getAllocCount()
                     == sSharedPlan->qmuMem->getAllocCount() );
        IDE_DASSERT( sSharedPlan->qmpMem->getAllocSize()
                     == sSharedPlan->qmuMem->getAllocSize() );
        
        aPlanCacheContext->mSharedPlanSize += (UInt) sSharedPlan->qmuMem->getAllocSize();
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // prepared private template을 생성한다.
    //------------------------------------------

    sPrepContextCnt = QCU_SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT;

    if( sPrepContextCnt > 0 )
    {
        // pmhMem을 생성한다.
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_TEST( qcg::allocIduVarMemList(&sAllocPtr)
                  != IDE_SUCCESS);

        sPmhMem = new (sAllocPtr) iduVarMemList;
        IDE_TEST( sPmhMem->init(IDU_MEM_QMP) != IDE_SUCCESS);
        sIsPmhInited = ID_TRUE;

        // pmhMem으로 sPrepTemplateHeader을 생성한다.
        IDE_TEST( sPmhMem->alloc( ID_SIZEOF(qcPrepTemplateHeader),
                                  (void**)&sPrepTemplateHeader )
                  != IDE_SUCCESS );

        // sPrepTemplateHeader를 초기화한다.
        sPrepTemplateHeader->pmhMem       = sPmhMem;
        /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
           Dynamic SQL 환경에서는 개선이 필요하다.
        */
        sPrepTemplateHeader->freeCount    = 0;
        IDU_LIST_INIT(&(sPrepTemplateHeader->freeList));
        IDU_LIST_INIT(&(sPrepTemplateHeader->usedList));

        IDE_TEST( sPrepTemplateHeader->prepMutex.initialize(
                      (SChar*) "SQL_PLAN_CACHE_PREPARED_TEMPLATE_MUTEX",
                      IDU_MUTEX_KIND_NATIVE,
                      IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );
        sMutexState = 1;

        for( i = 0; i < sPrepContextCnt; i++ )
        {
            IDE_TEST( allocPrepTemplate( sStatement,
                                         & sPrepTemplate )
                      != IDE_SUCCESS );
            // 연결한다.
            /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
               Dynamic SQL 환경에서는 개선이 필요하다.
               free list에  prepared execution template를  추가한다.
            */
            IDU_LIST_ADD_LAST(&(sPrepTemplateHeader->freeList), &(sPrepTemplate->prepListNode));
            sPrepTemplateHeader->freeCount++;

        }
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // aPlanCacheContext를 채운다.
    //------------------------------------------

    // shared plan
    aPlanCacheContext->mSharedPlanMemory = (void*) sSharedPlan;
    aPlanCacheContext->mSharedPlanSize +=
        (UInt) sSharedPlan->qmpMem->getAllocSize();

    if( sPrepTemplateHeader != NULL )
    {
        // prepared private template
        aPlanCacheContext->mPrepPrivateTemplate = (void*) sPrepTemplateHeader;
        aPlanCacheContext->mPrepPrivateTemplateSize +=
            (UInt) sPrepTemplateHeader->pmhMem->getAllocSize();

        /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
           Dynamic SQL 환경에서는 개선이 필요하다.
        */
        IDU_LIST_ITERATE(&(sPrepTemplateHeader->freeList),sIterator)
        {

            sPrepTemplate = (qcPrepTemplate*)sIterator->mObj;
            aPlanCacheContext->mPrepPrivateTemplateSize +=
                (UInt) sPrepTemplate->pmtMem->getAllocSize();
        }
    }
    else
    {
        aPlanCacheContext->mPrepPrivateTemplate = NULL;
    }

    // plan property
    aPlanCacheContext->mPlanEnv = & sSharedPlan->planEnv->planProperty;

    // PROJ-2163
    aPlanCacheContext->mPlanBindInfo = sSharedPlan->planEnv->planBindInfo;

    //------------------------------------------
    // BUG-44853 qcStatement의 재사용시 사용할 QMP Memory와 Shared Template을 생성한다.
    //------------------------------------------

    /* Plan Cache를 만들었으므로, Private Plan을 사용한다. */
    sStatement->myPlan = & sStatement->privatePlan;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST( qcg::allocIduVarMemList( (void **) & sQmpAllocPtr ) != IDE_SUCCESS );

    // To fix BUG-20676
    QC_QMP_MEM(sStatement) = new (sQmpAllocPtr) iduVarMemList;
    IDE_TEST( QC_QMP_MEM(sStatement)->init( IDU_MEM_QMP,
                                            iduProperty::getPrepareMemoryMax() )
              != IDE_SUCCESS );
    sIsQmpInited = ID_TRUE;

    IDE_TEST( qcg::allocSharedTemplate( sStatement,
                                        QC_SHARED_TMPLATE(sStatement)->tmplate.stackCount )
              != IDE_SUCCESS );

    sStatement->mQmpStackPositionForCheckInFailure = sStatement->qmpStackPosition;
    IDU_FIT_POINT( "qci::makePlanCacheInfo::BEFORE::iduVarMemList::getStatus" );
    IDE_TEST( QC_QMP_MEM(sStatement)->getStatus( & sStatement->qmpStackPosition ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sSharedPlan != NULL )
    {
        if ( QCU_EXECUTION_PLAN_MEMORY_CHECK == 1 )
        {
            if( sSharedPlan->qmuMem != NULL )
            {
                if ( sIsQmuInited == ID_TRUE )
                {
                    (void) sSharedPlan->qmuMem->destroy();
                }
                else
                {
                    /* Nothing to do */
                }
                
                /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                (void) qcg::freeIduVarMemList(sSharedPlan->qmuMem);
                
                sSharedPlan->qmuMem = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        if( sPrepTemplateHeader != NULL )
        {

        /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
           Dynamic SQL 환경에서는 개선이 필요하다.
           used list를  free list에 joint시켜 하나의 list으로 만들고나서
           prepared execution template들을 해제한다.
          */       
            IDU_LIST_JOIN_LIST(&(sPrepTemplateHeader->freeList),&(sPrepTemplateHeader->usedList));
            IDU_LIST_ITERATE_SAFE(&(sPrepTemplateHeader->freeList),sIterator,sNodeNext)
            {
                sPrepTemplate = (qcPrepTemplate*)sIterator->mObj;
                IDE_ASSERT( freePrepTemplate( sPrepTemplate ) == IDE_SUCCESS );
            } 
        }
        
        if( sMutexState == 1 )
        {
            (void) sPrepTemplateHeader->prepMutex.destroy();
        }
        
        if( sPmhMem != NULL )
        {
            if ( sIsPmhInited == ID_TRUE )
            {
                sPmhMem->destroy();
            }
            else
            {
                /* Nothing to do */
            }

            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            (void)qcg::freeIduVarMemList(sPmhMem);
            
            sPmhMem = NULL;
        }
    }

    /* sSharedPlan은 sStatement->privatePlan을 복제한 것이므로, 사용하면 안 된다. */
    sStatement->myPlan = & sStatement->privatePlan;

    // BUG-35828
    QC_SHARED_TMPLATE(sStatement)->stmt = sStatement;

    if ( sQmpAllocPtr != NULL )
    {
        if ( sIsQmpInited == ID_TRUE )
        {
            (void)QC_QMP_MEM(sStatement)->destroy();
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)qcg::freeIduVarMemList( QC_QMP_MEM(sStatement) );

        QC_QMP_MEM(sStatement)        = sSharedPlan->qmpMem;
        QC_SHARED_TMPLATE(sStatement) = sSharedPlan->sTmplate;
    }
    else
    {
        /* Nothing to do */
    }

    aPlanCacheContext->mSharedPlanMemory = NULL;
    aPlanCacheContext->mPrepPrivateTemplate = NULL;

    return IDE_FAILURE;
}

IDE_RC qci::allocPrepTemplate( qcStatement     * aStatement,
                               qcPrepTemplate ** aPrepTemplate )
{
    qcStatement        sDummyStatement;
    qcPrepTemplate   * sPrepTemplate;
    qciBindParamInfo * sBindParamInfo = NULL;
    void             * sAllocPtr = NULL;
    iduVarMemList    * sPmtMem = NULL;
    idBool             sIsInited = ID_FALSE;

    // pmhMem을 생성한다.
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST( qcg::allocIduVarMemList((void**)&sAllocPtr) != IDE_SUCCESS);
    
    sPmtMem = new (sAllocPtr) iduVarMemList;

    /* BUG-33688 iduVarMemList의 init 이 실패할수 있습니다. */
    IDE_TEST( sPmtMem->init(IDU_MEM_QMP) != IDE_SUCCESS);
    sIsInited = ID_TRUE;

    // 1681 MERGE
    // allocPrivateTemplate에서 session language를 필요로 하므로
    // dummy statement에 session을 연결해 준다.
    sDummyStatement.session = aStatement->session;

    // private template을 생성한다.
    IDE_TEST( qcg::allocPrivateTemplate( &sDummyStatement,
                                         sPmtMem )
              != IDE_SUCCESS );

    IDE_TEST( qcg::cloneTemplate( sPmtMem,
                                  QC_SHARED_TMPLATE(aStatement),
                                  QC_PRIVATE_TMPLATE(&sDummyStatement) )
              != IDE_SUCCESS );

    // bind param info를 생성한다.
    if( aStatement->myPlan->sBindParamCount > 0 )
    {
        IDE_TEST( sPmtMem->alloc( ID_SIZEOF(qciBindParamInfo) *
                                  aStatement->myPlan->sBindParamCount,
                                  (void**) & sBindParamInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // prepared template을 생성한다.
    IDE_TEST( sPmtMem->alloc( ID_SIZEOF(qcPrepTemplate),
                              (void**)& sPrepTemplate )
              != IDE_SUCCESS );

    /* PROJ-2462 Result Cache */
    qmxResultCache::allocResultCacheData( QC_PRIVATE_TMPLATE( &sDummyStatement ),
                                          sPmtMem );

    // aTemplateList를 초기화한다.
    sPrepTemplate->pmtMem    = sPmtMem;
    /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
       Dynamic SQL 환경에서는 개선이 필요하다.
    */
    IDU_LIST_INIT_OBJ(&(sPrepTemplate->prepListNode),sPrepTemplate);
    sPrepTemplate->tmplate   = QC_PRIVATE_TMPLATE(&sDummyStatement);
    sPrepTemplate->bindParam = sBindParamInfo;

    *aPrepTemplate = sPrepTemplate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sPmtMem != NULL )
    {
        if ( sIsInited == ID_TRUE )
        {
            sPmtMem->destroy();
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        qcg::freeIduVarMemList(sPmtMem);
        sPmtMem = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC qci::freePrepTemplate( qcPrepTemplate * aPrepTemplate )
{
    iduVarMemList  * sPmtMem;

    /* PROJ-2462 Reuslt Cache */
    if ( aPrepTemplate->tmplate->resultCache.count > 0 )
    {
        qmxResultCache::destroyResultCache( aPrepTemplate->tmplate );
    }
    else
    {
        /* Nothing to do */
    }

    sPmtMem = aPrepTemplate->pmtMem;
    sPmtMem->destroy();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(qcg::freeIduVarMemList(sPmtMem ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::freeSharedPlan( void * aSharedPlan )
{
    qcSharedPlan  * sSharedPlan;
    iduVarMemList * sQmpMem;
    iduVarMemList * sQmuMem;

    IDE_DASSERT( aSharedPlan != NULL );
    
    sSharedPlan = (qcSharedPlan *) aSharedPlan;

    // sSharedPlan은 qmpMem으로 생성되었으므로
    // qmpMem 해제와 함께 파괴되므로 미리 기억해둔다.
    sQmpMem = sSharedPlan->qmpMem;
    sQmuMem = sSharedPlan->qmuMem;
    
    if( sQmpMem != NULL )
    {
        sQmpMem->destroy();
        
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_TEST(qcg::freeIduVarMemList( sQmpMem ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    if ( QCU_EXECUTION_PLAN_MEMORY_CHECK == 1 )
    {
        if( sQmuMem != NULL )
        {
            (void)sQmuMem->destroy();
            
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(qcg::freeIduVarMemList( sQmuMem ) != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::freePrepPrivateTemplate( void * aPrepPrivateTemplate )
{
    qcPrepTemplateHeader  * sPrepTemplateHeader;
    qcPrepTemplate        * sPrepTemplate;
    iduVarMemList         * sPmhMem;
    iduListNode           * sIterator;
    iduListNode           * sNodeNext;

    // aPrepTemplateHeader는 property에 의해 NULL이 될 수 있다.
    if( aPrepPrivateTemplate != NULL )
    {
        sPrepTemplateHeader = (qcPrepTemplateHeader *) aPrepPrivateTemplate;
        /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
           Dynamic SQL 환경에서는 개선이 필요하다.
           used list를  free list에 joint시켜 하나의 list으로 만들고나서
           prepared execution template들을 해제한다.
        */
        IDU_LIST_JOIN_LIST(&(sPrepTemplateHeader->freeList),&(sPrepTemplateHeader->usedList));
        IDU_LIST_ITERATE_SAFE(&(sPrepTemplateHeader->freeList),sIterator,sNodeNext)
        {
            sPrepTemplate = (qcPrepTemplate*)sIterator->mObj;
            IDE_TEST( freePrepTemplate( sPrepTemplate )
                      != IDE_SUCCESS );
        }
         
        // mutex를 삭제한다.
        IDE_TEST( sPrepTemplateHeader->prepMutex.destroy() != IDE_SUCCESS );
    
        // sPrepTemplateHeader은 pmhMem으로 생성되었으므로
        // pmhMem 해제와 함께 파괴되므로 미리 기억해둔다.
        sPmhMem = sPrepTemplateHeader->pmhMem;
        
        if( sPmhMem != NULL )
        {
            sPmhMem->destroy();
            
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(qcg::freeIduVarMemList( sPmhMem ) != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1518
IDE_RC qci::atomicBegin( qciStatement  * aStatement,
                         smiStatement  * aSmiStmt )
{
    qcStatement * sStatement = &(aStatement->statement);

    sStatement = &(aStatement->statement);

    IDE_TEST(checkRebuild(aStatement) != IDE_SUCCESS);

    qcg::setSmiStmt( sStatement, aSmiStmt );

    //fix BUG-17553
    IDV_SQL_SET( sStatement->mStatistics, mMemoryTableAccessCount, 0 );

    IDE_TEST( checkExecuteFuncAndSetEnv(
                  aStatement,
                  EXEC_EXECUTE ) != IDE_SUCCESS );

    IDE_TEST( qcm::validateAndLockAllObjects( sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qmx::atomicExecuteInsertBefore( sStatement )
              != IDE_SUCCESS );

    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
            != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    changeStmtState( aStatement, EXEC_EXECUTE );

    if( sStatement->spvEnv->latched == ID_TRUE )
    {
        if ( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing To Do
        }

    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qci::atomicInsert( qciStatement    * aStatement )
{
    qcStatement * sStatement = NULL;

    sStatement = &(aStatement->statement);

    IDE_TEST( qmx::atomicExecuteInsert( sStatement )
              != IDE_SUCCESS);

    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    changeStmtState( aStatement, EXEC_EXECUTE );

    if( sStatement->spvEnv->latched == ID_TRUE )
    {
        if ( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qci::atomicSetPrepareState( qciStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      Atomic insert 의 반복 수행을 위한 상태 변경 함수
 *      기존 checkExecFuncAndSetEnv 에서는 다음과 같은 처리를 한다.
 *      STATE_OK | STATE_CLOSE | STATE_PSM_UNLATCH | STATE_PARAM_DATA_CLEAR
 *      STATE_CLOSE 는 이곳에서 하면 문제가 발생한다.
 *      STATE_PSM_UNLATCH는 계속 수행을 해야 하므로 필요성이 없다.
 *      따라서 이 함수에서는 STATE_PARAM_DATA_CLEAR 만 처리한다.
 *
 *      PROJ-2163
 *      EXEC_EXECUTE -> EXEC_PREPARE 로 상태전환
 *
 * Implementation :
 *      1. STATE_PARAM_DATA_CLEAR 를 위해 data bound flag 를 flase 로 세팅
 *      2. QCI statement state 를 PREPARED 로 변경
 *
 ***********************************************************************/

    qcStatement * sStatement = NULL;
    UShort        sCount;

    sStatement = &(aStatement->statement);

    // STATE_PARAM_DATA_CLEAR
    if( sStatement->pBindParamCount > 0 )
    {
        for( sCount = 0; sCount < sStatement->pBindParamCount; ++sCount )
        {
            sStatement->pBindParam[sCount].isParamDataBound = ID_FALSE;
        }
    }
    else
    {
        // parameter가 존재하지 않는 경우.
        // Nothing To Do
    }

    IDE_TEST( changeStmtState( aStatement, EXEC_PREPARE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::atomicEnd( qciStatement  * aStatement )
{
    qcStatement * sStatement = NULL;

    sStatement = &(aStatement->statement);

    IDE_TEST( qmx::atomicExecuteInsertAfter( sStatement )
              != IDE_SUCCESS );

    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
              != IDE_SUCCESS );

    // set success
    QC_PRIVATE_TMPLATE(sStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(sStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    changeStmtState( aStatement, EXEC_EXECUTE );

    return IDE_FAILURE;
}

// atomicFinalize 함수는 Finalize 상태에서 호출되는것이 아니다
// MM에서 clearStmt 등 Stmt가 끝났을때 Atomic 관련 자원을 정리용
void qci::atomicFinalize( qciStatement    * aStatement,
                          idBool          * aIsCursorOpen,
                          smiStatement    * aSmiStmt )
{
    qcStatement * sStatement = &(aStatement->statement);

    qcg::setSmiStmt( sStatement, aSmiStmt );

    // PROJ-1518
    // session이 갑자기 종료되었을때 커서를 close 해야 한다.
    if((*aIsCursorOpen) == ID_TRUE)
    {
        (*aIsCursorOpen) = ID_FALSE;

        (void) qmx::atomicExecuteFinalize( sStatement );
    }

    // Atomic Execute 중 session 이 종료되었을때
    // 상태가 EXEC_BIND_PARAM_INFO, EXEC_BIND_PARAM_DATA 일 수 있다.
    if( sStatement->spvEnv->latched == ID_TRUE )
    {
        if ( qsxRelatedProc::unlatchObjects( sStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            sStatement->spvEnv->latched = ID_FALSE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC qci::setBindTuple( qciStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      Variable tuple 의 row 가 pBindParam 의 data 를 참조하도록 세팅한다.
 *
 * Implementation :
 *      1. Variable tuple 의 row 에 pBindParam->data 주소를 세팅
 *      2. QCI statement state 를 DATA_BOUND 로 변경
 *
 ***********************************************************************/

    qcStatement * sStatement;
    qcTemplate  * sTemplate;
    UShort        sBindTuple;

    IDE_DASSERT(aStatement != NULL);
    IDE_DASSERT(QC_PRIVATE_TMPLATE(&aStatement->statement) != NULL);

    if( getParameterCount( aStatement ) > 0 )
    {
        sStatement = & aStatement->statement;
        sTemplate  = QC_PRIVATE_TMPLATE(sStatement);
        sBindTuple = sTemplate->tmplate.variableRow;

        // variableTuple 의 row 가 pBindParam->data 를 참조 (pointer set)
        sTemplate->tmplate.rows[sBindTuple].row = sStatement->pBindParam->param.data;
    }

    IDE_TEST( setParamDataState(aStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qci::isBindChanged( qciStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      Plan 의 호스트 변수 타입 정보와 사용자가 바인드한 타입 정보를
 *      비교하여 서로 다른지 검사하는 함수.
 *
 *      Plan 의 호스트 변수 타입 : Private template 의 variable tuple 정보
 *      사용자가 바인드한 호스트 변수 타입 : pBindParam 정보
 *
 * Implementation :
 *      1. Plan 생성 혹은 plan cache hit 이후에 pBindParam 이 바뀐적 있는지 검사
 *      2. 쿼리에 호스트 변수가 사용되었는지 검사
 *      3. type 정보 비교
 *
 ***********************************************************************/

    qcTemplate        * sTemplate;
    UShort              sRow;
    mtcColumn         * sPlanParam;
    UShort              sPlanParamCount;
    qciBindParamInfo  * sBindParam;
    UShort              sBindParamCount;
    idBool              sChanged;
    UShort              i;

    if( aStatement->statement.pBindParamChangedFlag == ID_FALSE )
    {
        // pBindParam 이 바뀌지 않았다면 비교할 필요도 없다.
        sChanged = ID_FALSE;
    }
    else
    {
        sTemplate = QC_PRIVATE_TMPLATE( &aStatement->statement );
        sRow  = sTemplate->tmplate.variableRow;

        if( sRow == ID_USHORT_MAX )
        {
            // 쿼리에 호스트 변수를 사용하지 않았다면 비교할 필요도 없다.
            sChanged = ID_FALSE;
        }
        else
        {
            if( sTemplate->tmplate.rows[sRow].columnCount == 0 )
            {
                // 쿼리에 호스트 변수를 사용하지 않았다면 비교할 필요도 없다.
                sChanged = ID_FALSE;
            }
            else
            {
                sPlanParam      = sTemplate->tmplate.rows[sRow].columns;
                sPlanParamCount = sTemplate->tmplate.rows[sRow].columnCount;

                sBindParam      = aStatement->statement.pBindParam;
                sBindParamCount = aStatement->statement.pBindParamCount;

                // 플랜의 호스트 변수 타입 정보와 현재 바인드된 타입 정보를 비교한다.
                if( sPlanParamCount == sBindParamCount )
                {
                    sChanged = ID_FALSE;

                    // PROJ-2163 BUGBUG
                    for( i = 0; i < sBindParamCount; i++ )
                    {
                        // memcmp 혹은 bitmap 등으로 비교한다.
                        if( ( sPlanParam[i].type.dataTypeId != sBindParam[i].param.type      ) ||
                            ( sPlanParam[i].precision       != sBindParam[i].param.precision ) ||
                            ( sPlanParam[i].scale           != sBindParam[i].param.scale     ) )
                        {
                            sChanged = ID_TRUE;
                            break;
                        }
                    }
                }
                else
                {
                    sChanged = ID_TRUE;
                }
            }
        }

        // 타입이 동일하다면 flag 를 제거한다.
        if( sChanged == ID_FALSE )
        {
            aStatement->statement.pBindParamChangedFlag = ID_FALSE;
        }
    }

    return sChanged;
}

idBool qci::isCacheAbleStatement( qciStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *     D$ 테이블 사용 여부와 NO_PLAN_CACHE 힌트 사용 여부를 판단한다.
 *
 * Implementation :
 *     Template 의 flag 를 masking 한 결과를 반환한다.
 *
 *     QC_TMP_PLAN_CACHE_IN_ON flag 는 NO_PLAN_CACHE 힌트가 사용됐거나
 *     plan 이 참조하는 테이블 중 D$ 테이블이 존재할 경우 세팅된다.
 *
 ***********************************************************************/

    idBool       sIsCacheAble = ID_FALSE;
    qcTemplate * sTemplate;

    sTemplate = QC_PRIVATE_TMPLATE( &aStatement->statement );

    if( sTemplate == NULL )
    {
        sTemplate = QC_SHARED_TMPLATE( &aStatement->statement );
    }

    if( ( sTemplate->flag & QC_TMP_PLAN_CACHE_IN_MASK ) ==
          QC_TMP_PLAN_CACHE_IN_ON )
    {
        sIsCacheAble = ID_TRUE;
    }
    else
    {
        sIsCacheAble = ID_FALSE;
    }

    return sIsCacheAble;
}

IDE_RC qci::clearStatement4Reprepare( qciStatement  * aStatement,
                                      smiStatement  * aSmiStmt )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      바인드 메모리를 유지하고 statement 를 clear 하는 함수
 *
 *      reprepare 나 rebuild 시 사용자가 바인드한 호스트 변수 정보는 남긴채로
 *      statement 를 초기화 해야 한다.
 *
 * Implementation :
 *      1. EXEC_REBUILD 로 상태 전이 체크 및 환경 설정
 *      2. rebuild true 로 qcg::clearStatement 실행 (QMB 유지)
 *      3. QCI statement state 를 INITIALIZED 로 변경
 *
 ***********************************************************************/

    qcStatement  * sStatement;
    smiStatement * sSmiStmtOrg;

    sStatement = &aStatement->statement;

    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, aSmiStmt );

    IDE_TEST( checkExecuteFuncAndSetEnv(
                  aStatement,
                  EXEC_REBUILD ) != IDE_SUCCESS );

    IDE_TEST(qcg::clearStatement( &(aStatement->statement),
                                  ID_TRUE ) // rebuild = TRUE
             != IDE_SUCCESS);

    IDE_TEST( changeStmtState( aStatement, EXEC_INITIALIZE )
              != IDE_SUCCESS );

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}

IDE_RC qci::setPrivateArea( qciStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      qcg::setPrivateArea 함수의 wrapper
 *
 *      mmcStatement::reprepare 에서 reprepare 실패 시 다른 프로토콜
 *      (type bind, data bind) 처리 시 영향이 없도록 private plan 의
 *      template 을 private template 으로 지정해야 할 필요가 있다.
 *      이때 qcg::setPrivateArea 함수를 호출하기 위한 wrapper 역할 함수이다.
 *
 * Implementation :
 *      qcg::setPrivateArea 함수를 호출한다.
 *
 ***********************************************************************/

    // BUG-37297 PRIVATE_TMPLATE 이 존재할때는 해서는 안된다.
    if( QC_PRIVATE_TMPLATE( &(aStatement->statement) ) == NULL )
    {
        IDE_TEST( qcg::setPrivateArea( &(aStatement->statement) )
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing todo.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
} 

IDE_RC qci::passwPolicyCheck( idvSQL *aStatistics, qciUserInfo *aUserInfo )
{
    UInt     sCurrentDate     = 0;
    UInt     sPasswLockTime   = 0;
    UInt     sPasswExpiryDate = 0;
    UInt     sPasswGraceTime  = 0;

    /* BUG-37553 */
    if ( aUserInfo->mIsSysdba == ID_TRUE )
    {
        IDE_TEST_RAISE( aUserInfo->mAccLimitOpts.mUserFailedCnt != 0,
                        IncorrectPassword);

        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing To Do */
    }

    /* GET CURRENT DATE */
    sCurrentDate     = aUserInfo->mAccLimitOpts.mCurrentDate;

    /* GET PASSWORD_EXPIRY_DATE */
    sPasswExpiryDate = aUserInfo->mAccLimitOpts.mPasswExpiryDate;

    /* GET PASSWORD_LOCK_TIME
     * ACCOUNT_LOCK_DATE 일수 + PASSWORD_LOCK_TIME */
    if( ( aUserInfo->mAccLimitOpts.mPasswLockTime != 0 ) &&
        ( aUserInfo->mAccLimitOpts.mLockDate != 0 ) )
    {
        sPasswLockTime = aUserInfo->mAccLimitOpts.mLockDate +
            aUserInfo->mAccLimitOpts.mPasswLockTime;
    }
    else
    {
        // Nothing To Do
    }

    /* GET PASSWORD_GRACE_TIME
     * PASSWORD_EXPIRY_DATE + PASSWORD_GRACE_TIME */
    sPasswGraceTime = sPasswExpiryDate + aUserInfo->mAccLimitOpts.mPasswGraceTime;
   
    if( aUserInfo->mAccLimitOpts.mPasswLimitFlag == QD_PASSWORD_POLICY_ENABLE )
    {       
        /* PASSWORD_LOCK_TIME */
        if( aUserInfo->mAccLimitOpts.mPasswLockTime != 0 )
        {
            if( sCurrentDate >= sPasswLockTime )
            {
                /* LOCKED(TIMED) -> OPEN */
                aUserInfo->mAccLimitOpts.mAccLockStatus = QCI_ACCOUNT_LOCKED_TO_OPEN;
            }
            else
            {
                // Nothing To Do
            }

        }
        else
        {
            // Nothing To DO
        }
        
        if( aUserInfo->mAccLimitOpts.mAccLockStatus !=
            QCI_ACCOUNT_LOCKED_TO_OPEN )
        {
            /* FAILED_LOGIN_ATTEMPTS 설정 횟수 이상이 되어 LOCKED 상태인 경우 ERROR */
            IDE_TEST_RAISE( aUserInfo->mAccLimitOpts.mAccountLock == QD_ACCOUNT_LOCK,
                            AccountStatusLocked );
        }
        else
        {
            // Nothing To Do
        }
        
        /* FAILED_LOGIN_ATTEMPTS */
        if( aUserInfo->mAccLimitOpts.mFailedLoginAttempts != 0 )
        {
            if( aUserInfo->mAccLimitOpts.mUserFailedCnt ==
                aUserInfo->mAccLimitOpts.mFailedLoginAttempts )
            {                      
                /* OPEN -> LOCKED */
                aUserInfo->mAccLimitOpts.mAccLockStatus = QCI_ACCOUNT_OPEN_TO_LOCKED;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // Nothing To Do
        }

        /* UPDATE FILED_LOGIN_COUNT, ACCOUNT_LOCK_DATE */
        IDE_TEST( updatePasswPolicy( aStatistics, aUserInfo )
                  != IDE_SUCCESS);

        /* 설정된 FAILED_LOGIN_COUNT 만큼 connect 실패 한 경우
         * OPEN 상태에서 LOCKED 상태로 변경 되며 ERROR */
        IDE_TEST_RAISE( aUserInfo->mAccLimitOpts.mAccLockStatus ==
                        QCI_ACCOUNT_OPEN_TO_LOCKED,
                        AccountStatusLocked );

        /* password failed */
        IDE_TEST_RAISE( aUserInfo->mAccLimitOpts.mUserFailedCnt != 0,
                        IncorrectPassword);
        
        /* PASSWORD_LIFE_TIME
         * BUG-37443 PASSWORD_GRACE_TIME 만 설정 하였을 경우는 설정과
         * 상관 무시하며 동작 */
        if ( ( sPasswExpiryDate != 0 ) &&
             ( ( aUserInfo->mAccLimitOpts.mPasswLifeTime != 0 ) ||
               ( aUserInfo->mAccLimitOpts.mPasswGraceTime != 0 ) ) )
        {
            /* OPEN -> EXPIRED */
            /* EXPIRED ERROR */
            IDE_TEST_RAISE( sCurrentDate >= sPasswGraceTime,
                            AccountStatusExpired );
        }
        else
        {
            // Nothing To Do
        }        
    }
    else
    {
        /* 명시적 LOCK 인 상태 */
        IDE_TEST_RAISE( aUserInfo->mAccLimitOpts.mLockDate != 0,
                        AccountStatusLocked );
        
        /* password failed */
        IDE_TEST_RAISE( aUserInfo->mAccLimitOpts.mUserFailedCnt != 0,
                        IncorrectPassword);
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(IncorrectPassword);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_PASSWORD_ERROR));
    }
    IDE_EXCEPTION(AccountStatusLocked);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_ACCOUNT_STATUS_LOCKED));
    }
    IDE_EXCEPTION(AccountStatusExpired);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_ACCOUNT_STATUS_EXPIRED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::updatePasswPolicy( idvSQL * aStatistics,  qciUserInfo  * aUserInfo )
{
    smiTrans      sTrans;
    smiStatement  sSmiStmt;
    smiStatement *sDummySmiStmt;
    UInt          sSmiStmtFlag = 0;
        
    UInt          sStage = 0;

    sSmiStmtFlag   = SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR;

    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
    sStage++; //1

    IDE_TEST(sTrans.begin( &sDummySmiStmt, aStatistics )
             != IDE_SUCCESS);
    sStage++; //2

    IDE_TEST(sSmiStmt.begin( aStatistics,
                             sDummySmiStmt,
                             sSmiStmtFlag )
             != IDE_SUCCESS);
    sStage++; //3

    IDE_TEST( qcmUser::updateFailedCount( &sSmiStmt,
                                          aUserInfo )
              != IDE_SUCCESS );     

    IDE_TEST( qcmUser::updateLockDate( &sSmiStmt,
                                       aUserInfo )
              != IDE_SUCCESS );
    
    sStage--; // 2

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sStage--; // 1

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sStage--; // 0

    IDE_TEST(sTrans.destroy( aStatistics ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 3:
                IDE_ASSERT(sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);
            case 2:
                IDE_ASSERT(sTrans.commit() == IDE_SUCCESS);
            case 1:
                IDE_ASSERT(sTrans.destroy( aStatistics ) == IDE_SUCCESS);
            default:
                break;
        }
    }

    return IDE_FAILURE;
}

/* PROJ-2160 CM 타입제거
   fetch시에 필요한 정보를 모아서 준다. */
void qci::getFetchColumnInfo( qciStatement       * aStatement,
                              UShort               aBindId,
                              qciFetchColumnInfo * aFetchColumnInfo)
{
    qcStatement     * sStatement   = NULL;
    qcTemplate      * sTemplate    = NULL;
    mtcStack        * sTargetStack = NULL;
    UInt            * sLocatorInfo = NULL;
    qcSimpleResult  * sResult;
    qmncPROJ        * sPROJ;

    sStatement   = &(aStatement->statement);
    sTemplate    = QC_PRIVATE_TMPLATE(sStatement);
    sTargetStack = &(sTemplate->tmplate.stack[aBindId]);

    // PROJ-2551 simple query 최적화
    // simple query인 경우 fast execute를 수행한다.
    if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        sResult = sStatement->simpleInfo.results;
        sPROJ = (qmncPROJ*)sStatement->myPlan->plan;
        
        aFetchColumnInfo->value = (UChar*)sResult->result +
            sResult->idx * sPROJ->simpleResultSize +
            sPROJ->simpleResultOffsets[aBindId];
        aFetchColumnInfo->dataTypeId =
            sPROJ->simpleValues[aBindId].column.type.dataTypeId;
    }
    else
    {
        aFetchColumnInfo->value      = (UChar*)sTargetStack->value;
        aFetchColumnInfo->dataTypeId = sTargetStack->column->type.dataTypeId;
    }

    // BUG-40427
    // 더 이상 내부적으로 사용하는 LOB Cursor가 아님을 표시한다.
    if ( (aFetchColumnInfo->dataTypeId == MTD_BLOB_LOCATOR_ID) ||
         (aFetchColumnInfo->dataTypeId == MTD_CLOB_LOCATOR_ID) )
    {
        if ( *(smLobLocator *)(sTargetStack->value) != MTD_LOCATOR_NULL )
        {
            // BUG-40427
            // 내부적으로 사용하는 LOB Cursor가 아님을 표시한다.
            // BUG-40840 left outer join 에서 null row 고려
            if (smiLob::getInfoPtr( *(smLobLocator*)sTargetStack->value,
                                    &sLocatorInfo )
                == IDE_SUCCESS)
            {
                *sLocatorInfo &= ~MTC_LOB_LOCATOR_CLIENT_MASK; 
                *sLocatorInfo |=  MTC_LOB_LOCATOR_CLIENT_TRUE; 
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // Nothing to do.
    }
}

/*
 * PROJ-1832 New database link
 */ 
IDE_RC qci::setDatabaseLinkCallback( qciDatabaseLinkCallback * aCallback )
{
    IDE_RC sRet;

    sRet = qdkSetDatabaseLinkCallback( aCallback );

    return sRet;
}

/*
 * PROJ-1832 New database link
 */ 
IDE_RC qci::setRemoteTableCallback( qciRemoteTableCallback * aCallback )
{
    IDE_RC sRet;

    sRet = qmrSetRemoteTableCallback( aCallback );

    return sRet;
}

/* BUG-49194 [mm-altiaudit] INC-45592 방어코드 추가 */
void qci::getAllRefObjectCount( qciStatement      * aStatement, 
                                UInt              * aRefObjectCount )
{
    qcStatement  *sQCStatement = &(aStatement->statement);

    if ( sQCStatement->myPlan != NULL)
    {
        if ( sQCStatement->myPlan->planEnv == NULL)
        {
            *aRefObjectCount = 0;
        }
        else
        {
            *aRefObjectCount = sQCStatement->myPlan->planEnv->auditInfo.refObjectCount;
        }
    }
    else
    {
        *aRefObjectCount = 0;
    }
}

void qci::getAllRefObjects( qciStatement       * aStatement,
                            qciAuditRefObject ** aRefObjects,
                            UInt               * aRefObjectCount )
{
    qcStatement  * sStatement = NULL;

    sStatement = &(aStatement->statement);
    
    if( ( ( aStatement->flag & QCI_STMT_AUDIT_MASK ) == QCI_STMT_AUDIT_TRUE )
        &&
        ( sStatement->myPlan->planEnv != NULL ) )
    {
        *aRefObjects     = sStatement->myPlan->planEnv->auditInfo.refObjects;
        *aRefObjectCount = sStatement->myPlan->planEnv->auditInfo.refObjectCount;
    }
    else
    {
        *aRefObjects     = NULL;
        *aRefObjectCount = 0;
    }
}

void qci::getAuditOperation( qciStatement     * aStatement,
                             qciAuditOperIdx  * aOperIndex,
                             const SChar     ** aOperString )
{
    qcStatement  * sStatement = NULL;

    sStatement = &(aStatement->statement);

    if( ( aStatement->flag & QCI_STMT_AUDIT_MASK ) == QCI_STMT_AUDIT_TRUE )
    {
        if( sStatement->myPlan->planEnv != NULL )
        {
            *aOperIndex = sStatement->myPlan->planEnv->auditInfo.operation;

            qdcAudit::getOperationString( *aOperIndex, aOperString );
        }
        else
        {
            qdcAudit::getOperation( sStatement, aOperIndex);
            
            qdcAudit::getOperationString( *aOperIndex, aOperString );
        }
    }
}

/* BUG-41986 User connection info can be audited */
void qci::getAuditOperationByOperID( qciAuditOperIdx  aOperIndex, 
                                     const SChar    **aOperString )
{
    qdcAudit::getOperationString( aOperIndex, aOperString );
}

/* BUG-36205 Plan Cache On/Off property for PSM */
idBool qci::isCalledByPSM( qciStatement * aStatement )
{
    idBool sRet;

    sRet = aStatement->statement.calledByPSMFlag;

    return sRet;
}

/* **********************************************************************
 * BUG-38496 Notify users when their password expiry date is approaching 
 *
 * This returns the remaining grace period of a user. 
 * @param [in] aUserInfo 
 * @return 0    : unlimited 
 *         else : the remaining days
 * @see : qcm/qcmCreate.cpp
 * ********************************************************************* */
UInt qci::getRemainingGracePeriod( qciUserInfo * aUserInfo )
{
    UInt             sExpiryGraceDate = 0;
    UInt             sRemainingDays = 0;
    qciAccLimitOpts *sPasswdInfo = NULL;

    sPasswdInfo = &(aUserInfo->mAccLimitOpts);
    
    sExpiryGraceDate = sPasswdInfo->mPasswExpiryDate + sPasswdInfo->mPasswGraceTime;

    if( ( sPasswdInfo->mCurrentDate >= sPasswdInfo->mPasswExpiryDate )
     && ( sPasswdInfo->mCurrentDate <= sExpiryGraceDate ) )
    {
        sRemainingDays = sExpiryGraceDate - sPasswdInfo->mCurrentDate;
    }

    return sRemainingDays;
}

/* **********************************************************************
 * BUG-39441
 * need a interface which returns whether DML on replication table or not
 *
 * INSERT/UPDATE/DELETE/MOVE/MERGE stmt 에 대해서
 * replication 이 걸려있는 table 에 수행된 경우 true 아니면 false
 * ********************************************************************* */
idBool qci::hasReplicationTable(qciStatement* aStatement)
{
    idBool      sResult;
    qcTemplate* sTemplate;

    sTemplate = QC_PRIVATE_TMPLATE(&aStatement->statement);

    if (sTemplate == NULL)
    {
        sTemplate = QC_SHARED_TMPLATE(&aStatement->statement);
    }
    else
    {
        /* nothing to do */
    }

    if ((sTemplate->flag & QC_TMP_REF_REPL_TABLE_MASK) ==
        QC_TMP_REF_REPL_TABLE_TRUE)
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

/**
 * PROJ-2474 SSL/TLS Support
 *
 * ---------------------------------------------------------
 *   UseSecureTCP | Connect Type |  Normal User
 * ---------------------------------------------------------
 *        0 or 1  | IPC/UNIX     |     O
 *                -------------------------------------------
 *                | Secure TCP   |     X
 *                -------------------------------------------
 *                | TCP          |   Optional( O or X )
 * ---------------------------------------------------------
 *
 * 위 표에서와 같이 User의 DISABLE_TCP 옵션에
 * 따라 접속의 제한한다.
 *
 */
IDE_RC qci::checkDisableTCPOption( qciUserInfo *aUserInfo )
{
    if ( aUserInfo->mConnectType == QCI_CONNECT_TCP )
    {
        /* User의 connectType이 TCP이고 DisableTCP Option이 켜져있다면
         * 접속을 제한한다.
         */
        IDE_TEST_RAISE( ( aUserInfo->mDisableTCP == QCI_DISABLE_TCP_TRUE ) ||
                        ( aUserInfo->mDisableTCP == QCI_DISABLE_TCP_NULL ),
                        DISABLED_TCP_CONNECTION );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( DISABLED_TCP_CONNECTION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCI_DISABLED_TCP ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qci::isSimpleQuery( qciStatement * aStatement )
{
    qcStatement  * sStatement = NULL;

    sStatement = &aStatement->statement;

    // PROJ-2551 simple query 최적화
    if ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
         == QC_STMT_FAST_EXEC_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

IDE_RC qci::fastExecute( smiTrans      * aSmiTrans,
                         qciStatement  * aStatement,
                         UShort        * aBindColInfo,
                         UChar         * aBindBuffer,
                         UInt            aShmSize,
                         UChar         * aShmResult,
                         UInt          * aRowCount )
{
    qcStatement     * sStatement;
    smiStatement    * sSmiStmtOrg;
    UChar           * sShmResult = NULL;
    UInt              sShmSize = 0;
    UInt              sResultSize = 0;

    sStatement = &aStatement->statement;
    
    qcg::getSmiStmt( sStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( sStatement, NULL );

    // qmxMem을 기록하고 재사용한다.
    if ( sStatement->simpleInfo.status.mSavedCurr == NULL )
    {
        IDE_TEST_RAISE( sStatement->qmxMem->getStatus(
                            &(sStatement->simpleInfo.status) )
                        != IDE_SUCCESS,
                        ERR_MEM_OP );
    }
    else
    {
        IDE_TEST_RAISE( sStatement->qmxMem->setStatus(
                            &(sStatement->simpleInfo.status) )
                        != IDE_SUCCESS,
                        ERR_MEM_OP );
    }
    
    if ( aShmResult != NULL )
    {
        // 8보다 커야함
        IDE_TEST_RAISE( aShmSize < ID_SIZEOF(ULong),
                        ERR_INVALID_SHM_SIZE );
        // 8byte align이어야 함
        IDE_TEST_RAISE( idlOS::align8( (ULong)(vULong)aShmResult )
                        != (ULong)(vULong)aShmResult,
                        ERR_INVALID_SHM_ADDRESS );
        
        sShmResult = aShmResult + ID_SIZEOF(ULong);
        sShmSize = aShmSize - ID_SIZEOF(ULong);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmxSimple::fastExecute( aSmiTrans,
                                      sStatement,
                                      aBindColInfo,
                                      aBindBuffer,
                                      sShmSize,
                                      sShmResult,
                                      & sResultSize,
                                      aRowCount )
              != IDE_SUCCESS );
    
    // result size를 첫 8byte에 기록한다.
    if ( aShmResult != NULL )
    {
        *(ULong*)aShmResult = (ULong)sResultSize;
    }
    else
    {
        // Nothing to do.
    }
    
    IDE_TEST( changeStmtState( aStatement, EXEC_EXECUTE )
              != IDE_SUCCESS );
    
    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qci::fastExecute"
                     " memory error" );
    }
    IDE_EXCEPTION( ERR_INVALID_SHM_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::fastExecute",
                                  "invalid shm size" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHM_ADDRESS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::fastExecute",
                                  "invalid shm address" ) );
    }
    IDE_EXCEPTION_END;

    qcg::setSmiStmt( sStatement, sSmiStmtOrg );

    return IDE_FAILURE;
}

idBool qci::isBindParamEnd( qciStatement * aStatement )
{
    UInt          i = 0;
    idBool        sBindEnd = ID_TRUE;
    qcStatement * sStatement;      

    sStatement = & aStatement->statement;

    for ( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        if ( sStatement->pBindParam[i].isParamInfoBound == ID_FALSE )
        {
            sBindEnd = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sBindEnd;
}

idBool qci::isBindDataEnd( qciStatement * aStatement )
{
    UInt          i = 0;
    idBool        sBindEnd = ID_TRUE;
    qcStatement * sStatement; 

    sStatement = & aStatement->statement;

    for ( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        if ( sStatement->pBindParam[i].isParamDataBound == ID_FALSE )
        {
            sBindEnd = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sBindEnd;
}

IDE_RC qci::setBindParamInfoByName( qciStatement  * aStatement,
                                    qciBindParam  * aBindParam )
{
    qcStatement * sStatement;
    qcBindNode  * sBindNode;
    UInt          sBindNameSize = 0;
    idBool        sFound = ID_FALSE;

    sBindNameSize = idlOS::strlen( aBindParam->name );

    sStatement = & aStatement->statement;

    for ( sBindNode = sStatement->myPlan->bindNode;
          sBindNode != NULL;
          sBindNode = sBindNode->mNext )
    {
        if ( idlOS::strCaselessMatch( aBindParam->name,
                             sBindNameSize,
                             sBindNode->mVarNode->columnName.stmtText
                                + sBindNode->mVarNode->columnName.offset + 1,
                             sBindNode->mVarNode->columnName.size - 1 ) == 0 )

        {
            aBindParam->id = sBindNode->mVarNode->node.column;

            IDE_TEST( qci::setBindParamInfoByNameInternal( aStatement,
                                                           aBindParam ) != IDE_SUCCESS );                                          

            sFound = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( sFound == ID_FALSE, err_invalid_binding );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::setBindParamInfoByNameInternal( qciStatement  * aStatement,
                                            qciBindParam  * aBindParam )
{
    qcStatement    * sStatement;
    qciBindParam     sBindParam;

    UInt             i;
    idBool           sBindEnd = ID_TRUE;

    sStatement = & aStatement->statement;

    IDE_TEST_RAISE( aBindParam->id >= sStatement->pBindParamCount,
                    err_invalid_binding );

    // PROJ-2002 Column Security
    // 암호 타입으로는 바인드할 수 없음
    IDE_TEST_RAISE( ( (aBindParam->type == MTD_ECHAR_ID) ||
                      (aBindParam->type == MTD_EVARCHAR_ID) ),
                    err_invalid_binding );

    // INPUT or INPUT_OUTPUT type의 마지막 ParamInfo의 Id를 기록
    switch (aBindParam->inoutType)
    {
        case CMP_DB_PARAM_INPUT_OUTPUT:
        {
            if ( sStatement->calledByPSMFlag == ID_TRUE )
            {
                IDE_TEST_RAISE( ( (aBindParam->type == MTD_BLOB_LOCATOR_ID) ||
                                  (aBindParam->type == MTD_CLOB_LOCATOR_ID) ),
                                err_invalid_binding );
            }
            else
            {
                // lob value, lob locator 모두 불가능
                IDE_TEST_RAISE( ( (aBindParam->type == MTD_BLOB_ID) ||
                                  (aBindParam->type == MTD_CLOB_ID) ||
                                  (aBindParam->type == MTD_BLOB_LOCATOR_ID) ||
                                  (aBindParam->type == MTD_CLOB_LOCATOR_ID) ),
                                err_invalid_binding );
            }
        }
        /* fall through */
        case CMP_DB_PARAM_INPUT:
        {
            sStatement->bindParamDataInLastId = aBindParam->id;
            break;
        }
        case CMP_DB_PARAM_OUTPUT:
        {
            break;
        }
        default:
        {
            IDE_DASSERT( 0 );
            break;
        }
    }

    // bind param 정보를 변경한다.

    sBindParam.id            = aBindParam->id;
    sBindParam.type          = aBindParam->type;
    sBindParam.language      = aBindParam->language;
    sBindParam.arguments     = aBindParam->arguments;
    sBindParam.precision     = aBindParam->precision;
    sBindParam.scale         = aBindParam->scale;
    sBindParam.inoutType     = aBindParam->inoutType;
    sBindParam.data          = NULL;
    sBindParam.dataSize      = 0; // PROJ-2163
    sBindParam.ctype         = 0;
    sBindParam.sqlctype      = 0;

    sStatement->pBindParam[aBindParam->id].param = sBindParam;

    // 바인드 되었음.
    sStatement->pBindParam[aBindParam->id].isParamInfoBound = ID_TRUE;

    // convert 정보를 초기화한다.
    // memory 낭비가 있지만 많지 않다.
    sStatement->pBindParam[aBindParam->id].convert = NULL;

    // canonize buffer를 초기화한다.
    // memory 낭비가 있지만 많지 않다.
    sStatement->pBindParam[aBindParam->id].canonBuf = NULL;

    // PROJ-2163
    // pBindParam 의 값이 바뀌었다.
    sStatement->pBindParamChangedFlag = ID_TRUE;

    // PROJ-2163
    for ( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        // BUG-15878
        if ( sStatement->pBindParam[i].isParamInfoBound == ID_FALSE )
        {
            sBindEnd = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sBindEnd == ID_TRUE )
    {
        // 호스트 변수의 값을 저장할 데이터 영역을 초기화 한다.
        // 이 영역은 variable tuple 의 row 로도 사용된다.
        IDE_TEST_RAISE( qcg::initBindParamData( sStatement ) != IDE_SUCCESS,
                        err_invalid_binding_init_data );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION( err_invalid_binding_init_data );
    {
        // BUG-37405
        for ( i = 0; i < sStatement->pBindParamCount; i++ )
        {
            sStatement->pBindParam[i].isParamInfoBound = ID_FALSE;
        }
    }
    IDE_EXCEPTION_END;

    // PROJ-2163
    sStatement->pBindParam[aBindParam->id].isParamInfoBound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qci::setBindParamDataByName( qciStatement  * aStatement,
                                    SChar         * aBindName,
                                    void          * aData,
                                    UInt            aDataSize )
{
    qcStatement * sStatement;
    qcBindNode  * sBindNode;
    UInt          sBindNameSize = 0;
    idBool        sFound = ID_FALSE;

    sBindNameSize = idlOS::strlen( aBindName );

    sStatement = & aStatement->statement;

    IDE_TEST_RAISE( qci::isBindParamEnd( aStatement ) == ID_FALSE, err_invalid_binding ); 

    for ( sBindNode = sStatement->myPlan->bindNode;
          sBindNode != NULL;
          sBindNode = sBindNode->mNext )
    {
        if ( idlOS::strCaselessMatch( aBindName,
                             sBindNameSize,
                             sBindNode->mVarNode->columnName.stmtText
                                + sBindNode->mVarNode->columnName.offset + 1,
                             sBindNode->mVarNode->columnName.size - 1 ) == 0 )

        {
            IDE_TEST( qci::setBindParamDataByNameInternal( aStatement,
                                                           sBindNode->mVarNode->node.column,
                                                           aData,
                                                           aDataSize ) != IDE_SUCCESS );                                          

            sFound = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( sFound == ID_FALSE, err_invalid_binding );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::setBindParamDataByNameInternal( qciStatement  * aStatement,
                                            UShort          aBindId,
                                            void          * aData,
                                            UInt            aDataSize )
{
    qcStatement      * sStatement;
    qciBindParam     * sBindParam;
    idBool             sBindEnd = ID_TRUE;
    UInt               i = 0;

    sStatement = & aStatement->statement;

    if ( aBindId == 0 )
    {
        IDE_TEST( checkExecuteFuncAndSetEnv( aStatement, EXEC_BIND_PARAM_DATA )
                != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    IDE_TEST_RAISE( aBindId >= sStatement->pBindParamCount,
                    err_invalid_binding );

    // 바인드되지 않았다면 에러
    IDE_TEST_RAISE( sStatement->pBindParam[aBindId].isParamInfoBound == ID_FALSE,
                    err_invalid_binding );

    sBindParam = & sStatement->pBindParam[aBindId].param;

    // in, inout 타입만 가능하다.
    IDE_TEST_RAISE( sBindParam->inoutType == CMP_DB_PARAM_OUTPUT,
                    err_invalid_binding );

    // PROJ-2163 
    // copy data to pBindParam->param.data (variableTuple's row)
    IDE_TEST( qcg::setBindData( sStatement,
                                aBindId,
                                sBindParam->inoutType,
                                aDataSize,
                                aData )
              != IDE_SUCCESS );

    // fix BUG-16482
    sStatement->pBindParam[aBindId].isParamDataBound = ID_TRUE;

    for ( i = 0; i < sStatement->pBindParamCount; i++ )
    {
        // BUG-15878
        if ( sStatement->pBindParam[i].isParamDataBound == ID_FALSE )
        {
            sBindEnd = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sBindEnd == ID_TRUE )
    {
        // BUG-34995
        // QCI_STMT_STATE 를 DATA_BOUND 로 변경한다.
        IDE_TEST( qci::setParamDataState( aStatement ) != IDE_SUCCESS);

    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qci::checkBindParamByName( qciStatement * aStatement,
                           SChar        * aBindName )
{
    qcStatement * sStatement;
    qcBindNode  * sBindNode;
    UInt          sBindNameSize = 0;

    sBindNameSize = idlOS::strlen( aBindName );

    sStatement = & aStatement->statement;

    for ( sBindNode = sStatement->myPlan->bindNode;
          sBindNode != NULL;
          sBindNode = sBindNode->mNext )
    {
        if ( idlOS::strCaselessMatch( aBindName,
                             sBindNameSize,
                             sBindNode->mVarNode->columnName.stmtText
                                + sBindNode->mVarNode->columnName.offset + 1,
                             sBindNode->mVarNode->columnName.size - 1 ) == 0 )

        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( sBindNode == NULL, err_invalid_binding );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::checkBindInfo( qciStatement *aStatement )
{
/***********************************************************************
 *
 * Description : BUG-42512 plan 의 bind 정보와 statement 의 bind 정보를 검증
 *
 * Implementation : plan cache hit 된 후 private template 이 생성된 직후 호출
 *                  - clonePrivateTemplate
 *                  - mmcStatement::reprepare()
 *
 ***********************************************************************/

    qcStatement  * sStatement = NULL;
    qcTemplate   * sTemplate  = NULL;
    UShort         sBindTuple = 0;
    mtcColumn    * sSrcColumn = NULL;
    UShort         sBindCount = 0;
    qciBindParam * sBindParam = NULL;
    UInt           i = 0;

    sStatement = & aStatement->statement;

    if ( sStatement->pBindParam != NULL )
    {
        if ( sStatement->pBindParam->isParamInfoBound == ID_TRUE )
        {
            sTemplate  = QC_PRIVATE_TMPLATE(sStatement);
            sBindTuple = sTemplate->tmplate.variableRow;
            sBindCount = qcg::getBindCount( sStatement );

            for ( i = 0; i < sBindCount; i++ )
            {
                sSrcColumn = & sTemplate->tmplate.rows[sBindTuple].columns[i];
                sBindParam = & sStatement->pBindParam[i].param;

                IDE_TEST_RAISE( sSrcColumn->type.dataTypeId != sBindParam->type,
                                ERR_ABORT_INVALID_COLUMN_TYPE );

                IDE_TEST_RAISE( sSrcColumn->precision != sBindParam->precision,
                                ERR_ABORT_INVALID_COLUMN_PRECISION );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_COLUMN_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::checkBindInfo",
                                  "mismatch column type" ) );
    }
    IDE_EXCEPTION( ERR_ABORT_INVALID_COLUMN_PRECISION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qci::checkBindInfo",
                                  "mismatch column precision" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2626 Snapshot Export */
void qci::setInplaceUpdateDisable( idBool aTrue )
{
    mInplaceUpdateDisable = aTrue;
}

idBool qci::getInplaceUpdateDisable( void )
{
    return mInplaceUpdateDisable;
}

IDE_RC qci::getShardAnalyzeInfo( qciStatement         * aStatement,
                                 qciShardAnalyzeInfo ** aAnalyzeInfo )
{
    qciShardAnalyzeInfo * sAnalyzeInfo = NULL;

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aAnalyzeInfo != NULL );

    sAnalyzeInfo = aStatement->statement.myPlan->mShardAnalysis;

    IDE_TEST_RAISE( sAnalyzeInfo == NULL, ERR_ANALYZE_INFO );

    *aAnalyzeInfo = sAnalyzeInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ANALYZE_INFO )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qci::getShardAnalyzeInfo",
                                "AnalyzeInfo is NULL"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::checkShardPlanRebuild( qcStatement * aStatement )
{
    qcPlanProperty * sPlanProperty = NULL;
    qciStmtType      sStmtType;

    sStmtType = aStatement->myPlan->parseTree->stmtKind;

    /* SELECT FROM (Fixed table) 과 같은 DCL 이 Rebuild 되는 경우에
     * doRebuild 에서 트랜잭션이 begin 되어 있지 않아 fault 발생하는것을
     * 임시로!!! 예외처리 한다 */
    if( qciMisc::isStmtDCL( sStmtType ) == ID_FALSE )
    {
        if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
             != QCI_STMT_SHARD_DDL )
        {
            //------------------------------------------
            // SMN이 변경 되었다면 rebuild를 통해 plan을 재생성 한다.
            //------------------------------------------
            if ( aStatement->myPlan->planEnv != NULL )
            {
                sPlanProperty = &(aStatement->myPlan->planEnv->planProperty);

                IDE_TEST_RAISE( ( sPlanProperty->mSMNForDataNodeRef == ID_TRUE ) &&
                                ( sPlanProperty->mSMNForDataNode !=
                                  sdi::getSMNForDataNode() ),
                                ERR_REBUILD_QCI_EXEC );

                IDE_TEST_RAISE( ( sPlanProperty->mSMNForSessionRef == ID_TRUE ) &&
                                ( sPlanProperty->mSMNForSession !=
                                  QCG_GET_SESSION_SHARD_META_NUMBER(aStatement) ),
                                ERR_REBUILD_QCI_EXEC );
            }
            else
            {
                // Nothing to do.
            }

            //------------------------------------------
            // Rebuild transformation에 의한 dummy plan일 경우
            // rebuild를 통해 plan을 재생성한다.
            //------------------------------------------
            IDE_TEST_RAISE( ( aStatement->mFlag & QC_STMT_SHARD_REBUILD_FORCE_MASK ) ==
                            QC_STMT_SHARD_REBUILD_FORCE_TRUE,
                            ERR_REBUILD_QCI_EXEC );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REBUILD_QCI_EXEC );
    {
        IDE_SET(ideSetErrorCode(qpERR_REBUILD_QCI_EXEC));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

qcStatement * qci::getSelectStmtOfDDL( qcStatement * aStatement )
{
    qdTableParseTree * sParseTree = NULL;
    qcStatement      * sSelect = NULL;

    if ( ( getStartupPhase() == QCI_STARTUP_SERVICE ) &&
         ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SCHEMA_DDL ) )
    {
        sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
        if ( sParseTree->common.parse == qdbCreate::parseCreateTableAsSelect )
        {
            if ( sParseTree->select->myPlan->parseTree->stmtKind == QCI_STMT_SELECT )
            {
                sSelect = sParseTree->select;
            }
        }
    }

    return sSelect;
}

// BUG-47790
IDE_RC qci::reloadShardMetaNumber( qciStatement * aStatement,
                                   void         * aSession )
{
    qcStatement * sStatement = &aStatement->statement;

    if (( sStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
        QC_SESSION_SHARD_DDL_TRUE )
    {
        if ( sStatement->session->mQPSpecific.mClientInfo != NULL )
        {
            IDE_TEST( qci::mSessionCallback.mReloadShardMetaNumber(
                          aSession,
                          ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qci::mSessionCallback.mReloadShardMetaNumber(
                          aSession,
                          ID_TRUE )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // nothing to do
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qci::setPropertyForShardMeta( qciStatement *aStatement )
{
    qcStatement * sStatement = &aStatement->statement;
    SChar         sSqlStr[QD_MAX_SQL_LENGTH + 1];

    if ( QCG_GET_SESSION_TRANSACTIONAL_DDL( sStatement) == ID_TRUE )
    {
        sStatement->session->mBakSessionProperty.mTransactionalDDL = 1;
    }
    else
    {
        sStatement->session->mBakSessionProperty.mTransactionalDDL = 0;
    }
    
    if ( QCG_GET_SESSION_TRANSACTIONAL_DDL( sStatement) != ID_TRUE )
    {   
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER SESSION SET TRANSACTIONAL_DDL = 1 " );

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sSqlStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }

    sStatement->session->mBakSessionProperty.mGlobalTransactionLevel = QCG_GET_SESSION_GTX_LEVEL( sStatement );    
    
    if ( QCG_GET_SESSION_GTX_LEVEL( sStatement ) < 2 )
    {        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER SESSION SET  GLOBAL_TRANSACTION_LEVEL = 2 " );

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sSqlStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qci::revertPropertyForShardMeta( qciStatement *aStatement )
{
    qcStatement * sStatement = &aStatement->statement;
    SChar         sSqlStr[QD_MAX_SQL_LENGTH + 1];

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "ALTER SESSION SET TRANSACTIONAL_DDL = %"ID_INT32_FMT" ",
                     sStatement->session->mBakSessionProperty.mTransactionalDDL );

    IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                          sSqlStr,
                                          sStatement->session->mMmSession )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "ALTER SESSION SET  GLOBAL_TRANSACTION_LEVEL = %"ID_INT32_FMT" ",
                     sStatement->session->mBakSessionProperty.mGlobalTransactionLevel );

    IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                          sSqlStr,
                                          sStatement->session->mMmSession )
              != IDE_SUCCESS );

    /* BUG-49129 shard DDL시session에 internal operation이 세팅되는데 DDL 정리시 해제해줘야 한다. */
    IDE_TEST( qci::mSessionCallback.mSetShardInternalLocalOperation( sStatement->session->mMmSession,
                                                                     SDI_INTERNAL_OP_NOT ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * BUG-48224
 * v$plantext 에서 plan 출력시 graph 정보는 출력하면 않된다.
 * graph는 optimize과정을 거져야하지만 v$plantext  은 plan
 * 의 text만 출력해야하는 데 의도치 않게 graph가 같이 출력됨.
 */
IDE_RC qci::getPlanTreeTextForFixedTable( qciStatement * aStatement,
                                          iduVarString * aString,
                                          idBool         aIsCodeOnly )
{
    UInt           sStage = 0;
    qcStatement  * sStatement = &(aStatement->statement);

    qcg::lock( sStatement );

    sStage = 1;

    if ( qcg::getPlanTreeState( sStatement ) == ID_TRUE )
    {
        IDE_TEST( qci::makePlanTreeText( aStatement,
                                         aString,
                                         aIsCodeOnly,
                                         ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    sStage = 0;

    qcg::unlock( sStatement );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        qcg::unlock( sStatement );
    }

    return IDE_FAILURE;
}

idBool qci::isShardDbmsPkg( qciStatement * aStatement )
{
    qcStatement * sStatement = &aStatement->statement;
    idBool        sResult = ID_FALSE;
    
    if (( sStatement->mFlag & QC_STMT_SHARD_DBMS_PKG_MASK )
        == QC_STMT_SHARD_DBMS_PKG_TRUE )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

/* TASK-7219 Non-shard DML */
IDE_RC qci::setShardPartialExecType( qciStatement            * aStatement,
                                     sdiShardPartialExecType   aShardPartialExecType )
{
    aStatement->statement.mShardPartialExecType = aShardPartialExecType;
    
    return IDE_SUCCESS;
}
