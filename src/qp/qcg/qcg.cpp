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
 * $Id: qcg.cpp 17317 2006-07-27 06:01:46Z peh $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <iduMemPool.h>
#include <idx.h>
#include <cm.h>
#include <mtdTypes.h>
#include <mtuProperty.h>
#include <qc.h>
#include <qcm.h>
#include <qcg.h>
#include <qmcThr.h>
#include <qcpManager.h>
#include <qsvEnv.h>
#include <qsxEnv.h>
#include <qmnProject.h>
#include <qmnDelete.h>
#include <qsxCursor.h>
#include <qsx.h>
#include <qcuProperty.h>
#include <qdnTrigger.h>
#include <qmv.h>
#include <qmvWith.h>
#include <qcuSessionObj.h>
#include <qmg.h>
#include <qsf.h>
#include <qsxExecutor.h>
#include <qcmTableInfo.h>
#include <qsxRelatedProc.h>
#include <qcmSynonym.h>
#include <qdpPrivilege.h>
#include <qcgPlan.h>
#include <qcsModule.h>
#include <qcuTemporaryObj.h>
#include <mtlTerritory.h>
#include <qsc.h>
#include <qtcCache.h>
#include <qmxResultCache.h>
#include <qcuSessionPkg.h>
#include <qcd.h>
#include <sdi.h>
#include <qsxLibrary.h>
#include <qrc.h>
#include <sdiGlobalDDL.h>

extern mtdModule mtdBigint;

qcgDatabaseCallback qcg::mStartupCallback;
qcgDatabaseCallback qcg::mShutdownCallback;
qcgDatabaseCallback qcg::mCreateCallback;
qcgDatabaseCallback qcg::mDropCallback;
qcgDatabaseCallback qcg::mCloseSessionCallback;
qcgDatabaseCallback qcg::mCommitDTXCallback;
qcgDatabaseCallback qcg::mRollbackDTXCallback;

// Proj-1360 Queue
qcgQueueCallback    qcg::mCreateQueueFuncPtr;
qcgQueueCallback    qcg::mDropQueueFuncPtr;
qcgQueueCallback    qcg::mEnqueueQueueFuncPtr;
//PROJ-1677 DEQUEUE
qcgDeQueueCallback  qcg::mDequeueQueueFuncPtr;
qcgQueueCallback    qcg::mSetQueueStampFuncPtr;

UInt    qcg::mInternalUserID = QC_SYSTEM_USER_ID;
SChar * qcg::mInternalUserName = QC_SYSTEM_USER_NAME;
idBool  qcg::mInitializedMetaCaches = ID_FALSE;
/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
/* To reduce the number of system calls and eliminate the contention
   on iduMemMgr::mClientMutex[IDU_MEM_QCI] in the moment of calling iduMemMgr::malloc/free. */
iduMemPool qcg::mIduVarMemListPool;
iduMemPool qcg::mIduMemoryPool;
iduMemPool qcg::mSessionPool;
iduMemPool qcg::mStatementPool;

/* PROJ-1071 Parallel Query */
qcgPrlThrUseCnt qcg::mPrlThrUseCnt;

/* PROJ-2451 Concurrent Execute Package */
qcgPrlThrUseCnt qcg::mConcThrUseCnt;

IDE_RC
qcg::allocStatement( qcStatement * aStatement,
                     qcSession   * aSession,
                     qcStmtInfo  * aStmtInfo,
                     idvSQL      * aStatistics )
{
/***********************************************************************
 *
 * Description :
 *    qcStatement의 멤버를 초기화하고 Memory 공간이 필요한 member에
 *    대하여 memory를 할당한다.
 *
 *    SQLCLI의 개념 상 SQLAllocStmt()에 해당하는 처리 부분이나,
 *    실제 CLI에서는 해당 함수 호출시 Server로 접근하지 않으며,
 *    다음과 같은 함수가 최초 호출 시 실제 Statement를 할당한다.
 *        - SQLExecDirect() :
 *        - SQLPrepare() :
 *
 * Implementation :
 *
 ***********************************************************************/

    void           * sAllocPtr;
    UInt             sMutexState = 0;
    qcSession      * sSession = NULL;
    qcStmtInfo     * sStmtInfo = NULL;
    idBool           sIsQmpInited = ID_FALSE;
    idBool           sIsQmeInited = ID_FALSE;
    idBool           sIsQmbInited = ID_FALSE;
    idBool           sIsQixInited = ID_FALSE;

    // PROJ-1436
    aStatement->myPlan                   = & aStatement->privatePlan;
    aStatement->myPlan->planEnv          = NULL;
    aStatement->myPlan->stmtText         = NULL;
    aStatement->myPlan->stmtTextLen      = 0;
    aStatement->myPlan->mPlanFlag        = QC_PLAN_FLAG_INIT;
    aStatement->myPlan->mHintOffset      = NULL;
    aStatement->myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    aStatement->myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    aStatement->myPlan->qmpMem           = NULL;
    aStatement->myPlan->qmuMem           = NULL;
    aStatement->myPlan->parseTree        = NULL;
    aStatement->myPlan->hints            = NULL;
    aStatement->myPlan->plan             = NULL;
    aStatement->myPlan->graph            = NULL;
    // PROJ-1446 Host variable을 포함한 질의 최적화
    // qcStatement에 scanDecisionFactors 멤버가 추가됨
    aStatement->myPlan->scanDecisionFactors = NULL;
    aStatement->myPlan->procPlanList = NULL;
    aStatement->myPlan->mShardAnalysis = NULL;
    aStatement->myPlan->mShardParamInfo = NULL; /* TASK-7219 Non-shard DML*/
    aStatement->myPlan->mShardParamOffset = ID_USHORT_MAX;
    aStatement->myPlan->mShardParamCount = 0;

    aStatement->myPlan->sBindColumn = NULL;
    aStatement->myPlan->sBindColumnCount = 0;
    aStatement->myPlan->sBindParam  = NULL;
    aStatement->myPlan->sBindParamCount = 0;
    aStatement->myPlan->stmtListMgr = NULL;

    // BUG-41248 DBMS_SQL package
    aStatement->myPlan->bindNode = NULL;

    aStatement->prepTemplateHeader  = NULL;
    aStatement->prepTemplate        = NULL;

    aStatement->qmeMem = NULL;
    aStatement->qmxMem = NULL;
    aStatement->qmbMem = NULL;
    aStatement->qmtMem = NULL;
    aStatement->qxcMem = NULL;  // PROJ-2452
    aStatement->qixMem = NULL;  // PROJ-2717 Internal procedure
    aStatement->pTmplate = NULL;
    aStatement->session = NULL;
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    aStatement->mutexpoolFlag= ID_FALSE;
    aStatement->allocFlag    = ID_FALSE;
    aStatement->planTreeFlag = ID_FALSE;
    aStatement->sharedFlag   = ID_FALSE;
    aStatement->disableLeftStore = ID_FALSE;
    aStatement->pBindParam = NULL;
    aStatement->pBindParamCount = 0;
    aStatement->bindParamDataInLastId = 0;
    aStatement->stmtInfo = NULL;
    // PROJ-2163 : add flag of bind type change
    aStatement->pBindParamChangedFlag = ID_FALSE;

    // BUG-36986
    aStatement->namedVarNode = NULL;

    /* BUG-38294 */
    aStatement->mPRLQMemList = NULL;
    aStatement->mPRLQChdTemplateList = NULL;

    // PROJ-2551 simple query 최적화
    qciMisc::initSimpleInfo( &(aStatement->simpleInfo) );
    aStatement->mFlag = 0;

    // BUG-43158 Enhance statement list caching in PSM
    aStatement->mStmtList = NULL;
    aStatement->mStmtList2 = NULL;
    
    /* TASK-6744 */
    aStatement->mRandomPlanInfo = NULL;

    /* BUG-45899 */
    SDI_INIT_PRINT_INFO( &(aStatement->mShardPrintInfo) );

    /* TASK-7219 Non-shard DML */
    aStatement->mShardPartialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;

    //--------------------------------------------
    // 각종 Memory Manager를 위한 Memory 할당
    //--------------------------------------------

    //------------------
    // qmpMem
    //------------------

    IDU_FIT_POINT( "qcg::allocStatement::alloc::sAllocPtr1",
                    idERR_ABORT_InsufficientMemory );

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mIduVarMemListPool.alloc((void**)&sAllocPtr)
             != IDE_SUCCESS);

    // To fix BUG-20676
    // prepare memory limit
    aStatement->myPlan->qmpMem = new (sAllocPtr) iduVarMemList;

    /* BUG-33688 iduVarMemList의 init 이 실패할수 있습니다. */
    IDE_TEST( aStatement->myPlan->qmpMem->init( IDU_MEM_QMP,
                                                iduProperty::getPrepareMemoryMax())
              != IDE_SUCCESS);
    sIsQmpInited = ID_TRUE;

    //------------------
    // qmeMem
    //------------------

    IDU_FIT_POINT( "qcg::allocStatement::alloc::sAllocPtr2",
                    idERR_ABORT_InsufficientMemory );

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mIduVarMemListPool.alloc((void**)&sAllocPtr) != IDE_SUCCESS);

    // To fix BUG-20676
    // prepare memory limit
    aStatement->qmeMem = new (sAllocPtr) iduVarMemList;

    /* BUG-33688 iduVarMemList의 init 이 실패할수 있습니다. */
    IDE_TEST( aStatement->qmeMem->init( IDU_MEM_QMP,
                                        iduProperty::getPrepareMemoryMax())
              != IDE_SUCCESS);
    sIsQmeInited = ID_TRUE;

    //------------------
    // qmxMem
    //------------------

    IDU_FIT_POINT( "qcg::allocStatement::alloc::sAllocPtr3",
                    idERR_ABORT_InsufficientMemory );

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mIduMemoryPool.alloc((void**)&sAllocPtr) != IDE_SUCCESS);

    aStatement->qmxMem = new (sAllocPtr) iduMemory();
    aStatement->qmxMem->init(IDU_MEM_QMX);

    //------------------
    // qmbMem
    //------------------
    
    IDU_FIT_POINT( "qcg::allocStatement::alloc::sAllocPtr4",
                    idERR_ABORT_InsufficientMemory );

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mIduVarMemListPool.alloc((void**)&sAllocPtr) != IDE_SUCCESS);
    // fix BUG-10563, bind memory 축소
    aStatement->qmbMem = new (sAllocPtr) iduVarMemList;

    /* BUG-33688 iduVarMemList의 init 이 실패할수 있습니다. */
    IDE_TEST( aStatement->qmbMem->init(IDU_MEM_QMB) != IDE_SUCCESS);
    sIsQmbInited = ID_TRUE;

    //------------------
    // qmtMem
    //------------------

    IDU_FIT_POINT( "qcg::allocStatement::alloc::sAllocPtr5",
                    idERR_ABORT_InsufficientMemory );

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mIduMemoryPool.alloc((void**)&sAllocPtr) != IDE_SUCCESS);

    aStatement->qmtMem = new (sAllocPtr) iduMemory();
    aStatement->qmtMem->init(IDU_MEM_QMT);

    //------------------
    // qxcMem
    //------------------

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    IDU_FIT_POINT( "qcg::allocStatement::alloc::sAllocPtr6",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( mIduMemoryPool.alloc( (void**)&sAllocPtr ) != IDE_SUCCESS);

    aStatement->qxcMem = new (sAllocPtr) iduMemory();
    aStatement->qxcMem->init( IDU_MEM_QXC );

    // PROJ-2717 Internal procedure
    IDE_TEST(mIduVarMemListPool.alloc((void**)&sAllocPtr) != IDE_SUCCESS);

    // To fix BUG-20676
    // prepare memory limit
    aStatement->qixMem = new (sAllocPtr) iduVarMemList;

    IDE_TEST( aStatement->qixMem->init( IDU_MEM_QMX,
                                        iduProperty::getExecuteMemoryMax())
              != IDE_SUCCESS);
    sIsQixInited = ID_TRUE;

    //------------------
    // statement environment 생성과 초기화
    //------------------

    IDU_FIT_POINT( "qcg::allocStatement::alloc::propertyEnv",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aStatement->qmeMem->alloc( ID_SIZEOF(qcPlanProperty),
                                         (void**) & aStatement->propertyEnv )
              != IDE_SUCCESS);

    qcgPlan::initPlanProperty( aStatement->propertyEnv );

    //------------------
    // qsvEnv
    //------------------
    IDE_TEST(qsvEnv::allocEnv( aStatement ) != IDE_SUCCESS);

    IDU_FIT_POINT( "qcg::allocStatement::cralloc::spxEnv",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(aStatement->qmeMem->cralloc( ID_SIZEOF(qsxEnvInfo),
                                          (void**)&(aStatement->spxEnv))
             != IDE_SUCCESS);
    (void)qsxEnv::initializeRaisedExcpInfo( &QC_QSX_ENV(aStatement)->mRaisedExcpInfo );   /* BUG-43160 */

    aStatement->spxEnv->mIsInitialized = ID_FALSE ;

    IDU_FIT_POINT( "qcg::allocStatement::alloc::parentInfo",
                    idERR_ABORT_InsufficientMemory );

    /* PROJ-2197 PSM Renewal */
    IDE_TEST(aStatement->qmeMem->alloc( ID_SIZEOF(qsxEnvParentInfo),
                                        (void**)&(aStatement->parentInfo))
             != IDE_SUCCESS);

    aStatement->parentInfo->parentTmplate    = NULL;
    aStatement->parentInfo->parentReturnInto = NULL;

    aStatement->calledByPSMFlag = ID_FALSE;
    aStatement->mInplaceUpdateDisableFlag = ID_FALSE;

    /* PROJ-2206 withStmtList manager alloc
     * PROJ-2415 Grouping Sets Clause
     * withStmtList manager alloc -> stmtList manager alloc for Re-parsing */
    IDE_TEST( allocStmtListMgr( aStatement ) != IDE_SUCCESS );

    //--------------------
    // initialize session
    //--------------------
    if( aSession != NULL )
    {
        // 일반 sql구문 처리시,
        // mm에서 session 객체를 넘김.
        aStatement->session = aSession;
    }
    else
    {
        // connect protocol에 대한 처리와
        // 내부에서 처리되는 sql문들에 대해서는 session 정보가 없으므로
        // NULL이 넘어옴.
        // 이때, 내부적으로 session객체를 하나 만들고,
        // mMmSession = NULL 로 초기화시켜,
        // session 정보 접근시 default 값들을 얻도록 한다.
        // 내부에서 처리되는 sql문 :
        //      qcg::runDMLforDDL()
        //      qcmCreate::runDDLforMETA()
        //      qcmCreate::upgradeMeta()
        //      qcmPerformanceView::runDDLforPV()
        //      qcmProc::buildProcText
        //      qdnTrigger::allocTriggerCache()
        //      qdnTrigger::recompileTrigger()
        //      qsxLoadProc()
        //      qsxRecompileProc()

        IDU_FIT_POINT( "qcg::allocStatement::alloc::sSession",
                        idERR_ABORT_InsufficientMemory );

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_TEST(mSessionPool.alloc((void**)&sSession) != IDE_SUCCESS);

        sSession->mQPSpecific.mFlag = 0;
        sSession->mQPSpecific.mFlag &= ~QC_SESSION_INTERNAL_ALLOC_MASK;
        sSession->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_ALLOC_TRUE;

        sSession->mQPSpecific.mPropertyFlag = 0;
        
        sSession->mQPSpecific.mCache.mSequences_ = NULL;
        sSession->mQPSpecific.mSessionObj        = NULL;

        sSession->mMmSession = NULL;

        // PROJ-1073 Package
        sSession->mQPSpecific.mSessionPkg = NULL;

        // BUG-38129
        sSession->mQPSpecific.mLastRowGRID = SC_NULL_GRID;

        // BUG-24470
        // 내부에서 생성된 session userID는 qcg::mInternalUserID 이다.
        sSession->mUserID = qcg::mInternalUserID;

        /* BUG-37093 */
        sSession->mMmSessionForDatabaseLink = NULL;

        sSession->mBakSessionProperty.mTransactionalDDL = 0;

        IDE_TEST(idp::read("GLOBAL_TRANSACTION_LEVEL",
                           &sSession->mBakSessionProperty.mGlobalTransactionLevel)
                 != IDE_SUCCESS);

        aStatement->session = sSession;
    }

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* If MMSession, qciStmtInfo have been already initailized,
       request a mutex from the mutex pool in mmcSession.
       If not, just use qcStatement.stmtMutex as before. */
    /* Session 종속적인 statement 할당시에는 mMmSession, aStmtInfo가 NULL이 아니며,
       session 독립적인 statement 할당시에는 모두, 혹은 둘중하나는 NULL이다. ex> trigger */
    if( (aStatement->session->mMmSession != NULL) && (aStmtInfo != NULL) )
    {
        IDE_TEST( qci::mSessionCallback.mGetMutexFromPool(
                       aStatement->session->mMmSession,
                       &(aStatement->stmtMutexPtr ),
                       (SChar*) "QCI_STMT_MUTEX" )
                  != IDE_SUCCESS );

        aStatement->mutexpoolFlag = ID_TRUE;
    }
    else
    {
        IDE_TEST( aStatement->stmtMutex.initialize( (SChar*) "QCI_STMT_MUTEX",
                              IDU_MUTEX_KIND_NATIVE,
                              IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );

        aStatement->stmtMutexPtr  = &(aStatement->stmtMutex);
        aStatement->mutexpoolFlag = ID_FALSE;
    }

    sMutexState = 1;

    // BUG-38932 alloc, free cost of cursor mutex is too expensive.
    IDE_TEST( aStatement->mCursorMutex.initialize(
                                        (SChar*) "QCI_CURSOR_MUTEX",
                                        IDU_MUTEX_KIND_NATIVE,
                                        IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    sMutexState = 2;

    if( aStmtInfo != NULL )
    {
        aStatement->stmtInfo = aStmtInfo;
    }
    else
    {
        IDU_FIT_POINT( "qcg::allocStatement::alloc::sStmtInfo",
                        idERR_ABORT_InsufficientMemory );

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_TEST(mStatementPool.alloc((void**)&sStmtInfo) != IDE_SUCCESS);

        sStmtInfo->mMmStatement = NULL;
        sStmtInfo->mSmiStmtForExecute = NULL;
        sStmtInfo->mIsQpAlloc = ID_TRUE;

        aStatement->stmtInfo = sStmtInfo;
    }

    // PROJ-2242
    IDE_TEST(aStatement->qmeMem->cralloc( ID_SIZEOF(qmoSystemStatistics),
                                          (void**)&(aStatement->mSysStat))
             != IDE_SUCCESS);

    /*
     * PROJ-1071 Parallel Query
     * thread manager alloc & init
     */
    IDE_TEST(aStatement->qmeMem->alloc(ID_SIZEOF(qmcThrMgr),
                                       (void**)&(aStatement->mThrMgr))
             != IDE_SUCCESS);
    aStatement->mThrMgr->mThrCnt = 0;
    aStatement->mThrMgr->mMemory = aStatement->qmxMem;
    IDU_LIST_INIT(&aStatement->mThrMgr->mUseList);
    IDU_LIST_INIT(&aStatement->mThrMgr->mFreeList);

    //--------------------------------------------
    // qcTemplate을 위한 공간 할당
    //--------------------------------------------

    IDE_TEST( allocSharedTemplate( aStatement,
                                   QCG_GET_SESSION_STACK_SIZE( aStatement ) )
              != IDE_SUCCESS );

    //--------------------------------------------
    // qcStatement의 멤버 변수 초기화
    //--------------------------------------------

    aStatement->mStatistics = aStatistics;

    IDE_TEST( aStatement->myPlan->qmpMem->getStatus(
                  & aStatement->qmpStackPosition )
              != IDE_SUCCESS );

    IDE_TEST( aStatement->qmeMem->getStatus(
                  & aStatement->qmeStackPosition )
              != IDE_SUCCESS );

    /* PROJ-2677 DDL synchronization */
    qrc::setDDLSrcInfo( aStatement,
                        ID_FALSE,
                        0,
                        NULL,
                        0,
                        NULL );

    /* PROJ-2735 DDL Transaction */
    qrc::setDDLDestInfo( aStatement, 
                         0,
                         NULL,
                         0,
                         NULL );

    /* PROJ-2757 Advanced Global DDL */
    sdiGlobalDDL::clearInfo( aStatement );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( sdi::allocAndInitQuerySetList( aStatement ) != IDE_SUCCESS );

    aStatement->allocFlag = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( aStatement->myPlan->qmpMem != NULL )
    {
        if ( sIsQmpInited == ID_TRUE )
        {
            aStatement->myPlan->qmpMem->destroy();
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)mIduVarMemListPool.memfree(aStatement->myPlan->qmpMem);
        aStatement->myPlan->qmpMem = NULL;
    }
    if ( aStatement->qmeMem != NULL )
    {
        if ( sIsQmeInited == ID_TRUE )
        {
            aStatement->qmeMem->destroy();
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)mIduVarMemListPool.memfree(aStatement->qmeMem);
        aStatement->qmeMem = NULL;
    }
    if ( aStatement->qmxMem != NULL )
    {
        aStatement->qmxMem->destroy();
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)mIduMemoryPool.memfree(aStatement->qmxMem);
        aStatement->qmxMem = NULL;
    }
    if ( aStatement->qmbMem != NULL )
    {
        if ( sIsQmbInited == ID_TRUE )
        {
            aStatement->qmbMem->destroy();
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)mIduVarMemListPool.memfree(aStatement->qmbMem);
        aStatement->qmbMem = NULL;
    }
    if ( aStatement->qmtMem != NULL )
    {
        aStatement->qmtMem->destroy();
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)mIduMemoryPool.memfree(aStatement->qmtMem);
        aStatement->qmtMem = NULL;
    }

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    if ( aStatement->qxcMem != NULL )
    {
        aStatement->qxcMem->destroy();
        (void)mIduMemoryPool.memfree(aStatement->qxcMem);
        aStatement->qxcMem = NULL;
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2717 Internal procedure
    if ( aStatement->qixMem != NULL )
    {
        if ( sIsQixInited == ID_TRUE )
        {
            aStatement->qixMem->destroy();
        }

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        (void)mIduVarMemListPool.memfree(aStatement->qixMem);
        aStatement->qixMem = NULL;
    }

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* If the mutex was obtained from the mutex pool in mmcSession, free from mutex pool.
       If not, destroy qcStatement.stmtMutex as before. */
    // BUG-38932 alloc, free cost of cursor mutex is too expensive.
    switch ( sMutexState )
    {
        case 2:
            IDE_ASSERT( aStatement->mCursorMutex.destroy() == IDE_SUCCESS );
        /* fallthrough */
        case 1:

            if( aStatement->mutexpoolFlag == ID_TRUE )
            {
                IDE_ASSERT( qci::mSessionCallback.mFreeMutexFromPool(
                                aStatement->session->mMmSession,
                                aStatement->stmtMutexPtr ) == IDE_SUCCESS );

                aStatement->mutexpoolFlag = ID_FALSE;
            }
            else
            {
                IDE_ASSERT( aStatement->stmtMutex.destroy() == IDE_SUCCESS );
            }
        /* fallthrough */
        default :
            break;
    }

    // fix BUG-33057
    if( sSession != NULL )
    {
        if ( ( sSession->mQPSpecific.mFlag
               & QC_SESSION_INTERNAL_ALLOC_MASK )
             == QC_SESSION_INTERNAL_ALLOC_TRUE )
        {
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            (void)mSessionPool.memfree(sSession);
        }
    }
    else
    {
        // Nothing To Do
    }

    // fix BUG-33057
    if( sStmtInfo != NULL )
    {
        if( sStmtInfo->mIsQpAlloc == ID_TRUE )
        {
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            (void)mStatementPool.memfree(sStmtInfo);
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

    aStatement->spvEnv     = NULL;
    aStatement->spxEnv     = NULL;
    aStatement->parentInfo = NULL;
    aStatement->session    = NULL;
    aStatement->stmtInfo   = NULL;

    aStatement->myPlan->sTmplate = NULL;

    return IDE_FAILURE;
    
}

IDE_RC qcg::allocPrivateTemplate( qcStatement * aStatement,
                                  iduMemory   * aMemory )
{
    qcTemplate * sTemplate;
    UInt         sCount;

    IDU_FIT_POINT( "qcg::allocPrivateTemplate::cralloc::aStatement1",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(aMemory->cralloc( ID_SIZEOF(qcTemplate),
                                     (void**)&(QC_PRIVATE_TMPLATE(aStatement)))
                   != IDE_SUCCESS);

    sTemplate = QC_PRIVATE_TMPLATE(aStatement);

    // stack은 추후 설정한다.
    sTemplate->tmplate.stackBuffer     = NULL;
    sTemplate->tmplate.stack           = NULL;
    sTemplate->tmplate.stackCount      = 0;
    sTemplate->tmplate.stackRemain     = 0;

    sTemplate->tmplate.dataSize        = 0;
    sTemplate->tmplate.data            = NULL;
    sTemplate->tmplate.execInfoCnt     = 0;
    sTemplate->tmplate.execInfo        = NULL;
    sTemplate->tmplate.initSubquery    = qtc::initSubquery;
    sTemplate->tmplate.fetchSubquery   = qtc::fetchSubquery;
    sTemplate->tmplate.finiSubquery    = qtc::finiSubquery;
    sTemplate->tmplate.setCalcSubquery = qtc::setCalcSubquery;
    sTemplate->tmplate.getOpenedCursor =
        (mtcGetOpenedCursorFunc)qtc::getOpenedCursor;
    sTemplate->tmplate.addOpenedLobCursor =
        (mtcAddOpenedLobCursorFunc)qtc::addOpenedLobCursor;
    sTemplate->tmplate.isBaseTable     = qtc::isBaseTable;
    sTemplate->tmplate.closeLobLocator = qtc::closeLobLocator;
    sTemplate->tmplate.getSTObjBufSize = qtc::getSTObjBufSize;

    // PROJ-2002 Column Security
    sTemplate->tmplate.encrypt         = qcsModule::encryptCallback;
    sTemplate->tmplate.decrypt         = qcsModule::decryptCallback;
    sTemplate->tmplate.encodeECC       = qcsModule::encodeECCCallback;
    sTemplate->tmplate.getDecryptInfo  = qcsModule::getDecryptInfoCallback;
    sTemplate->tmplate.getECCInfo      = qcsModule::getECCInfoCallback;
    sTemplate->tmplate.getECCSize      = qcsModule::getECCSize;

    sTemplate->tmplate.variableRow     = ID_USHORT_MAX;

    // dateFormat은 추후 설정한다.
    sTemplate->tmplate.dateFormat      = NULL;
    sTemplate->tmplate.dateFormatRef   = ID_FALSE;

    /* PROJ-2208 */
    sTemplate->tmplate.nlsCurrency     = qtc::getNLSCurrencyCallback;
    sTemplate->tmplate.nlsCurrencyRef  = ID_FALSE;

    // BUG-37247
    sTemplate->tmplate.groupConcatPrecisionRef = ID_FALSE;
    
    // BUG-41944
    sTemplate->tmplate.arithmeticOpMode    = MTC_ARITHMETIC_OPERATION_DEFAULT;
    sTemplate->tmplate.arithmeticOpModeRef = ID_FALSE;

    // PROJ-2527 WITHIN GROUP AGGR
    sTemplate->tmplate.funcData = NULL;
    sTemplate->tmplate.funcDataCnt = 0;

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    sTemplate->tmplate.rowCount      = 0;
    sTemplate->tmplate.rowArrayCount = 0;

    for ( sCount = 0; sCount < MTC_TUPLE_TYPE_MAXIMUM; sCount++ )
    {
        sTemplate->tmplate.currentRow[sCount] = ID_USHORT_MAX;
    }

    //-------------------------------------------------------
    // PROJ-1358
    // Internal Tuple의 자동확장과 관련하여
    // Internal Tuple의 공간을 할당하고,
    // Table Map의 공간을 초기화하여 할당한다.
    //-------------------------------------------------------

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    // sTemplate->tmplate.rowArrayCount = MTC_TUPLE_ROW_INIT_CNT;

    // IDE_TEST( allocInternalTuple( sTemplate,
    //                               QC_QMP_MEM(aStatement),
    //                               sTemplate->tmplate.rowArrayCount )
    //           != IDE_SUCCESS );

    sTemplate->planCount = 0;
    sTemplate->planFlag = NULL;

    sTemplate->cursorMgr = NULL;
    sTemplate->tempTableMgr = NULL;
    sTemplate->numRows = 0;
    sTemplate->stmtType = QCI_STMT_MASK_MAX;
    sTemplate->fixedTableAutomataStatus = 0;
    sTemplate->stmt = aStatement;
    sTemplate->flag = QC_TMP_INITIALIZE;
    sTemplate->smiStatementFlag = 0;
    sTemplate->insOrUptStmtCount = 0;
    sTemplate->insOrUptRowValueCount = NULL;
    sTemplate->insOrUptRow = NULL;

    /* PROJ-2209 DBTIMEZONE */
    sTemplate->unixdate = NULL;
    sTemplate->sysdate = NULL;
    sTemplate->currentdate = NULL;

    // PROJ-1413 tupleVarList 초기화
    sTemplate->tupleVarGenNumber = 0;
    sTemplate->tupleVarList = NULL;

    // BUG-16422 execute중 임시 생성된 tableInfo의 관리
    sTemplate->tableInfoMgr = NULL;

    // PROJ-1436
    sTemplate->indirectRef = ID_FALSE;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    sTemplate->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    sTemplate->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    sTemplate->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    sTemplate->cacheObjCnt    = 0;
    sTemplate->cacheObjects   = NULL;

    /* PROJ-2448 Subquery caching */
    sTemplate->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    sTemplate->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    /* PROJ-2462 Result Cache */
    QC_RESULT_CACHE_INIT( &sTemplate->resultCache );

    // BUG-44710
    sdi::initDataInfo( &(sTemplate->shardExecData) );

    // BUG-44795
    sTemplate->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    sTemplate->mUnnestViewNameIdx = 0;

    /* BUG-48776 */
    sTemplate->mSubqueryMode = QCU_SUBQUERY_MODE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qcg::allocPrivateTemplate( qcStatement   * aStatement,
                                  iduVarMemList * aMemory )
{
    qcTemplate * sTemplate;
    UInt         sCount;

    IDU_FIT_POINT( "qcg::allocPrivateTemplate::cralloc::aStatement2",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(aMemory->cralloc( ID_SIZEOF(qcTemplate),
                               (void**)&(QC_PRIVATE_TMPLATE(aStatement)))
             != IDE_SUCCESS);

    sTemplate = QC_PRIVATE_TMPLATE(aStatement);

    // stack은 추후 설정한다.
    sTemplate->tmplate.stackBuffer     = NULL;
    sTemplate->tmplate.stack           = NULL;
    sTemplate->tmplate.stackCount      = 0;
    sTemplate->tmplate.stackRemain     = 0;

    sTemplate->tmplate.dataSize        = 0;
    sTemplate->tmplate.data            = NULL;
    sTemplate->tmplate.execInfoCnt     = 0;
    sTemplate->tmplate.execInfo        = NULL;
    sTemplate->tmplate.initSubquery    = qtc::initSubquery;
    sTemplate->tmplate.fetchSubquery   = qtc::fetchSubquery;
    sTemplate->tmplate.finiSubquery    = qtc::finiSubquery;
    sTemplate->tmplate.setCalcSubquery = qtc::setCalcSubquery;
    sTemplate->tmplate.getOpenedCursor =
        (mtcGetOpenedCursorFunc)qtc::getOpenedCursor;
    sTemplate->tmplate.addOpenedLobCursor =
        (mtcAddOpenedLobCursorFunc)qtc::addOpenedLobCursor;
    sTemplate->tmplate.isBaseTable     = qtc::isBaseTable;
    sTemplate->tmplate.closeLobLocator = qtc::closeLobLocator;
    sTemplate->tmplate.getSTObjBufSize = qtc::getSTObjBufSize;
    sTemplate->tmplate.variableRow     = ID_USHORT_MAX;

    // PROJ-2002 Column Security
    sTemplate->tmplate.encrypt         = qcsModule::encryptCallback;
    sTemplate->tmplate.decrypt         = qcsModule::decryptCallback;
    sTemplate->tmplate.encodeECC       = qcsModule::encodeECCCallback;
    sTemplate->tmplate.getDecryptInfo  = qcsModule::getDecryptInfoCallback;
    sTemplate->tmplate.getECCInfo      = qcsModule::getECCInfoCallback;
    sTemplate->tmplate.getECCSize      = qcsModule::getECCSize;

    // dateFormat은 추후 설정한다.
    sTemplate->tmplate.dateFormat      = NULL;
    sTemplate->tmplate.dateFormatRef   = ID_FALSE;

    /* PROJ-2208 */
    sTemplate->tmplate.nlsCurrency    = qtc::getNLSCurrencyCallback;
    sTemplate->tmplate.nlsCurrencyRef = ID_FALSE;

    // BUG-37247
    sTemplate->tmplate.groupConcatPrecisionRef = ID_FALSE;

    // BUG-41944
    sTemplate->tmplate.arithmeticOpMode    = MTC_ARITHMETIC_OPERATION_DEFAULT;
    sTemplate->tmplate.arithmeticOpModeRef = ID_FALSE;

    // PROJ-2527 WITHIN GROUP AGGR
    sTemplate->tmplate.funcData = NULL;
    sTemplate->tmplate.funcDataCnt = 0;

    // PROJ-1579 NCHAR
    sTemplate->tmplate.nlsUse          =
        QCG_GET_SESSION_LANGUAGE( aStatement );

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    sTemplate->tmplate.rowCount      = 0;
    sTemplate->tmplate.rowArrayCount = 0;

    for ( sCount = 0; sCount < MTC_TUPLE_TYPE_MAXIMUM; sCount++ )
    {
        sTemplate->tmplate.currentRow[sCount] = ID_USHORT_MAX;
    }

    //-------------------------------------------------------
    // PROJ-1358
    // Internal Tuple의 자동확장과 관련하여
    // Internal Tuple의 공간을 할당하고,
    // Table Map의 공간을 초기화하여 할당한다.
    //-------------------------------------------------------

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    // sTemplate->tmplate.rowArrayCount = MTC_TUPLE_ROW_INIT_CNT;

    // IDE_TEST( allocInternalTuple( sTemplate,
    //                               QC_QMP_MEM(aStatement),
    //                               sTemplate->tmplate.rowArrayCount )
    //           != IDE_SUCCESS );

    sTemplate->planCount = 0;
    sTemplate->planFlag = NULL;

    sTemplate->cursorMgr = NULL;
    sTemplate->tempTableMgr = NULL;
    sTemplate->numRows = 0;
    sTemplate->stmtType = QCI_STMT_MASK_MAX;
    sTemplate->fixedTableAutomataStatus = 0;
    sTemplate->stmt = aStatement;
    sTemplate->flag = QC_TMP_INITIALIZE;
    sTemplate->smiStatementFlag = 0;
    sTemplate->insOrUptStmtCount = 0;
    sTemplate->insOrUptRowValueCount = NULL;
    sTemplate->insOrUptRow = NULL;

    /* PROJ-2209 DBTIMEZONE */
    sTemplate->unixdate = NULL;
    sTemplate->sysdate = NULL;
    sTemplate->currentdate = NULL;

    // PROJ-1413 tupleVarList 초기화
    sTemplate->tupleVarGenNumber = 0;
    sTemplate->tupleVarList = NULL;

    // BUG-16422 execute중 임시 생성된 tableInfo의 관리
    sTemplate->tableInfoMgr = NULL;

    // PROJ-1436
    sTemplate->indirectRef = ID_FALSE;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    sTemplate->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    sTemplate->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    sTemplate->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    sTemplate->cacheObjCnt    = 0;
    sTemplate->cacheObjects   = NULL;

    /* PROJ-2448 Subquery caching */
    sTemplate->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    sTemplate->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    /* PROJ-2462 Result Cache */
    QC_RESULT_CACHE_INIT( &sTemplate->resultCache );

    // BUG-44710
    sdi::initDataInfo( &(sTemplate->shardExecData) );

    // BUG-44795
    sTemplate->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    sTemplate->mUnnestViewNameIdx = 0;

    /* BUG-48776 */
    sTemplate->mSubqueryMode = QCU_SUBQUERY_MODE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qcg::allocSharedTemplate( qcStatement * aStatement,
                                 SInt          aStackSize )
{
    qcTemplate * sTemplate;
    UInt         sCount;

    IDU_FIT_POINT( "qcg::allocSharedTemplate::alloc::aStatement",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcTemplate),
                                            (void**)&(QC_SHARED_TMPLATE(aStatement)))
             != IDE_SUCCESS);

    sTemplate = QC_SHARED_TMPLATE(aStatement);

    if ( aStackSize > 0 )
    {
        IDU_FIT_POINT( "qcg::allocSharedTemplate::alloc::tmplateStackBuffer",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                ID_SIZEOF(mtcStack) * aStackSize,
                (void**)&(sTemplate->tmplate.stackBuffer))
            != IDE_SUCCESS);

        sTemplate->tmplate.stack       = sTemplate->tmplate.stackBuffer;
        sTemplate->tmplate.stackCount  = aStackSize;
        sTemplate->tmplate.stackRemain = aStackSize;
    }
    else
    {
        sTemplate->tmplate.stackBuffer = NULL;
        sTemplate->tmplate.stack       = NULL;
        sTemplate->tmplate.stackCount  = 0;
        sTemplate->tmplate.stackRemain = 0;
    }

    sTemplate->tmplate.dataSize        = 0;
    sTemplate->tmplate.data            = NULL;
    sTemplate->tmplate.execInfoCnt     = 0;
    sTemplate->tmplate.execInfo        = NULL;
    sTemplate->tmplate.initSubquery    = qtc::initSubquery;
    sTemplate->tmplate.fetchSubquery   = qtc::fetchSubquery;
    sTemplate->tmplate.finiSubquery    = qtc::finiSubquery;
    sTemplate->tmplate.setCalcSubquery = qtc::setCalcSubquery;
    sTemplate->tmplate.getOpenedCursor =
        (mtcGetOpenedCursorFunc)qtc::getOpenedCursor;
    sTemplate->tmplate.addOpenedLobCursor =
        (mtcAddOpenedLobCursorFunc)qtc::addOpenedLobCursor;
    sTemplate->tmplate.isBaseTable     = qtc::isBaseTable;
    sTemplate->tmplate.closeLobLocator = qtc::closeLobLocator;
    sTemplate->tmplate.getSTObjBufSize = qtc::getSTObjBufSize;
    sTemplate->tmplate.variableRow     = ID_USHORT_MAX;
    sTemplate->tmplate.dateFormat      =
        QCG_GET_SESSION_DATE_FORMAT( aStatement );
    sTemplate->tmplate.dateFormatRef   = ID_FALSE;

    /* PROJ-2208 */
    sTemplate->tmplate.nlsCurrency     = qtc::getNLSCurrencyCallback;
    sTemplate->tmplate.nlsCurrencyRef  = ID_FALSE;

    // BUG-37247
    sTemplate->tmplate.groupConcatPrecisionRef = ID_FALSE;

    // BUG-41944
    sTemplate->tmplate.arithmeticOpMode    = MTC_ARITHMETIC_OPERATION_DEFAULT;
    sTemplate->tmplate.arithmeticOpModeRef = ID_FALSE;

    // PROJ-2527 WITHIN GROUP AGGR
    sTemplate->tmplate.funcData = NULL;
    sTemplate->tmplate.funcDataCnt = 0;

    // PROJ-2002 Column Security
    sTemplate->tmplate.encrypt         = qcsModule::encryptCallback;
    sTemplate->tmplate.decrypt         = qcsModule::decryptCallback;
    sTemplate->tmplate.encodeECC       = qcsModule::encodeECCCallback;
    sTemplate->tmplate.getDecryptInfo  = qcsModule::getDecryptInfoCallback;
    sTemplate->tmplate.getECCInfo      = qcsModule::getECCInfoCallback;
    sTemplate->tmplate.getECCSize      = qcsModule::getECCSize;

    // PROJ-1579 NCHAR
    sTemplate->tmplate.nlsUse          =
        QCG_GET_SESSION_LANGUAGE( aStatement );

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    sTemplate->tmplate.rowCount      = 0;
    sTemplate->tmplate.rowArrayCount = 0;

    for ( sCount = 0; sCount < MTC_TUPLE_TYPE_MAXIMUM; sCount++ )
    {
        sTemplate->tmplate.currentRow[sCount] = ID_USHORT_MAX;
    }

    //-------------------------------------------------------
    // PROJ-1358
    // Internal Tuple의 자동확장과 관련하여
    // Internal Tuple의 공간을 할당하고,
    // Table Map의 공간을 초기화하여 할당한다.
    //-------------------------------------------------------

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    // sTemplate->tmplate.rowArrayCount = MTC_TUPLE_ROW_INIT_CNT;

    // IDE_TEST( allocInternalTuple( sTemplate,
    //                               QC_QMP_MEM(aStatement),
    //                               sTemplate->tmplate.rowArrayCount )
    //           != IDE_SUCCESS );

    sTemplate->planCount = 0;
    sTemplate->planFlag = NULL;

    sTemplate->cursorMgr = NULL;
    sTemplate->tempTableMgr = NULL;
    sTemplate->numRows = 0;
    sTemplate->stmtType = QCI_STMT_MASK_MAX;
    sTemplate->fixedTableAutomataStatus = 0;
    sTemplate->stmt = aStatement;
    sTemplate->flag = QC_TMP_INITIALIZE;
    sTemplate->smiStatementFlag = 0;
    sTemplate->insOrUptStmtCount = 0;
    sTemplate->insOrUptRowValueCount = NULL;
    sTemplate->insOrUptRow = NULL;

    /* PROJ-2209 DBTIMEZONE */
    sTemplate->unixdate = NULL;
    sTemplate->sysdate = NULL;
    sTemplate->currentdate = NULL;

    // PROJ-1413 tupleVarList 초기화
    sTemplate->tupleVarGenNumber = 0;
    sTemplate->tupleVarList = NULL;

    // BUG-16422 execute중 임시 생성된 tableInfo의 관리
    sTemplate->tableInfoMgr = NULL;

    // PROJ-1436
    sTemplate->indirectRef = ID_FALSE;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    sTemplate->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    sTemplate->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    sTemplate->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    sTemplate->cacheObjCnt    = 0;
    sTemplate->cacheObjects   = NULL;

    /* PROJ-2448 Subquery caching */
    sTemplate->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    sTemplate->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    /* PROJ-2462 Result Cache */
    QC_RESULT_CACHE_INIT( &sTemplate->resultCache );

    // BUG-44710
    sdi::initDataInfo( &(sTemplate->shardExecData) );

    // BUG-44795
    sTemplate->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    sTemplate->mUnnestViewNameIdx = 0;

    /* BUG-48776 */
    sTemplate->mSubqueryMode = QCU_SUBQUERY_MODE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qcg::freeStatement( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : qcStatement를 자료구조 선언시의 상태로 만듦.
 *
 * Implementation :
 *
 *    template member초기화 ( cursor close, temp table drop, ... )
 *    할당받은 각종 메모리 해제 및 변수 초기화
 *
 *    iduMemMgr::free 도중 에러가 발생하면 IDE_ASSERT 됨.
 *
 ***********************************************************************/

    UInt sStage      = 0;
    UInt sMutexStage = 0;

    qsxStmtList  * sStmtList  = aStatement->mStmtList;
    qsxStmtList2 * sStmtList2 = aStatement->mStmtList2;

    if ( aStatement->allocFlag == ID_TRUE )
    {
        sStage      = 2;
        sMutexStage = 2;

        // PROJ-1071
        IDE_TEST( joinThread( aStatement ) != IDE_SUCCESS );

        resetTemplateMember( aStatement );

        sStage = 1;
        IDE_TEST( finishAndReleaseThread( aStatement ) != IDE_SUCCESS );

        /* BUG-38294 */
        sStage = 0;
        freePRLQExecutionMemory(aStatement);

        // BUG-43158 Enhance statement list caching in PSM
        if ( aStatement->session != NULL )
        {
            for ( ; sStmtList != NULL; sStmtList = aStatement->mStmtList )
            {
                idlOS::memset( sStmtList->mStmtPoolStatus,
                               0,
                               aStatement->session->mQPSpecific.mStmtListInfo.mStmtPoolStatusSize );

                sStmtList->mParseTree = NULL;

                aStatement->mStmtList = sStmtList->mNext;
                sStmtList->mNext      = NULL;

                aStatement->session->mQPSpecific.mStmtListInfo.mStmtListFreedCount++;
            }

            for ( ; sStmtList2 != NULL; sStmtList2 = aStatement->mStmtList2 )
            {
                idlOS::memset( sStmtList2->mStmtPoolStatus,
                               0,
                               aStatement->session->mQPSpecific.mStmtListInfo.mStmtPoolStatusSize );

                sStmtList2->mParseTree = NULL;

                aStatement->mStmtList2 = sStmtList2->mNext;
                sStmtList2->mNext      = NULL;

                aStatement->session->mQPSpecific.mStmtListInfo.mStmtListFreedCount++;
            }
        }
        else
        {
            /* Nothing to do */
        }

        if( aStatement->sharedFlag == ID_FALSE )
        {
            // 공유되지 않은 plan인 경우
            // privatePlan을 해제한다.

            // BUG-23050
            // myPlan을 privatePlan으로 초기화한다.
            aStatement->myPlan = & aStatement->privatePlan;

            /* PROJ-2462 Reuslt Cache */
            if ( QC_PRIVATE_TMPLATE( aStatement ) != NULL )
            {
                qmxResultCache::destroyResultCache( QC_PRIVATE_TMPLATE( aStatement ) );

                // BUG-44710
                sdi::clearDataInfo( aStatement,
                                    &(QC_PRIVATE_TMPLATE( aStatement )->shardExecData) );
            }
            else
            {
                /* Nothing to do */
            }

            if ( aStatement->myPlan->qmpMem != NULL )
            {
                aStatement->myPlan->qmpMem->destroy();
                /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                IDE_TEST(mIduVarMemListPool.memfree(aStatement->myPlan->qmpMem)
                         != IDE_SUCCESS);

                aStatement->myPlan->qmpMem = NULL;
            }
        }
        else
        {
            // 공유된 plan인 경우
            // 다른 statement가 생성한 plan을 공유한 경우
            // privatePlan은 존재하고 해제해야 한다.
            aStatement->myPlan = & aStatement->privatePlan;

            // BUG-44710
            sdi::clearDataInfo( aStatement,
                                &(QC_PRIVATE_TMPLATE( aStatement )->shardExecData) );

            // prepared private template을 사용하는 경우 사용이 끝났음을 설정한다.
            /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
                Dynamic SQL 환경에서는 개선이 필요하다.
            */
            freePrepTemplate(aStatement,ID_FALSE);

            if ( aStatement->myPlan->qmpMem != NULL )
            {
                aStatement->myPlan->qmpMem->destroy();
                /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                IDE_TEST(mIduVarMemListPool.memfree(aStatement->myPlan->qmpMem)
                         != IDE_SUCCESS);

                aStatement->myPlan->qmpMem = NULL;
            }
        }

        if ( aStatement->qmeMem != NULL )
        {
            aStatement->qmeMem->destroy();
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(mIduVarMemListPool.memfree(aStatement->qmeMem)
                     != IDE_SUCCESS);

            aStatement->qmeMem = NULL;
        }
        if ( aStatement->qmxMem != NULL )
        {
            aStatement->qmxMem->destroy();

            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(mIduMemoryPool.memfree(aStatement->qmxMem)
                     != IDE_SUCCESS);

            aStatement->qmxMem = NULL;
        }
        if ( aStatement->qmbMem != NULL )
        {
            aStatement->qmbMem->destroy();

            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(mIduVarMemListPool.memfree(aStatement->qmbMem)
                     != IDE_SUCCESS);

            aStatement->qmbMem = NULL;
        }
        if ( aStatement->qmtMem != NULL )
        {
            aStatement->qmtMem->destroy();

            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(mIduMemoryPool.memfree(aStatement->qmtMem)
                     != IDE_SUCCESS);

            aStatement->qmtMem = NULL;
        }

        /* PROJ-2452 Caching for DETERMINISTIC Function */
        if ( aStatement->qxcMem != NULL )
        {
            aStatement->qxcMem->destroy();

            IDE_TEST( mIduMemoryPool.memfree( aStatement->qxcMem )
                      != IDE_SUCCESS );

            aStatement->qxcMem = NULL;
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2717 Internal procedure
        if ( aStatement->qixMem != NULL )
        {
            aStatement->qixMem->destroy();

            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST(mIduVarMemListPool.memfree(aStatement->qixMem)
                     != IDE_SUCCESS);

            aStatement->qixMem = NULL;
        }

        sMutexStage = 1;
        // BUG-38932 alloc, free cost of cursor mutex is too expensive.
        IDE_TEST( aStatement->mCursorMutex.destroy() != IDE_SUCCESS );

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        /* If the mutex was obtained from the mutex pool in mmcSession, free from mutex pool.
           If not, destroy qcStatement.stmtMutex as before. */
        sMutexStage = 0;
        if( aStatement->mutexpoolFlag == ID_TRUE )
        {
            if ( aStatement->session != NULL )
            {
                IDE_ASSERT( qci::mSessionCallback.mFreeMutexFromPool(
                                 aStatement->session->mMmSession,
                                 aStatement->stmtMutexPtr ) == IDE_SUCCESS );
            }
            else
            {
                IDE_DASSERT( 0 );
            }
        }
        else
        {
            IDE_TEST( aStatement->stmtMutex.destroy() != IDE_SUCCESS );
        }

        if( aStatement->session != NULL )
        {
            if ( ( aStatement->session->mQPSpecific.mFlag
                   & QC_SESSION_INTERNAL_ALLOC_MASK )
                 == QC_SESSION_INTERNAL_ALLOC_TRUE )
            {
                /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                IDE_TEST( mSessionPool.memfree(aStatement->session)
                          != IDE_SUCCESS );
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

        if( aStatement->stmtInfo != NULL )
        {
            if( aStatement->stmtInfo->mIsQpAlloc == ID_TRUE )
            {
                /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                IDE_TEST( mStatementPool.memfree(aStatement->stmtInfo)
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

        aStatement->myPlan = NULL;
        aStatement->prepTemplateHeader = NULL;
        aStatement->prepTemplate = NULL;

        aStatement->pTmplate = NULL;
        aStatement->pBindParam = NULL;
        aStatement->pBindParamCount = 0;
        aStatement->bindParamDataInLastId = 0;

        aStatement->sharedFlag      = ID_FALSE;
        aStatement->calledByPSMFlag = ID_FALSE;
        aStatement->disableLeftStore= ID_FALSE;
        aStatement->mInplaceUpdateDisableFlag = ID_FALSE;

        aStatement->spvEnv     = NULL;
        aStatement->spxEnv     = NULL;
        aStatement->parentInfo = NULL;
        aStatement->session    = NULL;
        aStatement->stmtInfo   = NULL;

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        aStatement->mutexpoolFlag = ID_FALSE;
        aStatement->allocFlag     = ID_FALSE;

        // BUG-36986
        aStatement->namedVarNode = NULL;
        
        // PROJ-2551 simple query 최적화
        qciMisc::initSimpleInfo( &(aStatement->simpleInfo) );
        aStatement->mFlag = 0;

        /* TASK-6744 */
        aStatement->mRandomPlanInfo = NULL;

        /* PROJ-2677 DDL synchronization */
        qrc::setDDLSrcInfo( aStatement,
                            ID_FALSE,
                            0,
                            NULL,
                            0,
                            NULL );

        /* PROJ-2735 DDL Transaction */
        qrc::setDDLDestInfo( aStatement, 
                             0,
                             NULL,
                             0,
                             NULL );

        /* PROJ-2757 Advanced Global DDL */
        sdiGlobalDDL::clearInfo( aStatement );

        /* BUG-45899 */
        SDI_INIT_PRINT_INFO( &(aStatement->mShardPrintInfo) );

        /* TASK-7219 Shard Transformer Refactoring */
        aStatement->mShardQuerySetList = NULL;

        /* TASK-7219 Non-shard DML */
        aStatement->mShardPartialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sMutexStage)
    {
        case 2:
            IDE_ASSERT( aStatement->mCursorMutex.destroy() == IDE_SUCCESS );
            /* fallthrough */
        case 1:
            if( aStatement->mutexpoolFlag == ID_TRUE )
            {
                if ( aStatement->session != NULL )
                {
                    IDE_ASSERT( qci::mSessionCallback.mFreeMutexFromPool(
                                    aStatement->session->mMmSession,
                                    aStatement->stmtMutexPtr ) == IDE_SUCCESS );
                }
                else
                {
                    IDE_DASSERT( 0 );
                }

                aStatement->mutexpoolFlag = ID_FALSE;
            }
            else
            {
                IDE_ASSERT( aStatement->stmtMutex.destroy() == IDE_SUCCESS );
            }
            /* fallthrough */
        default:
            break;
    }

    switch (sStage)
    {
        case 2:
            (void)finishAndReleaseThread(aStatement);
            /* fallthrough */
        case 1:
            freePRLQExecutionMemory(aStatement);
            /* fallthrough */
        default:
            break;
    }

    return IDE_FAILURE;
}

void qcg::lock( qcStatement * aStatement )
{
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT( aStatement->stmtMutexPtr->lock(NULL /* idvSQL* */)
                == IDE_SUCCESS );
}

void qcg::unlock( qcStatement * aStatement )
{
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT( aStatement->stmtMutexPtr->unlock() == IDE_SUCCESS );
}

void qcg::setPlanTreeState(qcStatement * aStatement,
                           idBool        aPlanTreeFlag)
{
    lock(aStatement);
    aStatement->planTreeFlag = aPlanTreeFlag;
    unlock(aStatement);
}

idBool
qcg::getPlanTreeState(qcStatement * aStatement)
{
    return aStatement->planTreeFlag;
}

IDE_RC qcg::clearStatement( qcStatement * aStatement, idBool aRebuild )
{
/***********************************************************************
 *
 * Description : qcStatement 가
 *               allocStatement()호출시의 상태로 만듦.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcTemplate   * sTemplate;
    UInt           sCount;
    UInt           sStage;
    qsxStmtList  * sStmtList  = aStatement->mStmtList;
    qsxStmtList2 * sStmtList2 = aStatement->mStmtList2;

    setPlanTreeState(aStatement, ID_FALSE);

    sStage = 2;
    /* PROJ-1071 Parallel Query */
    IDE_TEST(joinThread(aStatement) != IDE_SUCCESS);

    resetTemplateMember( aStatement );

    sStage = 1;
    IDE_TEST(finishAndReleaseThread(aStatement) != IDE_SUCCESS);

    /* BUG-38294 */
    sStage = 0;
    freePRLQExecutionMemory(aStatement);

    if( aStatement->sharedFlag == ID_FALSE )
    {
        // 공유되지 않은 plan인 경우
        // qmpMem을 qmpStackPosition까지 free한다.
        aStatement->myPlan = & aStatement->privatePlan;

        /* PROJ-2462 Reuslt Cache */
        if ( QC_PRIVATE_TMPLATE( aStatement ) != NULL )
        {
            qmxResultCache::destroyResultCache( QC_PRIVATE_TMPLATE( aStatement ) );

            // BUG-44710
            sdi::clearDataInfo( aStatement,
                                &(QC_PRIVATE_TMPLATE( aStatement )->shardExecData) );
        }
        else
        {
            /* Nothing to do */
        }

        if ( aStatement->myPlan->qmpMem != NULL )
        {
            (void) aStatement->myPlan->qmpMem->setStatus(
                & aStatement->qmpStackPosition );
            aStatement->myPlan->qmpMem->freeUnused();
        }
        else
        {
            /* Nothing to do */
        }

        // prepared private template을 초기화한다.
        aStatement->prepTemplateHeader = NULL;
        aStatement->prepTemplate = NULL;

        // initialize template
        if ( QC_PRIVATE_TMPLATE(aStatement) != NULL )
        {
            // private template이 생성된 경우
            QC_SHARED_TMPLATE(aStatement) = QC_PRIVATE_TMPLATE(aStatement);
            QC_PRIVATE_TMPLATE(aStatement) = NULL;
        }
        else
        {
            // private template이 생성되지 않은 경우
            // (shared template이 생성되었지만 아직 공유되지 않은 경우)

            // Nothing to do.
        }
    }
    else
    {
        if ( ( QCU_EXECUTION_PLAN_MEMORY_CHECK == 1 ) &&
             ( QC_QMU_MEM(aStatement) != NULL ) )
        {
            // 공유된 plan을 오염시켜서는 안된다.
            if ( QC_QMP_MEM(aStatement)->compare( QC_QMU_MEM(aStatement) ) != 0 )
            {
                ideLog::log( IDE_SERVER_0,"[Notify : SQL Plan Cache] Plan Pollution : Session ID = "
                             "%d\n    Query => %s\n",
                             QCG_GET_SESSION_ID(aStatement),
                             aStatement->myPlan->stmtText );

                IDE_DASSERT( 0 );
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

        // 공유된 plan인 경우
        // 다른 statement가 생성한 plan을 공유한 경우
        // privatePlan을 원복한다. 그리고
        // privatePlan의 qmpMem은 이미 allocStatement 상태이다.
        aStatement->myPlan = & aStatement->privatePlan;

        // BUG-44710
        sdi::clearDataInfo( aStatement,
                            &(QC_PRIVATE_TMPLATE( aStatement )->shardExecData) );

        // prepared private template을 사용하는 경우 사용이 끝났음을 설정한다.
        /* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
                Dynamic SQL 환경에서는 개선이 필요하다.
        */
        freePrepTemplate(aStatement,aRebuild);

        // private template은 qmeMem으로 생성되었으므로
        // qmeMem과 함께 삭제된다.
        QC_PRIVATE_TMPLATE(aStatement) = NULL;
    }

    if( aStatement->qmeMem != NULL )
    {
        (void) aStatement->qmeMem->setStatus(
            & aStatement->qmeStackPosition );
        aStatement->qmeMem->freeUnused();
    }

    if ( aStatement->qmxMem != NULL )
    {
        //ideLog::log(IDE_QP_2, "[INFO] Query Execution Memory Size : %llu KB",
        //            aStatement->qmxMem->getSize()/1024);

        aStatement->qmxMem->freeAll(1);
    }

    if ( aStatement->qmtMem != NULL )
    {
        aStatement->qmtMem->freeAll(1);
    }

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    if ( aStatement->qxcMem != NULL )
    {
        aStatement->qxcMem->freeAll(1);
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2717 Internal procedure
    if ( aStatement->qixMem != NULL )
    {
        aStatement->qixMem->freeAll();
    }

    // PROJ-2163
    // 바인드 메모리 유지
    // qcg::clearStatement가 호출될 수 있는 경우는
    // (1) client에서 statement 재사용시
    // (2) rebuild, bind type 변경 등에 의한 re-hardprepare 시
    if ( aRebuild == ID_FALSE )
    {
        if ( aStatement->qmbMem != NULL )
        {
            aStatement->qmbMem->freeAll();
        }

        aStatement->pBindParam        = NULL;
        aStatement->pBindParamCount   = 0;

        // Plan 과 pBindParam 모두 초기화 되므로 flag 를 초기화 한다.
        aStatement->pBindParamChangedFlag = ID_FALSE;
    }
    else
    {
        // Plan 만 초기화 되고 pBindParam 은 유지되므로 서로 타입이 달라졌을 수 있다.
        aStatement->pBindParamChangedFlag = ID_TRUE;
    }

    aStatement->myPlan->planEnv          = NULL;
    aStatement->myPlan->stmtText         = NULL;
    aStatement->myPlan->stmtTextLen      = 0;
    aStatement->myPlan->mPlanFlag        = QC_PLAN_FLAG_INIT;
    aStatement->myPlan->mHintOffset      = NULL;;
    aStatement->myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    aStatement->myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    aStatement->myPlan->parseTree        = NULL;
    aStatement->myPlan->hints            = NULL;
    aStatement->myPlan->plan             = NULL;
    aStatement->myPlan->graph            = NULL;
    aStatement->myPlan->scanDecisionFactors = NULL;
    aStatement->myPlan->procPlanList = NULL;
    aStatement->myPlan->mShardAnalysis = NULL;
    aStatement->myPlan->mShardParamInfo = NULL; /* TASK-7219 Non-shard DML*/
    aStatement->myPlan->mShardParamOffset = ID_USHORT_MAX;
    aStatement->myPlan->mShardParamCount = 0;

    aStatement->myPlan->sBindColumn = NULL;
    aStatement->myPlan->sBindColumnCount = 0;
    aStatement->myPlan->sBindParam  = NULL;
    aStatement->myPlan->sBindParamCount = 0;

    aStatement->sharedFlag = ID_FALSE;

    /* PROJ-2677 DDL synchronization */
    qrc::setDDLSrcInfo( aStatement,
                        ID_FALSE,
                        0,
                        NULL,
                        0,
                        NULL );

    /* PROJ-2735 DDL Transaction */
    qrc::setDDLDestInfo( aStatement, 
                         0,
                         NULL,
                         0,
                         NULL );

    /* PROJ-2757 Advanced Global DDL */
    sdiGlobalDDL::clearInfo( aStatement );

    /* PROJ-2206 withStmtList manager alloc
     * PROJ-2415 Grouping Sets Clause
     * withStmtList manager clear -> stmtList manager clear for Re-parsing */    
    clearStmtListMgr( aStatement->myPlan->stmtListMgr );

    // BUG-41248 DBMS_SQL package
    aStatement->myPlan->bindNode = NULL;

    // BUG-36986
    aStatement->namedVarNode = NULL;

    // PROJ-2551 simple query 최적화
    qciMisc::initSimpleInfo( &(aStatement->simpleInfo) );
    aStatement->mFlag = 0;

    // BUG-43158 Enhance statement list caching in PSM
    for ( ; sStmtList != NULL; sStmtList = aStatement->mStmtList )
    {
        idlOS::memset( sStmtList->mStmtPoolStatus,
                       0,
                       aStatement->session->mQPSpecific.mStmtListInfo.mStmtPoolStatusSize );

        sStmtList->mParseTree = NULL;

        aStatement->mStmtList = sStmtList->mNext;
        sStmtList->mNext      = NULL;

        aStatement->session->mQPSpecific.mStmtListInfo.mStmtListFreedCount++;
    }

    // BUG-43158 Enhance statement list caching in PSM
    for ( ; sStmtList2 != NULL; sStmtList2 = aStatement->mStmtList2 )
    {
        idlOS::memset( sStmtList2->mStmtPoolStatus,
                       0,
                       aStatement->session->mQPSpecific.mStmtListInfo.mStmtPoolStatusSize );

        sStmtList2->mParseTree = NULL;

        aStatement->mStmtList2 = sStmtList2->mNext;
        sStmtList2->mNext      = NULL;

        aStatement->session->mQPSpecific.mStmtListInfo.mStmtListFreedCount++;
    }
    /* TASK-6744 */
    aStatement->mRandomPlanInfo = NULL;

    /* BUG-45899 */
    SDI_INIT_PRINT_INFO( &(aStatement->mShardPrintInfo) );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( sdi::allocAndInitQuerySetList( aStatement ) != IDE_SUCCESS );

    /* TASK-7219 Non-shard DML */
    aStatement->mShardPartialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;

    //----------------------------------
    // qci::initializeStatement()시에 mm으로부터 넘겨받은
    // session 정보로, freeStatement()이외에는 초기화시키면 안됨.
    // aStatement->session = NULL;
    //----------------------------------

    qcgPlan::initPlanProperty( aStatement->propertyEnv );

    qsvEnv::clearEnv(aStatement->spvEnv);

    /* BUG-45678 */
    if ( (aStatement->calledByPSMFlag == ID_FALSE) &&
         (aStatement->spxEnv->mIsInitialized == ID_TRUE) )
    {
        qsxEnv::reset(aStatement->spxEnv);
    }

    // initialize template
    sTemplate = QC_SHARED_TMPLATE(aStatement);

    if ( sTemplate != NULL )
    {
        sTemplate->planCount = 0;
        sTemplate->planFlag = NULL;
        sTemplate->tmplate.dataSize = 0;
        sTemplate->tmplate.data = NULL;
        sTemplate->tmplate.execInfoCnt = 0;
        sTemplate->tmplate.execInfo = NULL;
        sTemplate->tmplate.variableRow = ID_USHORT_MAX;
        sTemplate->numRows = 0;
        sTemplate->stmtType = QCI_STMT_MASK_MAX;
        sTemplate->flag = QC_TMP_INITIALIZE;
        sTemplate->smiStatementFlag = 0;
        sTemplate->insOrUptStmtCount = 0;
        sTemplate->insOrUptRowValueCount = NULL;
        sTemplate->insOrUptRow = NULL;

         /* PROJ-2209 DBTIMEZONE */
        sTemplate->unixdate = NULL;
        sTemplate->sysdate = NULL;
        sTemplate->currentdate = NULL;
        sTemplate->cursorMgr = NULL;
        sTemplate->tempTableMgr = NULL;
        sTemplate->fixedTableAutomataStatus = 0;

        // PROJ-1413 tupleVarList 초기화
        sTemplate->tupleVarGenNumber = 0;
        sTemplate->tupleVarList = NULL;

        // BUG-16422 execute중 임시 생성된 tableInfo의 관리
        sTemplate->tableInfoMgr = NULL;

        // PROJ-1436
        sTemplate->indirectRef = ID_FALSE;

        // To Fix PR-12659
        // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
        sTemplate->tmplate.rowCount = 0;
        sTemplate->tmplate.rowArrayCount = 0;

        for ( sCount = 0; sCount < MTC_TUPLE_TYPE_MAXIMUM; sCount++ )
        {
            sTemplate->tmplate.currentRow[sCount] = ID_USHORT_MAX;
        }

        // BUG-35828
        sTemplate->stmt = aStatement;

        /* PROJ-2452 Caching for DETERMINISTIC Function */
        sTemplate->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
        sTemplate->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
        sTemplate->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
        sTemplate->cacheObjCnt    = 0;
        sTemplate->cacheObjects   = NULL;

        /* PROJ-2448 Subquery caching */
        sTemplate->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

        // PROJ-2527 WITHIN GROUP AGGR
        sTemplate->tmplate.funcData = NULL;
        sTemplate->tmplate.funcDataCnt = 0;

        /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
        sTemplate->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                    ID_TRUE : ID_FALSE;
        sTemplate->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                    ID_TRUE : ID_FALSE;
        sTemplate->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

        //-------------------------------------------------------
        // PROJ-1358
        // 다음 질의 수행을 위하여 기본적인 공간을 할당받아둔다.
        // BUGBUG - clearStatement() 함수가 void 리턴이어서
        //          이를 위한 MM 코드를 수정하기 위한 부담이 크다.
        //        - 현재는 Memory 해제후 바로 할당하는 경우
        //          실패 가능성이 적기를 바랄 수 밖에...
        //-------------------------------------------------------

        // To Fix PR-12659
        // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
        // sTmplate->tmplate.rowArrayCount = MTC_TUPLE_ROW_INIT_CNT;

        // (void) allocInternalTuple( sTemplate,
        //                            QC_QMP_MEM(aStatement),
        //                            sTemplate->tmplate.rowArrayCount );
        /* PROJ-2462 Reuslt Cache */
        QC_RESULT_CACHE_INIT( &sTemplate->resultCache );

        // BUG-44710
        sdi::initDataInfo( &(sTemplate->shardExecData) );

        // BUG-44795
        sTemplate->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

        sTemplate->mUnnestViewNameIdx = 0;

        /* BUG-48776 */
        sTemplate->mSubqueryMode = QCU_SUBQUERY_MODE;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 2:
            (void)finishAndReleaseThread(aStatement);
            /* fallthrough */
        case 1:
            freePRLQExecutionMemory(aStatement);
            /* fallthrough */
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcg::closeStatement( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : 쿼리 수행중,
 *               execution수행에 관한 정보들을 삭제한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt sStage;
    qcTemplate * sPrivateTmplate = NULL;
    qcTemplate * sOrgTmplate     = NULL;

    IDE_ASSERT( aStatement->qmxMem != NULL );
    IDE_ASSERT( aStatement->qxcMem != NULL );

    sStage = 1;

    if ( ( QCU_EXECUTION_PLAN_MEMORY_CHECK == 1 ) &&
         ( QC_QMU_MEM(aStatement) != NULL ) )
    {
        if ( aStatement->sharedFlag == ID_TRUE )
        {
            // 공유된 plan을 오염시켜서는 안된다.
            if ( QC_QMP_MEM(aStatement)->compare( QC_QMU_MEM(aStatement) ) != 0 )
            {
                ideLog::log( IDE_SERVER_0,"[Notify : SQL Plan Cache] Plan Pollution : Session ID = "
                             "%d\n    Query => %s\n",
                             QCG_GET_SESSION_ID(aStatement),
                             aStatement->myPlan->stmtText );

                IDE_DASSERT( 0 );
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
    else
    {
        // Nothing to do.
    }

    // PROJ-1071
    IDE_TEST( joinThread( aStatement ) != IDE_SUCCESS );

    resetTemplateMember( aStatement );

    sStage = 0;
    IDE_TEST( finishAndReleaseThread( aStatement ) != IDE_SUCCESS );

    /* BUG-38294 */
    freePRLQExecutionMemory(aStatement);

    //ideLog::log(IDE_QP_2, "[INFO] Query Execution Memory Size : %"ID_UINT64_FMT" KB",
    //            aStatement->qmxMem->getSize()/1024);

    aStatement->qmxMem->freeAll(1);

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    aStatement->qxcMem->freeAll(1);

    // PROJ-2717 Internal procedure
    aStatement->qixMem->freeAll();

    // PROJ-2551 simple query 최적화
    qciMisc::initSimpleInfo( &(aStatement->simpleInfo) );

    sPrivateTmplate = QC_PRIVATE_TMPLATE( aStatement );

    // PROJ-2462 ResultCache
    // 쿼리가 끝나는 시점에 ResultCache가 MAX크기 만큼
    // 큰지 검사하고 만약 MAX보다 크다면 Result Cache를 free하고
    // 이후 수행부터는 ResultCache를 사용하지 않도록 한다.
    qmxResultCache::checkResultCacheMax( sPrivateTmplate );

    // BUG-44710
    sdi::closeDataInfo( aStatement,
                        &(sPrivateTmplate->shardExecData) );

    IDE_TEST( initTemplateMember( sPrivateTmplate,
                                  QC_QMX_MEM( aStatement ) )
              != IDE_SUCCESS );

    /* BUG-44279 Simple Query는 Template을 사용하지 않는다. */
    if ( ( ( aStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         ( ( aStatement->mFlag & QC_STMT_FAST_BIND_MASK )
           == QC_STMT_FAST_BIND_TRUE ) )
    {
        /* Nothing to do */
    }
    else
    {
        // allocStatement시 생성했던 private template 영역을 가져온다.
        if ( QC_SHARED_TMPLATE(aStatement) != NULL )
        {
            sOrgTmplate = aStatement->privatePlan.sTmplate;

            // stack 설정
            sPrivateTmplate->tmplate.stackBuffer = sOrgTmplate->tmplate.stackBuffer;
            sPrivateTmplate->tmplate.stack       = sOrgTmplate->tmplate.stack;
            sPrivateTmplate->tmplate.stackCount  = sOrgTmplate->tmplate.stackCount;
            sPrivateTmplate->tmplate.stackRemain = sOrgTmplate->tmplate.stackRemain;

            // date format 설정
            sPrivateTmplate->tmplate.dateFormat  = sOrgTmplate->tmplate.dateFormat;

            /* PROJ-2208 Multi Currency */
            sPrivateTmplate->tmplate.nlsCurrency = sOrgTmplate->tmplate.nlsCurrency;

            // stmt 설정
            sPrivateTmplate->stmt                = aStatement;
        }
        else
        {
            // fix BUG-33660
            sPrivateTmplate = QC_PRIVATE_TMPLATE(aStatement);

            // stack 설정
            sPrivateTmplate->tmplate.stack       = sPrivateTmplate->tmplate.stackBuffer;
            sPrivateTmplate->tmplate.stackRemain = sPrivateTmplate->tmplate.stackCount;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)finishAndReleaseThread(aStatement);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC
qcg::fixAfterParsing( qcStatement * aStatement )
{
    return qtc::fixAfterParsing( QC_SHARED_TMPLATE(aStatement) );
}

IDE_RC
qcg::fixAfterValidation( qcStatement * aStatement )
{
    // PROJ-2163
    IDE_TEST( qcg::fixOutBindParam( aStatement ) != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    if ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_EXEC_AB )
    {
        IDE_TEST( fixAfterValidationAB(aStatement) != IDE_SUCCESS );
    }

    //---------------------------------------------------------
    // [Cursor Flag의 보정]
    // Validation이 끝나면 질의문이 접근하는 Table의 유형에 따라
    // Disk, Memory, Disk/Memory 의 세 가지 형태로
    // Cursor의 접근 형태가 결정된다.
    // 그러나, Disk Table에 대한 DDL의 경우 Meta 갱신을 위하여
    // Memory Table에 대한 접근이 필요하게 된다.
    // 이를 위해 DDL의 경우 Memory Cursor의 Flag을 무조건 ORing한다.
    //---------------------------------------------------------

    if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
         == QCI_STMT_MASK_DDL )
    {
        // DDL일 경우
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}
IDE_RC qcg::fixAfterValidationAB( qcStatement * aQcStmt )
{
    UInt i;
    UInt sFlag;
    mtcTemplate * sDstTemplate;

    sDstTemplate = &(QC_SHARED_TMPLATE(aQcStmt)->tmplate);

    for (i = 0; i < sDstTemplate->rowCount; i++)
    {
        sFlag = sDstTemplate->rows[i].lflag;

        if ( (sFlag & MTC_TUPLE_ROW_ALLOCATE_TRUE) &&
             (sDstTemplate->rows[i].rowMaximum > 0) )
        {
            /* BUG-44382 clone tuple 성능개선 */
            if ( sFlag & MTC_TUPLE_ROW_MEMSET_TRUE )
            {
                // BUG-15548
                IDE_TEST( QC_QMP_MEM(aQcStmt)->cralloc(
                              ID_SIZEOF(SChar) * sDstTemplate->rows[i].rowMaximum,
                              (void**)&(sDstTemplate->rows[i].row))
                          != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( QC_QMP_MEM(aQcStmt)->alloc(
                              ID_SIZEOF(SChar) * sDstTemplate->rows[i].rowMaximum,
                              (void**)&(sDstTemplate->rows[i].row))
                          != IDE_SUCCESS);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcg::fixAfterOptimization( qcStatement * aStatement )
{
    IDE_TEST( qtc::fixAfterOptimization( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC
qcg::stepAfterPVO( qcStatement * aStatement )
{
    qcTemplate  * sPrivateTmplate;
    qcTemplate  * sOrgTmplate;
    iduVarMemList * sQmeMem = NULL;

    sPrivateTmplate = QC_PRIVATE_TMPLATE(aStatement);

    sQmeMem = QC_QME_MEM( aStatement );

    if ( sPrivateTmplate->planCount > 0 )
    {
        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::sPrivateTmplate->planFlag",
                        idERR_ABORT_InsufficientMemory );
        IDE_TEST( sQmeMem->alloc(ID_SIZEOF(UInt) * sPrivateTmplate->planCount,
                    (void**)&(sPrivateTmplate->planFlag) )
                != IDE_SUCCESS );

        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::sPrivateTmplate->cursorMgr",
                        idERR_ABORT_InsufficientMemory );
        IDE_TEST( sQmeMem->alloc(ID_SIZEOF(qmcCursor),
                    (void**)&(sPrivateTmplate->cursorMgr) )
                != IDE_SUCCESS );

        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::sPrivateTmplate->tempTableMgr",
                        idERR_ABORT_InsufficientMemory );
        IDE_TEST( sQmeMem->cralloc(ID_SIZEOF(qmcdTempTableMgr),
                    (void**) &(sPrivateTmplate->tempTableMgr) )
                != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    if ( sPrivateTmplate->tmplate.dataSize > 0 )
    {
        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::sPrivateTmplate->data",
                        idERR_ABORT_InsufficientMemory );
        IDE_TEST(sQmeMem->alloc(sPrivateTmplate->tmplate.dataSize,
                    (void**)&(sPrivateTmplate->tmplate.data))
                != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    if ( sPrivateTmplate->tmplate.execInfoCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::sPrivateTmplate->execInfo",
                        idERR_ABORT_InsufficientMemory );
        IDE_TEST(sQmeMem->alloc(sPrivateTmplate->tmplate.execInfoCnt *
                    ID_SIZEOF(UInt),
                    (void**)&(sPrivateTmplate->tmplate.execInfo))
                != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-2527 WITHIN GROUP AGGR
    if ( sPrivateTmplate->tmplate.funcDataCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::tmplate.funcData",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( sQmeMem->alloc( sPrivateTmplate->tmplate.funcDataCnt *
                                  ID_SIZEOF(mtfFuncDataBasicInfo*),
                                  (void**)&(sPrivateTmplate->tmplate.funcData) )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    if ( sPrivateTmplate->cacheObjCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::stepAfterPVO::alloc::cacheObjects",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sQmeMem->alloc( sPrivateTmplate->cacheObjCnt * ID_SIZEOF(qtcCacheObj),
                                  (void**)&(sPrivateTmplate->cacheObjects) )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( initTemplateMember( sPrivateTmplate,
                                  QC_QMX_MEM( aStatement ) )
              != IDE_SUCCESS );

    // allocStatement시 생성했던 private template 영역을 가져온다.
    if ( QC_SHARED_TMPLATE(aStatement) != NULL )
    {
        sOrgTmplate = aStatement->privatePlan.sTmplate;

        // stack 설정
        sPrivateTmplate->tmplate.stackBuffer = sOrgTmplate->tmplate.stackBuffer;
        sPrivateTmplate->tmplate.stack       = sOrgTmplate->tmplate.stack;
        sPrivateTmplate->tmplate.stackCount  = sOrgTmplate->tmplate.stackCount;
        sPrivateTmplate->tmplate.stackRemain = sOrgTmplate->tmplate.stackRemain;

        // date format 설정
        sPrivateTmplate->tmplate.dateFormat     = sOrgTmplate->tmplate.dateFormat;

        /* PROJ-2208 Multi Currency */
        sPrivateTmplate->tmplate.nlsCurrency    = sOrgTmplate->tmplate.nlsCurrency;

        // stmt 설정
        sPrivateTmplate->stmt = aStatement;
    }
    else
    {
        // fix BUG-33660
        sPrivateTmplate = QC_PRIVATE_TMPLATE(aStatement);

        // stack 설정
        sPrivateTmplate->tmplate.stack       = sPrivateTmplate->tmplate.stackBuffer;
        sPrivateTmplate->tmplate.stackRemain = sPrivateTmplate->tmplate.stackCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-29907
    sPrivateTmplate->planFlag         = NULL;
    sPrivateTmplate->cursorMgr        = NULL;
    sPrivateTmplate->tempTableMgr     = NULL;
    sPrivateTmplate->tmplate.data     = NULL;
    sPrivateTmplate->tmplate.execInfo = NULL;
    sPrivateTmplate->tmplate.funcData = NULL;
    sPrivateTmplate->cacheObjects     = NULL;

    return IDE_FAILURE;
}

IDE_RC
qcg::setPrivateArea( qcStatement * aStatement )
{
    // private template 설정
    QC_PRIVATE_TMPLATE(aStatement) = QC_SHARED_TMPLATE(aStatement);
    QC_SHARED_TMPLATE(aStatement) = NULL;

    // prepared private template 설정
    aStatement->prepTemplateHeader = NULL;
    aStatement->prepTemplate = NULL;

    // stmt 설정
    QC_PRIVATE_TMPLATE(aStatement)->stmt = aStatement;

    // bind parameter
    // PROJ-2163
    if( aStatement->myPlan->sBindParamCount > 0 )
    {
        if( aStatement->pBindParam == NULL )
        {
            IDE_TEST( QC_QMB_MEM(aStatement)->alloc(
                            ID_SIZEOF(qciBindParamInfo)
                                * aStatement->myPlan->sBindParamCount,
                            (void**) & aStatement->pBindParam )
                    != IDE_SUCCESS );

            idlOS::memcpy( (void*) aStatement->pBindParam,
                           (void*) aStatement->myPlan->sBindParam,
                           ID_SIZEOF(qciBindParamInfo)
                               * aStatement->myPlan->sBindParamCount );

            aStatement->pBindParamCount = aStatement->myPlan->sBindParamCount;

            // pBindParam 의 값이 모두 바뀌었으므로 flag 를 세팅한다.
            aStatement->pBindParamChangedFlag = ID_TRUE;
        }
        else
        {
            // 사용자가 바인드한 정보는 덮어쓰지 않는다.
            // Nothing to do.
        }
    }
    else
    {
        // when host variable count == 0
        // Nothing to do.
    }

    aStatement->myPlan->sBindParam = NULL;
    aStatement->myPlan->sBindParamCount = 0;

    /* PROJ-2462 Result Cache */
    qmxResultCache::allocResultCacheData( QC_PRIVATE_TMPLATE( aStatement ),
                                          QC_QME_MEM( aStatement ) );

    // BUG-44710
    IDE_TEST( sdi::allocDataInfo(
                  &(QC_PRIVATE_TMPLATE( aStatement )->shardExecData),
                  QC_QME_MEM( aStatement ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcg::allocAndInitTemplateMember( qcTemplate    * aTemplate,
                                 iduMemory     * aMemory )
{
    IDE_ASSERT( aTemplate != NULL );

    aTemplate->planFlag     = NULL;
    aTemplate->cursorMgr    = NULL;
    aTemplate->tempTableMgr = NULL;

    if (aTemplate->planCount > 0)
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF(UInt) * aTemplate->planCount,
                                 (void**)&(aTemplate->planFlag))
                 != IDE_SUCCESS);

        IDE_TEST(aMemory->alloc( ID_SIZEOF(qmcCursor),
                                 (void**)&(aTemplate->cursorMgr) )
                 != IDE_SUCCESS);

        IDE_TEST(aMemory->cralloc( ID_SIZEOF(qmcdTempTableMgr),
                                   (void**) &(aTemplate->tempTableMgr) )
                 != IDE_SUCCESS);
    }

    if ( aTemplate->tmplate.dataSize > 0 )
    {
        IDE_TEST(aMemory->alloc( aTemplate->tmplate.dataSize,
                                 (void**)&(aTemplate->tmplate.data))
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    if ( aTemplate->tmplate.execInfoCnt > 0 )
    {
        IDE_TEST(aMemory->alloc( aTemplate->tmplate.execInfoCnt *
                                 ID_SIZEOF(UInt),
                                 (void**)&(aTemplate->tmplate.execInfo))
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-2527 WITHIN GROUP AGGR
    if ( aTemplate->tmplate.funcDataCnt > 0 )
    {
        IDE_TEST( aMemory->alloc( aTemplate->tmplate.funcDataCnt *
                                  ID_SIZEOF(mtfFuncDataBasicInfo*),
                                  (void**)&(aTemplate->tmplate.funcData) )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    if ( aTemplate->cacheObjCnt > 0 )
    {
        IDE_TEST( aMemory->alloc( aTemplate->cacheObjCnt * ID_SIZEOF(qtcCacheObj),
                                  (void**)&(aTemplate->cacheObjects) )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( initTemplateMember( aTemplate,
                                  aMemory ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-29907
    aTemplate->planFlag         = NULL;
    aTemplate->cursorMgr        = NULL;
    aTemplate->tempTableMgr     = NULL;
    aTemplate->tmplate.data     = NULL;
    aTemplate->tmplate.execInfo = NULL;
    aTemplate->tmplate.funcData = NULL;
    aTemplate->cacheObjects     = NULL;

    return IDE_FAILURE;
}

void
qcg::setSmiTrans( qcStatement * aStatement, smiTrans * aSmiTrans )
{
    (QC_SMI_STMT( aStatement ))->mTrans = aSmiTrans;
}

void
qcg::getSmiTrans( qcStatement * aStatement, smiTrans ** aSmiTrans )
{
    *aSmiTrans = (QC_SMI_STMT( aStatement ))->mTrans;
}

void qcg::getStmtText( qcStatement  * aStatement,
                       SChar       ** aText,
                       SInt         * aTextLen )
{
    *aText    = aStatement->myPlan->stmtText;
    *aTextLen = aStatement->myPlan->stmtTextLen;
}

IDE_RC qcg::setStmtText( qcStatement * aStatement,
                         SChar       * aText,
                         SInt          aTextLen )
{
    // PROJ-2163
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  aTextLen + 1,
                  (void **)&aStatement->myPlan->stmtText )
              != IDE_SUCCESS );

    idlOS::memcpy( aStatement->myPlan->stmtText,
                   aText,
                   aTextLen );

    aStatement->myPlan->stmtText[aTextLen] = '\0';
    aStatement->myPlan->stmtTextLen = aTextLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


//--------------------------------------------------
// 아래 session 정보를 구하는 함수들에서
// aStatement->session->mMmSession이 NULL인 부분은
// qp내부적으로 처리하는 질의문의 경우,
// session 정보가 따로 없기 때문에
// default session 정보를 얻기 위함임.
// ( 코드내에서 qp내부에서 처리하는 질의문인지 아닌지의 여부를
//   따지지 않고, 공통으로 사용하기 위함. )
//--------------------------------------------------
const mtlModule* qcg::getSessionLanguage( qcStatement * aStatement )
{
    const mtlModule *     sMtlModule ;

    if ( aStatement->session->mMmSession != NULL )
    {
        sMtlModule = qci::mSessionCallback.mGetLanguage(
            aStatement->session->mMmSession );
    }
    else
    {
        sMtlModule = mtl::defaultModule();
    }

    return sMtlModule;
}

scSpaceID qcg::getSessionTableSpaceID( qcStatement * aStatement )
{
    scSpaceID    sSmSpaceID;

    if ( aStatement->session->mMmSession != NULL )
    {
        sSmSpaceID = qci::mSessionCallback.mGetTableSpaceID(
            aStatement->session->mMmSession );
    }
    else
    {
        sSmSpaceID = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
    }

    return sSmSpaceID;
}

scSpaceID qcg::getSessionTempSpaceID( qcStatement * aStatement )
{
    scSpaceID    sSmSpaceID;

    if ( aStatement->session->mMmSession != NULL )
    {
        sSmSpaceID = qci::mSessionCallback.mGetTempSpaceID(
            aStatement->session->mMmSession );
    }
    else
    {
        sSmSpaceID = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
    }

    return sSmSpaceID;
}

void  qcg::setSessionUserID( qcStatement * aStatement,
                             UInt          aUserID )
{
    if( aStatement->session->mMmSession != NULL )
    {
        qci::mSessionCallback.mSetUserID( aStatement->session->mMmSession,
                                          aUserID );
    }
    else
    {
        // BUG-24470
        // 내부에서 생성된 session이라도 setSessionUserID가 호출되는
        // 경우는 userID를 별도로 저장해야 한다.
        aStatement->session->mUserID = aUserID;
    }
}

UInt qcg::getSessionUserID( qcStatement * aStatement )
{
    UInt  sUserID;

    if( aStatement->session->mMmSession != NULL )
    {
        sUserID = qci::mSessionCallback.mGetUserID(
            aStatement->session->mMmSession );
    }
    else
    {
        // BUG-24470
        sUserID = aStatement->session->mUserID;
    }

    return sUserID;
}

void  qcg::setSessionInvokeUserName( qcStatement * aStatement,
                                     SChar       * aInvokeUserName )
{
    if( aStatement->session->mMmSession != NULL )
    {
        qci::mSessionCallback.mSetInvokeUserName( aStatement->session->mMmSession,
                                                  aInvokeUserName );
    }
}

SChar * qcg::getSessionInvokeUserName( qcStatement * aStatement )
{
    SChar * sInvokeUserName;

    if( aStatement->session->mMmSession != NULL )
    {
        sInvokeUserName = qci::mSessionCallback.mGetInvokeUserName(
            aStatement->session->mMmSession );
    }
    else
    {
        sInvokeUserName = qcg::mInternalUserName;
    }

    return sInvokeUserName;
}

idBool qcg::getSessionIsSysdbaUser( qcStatement * aStatement )
{
    idBool   sIsSysdbaUser;

    if( aStatement->session->mMmSession != NULL )
    {
        sIsSysdbaUser = qci::mSessionCallback.mIsSysdbaUser(
            aStatement->session->mMmSession );
    }
    else
    {
        sIsSysdbaUser = ID_FALSE;
    }

    return sIsSysdbaUser;
}

UInt qcg::getSessionOptimizerMode( qcStatement * aStatement )
{
    UInt  sOptimizerMode;

    if( aStatement->session->mMmSession != NULL )
    {
        sOptimizerMode = qci::mSessionCallback.mGetOptimizerMode(
            aStatement->session->mMmSession );
    }
    else
    {
        if ( ( aStatement->session->mQPSpecific.mFlag
               & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
             == QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
        {
            // BUG-26017
            // [PSM] server restart시 수행되는
            // psm load과정에서 관련프로퍼티를 참조하지 못하는 경우 있음.
            sOptimizerMode = QCU_OPTIMIZER_MODE;
        }
        else
        {
            // 내부에서 처리되는 sql문들은 cost로 처리한다.
            // ( meta테이블관련 )
            // optimizer mode = cost
            sOptimizerMode = QCG_INTERNAL_OPTIMIZER_MODE;
        }
    }

    return sOptimizerMode;
}

UInt qcg::getSessionSelectHeaderDisplay( qcStatement * aStatement )
{
    UInt   sSelectHeaderDisplay;

    if( aStatement->session->mMmSession != NULL )
    {
        sSelectHeaderDisplay = qci::mSessionCallback.mGetSelectHeaderDisplay(
            aStatement->session->mMmSession );
    }
    else
    {
        sSelectHeaderDisplay = QCG_INTERNAL_SELECT_HEADER_DISPLAY;
    }

    return sSelectHeaderDisplay;
}

UInt qcg::getSessionStackSize( qcStatement * aStatement )
{
    UInt   sStackSize;

    if( aStatement->session->mMmSession != NULL )
    {
        sStackSize = qci::mSessionCallback.mGetStackSize(
            aStatement->session->mMmSession );
    }
    else
    {
        // BUG-26017
        // [PSM] server restart시 수행되는
        // psm load과정에서 관련프로퍼티를 참조하지 못하는 경우 있음.
        sStackSize = QCU_QUERY_STACK_SIZE;
    }

    return sStackSize;
}

UInt qcg::getSessionNormalFormMaximum( qcStatement * aStatement )
{
    UInt   sNormalFormMax;

    if( aStatement->session->mMmSession != NULL )
    {
        sNormalFormMax = qci::mSessionCallback.mGetNormalFormMaximum(
            aStatement->session->mMmSession );
    }
    else
    {
        sNormalFormMax = QCU_NORMAL_FORM_MAXIMUM;
    }

    return sNormalFormMax;
}

SChar* qcg::getSessionDateFormat( qcStatement * aStatement )
{
    SChar * sDateFormat;

    if( aStatement->session->mMmSession != NULL )
    {
        sDateFormat = qci::mSessionCallback.mGetDateFormat(
            aStatement->session->mMmSession);
    }
    else
    {
        sDateFormat = MTU_DEFAULT_DATE_FORMAT;
    }

    return sDateFormat;
}

/* PROJ-2209 DBTIMEZONE */
SChar* qcg::getSessionTimezoneString( qcStatement * aStatement )
{
    SChar * sTimezoneString;

    if ( aStatement->session->mMmSession != NULL )
    {
        sTimezoneString = qci::mSessionCallback.mGetSessionTimezoneString(
            aStatement->session->mMmSession );
    }
    else
    {
        sTimezoneString = MTU_DB_TIMEZONE_STRING;
    }

    return sTimezoneString;
}

SLong qcg::getSessionTimezoneSecond ( qcStatement * aStatement )
{
    SLong  sTimezoneSecond;

    if ( aStatement->session->mMmSession != NULL )
    {
        sTimezoneSecond = qci::mSessionCallback.mGetSessionTimezoneSecond(
            aStatement->session->mMmSession );
    }
    else
    {
        sTimezoneSecond = MTU_DB_TIMEZONE_SECOND;
    }

    return sTimezoneSecond;
}

UInt qcg::getSessionSTObjBufSize( qcStatement * aStatement )
{
    UInt   sObjBufSize = QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE; // To Fix BUG-32072

    if( aStatement->session->mMmSession != NULL )
    {
        sObjBufSize = qci::mSessionCallback.mGetSTObjBufSize(
            aStatement->session->mMmSession);
    }
    else
    {
        // Nothing to do.
    }

    if ( sObjBufSize == QMS_NOT_DEFINED_ST_OBJECT_BUFFER_SIZE ) // To Fix BUG-32072
    {
        sObjBufSize = QCU_ST_OBJECT_BUFFER_SIZE;
    }
    else
    {
        // Nothing to do.
    }

    return sObjBufSize;
}

// BUG-20767
SChar* qcg::getSessionUserName( qcStatement * aStatement )
{
    SChar * sUserName;

    if( aStatement->session->mMmSession != NULL )
    {
        sUserName = qci::mSessionCallback.mGetUserName(
            aStatement->session->mMmSession);
    }
    else
    {
        sUserName = qcg::mInternalUserName;
    }

    return sUserName;
}

// BUG-19041
UInt qcg::getSessionID( qcStatement * aStatement )
{
    UInt  sSessionID;

    if( aStatement->session->mMmSession != NULL )
    {
        sSessionID = qci::mSessionCallback.mGetSessionID(
            aStatement->session->mMmSession );
    }
    else
    {
        sSessionID = ID_UINT_MAX;
    }

    return sSessionID;
}

// PROJ-2002 Column Security
SChar* qcg::getSessionLoginIP( qcStatement * aStatement )
{    
    if( aStatement->session->mMmSession != NULL )
    {
        return qci::mSessionCallback.mGetSessionLoginIP(
            aStatement->session->mMmSession );
    }
    else
    {
        return (SChar*)"127.0.0.1";
    }
}

// PROJ-2638
SChar* qcg::getSessionUserPassword( qcStatement * aStatement )
{
    static SChar   sNullPassword[1] = { 0 };
    SChar        * sUserPassword;

    if ( aStatement->session->mMmSession != NULL )
    {
        sUserPassword = qci::mSessionCallback.mGetUserPassword(
            aStatement->session->mMmSession);
    }
    else
    {
        sUserPassword = sNullPassword;
    }

    return sUserPassword;
}

ULong qcg::getSessionShardPIN( qcStatement  * aStatement )
{
    ULong  sShardPIN;

    if ( aStatement->session->mMmSession != NULL )
    {
        sShardPIN = qci::mSessionCallback.mGetShardPIN(
            aStatement->session->mMmSession );
    }
    else
    {
        sShardPIN = 0;
    }

    return sShardPIN;
}

ULong qcg::getSessionShardMetaNumber( qcStatement * aStatement )
{
    ULong  sShardMetaNumber = ID_ULONG(0);

    if ( aStatement->session->mMmSession != NULL )
    {
        sShardMetaNumber = qci::mSessionCallback.mGetShardMetaNumber(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sShardMetaNumber;
}

ULong qcg::getLastSessionShardMetaNumber( qcStatement * aStatement )
{
    ULong sShardMetaNumber = SDI_NULL_SMN;

    if ( aStatement->session->mMmSession != NULL )
    {
        sShardMetaNumber = qci::mSessionCallback.mGetLastShardMetaNumber(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sShardMetaNumber;
}

idBool qcg::getSessionIsShardUserSession( qcStatement * aStatement )
{
    idBool sIsShardUserSession = ID_FALSE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sIsShardUserSession = qci::mSessionCallback.mIsShardUserSession(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do. */
    }

    return sIsShardUserSession;
}

/* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
idBool qcg::getCallByShardAnalyzeProtocol( qcStatement * aStatement )
{
    idBool sCallByShardAnalyzeProtocol = ID_FALSE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sCallByShardAnalyzeProtocol = qci::mSessionCallback.mCallByShardAnalyzeProtocol(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do. */
    }

    return sCallByShardAnalyzeProtocol;
}

SChar* qcg::getSessionShardNodeName( qcStatement  * aStatement )
{
    SChar  * sNodeName;

    if ( aStatement->session->mMmSession != NULL )
    {
        sNodeName = qci::mSessionCallback.mGetShardNodeName(
            aStatement->session->mMmSession );
    }
    else
    {
        sNodeName = NULL;
    }

    return sNodeName;
}

UInt qcg::getSessionShardSessionType( qcStatement * aStatement )
{
    UInt sShardSessionType = SDI_SESSION_TYPE_USER;

    if ( aStatement->session->mMmSession != NULL )
    {
        sShardSessionType = qci::mSessionCallback.mGetShardSessionType( 
            aStatement->session->mMmSession );
    }

    return sShardSessionType;
}

UChar qcg::getSessionExplainPlan( qcStatement  * aStatement )
{
    UChar  sExplainPlan;

    if ( aStatement->session->mMmSession != NULL )
    {
        sExplainPlan = qci::mSessionCallback.mGetExplainPlan(
            aStatement->session->mMmSession );
    }
    else
    {
        sExplainPlan = 0;
    }

    return sExplainPlan;
}

UInt qcg::getSessionGTXLevel( qcStatement * aStatement )
{
    UInt sGTXLevel = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sGTXLevel = qci::mSessionCallback.mGetGTXLevel( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sGTXLevel;
}

idBool qcg::isGTxSession( qcStatement * aStatement )
{
    return aStatement->session->mQPSpecific.mIsGTx;
}

idBool qcg::isGCTxSession( qcStatement * aStatement )
{
    return aStatement->session->mQPSpecific.mIsGCTx;
}

UInt qcg::getReplicationDDLSync( qcStatement * aStatement )
{
    UInt sDDLSync;

    if ( aStatement->session->mMmSession != NULL )
    {
        sDDLSync = qci::mSessionCallback.mGetReplicationDDLSync(
            aStatement->session->mMmSession );
    }
    else
    {
        sDDLSync = 0;
    }

    return sDDLSync;

}

idBool qcg::getIsNeedDDLInfo( qcStatement * aStatement )
{
    idBool sIsNeedDDLInfo = ID_FALSE;

    if ( getTransactionalDDL( aStatement ) == ID_TRUE ) 
    {
        sIsNeedDDLInfo = ID_TRUE;
    }

    return sIsNeedDDLInfo;
}

idBool qcg::getTransactionalDDL( qcStatement * aStatement )
{
    idBool sTransactionalDDL = ID_FALSE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sTransactionalDDL = qci::mSessionCallback.mTransactionalDDL( aStatement->session->mMmSession );
    }
    else
    {
        sTransactionalDDL = getIsRollbackableInternalDDL( aStatement );
    }

    return sTransactionalDDL;
}

idBool qcg::getIsRollbackableInternalDDL( qcStatement * aStatement )
{
    idBool sIsRollbackableInternalDDL = ID_FALSE;

    /* internal tranasctional DDL operation */
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_ROLLBACKABLE_DDL_MASK )
         == QC_SESSION_ROLLBACKABLE_DDL_TRUE )
    {
        sIsRollbackableInternalDDL = ID_TRUE;
    }

    return sIsRollbackableInternalDDL;
}

idBool qcg::getGlobalDDL( qcStatement * aStatement )
{
    idBool sGlobalDDL = ID_FALSE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sGlobalDDL = qci::mSessionCallback.mGlobalDDL( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sGlobalDDL;
}

// BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
UInt qcg::getSessionOptimizerDefaultTempTbsType( qcStatement * aStatement )
{
    UInt   sOptimizerDefaultTempTbsType;

    if( aStatement->session->mMmSession != NULL )
    {
        sOptimizerDefaultTempTbsType
            = qci::mSessionCallback.mGetOptimizerDefaultTempTbsType(
                aStatement->session->mMmSession );
    }
    else
    {
        if ( ( aStatement->session->mQPSpecific.mFlag
               & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
             == QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
        {
            // BUG-26017
            // [PSM] server restart시 수행되는
            // psm load과정에서 관련프로퍼티를 참조하지 못하는 경우 있음.
            sOptimizerDefaultTempTbsType = QCU_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE;
        }
        else
        {
            // 내부에서 처리되는 sql문들은 0으로 처리한다.
            // ( meta테이블관련 )
            // optimizer내부에서 판단해서 처리
            sOptimizerDefaultTempTbsType = QCG_INTERNAL_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE;
        }
    }

    return sOptimizerDefaultTempTbsType;
}

// PROJ-1579 NCHAR
UInt qcg::getSessionNcharLiteralReplace( qcStatement * aStatement )
{
    UInt     sLiteralRep;

    if( aStatement->session->mMmSession != NULL )
    {
        sLiteralRep = qci::mSessionCallback.mGetNcharLiteralReplace(
            aStatement->session->mMmSession );
    }
    else
    {
        sLiteralRep = MTU_NLS_NCHAR_LITERAL_REPLACE;
    }

    return sLiteralRep;
}

//BUG-21122
UInt qcg::getSessionAutoRemoteExec( qcStatement * aStatement )
{
    UInt     sAutoRemoteExec;

    if( aStatement->session->mMmSession != NULL )
    {
        sAutoRemoteExec = qci::mSessionCallback.mGetAutoRemoteExec(
            aStatement->session->mMmSession );
    }
    else
    {
        if ( ( aStatement->session->mQPSpecific.mFlag
               & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
             == QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
        {
            // BUG-26017
            // [PSM] server restart시 수행되는
            // psm load과정에서 관련프로퍼티를 참조하지 못하는 경우 있음.
            sAutoRemoteExec = QCU_AUTO_REMOTE_EXEC;
        }
        else
        {
            sAutoRemoteExec = QC_AUTO_REMOTE_EXEC_DISABLE; //default disable
        }
    }

    return sAutoRemoteExec;
}

// BUG-34830
UInt qcg::getSessionTrclogDetailPredicate(qcStatement * aStatement)
{
    UInt sTrclogDetailPredicate;

    if (aStatement->session->mMmSession != NULL)
    {
        sTrclogDetailPredicate =
            qci::mSessionCallback.mGetTrclogDetailPredicate(aStatement->session->mMmSession);
    }
    else
    {
        sTrclogDetailPredicate = QCU_TRCLOG_DETAIL_PREDICATE;
    }

    return sTrclogDetailPredicate;
}

SInt qcg::getOptimizerDiskIndexCostAdj( qcStatement * aStatement )
{
    SInt    sOptimizerDiskIndexCostAdj;

    if( aStatement->session->mMmSession != NULL )
    {
        sOptimizerDiskIndexCostAdj = qci::mSessionCallback.mGetOptimizerDiskIndexCostAdj(
            aStatement->session->mMmSession );
    }
    else
    {
        sOptimizerDiskIndexCostAdj = QCU_OPTIMIZER_DISK_INDEX_COST_ADJ;
    }

    return sOptimizerDiskIndexCostAdj;
}

// BUG-43736
SInt qcg::getOptimizerMemoryIndexCostAdj( qcStatement * aStatement )
{
    SInt    sOptimizerMemoryIndexCostAdj;

    if( aStatement->session->mMmSession != NULL )
    {
        sOptimizerMemoryIndexCostAdj = qci::mSessionCallback.mGetOptimizerMemoryIndexCostAdj(
            aStatement->session->mMmSession );
    }
    else
    {
        sOptimizerMemoryIndexCostAdj = QCU_OPTIMIZER_MEMORY_INDEX_COST_ADJ;
    }

    return sOptimizerMemoryIndexCostAdj;
}

/* PROJ-2047 Strengthening LOB - LOBCACHE */
UInt qcg::getLobCacheThreshold( qcStatement * aStatement )
{
    UInt sLobCacheThreshold = 0;

    if( aStatement->session->mMmSession != NULL )
    {
        sLobCacheThreshold = qci::mSessionCallback.mGetLobCacheThreshold(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sLobCacheThreshold;
}

/* PROJ-1090 Function-based Index */
UInt qcg::getSessionQueryRewriteEnable( qcStatement * aStatement )
{
    UInt sQueryRewriteEnable = 0;

    if( aStatement->session->mMmSession != NULL )
    {
        sQueryRewriteEnable = qci::mSessionCallback.mGetQueryRewriteEnable(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sQueryRewriteEnable;
}

// BUG-38430
ULong qcg::getSessionLastProcessRow( qcStatement * aStatement )
{
    ULong sLastProcessRow;

    if( aStatement->session->mMmSession != NULL )
    {
        sLastProcessRow = qci::mSessionCallback.mGetSessionLastProcessRow(
            aStatement->session->mMmSession );
    }
    else
    {
        sLastProcessRow = 0;
    }

    return sLastProcessRow;
}

/* PROJ-1812 ROLE */
UInt* qcg::getSessionRoleList( qcStatement * aStatement )
{
    static UInt   sInternalRoleList[2] = { QC_SYSTEM_USER_ID, ID_UINT_MAX };
    UInt        * sRoleList;

    if( aStatement->session->mMmSession != NULL )
    {
        sRoleList = qci::mSessionCallback.mGetRoleList(
            aStatement->session->mMmSession );
    }
    else
    {
        /* internal user용 */
        sRoleList = sInternalRoleList;
    }

    return sRoleList;
}

/* PROJ-2441 flashback */
UInt qcg::getSessionRecyclebinEnable( qcStatement * aStatement )
{
    UInt sRecyclebinEnable = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sRecyclebinEnable = qci::mSessionCallback.mGetRecyclebinEnable(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sRecyclebinEnable;
}

UInt qcg::getSessionPrintOutEnable( qcStatement * aStatement )
{
    UInt sPrintOutEnable = 1;

    if ( aStatement->session->mMmSession != NULL )
    {
        sPrintOutEnable = qci::mSessionCallback.mGetPrintOutEnable(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sPrintOutEnable;
}

// BUG-41398 use old sort
UInt qcg::getSessionUseOldSort( qcStatement * aStatement )
{
    UInt sUseOldSort = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sUseOldSort = qci::mSessionCallback.mGetUseOldSort(
            aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sUseOldSort;
}

/* BUG-41561 */
UInt qcg::getSessionLoginUserID( qcStatement * aStatement )
{
    UInt sLoginUserID = QC_EMPTY_USER_ID;

    if ( aStatement->session->mMmSession != NULL )
    {
        sLoginUserID = qci::mSessionCallback.mGetLoginUserID(
            aStatement->session->mMmSession );
    }
    else
    {
        sLoginUserID = aStatement->session->mUserID;
    }

    return sLoginUserID; 
}

// BUG-41944
mtcArithmeticOpMode qcg::getArithmeticOpMode( qcStatement * aStatement )
{
    mtcArithmeticOpMode sOpMode;

    if ( aStatement->session->mMmSession != NULL )
    {
        sOpMode = (mtcArithmeticOpMode)
            qci::mSessionCallback.mGetArithmeticOpMode(
                aStatement->session->mMmSession );
    }
    else
    {
        sOpMode = MTC_ARITHMETIC_OPERATION_DEFAULT;
    }

    return sOpMode; 
}

/* PROJ-2462 Result Cache */
UInt qcg::getSessionResultCacheEnable( qcStatement * aStatement )
{
    UInt sResultCacheEnable = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sResultCacheEnable =
            qci::mSessionCallback.mGetResultCacheEnable( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sResultCacheEnable;
}
/* PROJ-2462 Result Cache */
UInt qcg::getSessionTopResultCacheMode( qcStatement * aStatement )
{
    UInt sTopResultCache = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sTopResultCache = qci::mSessionCallback.mGetTopResultCacheMode( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sTopResultCache;
}
/* PROJ-2492 Dynamic sample selection */
UInt qcg::getSessionOptimizerAutoStats( qcStatement * aStatement )
{
    UInt sOptimizerAutoStats = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sOptimizerAutoStats =
            qci::mSessionCallback.mGetOptimizerAutoStats( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sOptimizerAutoStats;
}

idBool qcg::getSessionIsAutoCommit( qcStatement * aStatement )
{
    idBool sIsAutoCommit = ID_TRUE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sIsAutoCommit =
            qci::mSessionCallback.mIsAutoCommit( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sIsAutoCommit;
}

idBool qcg::getSessionIsInternalLocalOperation( qcStatement * aStatement )
{
    idBool                 sIsInternalLocalOperation = ID_FALSE;
    sdiInternalOperation   sValue = SDI_INTERNAL_OP_NOT;

    if ( aStatement->session->mMmSession != NULL )
    {
        sValue =
            qci::mSessionCallback.mGetShardInternalLocalOperation( aStatement->session->mMmSession );

        if ( sValue != SDI_INTERNAL_OP_NOT )
        {
            sIsInternalLocalOperation = ID_TRUE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return sIsInternalLocalOperation;
}

IDE_RC qcg::setSessionIsInternalLocalOperation( qcStatement * aStatement, sdiInternalOperation aValue )
{
    IDE_DASSERT( aStatement->session->mMmSession != NULL );
    return qci::mSessionCallback.mSetShardInternalLocalOperation( aStatement->session->mMmSession, aValue );

}

sdiInternalOperation qcg::getSessionInternalLocalOperation( qcStatement * aStatement )
{
    sdiInternalOperation  sValue = SDI_INTERNAL_OP_NOT;

    if ( aStatement->session->mMmSession != NULL )
    {
        sValue =
            qci::mSessionCallback.mGetShardInternalLocalOperation( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing to do */
    }

    return sValue;
}

/* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
UInt qcg::getSessionOptimizerTransitivityOldRule( qcStatement * aStatement )
{
    UInt sOptimizerTransitivityOldRule = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sOptimizerTransitivityOldRule =
            qci::mSessionCallback.mGetOptimizerTransitivityOldRule( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sOptimizerTransitivityOldRule;
}

// PROJ-2727
UInt qcg::getSessionPropertyAttribute( qcStatement * aStatement )
{
    UInt sPropertyAttribute = 0;

    if ( aStatement->session->mMmSession != NULL )
    {
        sPropertyAttribute =
            qci::mSessionCallback.mGetPropertyAttribute( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sPropertyAttribute;
}

// BUG-38129
void qcg::getLastModifiedRowGRID( qcStatement * aStatement,
                                  scGRID      * aRowGRID )
{
    SC_COPY_GRID( aStatement->session->mQPSpecific.mLastRowGRID, *aRowGRID );
}

void qcg::setLastModifiedRowGRID( qcStatement * aStatement,
                                  scGRID        aRowGRID )
{
    SC_COPY_GRID( aRowGRID, aStatement->session->mQPSpecific.mLastRowGRID );
}

/* BUG-42639 Monitoring query */
UInt qcg::getSessionOptimizerPerformanceView( qcStatement * aStatement )
{
    UInt sOptimizerPerformanceView = 1;

    if ( aStatement->session->mMmSession != NULL )
    {
        sOptimizerPerformanceView = qci::mSessionCallback.mGetOptimizerPerformanceView( aStatement->session->mMmSession );
    }
    else
    {
        /* Nothing */
    }

    return sOptimizerPerformanceView;
}

idBool qcg::isInitializedMetaCaches()
{
    return mInitializedMetaCaches;
}

/* PROJ-2632 */
UInt qcg::getSerialExecuteMode( qcStatement * aStatement )
{
    UInt sMode;

    if ( aStatement->session->mMmSession != NULL )
    {
        sMode = qci::mSessionCallback.mGetSerialExecuteMode( aStatement->session->mMmSession );
    }
    else
    {
        sMode = QCU_SERIAL_EXECUTE_MODE;
    }

    return sMode;
}

UInt qcg::getSessionTrclogDetailInformation( qcStatement * aStatement )
{
    UInt sMode;

    if (aStatement->session->mMmSession != NULL)
    {
        sMode = qci::mSessionCallback.mGetTrclogDetailInformation( aStatement->session->mMmSession );
    }
    else
    {
        sMode = QCU_TRCLOG_DETAIL_INFORMATION;
    }

    return sMode;
}

/* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
UInt qcg::getReducePartPrepareMemory( qcStatement * aStatement )
{
    UInt sValue;

    if ( aStatement->session->mMmSession != NULL )
    {
        sValue = qci::mSessionCallback.mGetReducePartPrepareMemory( aStatement->session->mMmSession );
    }
    else
    {
        sValue = 0;
    }

    return sValue;
}

/* BUG-48132 */
UInt qcg::getPlanHashOrSortMethod( qcStatement * aStatement )
{
    UInt sValue;

    if ( aStatement->session->mMmSession != NULL )
    {
        sValue = qci::mSessionCallback.mGetPlanHashOrSortMethod( aStatement->session->mMmSession );
    }
    else
    {
        sValue = 0;
    }

    return sValue;
}

/* BUG-48161 */
UInt qcg::getBucketCountMax( qcStatement * aStatement )
{
    UInt sValue;

    if ( aStatement->session->mMmSession != NULL )
    {
        sValue = qci::mSessionCallback.mGetBucketCountMax( aStatement->session->mMmSession );
    }
    else
    {
        sValue = QMS_MAX_BUCKET_CNT;
    }

    return sValue;
}

/* BUG-48348 */
UInt qcg::getEliminateCommonSubexpression( qcStatement * aStatement )
{
    UInt sValue;

    if ( aStatement->session->mMmSession != NULL )
    {
        sValue = qci::mSessionCallback.mGetEliminateCommonSubexpression( aStatement->session->mMmSession );
    }
    else
    {
        sValue = QCU_OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION; 
    }
    return sValue;
}

IDE_RC
qcg::closeAllCursor( qcStatement * aStatement )
{
    qcTemplate  * sTemplate = NULL;

    sTemplate = QC_PRIVATE_TMPLATE(aStatement);
    
    if ( sTemplate != NULL )
    {
        if ( sTemplate->cursorMgr != NULL )
        {
            IDE_TEST( sTemplate->cursorMgr->closeAllCursor(aStatement->mStatistics)
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // Temp Table Manager가 존재하는 경우 해당 Temp Table을 모두 제거함.
        if ( ( sTemplate->tempTableMgr != NULL ) &&
             ( QC_SMI_STMT( aStatement ) != NULL ) )
        {
            IDE_TEST( qmcTempTableMgr::dropAllTempTable(
                          sTemplate->tempTableMgr ) != IDE_SUCCESS );
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
qcg::initTemplateMember( qcTemplate * aTemplate, iduMemory * aMemory )
{
    UInt i;

    IDE_DASSERT( QMN_PLAN_PREPARE_DONE_MASK == 0x00000000 );

    if (aTemplate->planCount > 0)
    {
        idlOS::memset( aTemplate->planFlag,
                       QMN_PLAN_PREPARE_DONE_MASK,
                       ID_SIZEOF(UInt) * aTemplate->planCount );

        IDE_TEST( aTemplate->cursorMgr->init( aMemory )
                  != IDE_SUCCESS );

        // Temp Table Manager의 초기화
        IDE_TEST( qmcTempTableMgr::init( aTemplate->tempTableMgr,
                                         aMemory )
                  != IDE_SUCCESS );
    }

    // To Fix PR-8531
    idlOS::memset( aTemplate->tmplate.execInfo,
                   QTC_WRAPPER_NODE_EXECUTE_FALSE,
                   ID_SIZEOF(UChar) * aTemplate->tmplate.execInfoCnt );

    // PROJ-2527 WITHIN GROUP AGGR
    idlOS::memset( aTemplate->tmplate.funcData,
                   0x00,
                   ID_SIZEOF(mtfFuncDataBasicInfo*) *
                   aTemplate->tmplate.funcDataCnt );

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    aTemplate->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    aTemplate->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    aTemplate->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;

    /* PROJ-2448 Subquery caching */
    aTemplate->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */
    if ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 )
    {
        aTemplate->memHashTempPartDisable = ID_TRUE;
    }
    else
    {
        aTemplate->memHashTempPartDisable = ID_FALSE;
    }

    if ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 )
    {
        aTemplate->memHashTempManualBucketCnt = ID_TRUE;
    }
    else
    {
        aTemplate->memHashTempManualBucketCnt = ID_FALSE;
    }

    aTemplate->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    for ( i = 0; i < aTemplate->cacheObjCnt; i++ )
    {
        QTC_CACHE_INIT_CACHE_OBJ( aTemplate->cacheObjects + i, aTemplate->cacheMaxSize );
    }

    // BUG-44795
    aTemplate->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    aTemplate->mUnnestViewNameIdx = 0;

    /* BUG-48776 */
    aTemplate->mSubqueryMode = QCU_SUBQUERY_MODE;

    resetTupleSet( aTemplate );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-38290
// Cursor manager mutex 의 destroy 를 위해 return type 변경
void qcg::resetTemplateMember( qcStatement * aStatement )
{
    qcTemplate  * sTemplate;

    if ( aStatement->sharedFlag == ID_FALSE )
    {
        // 공유되지 않은 plan인 경우

        if ( QC_PRIVATE_TMPLATE(aStatement) != NULL )
        {
            // private template이 생성된 경우
            sTemplate = QC_PRIVATE_TMPLATE(aStatement);
        }
        else
        {
            // private template이 생성되지 않은 경우
            sTemplate = QC_SHARED_TMPLATE(aStatement);
        }

        IDE_DASSERT( sTemplate != NULL );
    }
    else
    {
        // 공유된 plan인 경우

        sTemplate = QC_PRIVATE_TMPLATE(aStatement);
    }

    if ( sTemplate != NULL )
    {
        if (sTemplate->planCount > 0)
        {
            if ( sTemplate->cursorMgr != NULL )
            {
                sTemplate->cursorMgr->closeAllCursor(aStatement->mStatistics);
            }
            else
            {
                // Nothing To Do
            }
            if ( ( sTemplate->tempTableMgr != NULL ) &&
                 ( QC_SMI_STMT( aStatement ) != NULL ) )
            {
                (void) qmcTempTableMgr::dropAllTempTable(
                    sTemplate->tempTableMgr );

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

        // BUG-16422
        if ( sTemplate->tableInfoMgr != NULL )
        {
            if ( (sTemplate->flag & QC_TMP_EXECUTION_MASK)
                 == QC_TMP_EXECUTION_SUCCESS )
            {
                qcmTableInfoMgr::destroyAllOldTableInfo( aStatement );
            }
            else
            {
                qcmTableInfoMgr::revokeAllNewTableInfo( aStatement );
            }

            sTemplate->tableInfoMgr = NULL;
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2527 WITHIN GROUP AGGR
        // cloneTemplate시 생성한 객체를 해제한다.
        mtc::finiTemplate( &(sTemplate->tmplate) );

        // child template에서 생성한 객체를 해제한다.
        finiPRLQChildTemplate( aStatement );
        
        resetTupleSet( sTemplate );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41311
    qsxEnv::freeReturnArray( QC_QSX_ENV(aStatement) );

    return ;
}

void
qcg::resetTupleSet( qcTemplate * aTemplate )
{
    UShort      i;
    UShort      sRowCount;
    mtcTuple  * sRows;

    sRowCount = aTemplate->tmplate.rowCount;
    sRows = aTemplate->tmplate.rows;

    for ( i = sRowCount; i != 0; i--, sRows++ )
    {
        sRows->modify = 0;
    }
}

IDE_RC
qcg::allocInternalTuple( qcTemplate * aTemplate,
                         iduMemory  * aMemory,
                         UShort       aTupleCnt )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1358
 *     Internal Tuple Set의 자동 확장을 위하여 공간을 할당 받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    // PROJ-1075
    // TYPESET은 tuple count가 0임.
    if( aTupleCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::allocInternalTuple::alloc::tmplateRows",
                        idERR_ABORT_InsufficientMemory );

        // Internal Tuple Set의 공간 할당
        IDE_TEST( aMemory->alloc( ID_SIZEOF(mtcTuple) * aTupleCnt,
                                  (void**) & aTemplate->tmplate.rows )
                  != IDE_SUCCESS);

        IDU_FIT_POINT( "qcg::allocInternalTuple::cralloc::tableMap",
                        idERR_ABORT_InsufficientMemory );

        // Table Map 관리를 위한 공간 할당
        IDE_TEST( aMemory->cralloc( ID_SIZEOF(qcTableMap) * aTupleCnt,
                                    (void**) & aTemplate->tableMap )
                  != IDE_SUCCESS);
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
qcg::allocInternalTuple( qcTemplate    * aTemplate,
                         iduVarMemList * aMemory,
                         UShort          aTupleCnt )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1358
 *     Internal Tuple Set의 자동 확장을 위하여 공간을 할당 받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    // PROJ-1075
    // TYPESET은 tuple count가 0임.
    if( aTupleCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::allocInternalTuple::alloc::tmplateRows",     
                        idERR_ABORT_InsufficientMemory );

        // Internal Tuple Set의 공간 할당
        IDE_TEST( aMemory->alloc( ID_SIZEOF(mtcTuple) * aTupleCnt,
                                        (void**) & aTemplate->tmplate.rows )
                        != IDE_SUCCESS);

        IDU_FIT_POINT( "qcg::allocInternalTuple::cralloc::tableMap",
                        idERR_ABORT_InsufficientMemory );

        // Table Map 관리를 위한 공간 할당
        IDE_TEST( aMemory->cralloc( ID_SIZEOF(qcTableMap) * aTupleCnt,
                                          (void**) & aTemplate->tableMap )
                        != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qcg::setBindColumn( qcStatement * aStatement,
                           UShort        aId,
                           UInt          aDataTypeId,
                           UInt          aArguments,
                           SInt          aPrecision,
                           SInt          aScale )
{
    mtcColumn  * sColumn    = NULL;
    qcTemplate * sTemplate  = NULL;    // PROJ-2163
    UShort       sBindTuple = 0;

    sTemplate  = QC_SHARED_TMPLATE(aStatement);    // PROJ-2163
    sBindTuple = sTemplate->tmplate.variableRow;

    IDE_TEST_RAISE( sBindTuple == ID_USHORT_MAX, err_invalid_binding );
    IDE_TEST_RAISE( aId >= sTemplate->tmplate.rows[sBindTuple].columnCount,
                    err_invalid_binding );

    sColumn = & sTemplate->tmplate.rows[sBindTuple].columns[aId];

    // mtcColumn의 초기화
    IDE_TEST( mtc::initializeColumn( sColumn,
                                     aDataTypeId,
                                     aArguments,
                                     aPrecision,
                                     aScale )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UShort qcg::getBindCount( qcStatement * aStatement )
{
    qcTemplate * sTemplate;
    UShort       sBindTuple;
    UShort       sCount = 0;

    if ( QC_PRIVATE_TMPLATE(aStatement) != NULL )
    {
        sTemplate = QC_PRIVATE_TMPLATE(aStatement);
    }
    else
    {
        sTemplate = QC_SHARED_TMPLATE(aStatement);
    }

    sBindTuple = sTemplate->tmplate.variableRow;

    if( sBindTuple != ID_USHORT_MAX )
    {
        sCount = sTemplate->tmplate.rows[sBindTuple].columnCount;
    }
    else
    {
        // Nothing to do.
    }

    return sCount;
}

/****************************************************/
/* sungmin                                          */
/* 다음 함수가 올바로 동작하는지 확인해야 함   */
/****************************************************/
IDE_RC qcg::setBindData( qcStatement * aStatement,
                         UShort        aId,
                         UInt          aInOutType,
                         UInt          aSize,
                         void        * aData )
{
    qcTemplate * sTemplate  = NULL;
    UShort       sBindTuple = 0;

    sTemplate  = QC_PRIVATE_TMPLATE(aStatement);
    sBindTuple = sTemplate->tmplate.variableRow;
    
    IDE_TEST_RAISE( sBindTuple == ID_USHORT_MAX, err_invalid_binding );

    // PROJ-2163
    IDE_TEST_RAISE( aId >= aStatement->pBindParamCount,
                    err_invalid_binding );

    if ( (aInOutType == CMP_DB_PARAM_INPUT) ||
         (aInOutType == CMP_DB_PARAM_INPUT_OUTPUT) )
    {
        IDE_TEST_RAISE( aSize > aStatement->pBindParam[aId].param.dataSize,
                        err_invalid_binding );

        idlOS::memcpy( (SChar*)aStatement->pBindParam[aId].param.data,
                       aData,
                       aSize );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// prj-1697
// BindParamData protocol이 불리면 데이터가
// CM buffer에서 template으로 복사 (예전에는 heap을 거쳐서 복사)
// PROJ-2163 : CM buffer에서 pBindParam 으로 복사
IDE_RC
qcg::setBindData( qcStatement  * aStatement,
                  UShort         aId,
                  UInt           aInOutType,
                  void         * aBindParam,
                  void         * aBindContext,
                  void         * aSetParamDataCallback )
{
    qcTemplate * sTemplate  = NULL;
    UShort       sBindTuple = 0;
    SChar      * sTarget;
    UInt         sTargetSize;

    sTemplate  = QC_PRIVATE_TMPLATE(aStatement);
    sBindTuple = sTemplate->tmplate.variableRow;

    IDE_TEST_RAISE( sBindTuple == ID_USHORT_MAX, err_invalid_binding );

    // PROJ-2163
    IDE_TEST_RAISE( aId >= aStatement->pBindParamCount,
                    err_invalid_binding );

    if ( (aInOutType == CMP_DB_PARAM_INPUT) ||
         (aInOutType == CMP_DB_PARAM_INPUT_OUTPUT) )
    {
        // sTarget : aBindParam->data
        // sTargetSize : aBindParam.type 의 size
        sTarget     = (SChar*)aStatement->pBindParam[aId].param.data;
        sTargetSize = aStatement->pBindParam[aId].param.dataSize;

        // CM buffer 의 값을 pBindParam[].param.data 에 복사
        IDE_TEST( ((qciSetParamDataCallback) aSetParamDataCallback) (
                      aStatement->mStatistics,
                      aBindParam,
                      sTarget,
                      sTargetSize,
                      (void*)&(sTemplate->tmplate),
                      aBindContext )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::runDDLforInternalWithMmSession( idvSQL        * aStatistics,
                                            void          * aMmSession,
                                            smiStatement  * aSmiStmt,
                                            UInt            aUserID,
                                            UInt            aSessionFlag,
                                            SChar         * aSqlStr )
{
    idBool        sIsAlloc = ID_FALSE;
    qcStatement   sStatement;

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( & sStatement,
                              NULL,
                              NULL,
                              aStatistics ) != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    qsxEnv::initialize( sStatement.spxEnv, NULL );

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    sStatement.session->mMmSession = aMmSession;
    sStatement.session->mQPSpecific.mFlag |= aSessionFlag;
    /* 
     * PROJ-2737  Internal Replication
     *   runDDLforDDLSync() -> runDDLforInternal() 좀 더 범용적인 용도로 변경됨
     *   1. 복제된 DDL을 리모트에서 수행하는 경우 (PROJ-2677)
     *      - SQL 만을 전송받기 때문에 User 정보가 없으므로 User ID를 넘겨받아서 수행
     *   2. runDMLforDDL처럼 내부에서 쿼리를 만들어서 DDL을 수행하는 경우
     *      - aUserID - QC_EMPTY_USER_ID
     *   */
    if ( aUserID != QC_EMPTY_USER_ID )
    {
        sStatement.session->mUserID = aUserID;
    }
    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen( aSqlStr );
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST( qcpManager::parseIt( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( sStatement.myPlan->parseTree->parse( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterParsing( QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS );

    // validation
    IDE_TEST( sStatement.myPlan->parseTree->validate( &sStatement )
              != IDE_SUCCESS );    
    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( &sStatement ),
                                       QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS );

    // optimizaing
    IDE_TEST( sStatement.myPlan->parseTree->optimize( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( setPrivateArea( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( stepAfterPVO( &sStatement ) != IDE_SUCCESS );

    // execution
    IDE_TEST( QC_SMI_STMT(&sStatement)->getTrans()->writeDDLLog() != IDE_SUCCESS );
    IDE_TEST( sStatement.myPlan->parseTree->execute( &sStatement )
              != IDE_SUCCESS );
    // set success
    QC_PRIVATE_TMPLATE( &sStatement )->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE( &sStatement )->flag |= QC_TMP_EXECUTION_SUCCESS;

    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloc == ID_TRUE )
    {
        sIsAlloc = ID_FALSE;
        (void)qcg::freeStatement( &sStatement );
    }
    else
    {
        // Nothing to do
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC qcg::runDDLforInternal( idvSQL        * aStatistics,
                               smiStatement  * aSmiStmt,
                               UInt            aUserID,
                               UInt            aSessionFlag,
                               SChar         * aSqlStr )
{
    idBool        sIsAlloc = ID_FALSE;
    qcStatement   sStatement;

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( & sStatement,
                              NULL,
                              NULL,
                              aStatistics ) != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    qsxEnv::initialize( sStatement.spxEnv, NULL );

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    sStatement.session->mQPSpecific.mFlag |= aSessionFlag;
    /* 
     * PROJ-2737  Internal Replication
     *   runDDLforDDLSync() -> runDDLforInternal() 좀 더 범용적인 용도로 변경됨
     *   1. 복제된 DDL을 리모트에서 수행하는 경우 (PROJ-2677)
     *      - SQL 만을 전송받기 때문에 User 정보가 없으므로 User ID를 넘겨받아서 수행
     *   2. runDMLforDDL처럼 내부에서 쿼리를 만들어서 DDL을 수행하는 경우
     *      - aUserID - QC_EMPTY_USER_ID
     *   */
    if ( aUserID != QC_EMPTY_USER_ID )
    {
        sStatement.session->mUserID = aUserID;
    }
    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen( aSqlStr );
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST( qcpManager::parseIt( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( sStatement.myPlan->parseTree->parse( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterParsing( QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS );

    // validation
    IDE_TEST( sStatement.myPlan->parseTree->validate( &sStatement )
              != IDE_SUCCESS );    
    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( &sStatement ),
                                       QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS );

    // optimizaing
    IDE_TEST( sStatement.myPlan->parseTree->optimize( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( setPrivateArea( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( stepAfterPVO( &sStatement ) != IDE_SUCCESS );

    // execution
    IDE_TEST( QC_SMI_STMT(&sStatement)->getTrans()->writeDDLLog() != IDE_SUCCESS );
    IDE_TEST( sStatement.myPlan->parseTree->execute( &sStatement )
              != IDE_SUCCESS );
    // set success
    QC_PRIVATE_TMPLATE( &sStatement )->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE( &sStatement )->flag |= QC_TMP_EXECUTION_SUCCESS;

    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloc == ID_TRUE )
    {
        sIsAlloc = ID_FALSE;
        (void)qcg::freeStatement( &sStatement );
    }
    else
    {
        // Nothing to do
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC qcg::rebuildInternalStatement( qcStatement * aStatement )
{
    SChar        * sSql = aStatement->myPlan->stmtText;
    UInt           sUserID = aStatement->session->mUserID;
    smiStatement * sSmiStmt = QC_SMI_STMT( aStatement );
    UInt           sFlag = aStatement->session->mQPSpecific.mFlag;
    
    IDE_TEST(qcg::clearStatement( aStatement,
                                  ID_TRUE ) // rebuild = TRUE
            != IDE_SUCCESS);

    qsxEnv::initialize( aStatement->spxEnv, NULL );

    qcg::setSmiStmt( aStatement, sSmiStmt );
    
    aStatement->session->mQPSpecific.mFlag = sFlag;
    aStatement->session->mUserID         = sUserID;
    aStatement->myPlan->stmtText         = sSql;
    aStatement->myPlan->stmtTextLen      = idlOS::strlen( sSql );
    aStatement->myPlan->encryptedText    = NULL;
    aStatement->myPlan->encryptedTextLen = 0;
    aStatement->myPlan->parseTree        = NULL;
    aStatement->myPlan->plan             = NULL;
    aStatement->myPlan->graph            = NULL;
    aStatement->myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST( qcpManager::parseIt( aStatement ) != IDE_SUCCESS );

    IDE_TEST( aStatement->myPlan->parseTree->parse( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterParsing( QC_SHARED_TMPLATE( aStatement ) )
              != IDE_SUCCESS );

    // validation
    IDE_TEST( aStatement->myPlan->parseTree->validate( aStatement )
              != IDE_SUCCESS );
    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( aStatement ),
                                       QC_SHARED_TMPLATE( aStatement ) )
              != IDE_SUCCESS );

    // optimizaing
    IDE_TEST( aStatement->myPlan->parseTree->optimize( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( setPrivateArea( aStatement ) != IDE_SUCCESS );

    IDE_TEST( stepAfterPVO( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::ddlBeginSavepoint( qcStatement * aStatement )
{
    smiTrans     * sSmiTrans  = QC_SMI_STMT( aStatement )->getTrans();
    smiStatement * sSmiStmt   = QC_SMI_STMT( aStatement );
    smiStatement * sParent    = QC_SMI_STMT( aStatement )->mParent;
    idBool         sIsStmtEnd = ID_FALSE;
    idBool         sIsSetSvp  = ID_FALSE;
    UInt           sFlag      = sSmiStmt->mFlag;

    IDE_DASSERT(sParent != NULL);

    IDE_TEST( sSmiStmt->end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtEnd = ID_TRUE;

    IDE_TEST( sSmiTrans->savepoint( SMI_DDL_BEGIN_SAVEPOINT ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt->begin( QCI_STATISTIC( aStatement ), sParent, sFlag) != IDE_SUCCESS );
    sIsStmtEnd = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsSetSvp == ID_TRUE )
    {
        (void)sSmiTrans->rollback( SMI_DDL_BEGIN_SAVEPOINT, SMI_DO_NOT_RELEASE_TRANSACTION );
    }

    if( sIsStmtEnd == ID_TRUE )
    {
        /* internal ddl에서만 사용되고 있으므로 아래 begin이 성공하지 못하는 경우는 개발자 오류로 판단하여 assert 삽입함 */
        IDE_ASSERT( sSmiStmt->begin( QCI_STATISTIC( aStatement ), sParent, sFlag) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC qcg::backupOldTableInfo(qcStatement * aStatement)
{
    UInt i = 0;
    smiTrans * sSmiTrans = QC_SMI_STMT( aStatement )->getTrans();
    UInt       sSrcTableOIDCount  = 0;
    smOID    * sSrcTableOIDArray  = qciMisc::getDDLSrcTableOIDArray( (void*)aStatement, 
                                                                     &sSrcTableOIDCount );
    UInt       sSrcPartOIDCount   = 0;
    smOID    * sSrcPartOIDArray   = qciMisc::getDDLSrcPartTableOIDArray( (void*)aStatement, 
                                                                         &sSrcPartOIDCount );
    idBool     sIsStmtEnd = ID_FALSE;
    
    smiStatement* sSmiStmt = QC_SMI_STMT( aStatement );
    smiStatement* sParent = QC_SMI_STMT( aStatement )->mParent;

    UInt sFlag = sSmiStmt->mFlag;

    IDE_DASSERT(sParent != NULL);

    IDE_TEST( sSmiStmt->end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtEnd = ID_TRUE;

    for ( i = 0; i < sSrcTableOIDCount; i++ )
    {
        IDE_TEST( qciMisc::checkRollbackAbleDDLEnable( sSmiTrans, 
                                                       sSrcTableOIDArray[i],
                                                       ID_FALSE )
                  != IDE_SUCCESS );

        IDE_TEST( sSmiTrans->setExpSvpForBackupDDLTargetTableInfo( sSrcTableOIDArray[i], 
                                                                   sSrcPartOIDCount,
                                                                   sSrcPartOIDArray +
                                                                   (i * sSrcPartOIDCount),
                                                                   SM_OID_NULL,
                                                                   0,
                                                                   NULL )
                  != IDE_SUCCESS );
    }

    if ( sSrcTableOIDCount > 0 )
    {
        IDE_TEST(rebuildInternalStatement(aStatement) != IDE_SUCCESS);
    }

    sIsStmtEnd = ID_FALSE;
    IDE_TEST( sSmiStmt->begin( NULL, sParent, sFlag) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsStmtEnd == ID_TRUE )
    {
        /* internal ddl에서만 사용되고 있으므로 아래 begin이 성공하지 못하는 경우는 개발자 오류로 판단하여 assert 삽입함 */
        IDE_ASSERT( sSmiStmt->begin( NULL, sParent, sFlag) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC qcg::backupNewTableInfo(qcStatement * aStatement)
{
    UInt i = 0;
    smiTrans * sSmiTrans = QC_SMI_STMT( aStatement )->getTrans();
    UInt           sDestTableOIDCount = 0;
    smOID        * sDestTableOIDArray = qciMisc::getDDLDestTableOIDArray( aStatement, &sDestTableOIDCount );
    UInt           sDestPartOIDCount  = 0;
    smOID        * sDestPartOIDArray  = qciMisc::getDDLDestPartTableOIDArray( (void*)aStatement, 
                                                                              &sDestPartOIDCount );


    idBool         sIsStmtEnd = ID_FALSE;
    smiStatement * sSmiStmt = QC_SMI_STMT( aStatement );
    smiStatement * sParent = QC_SMI_STMT( aStatement )->mParent;

    UInt sFlag = sSmiStmt->mFlag;

    IDE_DASSERT(sParent != NULL);

    IDE_TEST( sSmiStmt->end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtEnd = ID_TRUE;

    for ( i = 0; i < sDestTableOIDCount; i++ )
    {
        IDE_TEST( sSmiTrans->setExpSvpForBackupDDLTargetTableInfo( SM_OID_NULL, 
                                                                   0,
                                                                   NULL,
                                                                   sDestTableOIDArray[i],
                                                                   sDestPartOIDCount,
                                                                   sDestPartOIDArray +
                                                                   (i * sDestPartOIDCount) )
                  != IDE_SUCCESS );
    }

    sIsStmtEnd = ID_FALSE;
    IDE_TEST( sSmiStmt->begin( NULL, sParent, sFlag) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsStmtEnd == ID_TRUE )
    {
        /* internal ddl에서만 사용되고 있으므로 아래 begin이 성공하지 못하는 경우는 개발자 오류로 판단하여 assert 삽입함 */
        IDE_ASSERT( sSmiStmt->begin( NULL, sParent, sFlag) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC qcg::runRollbackableInternalDDL( qcStatement   * aStatement,
                                        smiStatement  * aSmiStmt,
                                        UInt            aUserID,
                                        SChar         * aSqlStr )
{
    idvSQL                * sStatistics = aStatement->mStatistics;
    idBool                  sIsAlloc = ID_FALSE;
    qcStatement             sStatement;
    idBool                  sIsSetSvp = ID_FALSE;
    smiTrans              * sSmiTrans = aSmiStmt->getTrans();
    smiStatement          * sParent    = aSmiStmt->mParent;
    UInt                    sFlag      = aSmiStmt->mFlag;

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( &sStatement,
                              NULL,
                              NULL,
                              sStatistics ) != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    sStatement.session->mMmSession = aStatement->session->mMmSession;

    qsxEnv::initialize( sStatement.spxEnv, NULL );

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_INTERNAL_DDL_TRUE;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_ROLLBACKABLE_DDL_TRUE;
    /* PROJ-2677  remote 에서 DDL 수행시 SQL 만을 전송받기 때문에 User 정보가 없으므로
     * User ID 를 직접 지정하여 수행한다. */
    sStatement.session->mUserID = aUserID;

    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen( aSqlStr );
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST( qcpManager::parseIt( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( sStatement.myPlan->parseTree->parse( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterParsing( QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS );

    // validation
    IDE_TEST( sStatement.myPlan->parseTree->validate( &sStatement )
              != IDE_SUCCESS );
    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( &sStatement ),
                                       QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS );

    // optimizaing
    IDE_TEST( sStatement.myPlan->parseTree->optimize( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( setPrivateArea( &sStatement ) != IDE_SUCCESS );

    IDE_TEST( stepAfterPVO( &sStatement ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sStatement.mDDLInfo.mTransactionalDDLAvailable != ID_TRUE,
                    ERR_NOT_SUPPORT_DDL_TRANSACTION );

    IDE_TEST( ddlBeginSavepoint( &sStatement ) != IDE_SUCCESS );
    sIsSetSvp = ID_TRUE;

    IDE_TEST( backupOldTableInfo( &sStatement) != IDE_SUCCESS );

    // For Replication
    IDE_TEST( QC_SMI_STMT(&sStatement)->getTrans()->writeDDLLog() != IDE_SUCCESS );
    // execution
    IDE_TEST( sStatement.myPlan->parseTree->execute( &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( backupNewTableInfo( &sStatement) != IDE_SUCCESS );

    // set success
    QC_PRIVATE_TMPLATE( &sStatement )->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE( &sStatement )->flag |= QC_TMP_EXECUTION_SUCCESS;

    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_DDL_TRANSACTION )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR, "not supported transactional ddl", aSqlStr) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloc == ID_TRUE )
    {
        sIsAlloc = ID_FALSE;
        (void)qcg::freeStatement( &sStatement );
    }
    else
    {
        // Nothing to do
    }

    if ( sIsSetSvp == ID_TRUE )
    {
        (void)aSmiStmt->end( SMI_STATEMENT_RESULT_FAILURE );
        (void)sSmiTrans->rollback( SMI_DDL_BEGIN_SAVEPOINT, SMI_DO_NOT_RELEASE_TRANSACTION );
        (void)aSmiStmt->begin( sStatistics, sParent, sFlag );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC qcg::runDMLforDDL( smiStatement  * aSmiStmt,
                          SChar         * aSqlStr,
                          vSLong        * aRowCnt )
{
    IDE_TEST( runDMLforInternal( aSmiStmt,
                                 aSqlStr,
                                 aRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::runDMLforInternal( smiStatement  * aSmiStmt,
                               SChar         * aSqlStr,
                               vSLong        * aRowCnt )
{
    qcStatement   sStatement;

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( & sStatement,
                              NULL,
                              NULL,
                              NULL ) != IDE_SUCCESS );

    qsxEnv::initialize( sStatement.spxEnv, NULL );

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen(aSqlStr);
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST(qcpManager::parseIt(&sStatement) != IDE_SUCCESS);

    IDE_TEST(sStatement.myPlan->parseTree->parse(&sStatement)
             != IDE_SUCCESS);

    IDE_TEST(qtc::fixAfterParsing(QC_SHARED_TMPLATE(&sStatement))
             != IDE_SUCCESS);

    // validation
    IDE_TEST( sStatement.myPlan->parseTree->validate(&sStatement)
              != IDE_SUCCESS);
    IDE_TEST(qtc::fixAfterValidation(QC_QMP_MEM(&sStatement),
                                     QC_SHARED_TMPLATE(&sStatement))
             != IDE_SUCCESS);

    // optimizaing
    IDE_TEST(sStatement.myPlan->parseTree->optimize(&sStatement)
             != IDE_SUCCESS);
    //IDE_TEST(qtc::fixAfterOptimization(QC_QMP_MEM(&sStatement),
    //                                   QC_SHARED_TMPLATE(&sStatement))
    //         != IDE_SUCCESS);

    IDE_TEST(setPrivateArea(&sStatement) != IDE_SUCCESS);

    IDE_TEST(stepAfterPVO(&sStatement) != IDE_SUCCESS);

    // execution
    IDE_TEST(sStatement.myPlan->parseTree->execute(&sStatement)
             != IDE_SUCCESS);

    // set success
    QC_PRIVATE_TMPLATE(&sStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(&sStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    *aRowCnt = QC_PRIVATE_TMPLATE(&sStatement)->numRows;

    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void) qcg::freeStatement(&sStatement);

    return IDE_FAILURE;
    
}

IDE_RC qcg::runSQLforShardMeta( smiStatement  * aSmiStmt,
                                SChar         * aSqlStr,
                                vSLong        * aRowCnt,
                                qcSession     * aSession )
{
    qcStatement   sStatement;
    qcSession   * sSession = NULL;

    idBool        sSetFlag = ID_FALSE;
    idBool        sIsAlloc = ID_FALSE;

    if ( aSession != NULL )
    {
        sSession = aSession;
    }

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( & sStatement,
                              sSession,
                              NULL,
                              NULL ) != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    qsxEnv::initialize( sStatement.spxEnv, NULL );

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen(aSqlStr);
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    /* Shard meta information을 생성하는 시점에는 SMN validatation을 수행하지 말아야 한다. */
    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_ALTER_META_MASK;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_ALTER_META_ENABLE;
    sSetFlag = ID_TRUE;

    // parsing
    IDE_TEST(qcpManager::parseIt(&sStatement) != IDE_SUCCESS);

    IDE_TEST(sStatement.myPlan->parseTree->parse(&sStatement)
             != IDE_SUCCESS);

    IDE_TEST(qtc::fixAfterParsing(QC_SHARED_TMPLATE(&sStatement))
             != IDE_SUCCESS);

    // validation
    IDE_TEST( sStatement.myPlan->parseTree->validate(&sStatement)
              != IDE_SUCCESS);
    IDE_TEST(qtc::fixAfterValidation(QC_QMP_MEM(&sStatement),
                                     QC_SHARED_TMPLATE(&sStatement))
             != IDE_SUCCESS);

    // optimizaing
    IDE_TEST(sStatement.myPlan->parseTree->optimize(&sStatement)
             != IDE_SUCCESS);

    IDE_TEST(setPrivateArea(&sStatement) != IDE_SUCCESS);

    IDE_TEST(stepAfterPVO(&sStatement) != IDE_SUCCESS);

    // execution
    IDE_TEST(sStatement.myPlan->parseTree->execute(&sStatement)
             != IDE_SUCCESS);

    // set success
    QC_PRIVATE_TMPLATE(&sStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(&sStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    *aRowCnt = QC_PRIVATE_TMPLATE(&sStatement)->numRows;

    /* BUG-48763 Meta 변경이 끝났으면 Flag를 제거해 주어야 한다. */
    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_ALTER_META_MASK;

    if( sStatement.spvEnv->latched == ID_TRUE )
    {
        IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                  != IDE_SUCCESS );
        sStatement.spvEnv->latched = ID_FALSE;
    }

    sIsAlloc = ID_FALSE;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sSetFlag == ID_TRUE )
    {
        /* BUG-48763 Meta 변경이 끝났으면 Flag를 제거해 주어야 한다. */
        sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_ALTER_META_MASK;
    }

    if ( sIsAlloc == ID_TRUE )
    {
        if( sStatement.spvEnv->latched == ID_TRUE )
        {
            if ( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 == IDE_SUCCESS )
            {
                sStatement.spvEnv->latched = ID_FALSE;
            }
        }

        (void) qcg::freeStatement(&sStatement);
    }

    return IDE_FAILURE;
}

IDE_RC qcg::runSelectOneRowforDDL( smiStatement * aSmiStmt,
                                   SChar        * aSqlStr,
                                   void         * aResult,
                                   idBool       * aRecordExist,
                                   idBool         aCalledPWVerifyFunc )
{
/***********************************************************************
 *
 * Description : PROJ-2207 Password policy support
 *              alter password 수행시 select 를 수행
 *              one row, one column 만 가져온다. 다수의 row 시 마지막
 *              row를 가져온다, 다수의 column 이면 첫번째 column 가져옴
 *              alter password를 위해 만든 함수.
 *
 * Implementation :
 *            (1) execute 까지의 과정은 runDMLforDDL 동일
 *            (2) fetch 과정 추가
 ***********************************************************************/
    qcStatement   sStatement;
    qmcRowFlag    sRowFlag = QMC_ROW_INITIALIZE;
    UInt          sActualSize = 0;
    idBool        sAlloced = ID_FALSE;

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( & sStatement,
                              NULL,
                              NULL,
                              NULL ) != IDE_SUCCESS );
    sAlloced = ID_TRUE;

    qcg::setSmiStmt( &sStatement, aSmiStmt );

    qsxEnv::initialize( sStatement.spxEnv, NULL );

    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen(aSqlStr);
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST(qcpManager::parseIt(&sStatement) != IDE_SUCCESS);

    IDE_TEST(sStatement.myPlan->parseTree->parse(&sStatement)
             != IDE_SUCCESS);

    IDE_TEST(qtc::fixAfterParsing(QC_SHARED_TMPLATE(&sStatement))
             != IDE_SUCCESS);

    // validation
    IDE_TEST( sStatement.myPlan->parseTree->validate(&sStatement)
              != IDE_SUCCESS);
    IDE_TEST(qtc::fixAfterValidation(QC_QMP_MEM(&sStatement),
                                     QC_SHARED_TMPLATE(&sStatement))
             != IDE_SUCCESS);

    // optimizaing
    IDE_TEST(sStatement.myPlan->parseTree->optimize(&sStatement)
             != IDE_SUCCESS);

    IDE_TEST(setPrivateArea(&sStatement) != IDE_SUCCESS);

    IDE_TEST(stepAfterPVO(&sStatement) != IDE_SUCCESS);

    /* BUG-43154
       password_verify_function이 autonomous_transaction(AT) pragma가 선언된 function일 경우 비정상 종료 */
    sStatement.spxEnv->mExecPWVerifyFunc = aCalledPWVerifyFunc;

    // execution
    IDE_TEST(sStatement.myPlan->parseTree->execute(&sStatement)
             != IDE_SUCCESS);

    // set success
    QC_PRIVATE_TMPLATE(&sStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(&sStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    // fetch
    IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE(&sStatement),
                             sStatement.myPlan->plan,
                             & sRowFlag )
              != IDE_SUCCESS );

    if( ( sRowFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        *aRecordExist = ID_TRUE;

        sActualSize =  QC_PRIVATE_TMPLATE(&sStatement)->tmplate.stack->column->module->actualSize(
            QC_PRIVATE_TMPLATE(&sStatement)->tmplate.stack->column,
            QC_PRIVATE_TMPLATE(&sStatement)->tmplate.stack->value );

        idlOS::memcpy( aResult,
                       QC_PRIVATE_TMPLATE(&sStatement)->tmplate.stack->value,
                       sActualSize );
    }
    else
    {
        *aRecordExist = ID_FALSE;
    }

    /* DML 수행시 procedure 수행 시 lock을 잡기 때문에 unlock 필요 */
    if ( sStatement.spvEnv->latched == ID_TRUE )
    {
        IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                  != IDE_SUCCESS );
        sStatement.spvEnv->latched = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    // free
    sAlloced = ID_FALSE;
    IDE_TEST( qcg::freeStatement(&sStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sAlloced == ID_TRUE )
    {
        if ( sStatement.spvEnv->latched == ID_TRUE )
        {
            if ( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 == IDE_SUCCESS )
            {
                sStatement.spvEnv->latched = ID_FALSE;
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
    else
    {
        // Nothing to do.
    }

    (void) qcg::freeStatement(&sStatement);

    return IDE_FAILURE;
}

/* PROJ-2441 flashback */
IDE_RC qcg::runSelectOneRowMultiColumnforDDL( smiStatement * aSmiStmt,
                                              SChar        * aSqlStr,
                                              idBool       * aRecordExist,
                                              UInt           aColumnCount,
                                              ... )
{
/***********************************************************************
 *
 * Description : runSelectOneRowforDDL upgrade version
 *               mutil column result return
 *
 * Implementation :
 *            (1) execute 까지의 과정은 runDMLforDDL 동일
 *            (2) fetch 과정 추가
 ***********************************************************************/
    qcStatement   sStatement;
    qmcRowFlag    sRowFlag    = QMC_ROW_INITIALIZE;
    UInt          sActualSize = 0;
    void         *sResult     = NULL;
    idBool        sAlloced    = ID_FALSE;
    UInt          i           = 0;
    va_list       sArgs;
    
    va_start( sArgs, aColumnCount );
    
    IDE_TEST( allocStatement( & sStatement,
                              NULL,
                              NULL,
                              NULL ) != IDE_SUCCESS );
    sAlloced = ID_TRUE;

    qcg::setSmiStmt( &sStatement, aSmiStmt );
    
    qsxEnv::initialize( sStatement.spxEnv, NULL );
    
    sStatement.myPlan->stmtText         = aSqlStr;
    sStatement.myPlan->stmtTextLen      = idlOS::strlen( aSqlStr );
    sStatement.myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree        = NULL;
    sStatement.myPlan->plan             = NULL;
    sStatement.myPlan->graph            = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;
    
    IDE_TEST( qcpManager::parseIt( &sStatement ) != IDE_SUCCESS );
    
    IDE_TEST( sStatement.myPlan->parseTree->parse( &sStatement )
              != IDE_SUCCESS);
    
    IDE_TEST( qtc::fixAfterParsing( QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS);
    
    IDE_TEST( sStatement.myPlan->parseTree->validate( &sStatement )
              != IDE_SUCCESS);
    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( &sStatement ),
                                       QC_SHARED_TMPLATE( &sStatement ) )
              != IDE_SUCCESS);
    
    IDE_TEST( sStatement.myPlan->parseTree->optimize( &sStatement )
              != IDE_SUCCESS);
    
    IDE_TEST( setPrivateArea( &sStatement ) != IDE_SUCCESS );
    
    IDE_TEST( stepAfterPVO( &sStatement ) != IDE_SUCCESS );
    
    IDE_TEST( sStatement.myPlan->parseTree->execute( &sStatement )
              != IDE_SUCCESS);
    
    QC_PRIVATE_TMPLATE(&sStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(&sStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;
    
    IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE(&sStatement),
                             sStatement.myPlan->plan,
                             & sRowFlag )
              != IDE_SUCCESS );
    
    if ( ( sRowFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        *aRecordExist = ID_TRUE;
        
        for ( i = 0; i < aColumnCount; i++ )
        {
            sResult = va_arg( sArgs, void*);
            
            sActualSize =
                QC_PRIVATE_TMPLATE( &sStatement )->tmplate.stack[i].column->module->actualSize(
                    QC_PRIVATE_TMPLATE( &sStatement )->tmplate.stack[i].column,
                    QC_PRIVATE_TMPLATE( &sStatement )->tmplate.stack[i].value );
            
            idlOS::memcpy( sResult,
                           QC_PRIVATE_TMPLATE( &sStatement )->tmplate.stack[i].value,
                           sActualSize );
        }
    }
    else
    {
        *aRecordExist = ID_FALSE;
    }
    
    va_end( sArgs );
    
    /* DML 수행시 procedure 수행 시 lock을 잡기 때문에 unlock 필요 */
    if ( sStatement.spvEnv->latched == ID_TRUE )
    {
        IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 != IDE_SUCCESS );
        sStatement.spvEnv->latched = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    sAlloced = ID_FALSE;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sAlloced == ID_TRUE )
    {
        if ( sStatement.spvEnv->latched == ID_TRUE )
        {
            if ( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 == IDE_SUCCESS )
            {
                sStatement.spvEnv->latched = ID_FALSE;
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
    }
    else
    {
        /* Nothing to do */
    }
    
    (void) qcg::freeStatement( &sStatement );
    
    return IDE_FAILURE;
}


IDE_RC qcg::cloneTemplate( iduVarMemList * aMemory,
                           qcTemplate    * aSource,
                           qcTemplate    * aDestination )
{
    UInt i;

    // PROJ-1358
    // Template 복사 전에 Internal Tuple Set공간을
    // 할당해 주어야 함.
    aDestination->tmplate.rowArrayCount = aSource->tmplate.rowArrayCount;

    IDE_TEST( allocInternalTuple( aDestination,
                                  aMemory,
                                  aDestination->tmplate.rowArrayCount )
              != IDE_SUCCESS );

    IDE_TEST( mtc::cloneTemplate(
                  aMemory,
                  & aSource->tmplate,
                  & aDestination->tmplate ) != IDE_SUCCESS );

    // clone table map
    for (i = 0; i < aSource->tmplate.rowArrayCount; i++)
    {
        // Assign으로 변경한다. (성능 이슈)
        aDestination->tableMap[i] = aSource->tableMap[i];
    }

    // initialize execution info
    aDestination->planCount    = aSource->planCount;
    aDestination->planFlag     = NULL;
    aDestination->cursorMgr    = NULL;
    aDestination->tempTableMgr = NULL;

    // initialize basic members
    aDestination->stmt               = NULL;
    aDestination->flag               = aSource->flag;
    aDestination->smiStatementFlag   = aSource->smiStatementFlag;
    aDestination->numRows            = 0;
    aDestination->stmtType           = aSource->stmtType;

    // allocate insOrUptRow
    aDestination->insOrUptStmtCount = aSource->insOrUptStmtCount;
    if ( aDestination->insOrUptStmtCount > 0 )
    {
        IDU_FIT_POINT( "qcg::cloneTemplate::alloc::insOrUptRowValueCount",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc (ID_SIZEOF (smiValue*) *
                                 aDestination->insOrUptStmtCount,
                                 (void**)&(aDestination->insOrUptRowValueCount))
                 != IDE_SUCCESS);

        IDU_FIT_POINT( "qcg::cloneTemplate::alloc::insOrUptRow",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc (ID_SIZEOF (smiValue*) *
                                 aDestination->insOrUptStmtCount,
                                 (void**)&(aDestination->insOrUptRow))
                 != IDE_SUCCESS);

        for ( i = 0; i < aDestination->insOrUptStmtCount; i++ )
        {
            aDestination->insOrUptRowValueCount[i] =
                aSource->insOrUptRowValueCount[i];

            // 프로시저에서 0번 template의 insOrUptStmtCount 값은 0보다
            // 클 수 있다.
            // 하지만 insOrUptRowValueCount[i]의 값은 항상 0이다.
            // 이유는 statement 별로 별도의 template이 유지되기 때문이다.

            // 예)
            // create or replace procedure proc1 as
            // begin
            // insert into t1 values ( 1, 1 );
            // update t1 set a = a + 1;
            // end;
            // /

            if( aDestination->insOrUptRowValueCount[i] == 0 )
            {
                continue;
            }

            IDU_FIT_POINT( "qcg::cloneTemplate::alloc::insOrUptRowItem",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST(aMemory->alloc( ID_SIZEOF(smiValue) *
                                     aDestination->insOrUptRowValueCount[i],
                                     (void**)&(aDestination->insOrUptRow[i]))
                     != IDE_SUCCESS);
        }
    }
    else
    {
        aDestination->insOrUptRowValueCount = NULL;
        aDestination->insOrUptRow = NULL;
    }

    // PROJ-1413 tupleVarList 초기화
    aDestination->tupleVarGenNumber = 0;
    aDestination->tupleVarList = NULL;

    // BUG-16422 execute중 임시 생성된 tableInfo의 관리
    aDestination->tableInfoMgr = NULL;

    // PROJ-1436
    aDestination->indirectRef = aSource->indirectRef;

    /* PROJ-2209 DBTIMEZONE */
    if ( aSource->unixdate == NULL )
    {
        aDestination->unixdate = NULL;
    }
    else
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF (qtcNode ),
                                 (void**)&(aDestination->unixdate))
                 != IDE_SUCCESS);

        QTC_NODE_INIT( aDestination->unixdate );

        aDestination->unixdate->node.table = aSource->unixdate->node.table ;
        aDestination->unixdate->node.column = aSource->unixdate->node.column ;
    }

    // allocate sysdate

    if ( aSource->sysdate == NULL )
    {
        aDestination->sysdate = NULL;
    }
    else
    {
        IDU_FIT_POINT( "qcg::cloneTemplate::alloc::sysdate",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc( ID_SIZEOF (qtcNode ),
                                 (void**)&(aDestination->sysdate))
                 != IDE_SUCCESS);

        QTC_NODE_INIT( aDestination->sysdate );

        aDestination->sysdate->node.table = aSource->sysdate->node.table ;
        aDestination->sysdate->node.column = aSource->sysdate->node.column ;
    }

    if ( aSource->currentdate == NULL )
    {
        aDestination->currentdate = NULL;
    }
    else
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF (qtcNode ),
                                 (void**)&(aDestination->currentdate))
                 != IDE_SUCCESS);

        QTC_NODE_INIT( aDestination->currentdate );

        aDestination->currentdate->node.table = aSource->currentdate->node.table ;
        aDestination->currentdate->node.column = aSource->currentdate->node.column ;
    }

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    aDestination->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    aDestination->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    aDestination->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    aDestination->cacheObjCnt    = aSource->cacheObjCnt;
    aDestination->cacheObjects   = NULL;

    if ( aDestination->cacheObjCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::cloneTemplate::alloc::cacheObjects",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aMemory->alloc( aDestination->cacheObjCnt * ID_SIZEOF(qtcCacheObj),
                                  (void**)&(aDestination->cacheObjects) )
                  != IDE_SUCCESS);

        for ( i = 0; i < aDestination->cacheObjCnt; i++ )
        {
            QTC_CACHE_INIT_CACHE_OBJ( aDestination->cacheObjects+i, aDestination->cacheMaxSize );
        }
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-2448 Subquery caching */
    aDestination->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    aDestination->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                   ID_TRUE : ID_FALSE;
    aDestination->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                   ID_TRUE : ID_FALSE;
    aDestination->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    /* PROJ-2462 Result Cache */
    aDestination->resultCache.flag  = aSource->resultCache.flag;
    aDestination->resultCache.count = aSource->resultCache.count;
    aDestination->resultCache.stack = aSource->resultCache.stack;
    aDestination->resultCache.list  = aSource->resultCache.list;

    // BUG-44795
    aDestination->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    aDestination->mUnnestViewNameIdx = aSource->mUnnestViewNameIdx;

    /* BUG-48776 */
    aDestination->mSubqueryMode = QCU_SUBQUERY_MODE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::cloneAndInitTemplate( iduMemory   * aMemory,
                                  qcTemplate  * aSource,
                                  qcTemplate  * aDestination )
{
    UInt i;

    // PROJ-1358
    // Template 복사 전에 Internal Tuple Set공간을
    // 할당해 주어야 함.
    aDestination->tmplate.rowArrayCount = aSource->tmplate.rowArrayCount;

    IDE_TEST( allocInternalTuple( aDestination,
                                  aMemory,
                                  aDestination->tmplate.rowArrayCount )
              != IDE_SUCCESS );

    IDE_TEST( mtc::cloneTemplate(
                  aMemory,
                  & aSource->tmplate,
                  & aDestination->tmplate ) != IDE_SUCCESS );

    aDestination->planCount = aSource->planCount;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    aDestination->cacheObjCnt = aSource->cacheObjCnt;

    // PROJ-2163
    IDE_TEST( allocAndInitTemplateMember( aDestination,
                                          aMemory )
              != IDE_SUCCESS );

    // clone table map
    for (i = 0; i < aSource->tmplate.rowArrayCount; i++)
    {
        // Assign으로 변경한다. (성능 이슈)
        aDestination->tableMap[i] = aSource->tableMap[i];
    }

    // initialize basic members
    aDestination->stmt               = NULL;
    aDestination->flag               = aSource->flag;
    aDestination->smiStatementFlag   = aSource->smiStatementFlag;
    aDestination->numRows            = 0;
    aDestination->stmtType           = aSource->stmtType;

    // allocate insOrUptRow
    aDestination->insOrUptStmtCount = aSource->insOrUptStmtCount;
    if ( aDestination->insOrUptStmtCount > 0 )
    {
        IDU_FIT_POINT( "qcg::cloneAndInitTemplate::alloc::insOrUptRowValueCount",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc (ID_SIZEOF (UInt*) *
                                 aDestination->insOrUptStmtCount,
                                 (void**)&(aDestination->insOrUptRowValueCount))
                 != IDE_SUCCESS);

        IDU_FIT_POINT( "qcg::cloneAndInitTemplate::alloc::insOrUptRow",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc (ID_SIZEOF (smiValue*) *
                                 aDestination->insOrUptStmtCount,
                                 (void**)&(aDestination->insOrUptRow))
                 != IDE_SUCCESS);

        for ( i = 0; i < aDestination->insOrUptStmtCount; i++ )
        {
            aDestination->insOrUptRowValueCount[i] =
                aSource->insOrUptRowValueCount[i];

            // 프로시저에서 0번 template의 insOrUptStmtCount 값은 0보다
            // 클 수 있다.
            // 하지만 insOrUptRowValueCount[i]의 값은 항상 0이다.
            // 이유는 statement 별로 별도의 template이 유지되기 때문이다.

            // 예)
            // create or replace procedure proc1 as
            // begin
            // insert into t1 values ( 1, 1 );
            // update t1 set a = a + 1;
            // end;
            // /

            if( aDestination->insOrUptRowValueCount[i] == 0 )
            {
                continue;
            }

            IDU_FIT_POINT( "qcg::cloneAndInitTemplate::alloc::insOrUptRowItem",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST(aMemory->alloc( ID_SIZEOF(smiValue) *
                                     aDestination->insOrUptRowValueCount[i],
                                     (void**)&(aDestination->insOrUptRow[i]))
                     != IDE_SUCCESS);
        }
    }
    else
    {
        aDestination->insOrUptRowValueCount = NULL;
        aDestination->insOrUptRow = NULL;
    }

    // PROJ-1413 tupleVarList 초기화
    aDestination->tupleVarGenNumber = 0;
    aDestination->tupleVarList = NULL;

    // BUG-16422 execute중 임시 생성된 tableInfo의 관리
    aDestination->tableInfoMgr = NULL;

    // PROJ-1436
    aDestination->indirectRef = aSource->indirectRef;

    /* PROJ-2209 DBTIMEZONE */
    if ( aSource->unixdate == NULL )
    {
        aDestination->unixdate = NULL;
    }
    else
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF (qtcNode ),
                                 (void**)&(aDestination->unixdate))
                 != IDE_SUCCESS);

        aDestination->unixdate->node.table = aSource->unixdate->node.table ;
        aDestination->unixdate->node.column = aSource->unixdate->node.column ;
    }

    // allocate sysdate
    if ( aSource->sysdate == NULL )
    {
        aDestination->sysdate = NULL;
    }
    else
    {
        IDU_FIT_POINT( "qcg::cloneAndInitTemplate::alloc::sysdate",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc( ID_SIZEOF (qtcNode ),
                                 (void**)&(aDestination->sysdate))
                 != IDE_SUCCESS);

        aDestination->sysdate->node.table = aSource->sysdate->node.table ;
        aDestination->sysdate->node.column = aSource->sysdate->node.column ;
    }

    if ( aSource->currentdate == NULL )
    {
        aDestination->currentdate = NULL;
    }
    else
    {
        IDE_TEST(aMemory->alloc( ID_SIZEOF (qtcNode ),
                                 (void**)&(aDestination->currentdate))
                 != IDE_SUCCESS);

        aDestination->currentdate->node.table = aSource->currentdate->node.table ;
        aDestination->currentdate->node.column = aSource->currentdate->node.column ;
    }

    /* PROJ-2462 Result Cache */
    aDestination->resultCache.flag  = aSource->resultCache.flag;
    aDestination->resultCache.stack = aSource->resultCache.stack;
    aDestination->resultCache.count = aSource->resultCache.count;
    aDestination->resultCache.list  = aSource->resultCache.list;

    // BUG-44710
    sdi::initDataInfo( &(aDestination->shardExecData) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::refineStackSize( qcStatement * aStatement,
                             SInt          aStackSize )
{
    qcSession      * sSession;
    qcStmtInfo     * sStmtInfo;
    idvSQL         * sStatistics;

    if ( aStackSize == QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount )
    {
        // nothing to do
    }
    else
    {
        sSession = aStatement->session;
        sStmtInfo = aStatement->stmtInfo;

        //fix BUG-15658
        sStatistics = aStatement->mStatistics;

        IDE_TEST( qcg::freeStatement( aStatement ) != IDE_SUCCESS );

        IDE_TEST( allocStatement( aStatement,
                                  sSession,
                                  sStmtInfo,
                                  sStatistics ) != IDE_SUCCESS );

        qsxEnv::initialize( aStatement->spxEnv, sSession );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qcg::getSmiStatementCursorFlag( qcTemplate * aTemplate,
                                       UInt       * aFlag )
{
    *aFlag = aTemplate->smiStatementFlag;

    return IDE_SUCCESS;
}

UInt
qcg::getQueryStackSize()
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1358
 *    System의 Query Stack Size를 획득
 *
 * Implementation :
 *
 ***********************************************************************/

    return QCU_QUERY_STACK_SIZE;

}

UInt
qcg::getOptimizerMode()
{
/***********************************************************************
 *
 * Description :
 *    BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
 *              관련프로퍼티를 참조하지 못하는 경우 있음.
 *    OPTIMIZER MODE
 *
 * Implementation :
 *
 ***********************************************************************/

    return QCU_OPTIMIZER_MODE;
}

UInt
qcg::getAutoRemoteExec()
{
/***********************************************************************
 *
 * Description :
 *    BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
 *              관련프로퍼티를 참조하지 못하는 경우 있음.
 *    AUTO_REMOTE_EXEC
 *
 * Implementation :
 *
 ***********************************************************************/

    return QCU_AUTO_REMOTE_EXEC;
}

// BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
UInt
qcg::getOptimizerDefaultTempTbsType()
{
/***********************************************************************
 *
 * Description :
 *
 *     BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
 *
 * Implementation :
 *
 ***********************************************************************/

    return QCU_OPTIMIZER_DEFAULT_TEMP_TBS_TYPE;
}

void qcg::setDatabaseCallback(
    qcgDatabaseCallback aCreatedbFuncPtr,
    qcgDatabaseCallback aDropdbFuncPtr,
    qcgDatabaseCallback aCloseSession,
    qcgDatabaseCallback aCommitDTX,
    qcgDatabaseCallback aRollbackDTX,
    qcgDatabaseCallback aStartupFuncPtr,
    qcgDatabaseCallback aShutdownFuncPtr )
{
    mCreateCallback      = aCreatedbFuncPtr;
    mDropCallback        = aDropdbFuncPtr;
    mCloseSessionCallback = aCloseSession;
    mCommitDTXCallback   = aCommitDTX;
    mRollbackDTXCallback = aRollbackDTX;
    mStartupCallback     = aStartupFuncPtr;
    mShutdownCallback    = aShutdownFuncPtr;
}

// Proj-1360 Queue
// Proj-1677 Dequeue
void qcg::setQueueCallback(
    qcgQueueCallback aCreateQueueFuncPtr,
    qcgQueueCallback aDropQueueFuncPtr,
    qcgQueueCallback aEnqueueQueueFuncPtr,
    qcgDeQueueCallback aDequeueQueueFuncPtr,
    qcgQueueCallback   aSetQueueStampFuncPtr )
{
    mCreateQueueFuncPtr  = aCreateQueueFuncPtr;
    mDropQueueFuncPtr    = aDropQueueFuncPtr;
    mEnqueueQueueFuncPtr = aEnqueueQueueFuncPtr;
    mDequeueQueueFuncPtr = aDequeueQueueFuncPtr;
    mSetQueueStampFuncPtr = aSetQueueStampFuncPtr;
}

IDE_RC qcg::startupPreProcess( idvSQL *aStatistics )
{
    /*
     * PROJ-2109 : Remove the bottleneck of alloc/free stmts.
     *
     * To reduce the number of system calls and eliminate the contention
     * on iduMemMgr::mClientMutex[IDU_MEM_QCI]
     * in the moment of calling iduMemMgr::malloc/free,
     * we better use iduMemPool than call malloc/free directly.
     */
    IDE_TEST(mIduVarMemListPool.initialize(IDU_MEM_QCI,
                                           (SChar*)"QP_VAR_MEM_LIST_POOL",
                                           ID_SCALABILITY_SYS,
                                           ID_SIZEOF(iduVarMemList),
                                           QCG_MEMPOOL_ELEMENT_CNT,
                                           IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                           ID_TRUE,							/* UseMutex */
                                           IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                           ID_FALSE,						/* ForcePooling */
                                           ID_TRUE,							/* GarbageCollection */
                                           ID_TRUE,                         /* HWCacheLine */
                                           IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/) 
              != IDE_SUCCESS);			

    IDE_TEST(mIduMemoryPool.initialize(IDU_MEM_QCI,
                                       (SChar*)"QP_MEMORY_POOL",
                                       ID_SCALABILITY_SYS,
                                       ID_SIZEOF(iduMemory),
                                       QCG_MEMPOOL_ELEMENT_CNT,
                                       IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                       ID_TRUE,							/* UseMutex */
                                       IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                       ID_FALSE,						/* ForcePooling */
                                       ID_TRUE,							/* GarbageCollection */
                                       ID_TRUE,                         /* HWCacheLine */
                                       IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/) 
              != IDE_SUCCESS);			

    IDE_TEST(mSessionPool.initialize(IDU_MEM_QCI,
                                     (SChar*)"QP_SESSION_POOL",
                                     ID_SCALABILITY_SYS,
                                     ID_SIZEOF(qcSession),
                                     QCG_MEMPOOL_ELEMENT_CNT,
                                     IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                     ID_TRUE,							/* UseMutex */
                                     IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                     ID_FALSE,							/* ForcePooling */
                                     ID_TRUE,							/* GarbageCollection */
                                     ID_TRUE,                           /* HWCacheLine */
                                     IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
             != IDE_SUCCESS);			

    IDE_TEST(mStatementPool.initialize(IDU_MEM_QCI,
                                       (SChar*)"QP_STATEMENT_POOL",
                                       ID_SCALABILITY_SYS,
                                       ID_SIZEOF(qcStmtInfo),
                                       QCG_MEMPOOL_ELEMENT_CNT,
                                       IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                       ID_TRUE,							/* UseMutex */
                                       IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                       ID_FALSE,						/* ForcePooling */
                                       ID_TRUE,							/* GarbageCollection */
                                       ID_TRUE,                         /* HWCacheLine */
                                       IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/) 
             != IDE_SUCCESS);			

    /* BUG-46179 SD 관련 Property Load */
    IDE_TEST( sduProperty::initProperty( aStatistics ) != IDE_SUCCESS );

    // QP TRCLOG 관련 Property Load
    IDE_TEST( qcuProperty::initProperty( aStatistics ) != IDE_SUCCESS );

    // PROJ-1075 array type member function Initialize
    IDE_TEST( qsf::initialize() != IDE_SUCCESS );

    // PROJ-1685 ExtProc Agent List
    IDE_TEST( idxProc::initializeStatic() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::startupProcess()
{
    return IDE_SUCCESS;
}

IDE_RC qcg::startupControl()
{
    // query 수행 권한 설정, DDL,DML 금지

    return IDE_SUCCESS;
}

IDE_RC qcg::startupMeta()
{
    // open meta // alter database mydb upgrade meta SQL 추가로 가능. 그외 처리 불가.
    return IDE_SUCCESS;
}

IDE_RC qcg::startupService( idvSQL * aStatistics )
{
    smiTrans trans;
    iduMemory procBuildMem;

    smiStatement    sSmiStmt;
    smiStatement   *sDummySmiStmt;

    procBuildMem.init(IDU_MEM_QCI);

    IDE_TEST( trans.initialize() != IDE_SUCCESS);

    //-------------------------------------------
    // [1] check META and upgrade META
    //-------------------------------------------
    ideLog::log(IDE_SERVER_0,"\n      [QP] META DB checking....");

    IDE_TEST(trans.begin( &sDummySmiStmt, aStatistics ) != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt,
                             SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);

    IDE_TEST( qcm::check( & sSmiStmt ) != IDE_SUCCESS );

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    IDE_TEST(trans.commit() != IDE_SUCCESS);

    //-------------------------------------------
    // [2] initialize and load
    //-------------------------------------------

    IDE_TEST(trans.begin( &sDummySmiStmt, aStatistics ) != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.begin( aStatistics,
                             sDummySmiStmt,
                             SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);

    // initialize global meta handles ( table, index )
    IDE_TEST(qcm::initMetaHandles(&sSmiStmt) != IDE_SUCCESS );

    // initialize meta cache
    IDE_TEST(qcm::initMetaCaches(&sSmiStmt) != IDE_SUCCESS);

    // BUG-17224
    mInitializedMetaCaches = ID_TRUE;

    // BUG-38497
    // Server startup is failed when PSM has _prowid as a select target.
    /* PROJ-1789 PROWID */
    IDE_TEST(mtc::initializeColumn(&gQtcRidColumn,
                                   &mtdBigint,
                                   0,
                                   0,
                                   0)
             != IDE_SUCCESS);

    // consider XA heuristic trans indexes

    // PROJ-2717 Internal Procedures
    IDE_TEST( qsxLibrary::initializeStatic() != IDE_SUCCESS );

    // PROJ-1073 Package
    IDE_TEST( qsx::loadAllPkgSpec( &sSmiStmt, &procBuildMem )
              != IDE_SUCCESS );

    // To Fix PR-8671
    IDE_TEST( qsx::loadAllProc( &sSmiStmt, &procBuildMem )
              != IDE_SUCCESS );

    // PROJ-1073 Package
    IDE_TEST( qsx::loadAllPkgBody( &sSmiStmt, &procBuildMem )
              != IDE_SUCCESS );

    // PROJ-1359
    IDE_TEST( qdnTrigger::loadAllTrigger( & sSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    IDE_TEST(trans.commit() != IDE_SUCCESS);

    IDE_TEST(trans.destroy( aStatistics ) != IDE_SUCCESS);

    // PROJ-1407 Temporary Table
    IDE_TEST( qcuTemporaryObj::initializeStatic() != IDE_SUCCESS );

    /* PROJ-1071 Parallel Query */
    IDE_TEST(initPrlThrUseCnt() != IDE_SUCCESS);

    /* PROJ-2451 Concurrent Execute Package */
    IDE_TEST(initConcThrUseCnt() != IDE_SUCCESS);

    /* BUG-41307 User Lock 지원 */
    IDE_TEST( qcuSessionObj::initializeStatic() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0, "  [SUCCESS]\n");
    procBuildMem.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        (void) ideLog::log(IDE_SERVER_0, "[PREPARE] Query Processor........[FAILURE]\n");
    }
    procBuildMem.destroy();
    return IDE_FAILURE;

}

IDE_RC qcg::startupShutdown( idvSQL * aStatistics )
{
    // for freeing memory for tempinfo.
    iduMemory     sIduMem;
    smiTrans      sTrans;
    smiStatement  sSmiStmt;
    smiStatement *sDummySmiStmt;

    sIduMem.init(IDU_MEM_QCI);
    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
    IDE_TEST(sTrans.begin( &sDummySmiStmt,
                           aStatistics ) != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.begin( aStatistics,
                             sDummySmiStmt,
                             SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);

    if ( ! ( QCU_STORED_PROC_MODE & QCU_SPM_MASK_DISABLE ) )
    {
        IDE_TEST(qsx::unloadAllProc( &sSmiStmt, &sIduMem ) != IDE_SUCCESS);
    }

    IDE_TEST(qcm::finiMetaCaches(&sSmiStmt) != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    IDE_TEST(sTrans.destroy( aStatistics ) != IDE_SUCCESS);

    // PROJ-1407 Temporary Table
    qcuTemporaryObj::finilizeStatic();

    // PROJ-1685 ExtProc Agent List
    idxProc::destroyStatic();

    /* PROJ-1071 Parallel Query */
    IDE_TEST(finiPrlThrUseCnt() != IDE_SUCCESS);

    /* PROJ-2451 Concurrent Execute Package */
    IDE_TEST(finiConcThrUseCnt() != IDE_SUCCESS);

    /* BUG-41307 User Lock 지원 */
    qcuSessionObj::finalizeStatic();

    // PROJ-2717 Internal procedure
    qsxLibrary::finalizeStatic();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mIduVarMemListPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mIduMemoryPool.destroy()     != IDE_SUCCESS);
    IDE_TEST(mSessionPool.destroy()       != IDE_SUCCESS);
    IDE_TEST(mStatementPool.destroy()     != IDE_SUCCESS);

    sIduMem.destroy();

    IDE_TEST( qcuProperty::finalProperty( aStatistics ) != IDE_SUCCESS );

    /* BUG-46179 */
    IDE_TEST( sduProperty::finalProperty( aStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sIduMem.destroy();
    return IDE_FAILURE;
}

IDE_RC qcg::startupDowngrade( idvSQL * aStatistics )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2689 Downgrade meta
 *    alter database mydb downgrade; 또는
 *    startup downgrade 시에 수행한다.
 * Implementation :
 *
 ***********************************************************************/
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement   *sDummySmiStmt;
    UInt           sState = 0;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS);
    sState = 1;

    //-------------------------------------------
    // check META and Downgrade META
    //-------------------------------------------
    ideLog::log(IDE_SERVER_0, "\n      [QP] META DB downgrade....");

    IDE_TEST( sTrans.begin( &sDummySmiStmt, aStatistics ) != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sSmiStmt.begin( aStatistics, sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR)
              != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( qcm::checkMetaVersionAndDowngrade( & sSmiStmt ) != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sTrans.destroy( aStatistics ) != IDE_SUCCESS);

    ideLog::log(IDE_SERVER_0, "  [SUCCESS]\n");
 
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        (void) ideLog::log(IDE_SERVER_0, "[QP] META DB downgrade........[FAILURE]\n");

        switch (sState)
        {
            case 3:
                sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            case 2:
                sTrans.rollback();
            case 1:
                sTrans.destroy( aStatistics );
            case 0:
            default:
                break;
        }
    }
    return IDE_FAILURE;
}


IDE_RC qcg::printPlan( qcStatement       * aStatement,
                       qcTemplate        * aTemplate,
                       ULong               aDepth,
                       iduVarString      * aString,
                       qmnDisplay          aMode )
{
    qciStmtType  sStmtType;
    qmnPlan    * sPlan;

    sStmtType = aStatement->myPlan->parseTree->stmtKind;
    sPlan     = aStatement->myPlan->plan;

    switch( sStmtType )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
        case QCI_STMT_INSERT:
        case QCI_STMT_DELETE:
        case QCI_STMT_UPDATE:
        case QCI_STMT_MOVE:
        case QCI_STMT_MERGE:
            {
                IDE_TEST( sPlan->printPlan( aTemplate,
                                            sPlan,
                                            aDepth,
                                            aString,
                                            aMode )
                          != IDE_SUCCESS );

                break;
            }

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::buildPerformanceView( idvSQL * aStatistics )
{
/***********************************************************************
 *
 * Description :
 *  PROJ-1382, jhseong
 *      This function should be called in server start phase.
 *
 * Implementation :
 *  0) initialize fixed table ( automatically already done. )
 *  1) build fixed table tableinfo
 *  2) register performance view
 *  2) initialize performance view
 *  3) build performance view tableInfo
 *
 ***********************************************************************/

    IDE_TEST( qcmFixedTable::makeAndSetQcmTableInfo( aStatistics, (SChar *) "X$" )
              != IDE_SUCCESS );

#if !defined(SMALL_FOOTPRINT) || defined(WRS_VXWORKS)
    // PROJ-1618
    IDE_TEST( qcmFixedTable::makeAndSetQcmTableInfo( aStatistics, (SChar *) "D$" )
              != IDE_SUCCESS );

    IDE_TEST( qcmPerformanceView::registerPerformanceView( aStatistics, QCM_PV_TYPE_NORMAL )
              != IDE_SUCCESS );

    (void)smiFixedTable::initializeStatic( (SChar *) "V$" );

    IDE_TEST( qcmFixedTable::makeAndSetQcmTableInfo( aStatistics, (SChar *) "V$" )
              != IDE_SUCCESS );

    /* BUG-45646 */
    if ( sdi::isShardEnable() == ID_TRUE )
    {
        IDE_TEST( qcmPerformanceView::registerPerformanceView( aStatistics, QCM_PV_TYPE_SHARD )
                  != IDE_SUCCESS );
        (void)smiFixedTable::initializeStatic( (SChar *)"S$" );
        IDE_TEST( qcmFixedTable::makeAndSetQcmTableInfo( aStatistics, (SChar *) "S$" )
                  != IDE_SUCCESS );
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

idBool qcg::isFTnPV( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *  해당 쿼리가 Fixed Table or Performance View만을 ref하는지 검사
 *
 * Implementation :
 *  에러가 날 일이 없기 때문에 idBool을 리턴함.
 *
 ***********************************************************************/
    return ( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 1 ) ?
        ID_TRUE : ID_FALSE;
}

IDE_RC qcg::detectFTnPV( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *  해당 쿼리가 select일 경우 FT나 PV만을 ref하는지 detect
 *
 * Implementation :
 *  qmv::detectDollarTables함수를 이용.
 *
 ***********************************************************************/

    IDE_TEST( qmv::detectDollarTables( aStatement )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qcg::initSessionObjInfo( qcSessionObjInfo ** aSessionObj )
{
/***********************************************************************
 *
 * Description :
 *  session종속적인 object 초기화 및 메모리 할당
 *
 * Implementation :
 *  1. open된 파일리스트를 초기화
 *
 ***********************************************************************/

    qcSessionObjInfo * sSessionObj = NULL;

    IDU_FIT_POINT( "qcg::initSessionObjInfo::malloc::sSessionObj",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                 ID_SIZEOF(qcSessionObjInfo),
                                 (void**)&sSessionObj)
              != IDE_SUCCESS);

    IDE_TEST( qcuSessionObj::initOpenedFileList( sSessionObj )
              != IDE_SUCCESS );

    IDE_TEST( qcuSessionObj::initSendSocket( sSessionObj )
              != IDE_SUCCESS );
    
    // BUG-40854
    IDE_TEST( qcuSessionObj::initConnection( sSessionObj )
              != IDE_SUCCESS );

    /* BUG-41307 User Lock 지원 */
    qcuSessionObj::initializeUserLockList( sSessionObj );

    *aSessionObj = sSessionObj;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sSessionObj != NULL )
    {
        (void)iduMemMgr::free( sSessionObj );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;

}

IDE_RC qcg::finalizeSessionObjInfo( qcSessionObjInfo ** aSessionObj )
{
/***********************************************************************
 *
 * Description :
 *  session종속적인 object 삭제
 *
 * Implementation :
 *  1. open된 파일리스트를 close 하고 메모리 해제
 *
 ***********************************************************************/

    qcSessionObjInfo * sSessionObj = *aSessionObj;

    if( sSessionObj != NULL )
    {
        // PROJ-1904 Extend UDT
        (void)resetSessionObjInfo( aSessionObj );

        (void)iduMemMgr::free( sSessionObj );

        *aSessionObj = NULL;
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;
}

IDE_RC qcg::resetSessionObjInfo( qcSessionObjInfo ** aSessionObj )
{
/***********************************************************************
 *
 * Description :
 *  session종속적인 object 삭제
 *
 * Implementation :
 *  1. open된 파일리스트를 close
 *
 ***********************************************************************/

    qcSessionObjInfo * sSessionObj = *aSessionObj;
   
    if ( sSessionObj != NULL )
    {
        (void)qcuSessionObj::closeAllOpenedFile( sSessionObj );

        (void)qcuSessionObj::closeSendSocket( sSessionObj );

        // BUG-40854
        (void)qcuSessionObj::closeAllConnection( sSessionObj );

        /* BUG-41307 User Lock 지원 */
        qcuSessionObj::finalizeUserLockList( sSessionObj );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

ULong  qcg::getOptimizeMode(qcStatement * aStatement )
{
    ULong sMode;

    if ( (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT) ||
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE) )
    {
        sMode = 0;//(ULong)(((qmsParseTree *)(aStatement->myPlan->parseTree))->querySet->SFWGH->hints->optGoalType);
    }
    else
    {
        return 0;
    }
    
    return sMode;
}

ULong  qcg::getTotalCost(qcStatement * aStatement )
{
    ULong sCost;

    if (aStatement->myPlan->graph != NULL)
    {
        // To fix BUG-14503
        // totalAllCost가 SLong의 MAX보다 크면 보정.
        if( aStatement->myPlan->graph->costInfo.totalAllCost > ID_SLONG_MAX )
        {
            sCost = ID_SLONG_MAX;
        }
        else
        {
            sCost = (ULong)(aStatement->myPlan->graph->costInfo.totalAllCost);
        }
    }
    else
    {
        sCost = 0;
    }

    return sCost;
}

void
qcg::setBaseTableInfo( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     BUG-25109
 *     oledb, ado.net에서 사용할 simple select query에 대해
 *     base table name을 얻을 수 있는 함수가 필요함
 *     query string은 mm에서 관리하고, qp에서는 pointer만 가진다.
 *
 *     ex) select i1 from t1 a;
 *         --> T1, update enable
 *     ex) select i1 from v1 a;
 *         --> V1, update disable
 *     ex) select t1.i1 from t1, t2;
 *         --> 리턴할 필요없음
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree  * sParseTree;
    qmsFrom       * sFrom;
    qcmTableInfo  * sTableInfo;

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;
    
    // baseTableInfo 초기화
    idlOS::memset( (void*)&sParseTree->baseTableInfo,
                   0x00,
                   ID_SIZEOF(qmsBaseTableInfo) );

    sParseTree->baseTableInfo.isUpdatable = ID_FALSE;

    if ( sParseTree->querySet->setOp == QMS_NONE )
    {
        sFrom = sParseTree->querySet->SFWGH->from;

        if ( sFrom != NULL )
        {
            if ( ( sFrom->joinType == QMS_NO_JOIN ) &&
                 ( sFrom->next == NULL ) )
            {
                sTableInfo = sFrom->tableRef->tableInfo;

                if ( sTableInfo != NULL )
                {
                    idlOS::strncpy( sParseTree->baseTableInfo.tableOwnerName,
                                    sTableInfo->tableOwnerName,
                                    QC_MAX_OBJECT_NAME_LEN + 1 );
                    sParseTree->baseTableInfo.tableOwnerName[QC_MAX_OBJECT_NAME_LEN] = '\0';

                    idlOS::strncpy( sParseTree->baseTableInfo.tableName,
                                    sTableInfo->name,
                                    QC_MAX_OBJECT_NAME_LEN + 1 );
                    sParseTree->baseTableInfo.tableName[QC_MAX_OBJECT_NAME_LEN] = '\0';
                }
                else
                {
                    // Nothing to do.
                }

                if ( sFrom->tableRef->view == NULL )
                {
                    // view가 아니면 update할 수 있다.
                    sParseTree->baseTableInfo.isUpdatable = ID_TRUE;
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

/* fix BUG-29965 SQL Plan Cache에서 plan execution template 관리가
   Dynamic SQL 환경에서는 개선이 필요하다.
*/
void qcg::freePrepTemplate(qcStatement * aStatement,
                           idBool        aRebuild)
{
    idBool           sNeedToFreePrepStmt;
    
    if(aStatement->prepTemplateHeader != NULL)
    {

        // 반환한다.
        IDE_ASSERT( aStatement->prepTemplateHeader->prepMutex.lock(NULL /* idvSQL* */)
                    == IDE_SUCCESS );

        // used list에서 삭제.
        IDU_LIST_REMOVE(&(aStatement->prepTemplate->prepListNode));
        /* rebuld경우 old plan이기때문에 이에 대한 exection template 재활용 확률이 매우 떨어지기
         때문에 바로 해제 하여야 한다.*/
        if( ((aStatement->prepTemplateHeader->freeCount) >= QCU_SQL_PLAN_CACHE_PREPARED_EXECUTION_CONTEXT_CNT) ||
            (aRebuild == ID_TRUE) )
        {
            sNeedToFreePrepStmt  = ID_TRUE;
        }
        else
        {
            aStatement->prepTemplateHeader->freeCount++;
            //  free list으로 move.
            IDU_LIST_ADD_LAST(&(aStatement->prepTemplateHeader->freeList),&(aStatement->prepTemplate->prepListNode));
            // template을 재사용하기 위해 미리 초기화한다.
            qcgPlan::initPrepTemplate4Reuse( QC_PRIVATE_TMPLATE(aStatement) );
            sNeedToFreePrepStmt  = ID_FALSE;
        }
        IDE_ASSERT( aStatement->prepTemplateHeader->prepMutex.unlock()
                    == IDE_SUCCESS );
        if(sNeedToFreePrepStmt  == ID_TRUE)
        {
            IDE_ASSERT( qci::freePrepTemplate(aStatement->prepTemplate) == IDE_SUCCESS );
        }
        else
        {
            //nothing to do.
        }
        // prepared private template을 초기화한다.
        aStatement->prepTemplateHeader = NULL;
        aStatement->prepTemplate = NULL;
    }
    else
    {
        //nothing to do
    }
}

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
IDE_RC qcg::allocIduVarMemList(void  **aVarMemList)
{
    return mIduVarMemListPool.alloc(aVarMemList);
}

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
IDE_RC qcg::freeIduVarMemList(iduVarMemList *aVarMemList)
{
    return mIduVarMemListPool.memfree(aVarMemList);
}

/* PROJ-2462 Result Cache */
IDE_RC qcg::allocIduMemory( void ** aMemory )
{
    return mIduMemoryPool.alloc( aMemory );
}

/* PROJ-2462 Result Cache */
void qcg::freeIduMemory( iduMemory * aMemory )
{
    (void)mIduMemoryPool.memfree( aMemory );
}

IDE_RC qcg::initBindParamData( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      호스트 변수의 데이터 영역을 할당한다.
 *
 * Implementation :
 *      1. pBindParam 의 data 에 이미 할당된 메모리를 해제
 *      2. 호스트 변수들의 사이즈를 합산하고 메모리를 할당
 *      3. pBindParam 의 data 에 설정
 *      4. Variable tuple 의 row 에 설정 (BUG-34995)
 *
 ***********************************************************************/

    qciBindParamInfo  * sParam = NULL;
    UInt                sSize[MTC_TUPLE_COLUMN_ID_MAXIMUM];   // PROJ-2163 BUGBUG
    UInt                sOffset[MTC_TUPLE_COLUMN_ID_MAXIMUM]; // PROJ-2163 BUGBUG
    UInt                sTupleSize;
    UInt                i;
    void              * sTupleRow;
    qcTemplate        * sTemplate;
    UShort              sBindTuple;

    if ( ( aStatement->pBindParam != NULL  ) &&
         ( aStatement->pBindParamCount > 0 ) )
    {
        sParam = aStatement->pBindParam;

        // 1. pBindParam 의 data 에 이미 할당된 메모리를 해제

        // 첫번째 호스트 변수의 data 가 iduVarMemList 에서 얻어온 주소이다.
        // 두번째 이후의 호스트 변수는 이 메모리를 잘라서 사용하는 것이므로
        // 첫번째 호스트 변수의 data 를 보고 할당여부를 판단한다.
        if( sParam[0].param.data != NULL )
        {
            // iduVarMemList->free 함수는 EXCEPTION 을 발생시키지 않는다.
            // 따라서 IDE_TEST 로 분기하지 않는다.
            QC_QMB_MEM(aStatement)->free( sParam[0].param.data );

            for( i = 0; i < aStatement->pBindParamCount; i++ )
            {
                sParam[i].param.data = NULL;
            }
        }

        // 2. 호스트 변수들의 사이즈를 합산하고 메모리를 할당
        IDE_TEST( calculateVariableTupleSize( aStatement,
                                              sSize,
                                              sOffset,
                                              & sTupleSize )
                  != IDE_SUCCESS );

        // 메모리를 할당
        IDU_FIT_POINT( "qcg::initBindParamData::cralloc::sTupleRow",
                        idERR_ABORT_InsufficientMemory );

        // PROJ-1362
        // lob이 out-bind 되었을 때 execute중 에러가 발생하는 경우
        // (ex, update중 에러가 났을 경우)
        // 이미 open된 locator에 대해 close 하기위해
        // mm은 항상 getBindParamData를 호출한다.
        // variable tuple의 row를 초기화 함으로써
        // mm에서 locator가 open되었는지를 판한할 수 있게 한다.
        IDE_TEST( aStatement->qmbMem->cralloc( sTupleSize,
                                               (void**)&(sTupleRow) )
                  != IDE_SUCCESS);

        // 3. pBindParam 의 data 에 tuple 주소 설정
        for( i = 0; i < aStatement->pBindParamCount; i++ )
        {
            sParam[i].param.data     = (SChar*)sTupleRow + sOffset[i];
            sParam[i].param.dataSize = sSize[i];
        }

        // 4. Variable tuple 의 row 에 새로 할당한 sTupleRow 를 set (BUG-34995)
        sTemplate  = QC_PRIVATE_TMPLATE(aStatement);
        sBindTuple = sTemplate->tmplate.variableRow;

        sTemplate->tmplate.rows[sBindTuple].row = sTupleRow;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::calculateVariableTupleSize( qcStatement * aStatement,
                                        UInt        * aSize,
                                        UInt        * aOffset,
                                        UInt        * aTupleSize )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      pBindParam 정보로 각 호스트 변수의 size, offset 과
 *      tuple size 를 계산한다.
 *      PROJ-2163 전엔 type bind protocol 에서 mtcColumn 을 생성하고
 *      smiColumn 의 size 를 볼 수 있었지만
 *      이제 mtcColumn 은 prepare 단계에서 생성하므로 직접 계산해야 한다.
 *
 * Implementation :
 *      1. Type id 에 맞는 mtdModule 을 얻는다.
 *      2. Column size 를 얻기위해 estimate 를 수행한다.
 *      3. align 을 고려해 tuple size 를 구한다.
 *
 ***********************************************************************/

    UInt sSize = 0;
    UInt sOffset;
    UInt sArguments;
    SInt sPrecision;
    SInt sScale;
    UInt sTypeId;
    UInt i;
    const mtdModule * sModule;
    const mtdModule * sRealModule;

    for ( i = 0, sOffset = 0;
          i < aStatement->pBindParamCount;
          i++ )
    {
        sTypeId     = aStatement->pBindParam[i].param.type;
        sPrecision  = aStatement->pBindParam[i].param.precision;
        sArguments  = aStatement->pBindParam[i].param.arguments;
        sScale      = aStatement->pBindParam[i].param.scale;

        // 해당 mtdModule 찾아서 size calculate

        // 1. Type id 에 맞는 mtdModule 을 얻는다.
        IDE_TEST( mtd::moduleById( & sModule, sTypeId ) != IDE_SUCCESS);

        if ( sModule->id == MTD_NUMBER_ID )
        {
            if( sArguments == 0 )
            {
                IDE_TEST( mtd::moduleById( & sRealModule, MTD_FLOAT_ID )
                        != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( mtd::moduleById( & sRealModule, MTD_NUMERIC_ID )
                        != IDE_SUCCESS);
            }
        }
        else
        {
            sRealModule = sModule;
        }

        // 2. Column size 를 얻기위해 estimate 를 수행한다.
        // estimate 는 semantic 검사용 함수지만 size 측정을 위해 사용한다.
        IDE_TEST( sRealModule->estimate( & sSize,
                                         & sArguments,
                                         & sPrecision,
                                         & sScale )
                  != IDE_SUCCESS );

        // 3. align 을 고려해 tuple size 를 구한다.

        // offset 계산. 각 타입의 align 까지 고려해야 한다.
        sOffset = idlOS::align( sOffset, sRealModule->align );

        // 3.1. 호스트 변수의 offset
        aOffset[i] = sOffset;

        // 3.2. 호스트 변수의 size
        aSize[i] = sSize;

        sOffset += sSize;
    }

    *aTupleSize = sOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::fixOutBindParam( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      Type binding 중 out bind parameter 부분을 처리하는 함수
 *      실제 parameter 처리는 qcg::setOutBindColumn4SP 함수에서 수행한다.
 *
 * Implementation :
 *      Variable tuple 의 row로 생성 및 data copy
 *
 ***********************************************************************/

    qcTemplate       * sTemplate;
    mtcTuple         * sVariableTuple;
    qciBindParam     * sBindParam;
    mtcColumn        * sBindColumn;
    SChar            * sParamData;
    void             * sOrgParamData = NULL;
    qciBindParamInfo * sBindParamInfo = aStatement->pBindParam;
    UShort             sColumn;

    // 호스트 변수의 개수가 0보다 크고 pBindParam이 NULL이 아니면
    // reprepare, rebuild 상황에서 호출된 경우이다.
    if( ( qcg::getBindCount( aStatement ) > 0 ) &&
        ( sBindParamInfo != NULL ) )
    {
        // setPrivateTemplate이 호출되기 이전이다.
        // 따라서 shared template을 참조해야 한다.
        sTemplate      = QC_SHARED_TMPLATE(aStatement);
        sVariableTuple = &sTemplate->tmplate.rows[sTemplate->tmplate.variableRow];

        // Allocate variable tuple's new row
        IDE_TEST( QC_QMB_MEM(aStatement)->cralloc(
                      sVariableTuple->rowMaximum,
                      (void**)&(sParamData) )
                  != IDE_SUCCESS);

        sOrgParamData = sBindParamInfo[0].param.data;

        // Copy bound data
        for( sColumn = 0; sColumn < sVariableTuple->columnCount; sColumn++ )
        {
            sBindParam = &sBindParamInfo[sColumn].param;
            sBindColumn = sVariableTuple->columns + sColumn;

            // Copy data when CMP_DB_PARAM_INPUT or CMP_DB_PARAM_INPUT_OUTPUT
            if( sBindParam->inoutType != CMP_DB_PARAM_OUTPUT )
            {
                // Source : pBindParam[n].param.data (Old row)
                // Target : sParamData + mtcColumn.offset (New row)
                // Size   : mtcColumn.size (Old size == New size)
                idlOS::memcpy( sParamData + sBindColumn->column.offset,
                               sBindParam->data,
                               sBindColumn->column.size );
            }

            // Set pBindParam[n].param.data to new row
            sBindParam->data = sParamData + sBindColumn->column.offset;

            IDE_DASSERT( sBindColumn->column.offset + sBindColumn->column.size
                         <= sVariableTuple->rowMaximum );
        }

        // BUG-47128 Query rebuild시 query_binding이 계속 증가할 수 있습니다.
        if ( sOrgParamData != NULL )
        {
            QC_QMB_MEM(aStatement)->free( sOrgParamData );
        }

        // Set new row to variable tuple
        sVariableTuple->row = (void*) sParamData;
    }
    else
    {
        // There is no host variable
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
void * qcg::getDatabaseLinkSession( qcStatement * aStatement )
{
    void * sDatabaseLinkSession = NULL;

    if( aStatement->session->mMmSession != NULL )
    {
        sDatabaseLinkSession = qci::mSessionCallback.mGetDatabaseLinkSession(
            aStatement->session->mMmSession );
    }
    else
    {
        /* BUG-37093 */
        if ( aStatement->session->mMmSessionForDatabaseLink != NULL )
        {
            sDatabaseLinkSession =
                qci::mSessionCallback.mGetDatabaseLinkSession(
                    aStatement->session->mMmSessionForDatabaseLink );
        }
        else
        {
            sDatabaseLinkSession = NULL;
        }
    }

    return sDatabaseLinkSession;
}

// PROJ-1073 Package
IDE_RC qcg::allocSessionPkgInfo( qcSessionPkgInfo ** aSessionPkg )
{
    qcSessionPkgInfo * sSessionPkg;
    UInt               sStage = 0;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                 ID_SIZEOF( qcSessionPkgInfo ),
                                 ( void** )&sSessionPkg )
              != IDE_SUCCESS);

    sStage = 1;
    sSessionPkg->pkgInfoMem = NULL;
    sSessionPkg->next       = NULL;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                 ID_SIZEOF(iduMemory),
                                 ( void** ) &sSessionPkg->pkgInfoMem )
              != IDE_SUCCESS);

    sSessionPkg->pkgInfoMem  = new ( sSessionPkg->pkgInfoMem ) iduMemory;

    IDE_ASSERT( sSessionPkg->pkgInfoMem != NULL );

    // BUG-39295 Reduce a memory chunk size for the session package info.
    IDE_TEST( sSessionPkg->pkgInfoMem->init( IDU_MEM_QCI ) != IDE_SUCCESS);

    sStage = 2;

    IDE_TEST( allocTemplate4Pkg(sSessionPkg, sSessionPkg->pkgInfoMem ) != IDE_SUCCESS );

    *aSessionPkg = sSessionPkg;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    switch (sStage)
    {
        case 2:
            (void) iduMemMgr::free( sSessionPkg->pkgInfoMem );

        case 1:
            (void) iduMemMgr::free( sSessionPkg );
    }

    return IDE_FAILURE;
}

IDE_RC qcg::allocTemplate4Pkg( qcSessionPkgInfo * aSessionPkg, iduMemory * aMemory )
{
    qcTemplate       * sTemplate;
    UInt               sCount;

    IDE_TEST( aMemory->alloc( ID_SIZEOF(qcTemplate),
                              (void**)&( sTemplate ) )
              != IDE_SUCCESS);

    // stack은 추후 설정한다.
    sTemplate->tmplate.stackBuffer     = NULL;
    sTemplate->tmplate.stack           = NULL;
    sTemplate->tmplate.stackCount      = 0;
    sTemplate->tmplate.stackRemain     = 0;

    sTemplate->tmplate.dataSize        = 0;
    sTemplate->tmplate.data            = NULL;
    sTemplate->tmplate.execInfoCnt     = 0;
    sTemplate->tmplate.execInfo        = NULL;
    sTemplate->tmplate.initSubquery    = qtc::initSubquery;
    sTemplate->tmplate.fetchSubquery   = qtc::fetchSubquery;
    sTemplate->tmplate.finiSubquery    = qtc::finiSubquery;
    sTemplate->tmplate.setCalcSubquery = qtc::setCalcSubquery;
    sTemplate->tmplate.getOpenedCursor =
        (mtcGetOpenedCursorFunc)qtc::getOpenedCursor;
    sTemplate->tmplate.addOpenedLobCursor =
        (mtcAddOpenedLobCursorFunc)qtc::addOpenedLobCursor;
    sTemplate->tmplate.closeLobLocator = qtc::closeLobLocator;
    sTemplate->tmplate.getSTObjBufSize = qtc::getSTObjBufSize;
    sTemplate->tmplate.variableRow     = ID_USHORT_MAX;

    // PROJ-2002 Column Security
    sTemplate->tmplate.encrypt         = qcsModule::encryptCallback;
    sTemplate->tmplate.decrypt         = qcsModule::decryptCallback;
    sTemplate->tmplate.encodeECC       = qcsModule::encodeECCCallback;
    sTemplate->tmplate.getDecryptInfo  = qcsModule::getDecryptInfoCallback;
    sTemplate->tmplate.getECCInfo      = qcsModule::getECCInfoCallback;
    sTemplate->tmplate.getECCSize      = qcsModule::getECCSize;

    // dateFormat은 추후 설정한다.
    sTemplate->tmplate.dateFormat      = NULL;
    sTemplate->tmplate.dateFormatRef   = ID_FALSE;

    /* PROJ-2208 */
    sTemplate->tmplate.nlsCurrency    = qtc::getNLSCurrencyCallback;
    sTemplate->tmplate.nlsCurrencyRef = ID_FALSE;

    // BUG-37247
    sTemplate->tmplate.groupConcatPrecisionRef = ID_FALSE;

    // BUG-41944
    sTemplate->tmplate.arithmeticOpMode    = MTC_ARITHMETIC_OPERATION_DEFAULT;
    sTemplate->tmplate.arithmeticOpModeRef = ID_FALSE;

    // PROJ-2527 WITHIN GROUP AGGR
    sTemplate->tmplate.funcData = NULL;
    sTemplate->tmplate.funcDataCnt = 0;

    // PROJ-1579 NCHAR
    sTemplate->tmplate.nlsUse          = NULL;

    // To Fix PR-12659
    // Internal Tuple Set 관련 메모리는 필요에 할당받도록 함.
    sTemplate->tmplate.rowCount      = 0;
    sTemplate->tmplate.rowArrayCount = 0;

    for( sCount = 0; sCount < MTC_TUPLE_TYPE_MAXIMUM; sCount++ )
    {
        sTemplate->tmplate.currentRow[sCount] = ID_USHORT_MAX;
    }

    //-------------------------------------------------------
    // PROJ-1358
    // Internal Tuple의 자동확장과 관련하여
    // Internal Tuple의 공간을 할당하고,
    // Table Map의 공간을 초기화하여 할당한다.
    //-------------------------------------------------------

    sTemplate->planCount = 0;
    sTemplate->planFlag = NULL;

    sTemplate->cursorMgr = NULL;
    sTemplate->tempTableMgr = NULL;
    sTemplate->numRows = 0;
    sTemplate->stmtType = QCI_STMT_MASK_MAX;
    sTemplate->fixedTableAutomataStatus = 0;
    sTemplate->stmt = NULL;
    sTemplate->flag = QC_TMP_INITIALIZE;
    sTemplate->smiStatementFlag = 0;
    sTemplate->insOrUptStmtCount = 0;
    sTemplate->insOrUptRowValueCount = NULL;
    sTemplate->insOrUptRow = NULL;

    /* PROJ-2209 DBTIMEZONE */
    sTemplate->unixdate = NULL;
    sTemplate->sysdate = NULL;
    sTemplate->currentdate = NULL;

    // PROJ-1413 tupleVarList 초기화
    sTemplate->tupleVarGenNumber = 0;
    sTemplate->tupleVarList = NULL;

    // BUG-16422 execute중 임시 생성된 tableInfo의 관리
    sTemplate->tableInfoMgr = NULL;

    // PROJ-1436
    sTemplate->indirectRef = ID_FALSE;

    /* PROJ-2452 Caching for DETERMINISTIC Function */
    sTemplate->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    sTemplate->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    sTemplate->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    sTemplate->cacheObjCnt    = 0;
    sTemplate->cacheObjects   = NULL;

    /* PROJ-2448 Subquery caching */
    sTemplate->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    sTemplate->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                ID_TRUE : ID_FALSE;
    sTemplate->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    aSessionPkg->pkgTemplate = sTemplate;

    /* PROJ-2462 Result Cache */
    QC_RESULT_CACHE_INIT( &sTemplate->resultCache );

    // BUG-44710
    sdi::initDataInfo( &(sTemplate->shardExecData) );

    // BUG-44795
    sTemplate->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    sTemplate->mUnnestViewNameIdx = 0;

    /* BUG-48776 */
    sTemplate->mSubqueryMode = QCU_SUBQUERY_MODE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1438 Job Scheduler
 */
IDE_RC qcg::runProcforJob( UInt     aJobThreadIndex,
                           void   * aMmSession,
                           SInt     aJob,
                           SChar  * aSqlStr,
                           UInt     aSqlStrLen,
                           UInt   * aErrorCode )
{
    QCD_HSTMT     sHstmt;
    qciStmtType   sStmtType;
    vSLong        sAffectedRowCount;
    idBool        sResultSetExist;
    idBool        sRecordExist;

    ideLog::log( IDE_QP_3, "[JOB THREAD %d][JOB %d : BEGIN]",
                 aJobThreadIndex,
                 aJob );

    ideLog::log( IDE_JOB_1, "[JOB THREAD %d][JOB %d : BEGIN]",
                 aJobThreadIndex,
                 aJob );

    IDE_TEST( qcd::allocStmtNoParent( aMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );

    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            aSqlStr,
                            aSqlStrLen,
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    IDE_TEST( qcd::executeNoParent( sHstmt,
                                    NULL,
                                    & sAffectedRowCount,
                                    & sResultSetExist,
                                    & sRecordExist,
                                    ID_TRUE )
              != IDE_SUCCESS );
    
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    ideLog::log( IDE_QP_3, "[JOB THREAD %d][JOB %d : END SUCCESS]",
                 aJobThreadIndex,
                 aJob );

    ideLog::log( IDE_JOB_1, "[JOB THREAD %d][JOB %d : END SUCCESS]",
                 aJobThreadIndex,
                 aJob );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aErrorCode = ideGetErrorCode();

    ideLog::log( IDE_QP_3, "[JOB THREAD %d][JOB %d : END FAILURE] ERR-%05X : %s",
                 aJobThreadIndex, 
                 aJob,
                 E_ERROR_CODE(ideGetErrorCode()),
                 ideGetErrorMsg(ideGetErrorCode()));

    ideLog::log( IDE_JOB_1, "[JOB THREAD %d][JOB %d : END FAILURE] ERR-%05X : %s",
                 aJobThreadIndex, 
                 aJob,
                 E_ERROR_CODE(ideGetErrorCode()),
                 ideGetErrorMsg(ideGetErrorCode()));

    return IDE_FAILURE;
}

IDE_RC qcg::runTempSQL( void   * aMmSession,
                        SChar  * aSqlStr,
                        UInt     aSqlStrLen,
                        UInt   * aErrorCode )
{
    QCD_HSTMT    sHstmt;
    qciStmtType  sStmtType;
    vSLong       sAffectedRowCount;
    idBool       sResultSetExist;
    idBool       sRecordExist;
    idBool       sIsStmtAlloc = ID_FALSE;
    qcStatement * sStatement;
    qcSession   * sQcSession;
    
    IDE_DASSERT(aMmSession != NULL);

    ideLog::log( IDE_SD_17, "[Temporary SQL: BEGIN(%s)]", aSqlStr);

    IDE_TEST( qcd::allocStmtNoParent( aMmSession,
                                      ID_TRUE,  // dedicated mode
                                      & sHstmt )
              != IDE_SUCCESS );
    sIsStmtAlloc = ID_TRUE;

    IDE_TEST( qcd::getQcStmt( sHstmt,
                              &sStatement )
              != IDE_SUCCESS );

    sStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_TEMP_SQL_MASK;
    sStatement->session->mQPSpecific.mFlag |= QC_SESSION_TEMP_SQL_TRUE;

    sQcSession = qci::mSessionCallback.mGetQcSession( aMmSession );
    
    if (( sQcSession->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
        QC_SESSION_SHARD_DDL_TRUE )
    {        
        sStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_DDL_MASK;
        sStatement->session->mQPSpecific.mFlag |= QC_SESSION_SHARD_DDL_TRUE;
    }
    
    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            aSqlStr,
                            aSqlStrLen,
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    IDE_TEST( qcd::executeNoParent( sHstmt,
                                    NULL,
                                    & sAffectedRowCount,
                                    & sResultSetExist,
                                    & sRecordExist,
                                    ID_TRUE )
              != IDE_SUCCESS );

    sIsStmtAlloc = ID_FALSE;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    ideLog::log( IDE_SD_17, "[Temporary SQL: END SUCCESS]" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aErrorCode = ideGetErrorCode();

    ideLog::log( IDE_SD_17, "[Temporary SQL: END FAILURE] ERR-%05X : %s",
                 E_ERROR_CODE(ideGetErrorCode()),
                 ideGetErrorMsg(ideGetErrorCode()));
    IDE_PUSH();
    if ( sIsStmtAlloc != ID_FALSE )
    {
        (void)qcd::freeStmt( sHstmt, ID_TRUE );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*
 * ---------------------------------------------------------------------------
 * PROJ-1071 Parallel Query
 * ---------------------------------------------------------------------------
 * mPrlThrUseCnt
 * 현재 서버에서 총 몇개의 parallel worker thread 가 사용중인지를 나타냄
 * 모든 session 에서 공유
 * ---------------------------------------------------------------------------
 */

IDE_RC qcg::initPrlThrUseCnt()
{
    IDE_TEST(mPrlThrUseCnt.mMutex.initialize("PRL_THR_USE_CNT_MUTEX",
                                             IDU_MUTEX_KIND_NATIVE,
                                             IDV_WAIT_INDEX_NULL)
             != IDE_SUCCESS);

    mPrlThrUseCnt.mCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::finiPrlThrUseCnt()
{
    return mPrlThrUseCnt.mMutex.destroy();
}

IDE_RC qcg::reservePrlThr(UInt aRequire, UInt* aReserveCnt)
{
    UInt   sReserve;

    IDE_DASSERT(aRequire > 1);

    IDE_TEST(mPrlThrUseCnt.mMutex.lock(NULL) != IDE_SUCCESS);

    if (mPrlThrUseCnt.mCnt + aRequire <= QCU_PARALLEL_QUERY_THREAD_MAX)
    {
        // 요청받은 thread 수 만큼 예약할 수 있을 때
        sReserve = aRequire;
        mPrlThrUseCnt.mCnt += sReserve;
    }
    else
    {
        if (mPrlThrUseCnt.mCnt < QCU_PARALLEL_QUERY_THREAD_MAX)
        {
            // 요청받은 thread 수 만큼은 예약할 수 없지만
            // 보다 적은 수라도 예약할 수 있을 때 가능한만큼 예약한다
            sReserve = QCU_PARALLEL_QUERY_THREAD_MAX - mPrlThrUseCnt.mCnt;
            mPrlThrUseCnt.mCnt += sReserve;
        }
        else
        {
            // 이미 최대치까지 예약상태일 때
            sReserve = 0;
        }
    }

    IDE_TEST(mPrlThrUseCnt.mMutex.unlock() != IDE_SUCCESS);

    *aReserveCnt = sReserve;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::releasePrlThr(UInt aCount)
{
    IDE_TEST(mPrlThrUseCnt.mMutex.lock(NULL) != IDE_SUCCESS);

#if defined(DEBUG)
    IDE_DASSERT(mPrlThrUseCnt.mCnt >= aCount);
    mPrlThrUseCnt.mCnt -= aCount;
#else
    if (mPrlThrUseCnt.mCnt < aCount)
    {
        mPrlThrUseCnt.mCnt = 0;
    }
    else
    {
        mPrlThrUseCnt.mCnt -= aCount;
    }
#endif

    IDE_TEST(mPrlThrUseCnt.mMutex.unlock() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PROJ-1071 Parallel Query
 *
 * thread info free
 * thread count release
 * ------------------------------------------------------------------
 */
IDE_RC qcg::finishAndReleaseThread(qcStatement* aStatement)
{
    UInt sThrCount;
    UInt sStage;

    IDE_DASSERT(aStatement != NULL);
    IDE_DASSERT(aStatement->mThrMgr != NULL);

    sStage = 0;
    sThrCount = aStatement->mThrMgr->mThrCnt;

    if (sThrCount > 0)
    {
        sStage = 1;
        IDE_TEST(qmcThrObjFinal( aStatement->mThrMgr ) != IDE_SUCCESS);
        sStage = 0;
        IDE_TEST(releasePrlThr( sThrCount ) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)releasePrlThr(sThrCount);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PROJ-1071 Parallel Query
 *
 * worker thread 를 모두 종료한다 (thread join)
 * close cursor 를 하기 전에 반드시 이 작업을 수행해야함
 * ------------------------------------------------------------------
 */
IDE_RC qcg::joinThread(qcStatement* aStatement)
{
    UInt sThrCount;

    IDE_DASSERT(aStatement != NULL);
    IDE_DASSERT(aStatement->mThrMgr != NULL);

    sThrCount = aStatement->mThrMgr->mThrCnt;

    if (sThrCount > 0)
    {
        IDE_TEST(qmcThrJoin(aStatement->mThrMgr) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ------------------------------------------------------------------
 * PROJ-1071 Parallel Query
 * ------------------------------------------------------------------
 * parallel worker thread 에서 사용하는 template 할당을 위한 함수들
 *
 * - cloneTemplate4Parallel()
 * - allocAndCloneTemplate4Parallel()
 * - allocAndCloneTemplateMember()     (private)
 * - cloneTemplateMember()             (private)
 * ------------------------------------------------------------------
 */

IDE_RC qcg::cloneTemplate4Parallel( iduMemory  * aMemory,
                                    qcTemplate * aSource,
                                    qcTemplate * aDestination )
{
    // mtcTemplate 의 복사
    IDE_TEST( mtc::cloneTemplate4Parallel( aMemory,
                                           &aSource->tmplate,
                                           &aDestination->tmplate )
              != IDE_SUCCESS );

    // planFlag, data plan, execInfo 를 clone 하고
    // cursor manager, temp table manager 를 연결한다.
    cloneTemplateMember( aSource, aDestination );

    // initialize basic members
    aDestination->flag = aSource->flag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::allocAndCloneTemplate4Parallel( iduMemory  * aMemory,
                                            iduMemory  * aCacheMemory,
                                            qcTemplate * aSource,
                                            qcTemplate * aDestination )
{
    UInt   i;
    ULong  sMtcFlag;
    UInt   sRowSize;
    UShort sTupleID;

    // PROJ-1358
    // Template 복사 전에 Internal Tuple Set공간을
    // 할당해 주어야 함.
    aDestination->tmplate.rowArrayCount = aSource->tmplate.rowArrayCount;

    IDE_TEST( allocInternalTuple( aDestination,
                                  aMemory,
                                  aDestination->tmplate.rowArrayCount )
              != IDE_SUCCESS );

    // mtcTemplate 의 복사
    IDE_TEST( mtc::cloneTemplate4Parallel( aMemory,
                                           &aSource->tmplate,
                                           &aDestination->tmplate )
              != IDE_SUCCESS );

    // Intermediate tuple 은 row 를 재할당 해야한다.
    for ( i = 0; i < aSource->tmplate.rowCount; i++ )
    {
        sMtcFlag = aSource->tmplate.rows[i].lflag;

        // ROW ALLOC 인 tuple 은 INTERMEDIATE 밖에 없다.
        if ( (sMtcFlag & MTC_TUPLE_ROW_ALLOCATE_MASK)
             == MTC_TUPLE_ROW_ALLOCATE_TRUE )
        {
            if ( aDestination->tmplate.rows[i].rowMaximum > 0 )
            {
                IDE_TEST( aMemory->cralloc(
                        ID_SIZEOF(SChar) * aDestination->tmplate.rows[i].rowMaximum,
                        (void**)&(aDestination->tmplate.rows[i].row))
                    != IDE_SUCCESS);
            }
            else
            {
                aDestination->tmplate.rows[i].row = NULL;
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    // 다음 요소들을 할당하고 clone 하거나 원본 template 과 연결한다.
    // Clone : planCount, planFlag, tmplate.data, tmplate.execInfo
    // 연결  : cursorMgr, tempTableMgr
    IDE_TEST( allocAndCloneTemplateMember( aMemory,
                                           aSource,
                                           aDestination )
              != IDE_SUCCESS );

    // clone table map
    for ( i = 0; i < aSource->tmplate.rowArrayCount; i++ )
    {
        // Assign으로 변경한다. (성능 이슈)
        aDestination->tableMap[i] = aSource->tableMap[i];
    }

    // clone basic members
    aDestination->flag                     = aSource->flag;
    aDestination->smiStatementFlag         = aSource->smiStatementFlag;
    aDestination->numRows                  = aSource->numRows;
    aDestination->stmtType                 = aSource->stmtType;
    aDestination->fixedTableAutomataStatus = aSource->fixedTableAutomataStatus;

    // clone qcStatement
    idlOS::memcpy( (void*) aDestination->stmt,
                   (void*) aSource->stmt,
                   ID_SIZEOF(qcStatement) );

    /*
     * BUG-38843 The procedure or function call depth has exceeded
     * spxEnv 는 공유하지 않는다.
     */
    IDE_TEST(aMemory->alloc(ID_SIZEOF(qsxEnvInfo),
                            (void**)&aDestination->stmt->spxEnv)
             != IDE_SUCCESS);

    idlOS::memcpy((void*)aDestination->stmt->spxEnv,
                  (void*)aSource->stmt->spxEnv,
                  ID_SIZEOF(qsxEnvInfo));

    aDestination->stmt->qmxMem   = aMemory;
    aDestination->stmt->pTmplate = aDestination;

    // allocate insOrUptRow
    aDestination->insOrUptStmtCount        = aSource->insOrUptStmtCount;
    if ( aDestination->insOrUptStmtCount > 0 )
    {
        IDU_FIT_POINT( "qcg::allocAndCloneTemplate4Parallel::alloc",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc (ID_SIZEOF (UInt*) *
                                 aDestination->insOrUptStmtCount,
                                 (void**)&(aDestination->insOrUptRowValueCount))
                 != IDE_SUCCESS);

        IDE_TEST(aMemory->alloc (ID_SIZEOF (smiValue*) *
                                 aDestination->insOrUptStmtCount,
                                 (void**)&(aDestination->insOrUptRow))
                 != IDE_SUCCESS);

        for ( i = 0; i < aDestination->insOrUptStmtCount; i++ )
        {
            aDestination->insOrUptRowValueCount[i] =
                aSource->insOrUptRowValueCount[i];

            // 프로시저에서 0번 template의 insOrUptStmtCount 값은 0보다
            // 클 수 있다.
            // 하지만 insOrUptRowValueCount[i]의 값은 항상 0이다.
            // 이유는 statement 별로 별도의 template이 유지되기 때문이다.

            // 예)
            // create or replace procedure proc1 as
            // begin
            // insert into t1 values ( 1, 1 );
            // update t1 set a = a + 1;
            // end;
            // /

            if ( aDestination->insOrUptRowValueCount[i] > 0 )
            {
                IDE_TEST( aMemory->alloc(ID_SIZEOF(smiValue) *
                                         aDestination->insOrUptRowValueCount[i],
                                         (void**)&(aDestination->insOrUptRow[i]))
                          != IDE_SUCCESS);
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        aDestination->insOrUptRowValueCount = NULL;
        aDestination->insOrUptRow = NULL;
    }

    // PROJ-1413 tupleVarList 초기화
    aDestination->tupleVarGenNumber = aSource->tupleVarGenNumber;
    aDestination->tupleVarList      = aSource->tupleVarList;

    // execute중 임시 생성된 tableInfo의 관리
    aDestination->tableInfoMgr = aSource->tableInfoMgr;

    // PROJ-1436
    aDestination->indirectRef = aSource->indirectRef;

    // Date 는 원본 template 과 동일한 시간을 사용해야 한다.
    aDestination->unixdate    = aSource->unixdate;
    aDestination->sysdate     = aSource->sysdate;
    aDestination->currentdate = aSource->currentdate;

    /*
     * BUG-38830 constant wrapper + sysdate
     * date pseudo column 은 execute 시작시 한번만 setting 된다.
     * clone template 할때 원본에서 copy 해온다.
     */
    if (aDestination->unixdate != NULL)
    {
        sTupleID = aDestination->unixdate->node.table;
        sRowSize = aSource->tmplate.rows[sTupleID].rowMaximum;
        IDE_DASSERT(aSource->tmplate.rows[sTupleID].row != NULL);

        idlOS::memcpy(aDestination->tmplate.rows[sTupleID].row,
                      aSource->tmplate.rows[sTupleID].row,
                      ID_SIZEOF(SChar) * sRowSize);
    }
    else
    {
        /* nothing to do */
    }

    if (aDestination->sysdate != NULL)
    {
        sTupleID = aDestination->sysdate->node.table;
        sRowSize = aSource->tmplate.rows[sTupleID].rowMaximum;
        IDE_DASSERT(aSource->tmplate.rows[sTupleID].row != NULL);

        idlOS::memcpy(aDestination->tmplate.rows[sTupleID].row,
                      aSource->tmplate.rows[sTupleID].row,
                      ID_SIZEOF(SChar) * sRowSize);
    }
    else
    {
        /* nothing to do */
    }

    if (aDestination->currentdate != NULL)
    {
        sTupleID = aDestination->currentdate->node.table;
        sRowSize = aSource->tmplate.rows[sTupleID].rowMaximum;
        IDE_DASSERT(aSource->tmplate.rows[sTupleID].row != NULL);

        idlOS::memcpy(aDestination->tmplate.rows[sTupleID].row,
                      aSource->tmplate.rows[sTupleID].row,
                      ID_SIZEOF(SChar) * sRowSize);
    }
    else
    {
        /* nothing to do */
    }

    /* PROJ-2452 Cache for DETERMINISTIC function */
    aDestination->stmt->qxcMem   = aCacheMemory;
    aDestination->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    aDestination->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    aDestination->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    aDestination->cacheObjCnt    = aSource->cacheObjCnt;
    aDestination->cacheObjects   = NULL;

    if ( aDestination->cacheObjCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::allocAndCloneTemplate4Parallel::alloc::cacheObjects",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aMemory->alloc( aDestination->cacheObjCnt * ID_SIZEOF(qtcCacheObj),
                                  (void**)&(aDestination->cacheObjects) )
                  != IDE_SUCCESS);

        for ( i = 0; i < aDestination->cacheObjCnt; i++ )
        {
            QTC_CACHE_INIT_CACHE_OBJ( aDestination->cacheObjects+i, aDestination->cacheMaxSize );
        }
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-2448 Subquery caching */
    aDestination->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    aDestination->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                   ID_TRUE : ID_FALSE;
    aDestination->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                   ID_TRUE : ID_FALSE;
    aDestination->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    /* PROJ-2462 Result Cache */
    aDestination->resultCache = aSource->resultCache;

    // BUG-44710
    sdi::initDataInfo( &(aDestination->shardExecData) );

    // BUG-44795
    aDestination->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    aDestination->mUnnestViewNameIdx = aSource->mUnnestViewNameIdx;

    /* BUG-48776 */
    aDestination->mSubqueryMode = QCU_SUBQUERY_MODE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::allocAndCloneTemplateMember( iduMemory  * aMemory,
                                         qcTemplate * aSource,
                                         qcTemplate * aDestination )
{
    UInt i = 0;

    IDE_ASSERT( aSource      != NULL );
    IDE_ASSERT( aDestination != NULL );

    aDestination->planFlag         = NULL;
    aDestination->cursorMgr        = NULL;
    aDestination->tempTableMgr     = NULL;
    aDestination->tmplate.data     = NULL;
    aDestination->tmplate.execInfo = NULL;
    aDestination->tmplate.funcData = NULL;

    // Cursor manager 와 temp table manager 는 새로 할당하지 않는다.

    // Link cursor manager
    // Cursor manager 를 공유하여 statemnet 종료 시 한번에 모든 커서를
    // 종료하도록 한다.
    aDestination->cursorMgr = aSource->cursorMgr;

    // Link temp table manager
    // Temp table manager 를 공유하여 statemnet 종료 시 한번에 모든
    // temp table 들을 정리하도록 한다.
    aDestination->tempTableMgr = aSource->tempTableMgr;

    // Data plan 할당
    if( aSource->tmplate.dataSize > 0 )
    {
        IDU_FIT_POINT( "qcg::allocAndCloneTemplateMember::alloc::tmplateData",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(aMemory->alloc( aSource->tmplate.dataSize,
                                 (void**)&(aDestination->tmplate.data))
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    if( aSource->tmplate.execInfoCnt > 0 )
    {
        IDE_TEST(aMemory->alloc( aSource->tmplate.execInfoCnt *
                                 ID_SIZEOF(UInt),
                                 (void**)&(aDestination->tmplate.execInfo))
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-2527 WITHIN GROUP AGGR
    if ( aSource->tmplate.funcDataCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::allocAndCloneTemplateMember::alloc::tmplate.funcData",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aMemory->alloc( aSource->tmplate.funcDataCnt *
                                  ID_SIZEOF(mtfFuncDataBasicInfo*),
                                  (void**)&(aDestination->tmplate.funcData) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }
    
    /* BUG-38748 */
    IDE_ERROR(aSource->planCount > 0);
    IDE_TEST(aMemory->alloc(ID_SIZEOF(UInt) * aSource->planCount,
                            (void**)&aDestination->planFlag)
             != IDE_SUCCESS);

    /* PROJ-2452 Cache for DETERMINISTIC function */
    aDestination->cacheMaxCnt    = QCU_QUERY_EXECUTION_CACHE_MAX_COUNT;
    aDestination->cacheMaxSize   = QCU_QUERY_EXECUTION_CACHE_MAX_SIZE;
    aDestination->cacheBucketCnt = QCU_QUERY_EXECUTION_CACHE_BUCKET_COUNT;
    aDestination->cacheObjCnt    = aSource->cacheObjCnt;
    aDestination->cacheObjects   = NULL;

    if ( aDestination->cacheObjCnt > 0 )
    {
        IDU_FIT_POINT( "qcg::allocAndCloneTemplateMember::alloc::cacheObjects",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aMemory->alloc( aDestination->cacheObjCnt * ID_SIZEOF(qtcCacheObj),
                                  (void**)&(aDestination->cacheObjects) )
                  != IDE_SUCCESS);

        for ( i = 0; i < aDestination->cacheObjCnt; i++ )
        {
            QTC_CACHE_INIT_CACHE_OBJ( aDestination->cacheObjects+i, aDestination->cacheMaxSize );
        }
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-2448 Subquery caching */
    aDestination->forceSubqueryCacheDisable = QCU_FORCE_SUBQUERY_CACHE_DISABLE;

    /* PROJ-2553 Cache-aware Memory Hash Temp Table */ 
    aDestination->memHashTempPartDisable     = ( QCU_HSJN_MEM_TEMP_PARTITIONING_DISABLE == 1 ) ?
                                                   ID_TRUE : ID_FALSE;
    aDestination->memHashTempManualBucketCnt = ( QCU_HSJN_MEM_TEMP_AUTO_BUCKETCNT_DISABLE == 1 ) ?
                                                   ID_TRUE : ID_FALSE;
    aDestination->memHashTempTlbCount        = QCU_HSJN_MEM_TEMP_TLB_COUNT;

    // BUG-44710
    sdi::initDataInfo( &(aDestination->shardExecData) );

    // BUG-44795
    aDestination->optimizerDBMSStatPolicy = QCU_OPTIMIZER_DBMS_STAT_POLICY;

    aDestination->mUnnestViewNameIdx = aSource->mUnnestViewNameIdx;

    /* BUG-48776 */
    aDestination->mSubqueryMode = QCU_SUBQUERY_MODE;

    // planFlag, data plan, execInfo 를 clone 하고
    // cursor manager, temp table manager 를 연결한다.
    cloneTemplateMember( aSource, aDestination );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aDestination->planFlag         = NULL;
    aDestination->cursorMgr        = NULL;
    aDestination->tempTableMgr     = NULL;
    aDestination->tmplate.data     = NULL;
    aDestination->tmplate.execInfo = NULL;
    aDestination->tmplate.funcData = NULL;
    aDestination->cacheObjects     = NULL;

    return IDE_FAILURE;
}

void qcg::cloneTemplateMember( qcTemplate* aSource, qcTemplate* aDestination )
{
    UInt i;

    // copy planFlag
    aDestination->planCount = aSource->planCount;

    for (i = 0; i < aDestination->planCount; i++)
    {
        aDestination->planFlag[i] = aSource->planFlag[i];
    }

    // Clone data plan
    if ( aSource->tmplate.dataSize > 0 )
    {
        idlOS::memcpy( (void*) aDestination->tmplate.data,
                       (void*) aSource->tmplate.data,
                       aSource->tmplate.dataSize );
    }
    else
    {
        // Nothing to do.
    }

    // execInfo 도 복사한다.
    aDestination->tmplate.execInfoCnt = aSource->tmplate.execInfoCnt;

    for ( i = 0; i < aSource->tmplate.execInfoCnt; i++ )
    {
        aDestination->tmplate.execInfo[i] = QTC_WRAPPER_NODE_EXECUTE_FALSE;
    }

    // PROJ-2527 WITHIN GROUP AGGR
    aDestination->tmplate.funcDataCnt = aSource->tmplate.funcDataCnt;
    
    if ( aSource->tmplate.funcDataCnt > 0 )
    {
        idlOS::memset( aDestination->tmplate.funcData,
                       0x00,
                       ID_SIZEOF(mtfFuncDataBasicInfo*) *
                       aDestination->tmplate.funcDataCnt );
    }
    else
    {
        // Nothing to do.
    }
    
    // Tuple->modify 는 초기화 해야 한다.
    resetTupleSet( aDestination );
}

IDE_RC qcg::allocStmtListMgr( qcStatement * aStatement )
{
    qcStmtListMgr * sStmtListMgr;
    
    IDE_ASSERT( aStatement != NULL );

    IDE_TEST( STRUCT_ALLOC( aStatement->qmeMem,
                            qcStmtListMgr,
                            &sStmtListMgr )
              != IDE_SUCCESS );

    aStatement->myPlan->stmtListMgr = sStmtListMgr;

    clearStmtListMgr ( aStatement->myPlan->stmtListMgr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

void qcg::clearStmtListMgr( qcStmtListMgr * aStmtListMgr )
{
    IDE_ASSERT(aStmtListMgr != NULL );

    aStmtListMgr->head           = NULL;
    aStmtListMgr->current        = NULL;
    aStmtListMgr->tableIDSeqNext = 0;
    aStmtListMgr->mHostVarOffset = 0; /* TASK-7219 */
}

/***********************************************************************
 *
 * Description : BUG-38294
 *    Parallel query 에서 사용할 QMX, QXC 메모리를 생성한다.
 *
 *    여기서 생성된 QMX 메모리는 PRLQ data plan 의 template 에 연결된다.
 *    Worker thread 는 PRLQ data plan 의 template 을 전달받아 사용하며
 *    이때 여기서 생성된 QMX 메모리를 사용하게 된다.
 *
 *    QXC 메모리는 cache 수행을 위해 사용된다. (PROJ-2452)
 *
 *    QMX, QXC 메모리는 각 PRLQ 의 firstInit 시에 한번 할당되며
 *    할당과 동시에 qcStatement 의 mThrMemList 에 추가된다.
 *
 *    여기서 할당된 QMX, QXC 메모리는 qcStatement 의 QMX 메모리가 해제되기 전에
 *    mThrMemList 를 따라가며 해제된다.
 *        - SQLExecDirect() :
 *        - SQLPrepare() :
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qcg::allocPRLQExecutionMemory( qcStatement * aStatement,
                                      iduMemory  ** aMemory,
                                      iduMemory  ** aCacheMemory )
{
    qcPRLQMemObj* sMemObj = NULL;
    qcPRLQMemObj* sCacheMemObj = NULL;

    if (aStatement->mPRLQMemList == NULL)
    {
        /* list 생성 */
        IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(iduList),
                                               (void**)&aStatement->mPRLQMemList)
                 != IDE_SUCCESS);
        IDU_LIST_INIT(aStatement->mPRLQMemList);
    }
    else
    {
        /* nothing to do. */
    }

    /* node 생성 */
    IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(qcPRLQMemObj),
                                           (void**)&sMemObj)
             != IDE_SUCCESS);

    /* worker thread 가 사용할 qmx memory 생성 */
    IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(iduMemory),
                                           (void**)&sMemObj->mMemory)
             != IDE_SUCCESS);
    sMemObj->mMemory->init(IDU_MEM_QMX);

    IDU_LIST_ADD_LAST(aStatement->mPRLQMemList, (iduListNode*)sMemObj);

    *aMemory = sMemObj->mMemory;

    /* PROJ-2452 Cache for DETERMINISTIC function */
    IDU_FIT_POINT( "qcg::allocPRLQExecutionMemory::alloc::sCacheMemObj",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(qcPRLQMemObj),
                                             (void**)&sCacheMemObj )
              != IDE_SUCCESS);

    // worker thread 가 사용할 qxc memory 생성
    IDE_TEST( QC_QXC_MEM(aStatement)->alloc( ID_SIZEOF(iduMemory),
                                             (void**)&sCacheMemObj->mMemory )
              != IDE_SUCCESS);
    sCacheMemObj->mMemory->init( IDU_MEM_QXC );

    IDU_LIST_ADD_LAST( aStatement->mPRLQMemList, (iduListNode*)sCacheMemObj );

    *aCacheMemory = sCacheMemObj->mMemory;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : BUG-38294
 *    Parallel query 에서 사용한 QMX, QXC 메모리를 해제한다.
 *
 *    Parallel query 에 쓰이는 QMX 메모리는 각 PRLQ 의 firstInit 시에
 *    한번 할당되며 할당과 동시에 qcStatement 의 mThrMemList 에 추가된다.
 *
 *    이 QMX 메모리를 qcStatement 의 QMX, QXC 메모리가 해제되기 전에
 *    이 함수를 통해 해제한다.
 *
 *    이 함수를 호출하는 곳은 다음과 같다.
 *        - qcg::clearStatement()
 *        - qcg::closeStatement()
 *        - qcg::freeStatement()
 *
 * Implementation :
 *
 ***********************************************************************/
void qcg::freePRLQExecutionMemory(qcStatement * aStatement)
{
    iduListNode* sIter;
    iduListNode* sIterNext;

    if (aStatement->mPRLQMemList != NULL)
    {
        IDU_LIST_ITERATE_SAFE(aStatement->mPRLQMemList, sIter, sIterNext)
        {
            IDU_LIST_REMOVE(sIter);
            ((qcPRLQMemObj*)sIter)->mMemory->destroy();
        }
        aStatement->mPRLQMemList = NULL;
    }
    else
    {
        /* nothing to do */
    }
}

// PROJ-2527 WITHIN GROUP AGGR
IDE_RC qcg::addPRLQChildTemplate( qcStatement * aStatement,
                                  qcTemplate  * aTemplate )
{
    iduListNode * sNode = NULL;
    
    if ( aStatement->mPRLQChdTemplateList == NULL )
    {
        IDU_FIT_POINT("qcg::addPRLQChildTemplate::alloc::sNode",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(iduListNode),
                                                 (void**)&sNode )
                  != IDE_SUCCESS );
        
        IDU_LIST_INIT( sNode );
        
        aStatement->mPRLQChdTemplateList = sNode;
    }
    else
    {
        // Nothing to do.
    }
    IDU_FIT_POINT("qcg::addPRLQChildTemplate::alloc::sNode1",
                  idERR_ABORT_InsufficientMemory);    
    IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(iduListNode),
                                             (void**)&sNode )
              != IDE_SUCCESS );
    
    IDU_LIST_INIT_OBJ( sNode, aTemplate );
    
    IDU_LIST_ADD_LAST(aStatement->mPRLQChdTemplateList, sNode );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2527 WITHIN GROUP AGGR
void qcg::finiPRLQChildTemplate( qcStatement * aStatement )
{
    iduListNode * sIter = NULL;
    iduListNode * sIterNext = NULL;
    qcTemplate  * sTemplate = NULL;

    if ( aStatement->mPRLQChdTemplateList != NULL )
    {
        IDU_LIST_ITERATE_SAFE( aStatement->mPRLQChdTemplateList, sIter, sIterNext )
        {
            sTemplate = (qcTemplate*)(sIter->mObj);
            
            // PROJ-2527 WITHIN GROUP AGGR
            // cloneTemplate시 생성한 객체를 해제한다.
            mtc::finiTemplate( &(sTemplate->tmplate) );
            
            IDU_LIST_REMOVE(sIter);
        }
        
        aStatement->mPRLQChdTemplateList = NULL;
    }
    else
    {
        /* nothing to do */
    }
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC qcg::initConcThrUseCnt()
{
    IDE_TEST(mConcThrUseCnt.mMutex.initialize("CONC_THR_USE_CNT_MUTEX",
                                              IDU_MUTEX_KIND_NATIVE,
                                              IDV_WAIT_INDEX_NULL)
             != IDE_SUCCESS);

    mConcThrUseCnt.mCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC qcg::finiConcThrUseCnt()
{
    return mConcThrUseCnt.mMutex.destroy();
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC qcg::reserveConcThr(UInt aRequire, UInt* aReserveCnt)
{
    UInt   sReserve;

    IDE_DASSERT(aRequire > 0);

    IDE_TEST(mConcThrUseCnt.mMutex.lock(NULL) != IDE_SUCCESS);

    if (mConcThrUseCnt.mCnt + aRequire <= QCU_CONC_EXEC_DEGREE_MAX)
    {
        // 요청받은 thread 수 만큼 예약할 수 있을 때
        sReserve = aRequire;
        mConcThrUseCnt.mCnt += sReserve;
    }
    else
    {
        if (mConcThrUseCnt.mCnt < QCU_CONC_EXEC_DEGREE_MAX)
        {
            // 요청받은 thread 수 만큼은 예약할 수 없지만
            // 보다 적은 수라도 예약할 수 있을 때 가능한만큼 예약한다
            sReserve = QCU_CONC_EXEC_DEGREE_MAX - mConcThrUseCnt.mCnt;
            mConcThrUseCnt.mCnt += sReserve;
        }
        else
        {
            // 이미 최대치까지 예약상태일 때
            sReserve = 0;
        }
    }

    IDE_TEST(mConcThrUseCnt.mMutex.unlock() != IDE_SUCCESS);

    *aReserveCnt = sReserve;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC qcg::releaseConcThr(UInt aCount)
{
    IDE_TEST(mConcThrUseCnt.mMutex.lock(NULL) != IDE_SUCCESS);

#if defined(DEBUG)
    IDE_DASSERT(mConcThrUseCnt.mCnt >= aCount);
    mConcThrUseCnt.mCnt -= aCount;
#else
    if (mConcThrUseCnt.mCnt < aCount)
    {
        mConcThrUseCnt.mCnt = 0;
    }
    else
    {
        mConcThrUseCnt.mCnt -= aCount;
    }
#endif

    IDE_TEST(mConcThrUseCnt.mMutex.unlock() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-41248 DBMS_SQL package
IDE_RC qcg::openCursor( qcStatement * aStatement,
                        UInt        * aCursorId )
{
    UInt               i = 0;
    qcOpenCursorInfo * sCursor;

    for ( i = 0; i < QCU_PSM_CURSOR_OPEN_LIMIT; i++ )
    {
        if ( aStatement->session->mQPSpecific.mOpenCursorInfo[i].mMmStatement == NULL )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( i == QCU_PSM_CURSOR_OPEN_LIMIT, err_max_open_cursor ); 

    *aCursorId = i;

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[i]);

    qcd::initStmt( &sCursor->mMmStatement );

    IDE_TEST( qcd::allocStmtNoParent( aStatement->session->mMmSession,
                                      ID_FALSE,
                                      &sCursor->mMmStatement )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                 ID_SIZEOF( iduMemory ),
                                 (void **)&sCursor->mMemory )
              != IDE_SUCCESS );

    IDE_TEST( sCursor->mMemory->init( IDU_MEM_QCI )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_max_open_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_MAX_OPEN_CURSOR) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::isOpen( qcStatement * aStatement,
                    UInt          aCursorId,
                    idBool      * aIsOpen )
{
    qcOpenCursorInfo * sCursor;

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    if ( sCursor->mMmStatement == NULL )
    {
        *aIsOpen = ID_FALSE;
    }
    else
    {
        *aIsOpen = ID_TRUE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::parse( qcStatement * aStatement,
                   UInt          aCursorId,
                   SChar       * aSql )
{
    qciStmtType        sStmtType;
    qcOpenCursorInfo * sCursor;

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    // BUG-44856
    aStatement->session->mQPSpecific.mLastCursorId = aCursorId;

    IDE_TEST( qcd::prepare( sCursor->mMmStatement,
                            NULL,
                            NULL,
                            &sStmtType,
                            aSql,
                            idlOS::strlen( aSql ),
                            ID_FALSE )
              != IDE_SUCCESS );

    sCursor->mBindInfo = NULL;

    sCursor->mRecordExist = ID_FALSE;

    sCursor->mFetchCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::bindVariable( qcStatement * aStatement,
                          UInt          aCursorId,
                          mtdCharType * aName, 
                          mtcColumn   * aColumn,
                          void        * aValue )
{
    SChar            * sValueBuffer = NULL;
    UInt               sSize = 0;
    qcBindInfo       * sBindInfo;
    idBool             sFound = ID_FALSE;
    qcOpenCursorInfo * sCursor;
    SChar              sName[QC_MAX_BIND_NAME + 1];
    UInt               sNameSize;
    qcStatement      * sStatement;

    if ( aName->value[0] == ':' )
    {
        idlOS::memcpy( sName, &(aName->value[1]), aName->length - 1 );
        sName[aName->length - 1] = '\0';
        sNameSize = aName->length - 1;
    }
    else
    {
        idlOS::memcpy( sName, aName->value, aName->length );
        sName[aName->length] = '\0';
        sNameSize = aName->length;
    }

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );
   
    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    // BUG-44856
    aStatement->session->mQPSpecific.mLastCursorId = aCursorId;

    IDE_TEST( qcd::getQcStmt( sCursor->mMmStatement,
                              &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( qci::checkBindParamByName( (qciStatement *)sStatement,
                                         sName ) != IDE_SUCCESS );

    sSize = aColumn->module->actualSize( aColumn,
                                         aValue );

    IDE_TEST( sCursor->mMemory->alloc( sSize,
                                      (void **)&sValueBuffer )
              != IDE_SUCCESS ); 

    idlOS::memcpy( sValueBuffer, aValue, sSize );

    for ( sBindInfo = sCursor->mBindInfo;
          sBindInfo != NULL;
          sBindInfo = sBindInfo->mNext )
    {
        if ( idlOS::strCaselessMatch( sName,
                                      sNameSize,
                                      sBindInfo->mName,
                                      idlOS::strlen( sBindInfo->mName ) ) == 0 )
        {
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sFound == ID_TRUE )
    {
        sBindInfo->mColumn = *aColumn;
        sBindInfo->mValue = sValueBuffer;
        sBindInfo->mValueSize = sSize;
    }
    else
    {
        IDE_TEST( sCursor->mMemory->alloc( ID_SIZEOF( qcBindInfo ),
                                           (void **)&sBindInfo )
              != IDE_SUCCESS ); 

        idlOS::memcpy( sBindInfo->mName,
                       sName,
                       sNameSize );

        sBindInfo->mName[sNameSize] = '\0';

        sBindInfo->mColumn = *aColumn;
        sBindInfo->mValue = sValueBuffer;
        sBindInfo->mValueSize = sSize;

        sBindInfo->mNext = sCursor->mBindInfo;
        sCursor->mBindInfo = sBindInfo;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::executeCursor( qcStatement * aStatement,
                           UInt          aCursorId,
                           vSLong      * aRowCount )
{
    idBool             sResultSetExist;
    idBool             sRecordExist;
    qcBindInfo       * sBindInfo;
    qcOpenCursorInfo * sCursor;

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    // BUG-44856
    aStatement->session->mQPSpecific.mLastCursorId = aCursorId;

    for ( sBindInfo = sCursor->mBindInfo;
          sBindInfo != NULL;
          sBindInfo = sBindInfo->mNext )
    {
        IDE_TEST( qcd::bindParamInfoSetByName( sCursor->mMmStatement,
                                               &sBindInfo->mColumn,
                                               sBindInfo->mName,
                                               QS_IN )
                  != IDE_SUCCESS );
    }

    for ( sBindInfo = sCursor->mBindInfo;
          sBindInfo != NULL;
          sBindInfo = sBindInfo->mNext )
    {
        IDE_TEST( qcd::bindParamDataByName( sCursor->mMmStatement,
                                            sBindInfo->mValue,
                                            sBindInfo->mValueSize,
                                            sBindInfo->mName )
                  != IDE_SUCCESS );
    }
    
    IDE_TEST( qcd::execute( sCursor->mMmStatement,
                            aStatement,
                            NULL,
                            aRowCount,
                            &sResultSetExist,
                            &sRecordExist,
                            ID_TRUE ) != IDE_SUCCESS );

    sCursor->mRecordExist = sRecordExist;

    sCursor->mFetchCount = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::fetchRows( qcStatement * aStatement,
                       UInt          aCursorId,
                       UInt        * aRowCount )
{
    qcOpenCursorInfo * sCursor;
    idBool             sRecordExist;

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    // BUG-44856
    aStatement->session->mQPSpecific.mLastCursorId = aCursorId;

    if ( sCursor->mFetchCount == 0 )
    {
        if ( sCursor->mRecordExist == ID_TRUE )
        {
            *aRowCount = 1;
        }
        else
        {
            *aRowCount = 0;
        }
    }
    else
    {
        IDE_TEST( qcd::fetch( aStatement,
                              QC_QMX_MEM(aStatement),
                              sCursor->mMmStatement,
                              NULL,
                              &sRecordExist )
                  != IDE_SUCCESS );

        sCursor->mRecordExist = sRecordExist;

        if ( sCursor->mRecordExist == ID_TRUE )
        {
            *aRowCount = 1;
        }
        else
        {
            *aRowCount = 0;
        }
    }

    sCursor->mFetchCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::columnValue( qcStatement * aStatement,
                         UInt          aCursorId,
                         UInt          aPosition,
                         mtcColumn   * aColumn,
                         void        * aValue,
                         UInt        * aColumnError,
                         UInt        * aActualLength )
{
    qcStatement      * sStatement;
    qcOpenCursorInfo * sCursor;

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    // BUG-44856
    aStatement->session->mQPSpecific.mLastCursorId = aCursorId;

    IDE_TEST( qcd::getQcStmt( sCursor->mMmStatement,
                              &sStatement )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aPosition == 0, err_invalid_binding );

    IDE_TEST( qci::fetchColumn( QC_QMX_MEM(aStatement),
                                (qciStatement *)sStatement,
                                (UShort)aPosition - 1,
                                aColumn,
                                aValue ) != IDE_SUCCESS ); 

    *aColumnError = 0;

    *aActualLength = aColumn->module->actualSize( aColumn, aValue );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION( err_invalid_binding );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCI_INVALID_BINDING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcg::closeCursor( qcStatement * aStatement,
                         UInt          aCursorId )
{
    qcOpenCursorInfo * sCursor;

    IDE_TEST_RAISE( aCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[aCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    // BUG-44856
    aStatement->session->mQPSpecific.mLastCursorId = aCursorId;

    IDE_TEST( qcd::freeStmt( sCursor->mMmStatement,
                             ID_TRUE ) != IDE_SUCCESS );

    sCursor->mMmStatement = NULL;

    (void)sCursor->mMemory->destroy();

    sCursor->mMemory = NULL;

    sCursor->mBindInfo = NULL;

    sCursor->mRecordExist = ID_FALSE;

    sCursor->mFetchCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-44856
IDE_RC qcg::lastErrorPosition( qcStatement * aStatement,
                               SInt        * aErrorPosition )
{
    UInt               sCursorId;
    qcOpenCursorInfo * sCursor;
    qcStatement      * sStatement;

    sCursorId = aStatement->session->mQPSpecific.mLastCursorId;

    IDE_TEST_RAISE( sCursorId >= QCU_PSM_CURSOR_OPEN_LIMIT, err_invalid_cursor );

    sCursor = &(aStatement->session->mQPSpecific.mOpenCursorInfo[sCursorId]);

    IDE_TEST_RAISE( sCursor->mMmStatement == NULL, err_invalid_cursor );

    IDE_TEST( qcd::getQcStmt( sCursor->mMmStatement,
                              &sStatement )
              != IDE_SUCCESS );

    if ( sStatement->spxEnv != NULL )
    {
        *aErrorPosition = sStatement->spxEnv->mSqlInfo.offset;
    }
    else
    {
        *aErrorPosition = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_cursor )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_INVALID_CURSOR_OPERATION) );
    }
    IDE_EXCEPTION_END;

    *aErrorPosition = 0;

    return IDE_FAILURE;
}

/* BUG-44563 trigger 생성 후 server stop하면 비정상 종료하는 경우가 발생합니다. */
IDE_RC qcg::freeSession( qcStatement * aStatement )
{
    if ( aStatement->session != NULL )
    {
        if ( (aStatement->session->mQPSpecific.mFlag
              & QC_SESSION_INTERNAL_ALLOC_MASK )
             == QC_SESSION_INTERNAL_ALLOC_TRUE )
        {
            /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
            IDE_TEST( mSessionPool.memfree(aStatement->session)
                      != IDE_SUCCESS );
            aStatement->session = NULL;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-44978
void qcg::prsCopyStrDupAppo( SChar * aDest,
                             SChar * aSrc,
                             UInt    aLength )
{
    while ( aLength-- > 0 )
    {
        *aDest++ = *aSrc;
        aSrc++;
    }
    
    *aDest = '\0';    
}

/* BUG-45899 */
UInt qcg::getSessionTrclogDetailShard( qcStatement * aStatement )
{
    UInt sTrclogDetailShard = SDU_TRCLOG_DETAIL_SHARD;

    if ( aStatement->session->mMmSession != NULL )
    {
        sTrclogDetailShard =
            qci::mSessionCallback.mGetTrclogDetailShard( aStatement->session->mMmSession );
    }

    return sTrclogDetailShard;
}

void qcg::setSessionShardInPSMEnable( qcStatement * aStatement, idBool aVal )
{
    if ( aStatement->session->mMmSession != NULL )
    {
        qci::mSessionCallback.mSetShardInPSMEnable( aStatement->session->mMmSession, aVal );
    }
}

idBool qcg::getSessionShardInPSMEnable( qcStatement * aStatement )
{
    idBool sShardInPSMEnable = ID_FALSE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sShardInPSMEnable =
            qci::mSessionCallback.mGetShardInPSMEnable( aStatement->session->mMmSession );
    }

    return sShardInPSMEnable;
}

IDE_RC qcg::runDCLforInternal( qcStatement  * aStatement,
                               SChar        * aSqlStr,
                               void         * aSession )
{
    qciStatement  sQciStatement;
    qcStatement * sStatement;
    smiTrans    * sTrans = NULL;
    idBool        sIsAllocStatement = ID_FALSE;

    // make qcStatement : alloc the members of qcStatement
    IDE_TEST( allocStatement( &sQciStatement.statement,
                              NULL,
                              NULL,
                              NULL ) != IDE_SUCCESS );
    sIsAllocStatement = ID_TRUE;
    
    sStatement = (qcStatement*)&sQciStatement.statement;
    
    qsxEnv::initialize( sStatement->spxEnv, NULL );

    qcg::setSmiStmt( sStatement, QC_SMI_STMT( aStatement ) );

    sStatement->myPlan->stmtText         = aSqlStr;
    sStatement->myPlan->stmtTextLen      = idlOS::strlen(aSqlStr);
    sStatement->myPlan->encryptedText    = NULL;   /* PROJ-2550 PSM Encryption */
    sStatement->myPlan->encryptedTextLen = 0;      /* PROJ-2550 PSM Encryption */
    sStatement->myPlan->parseTree        = NULL;
    sStatement->myPlan->plan             = NULL;
    sStatement->myPlan->graph            = NULL;
    sStatement->myPlan->scanDecisionFactors = NULL;

    // parsing
    IDE_TEST(qcpManager::parseIt( sStatement ) != IDE_SUCCESS);
    
    // 기존의 session을 사용하는 경우
    sStatement->session->mMmSession = aSession;
    
    IDE_TEST( qci::changeStmtState( &sQciStatement,
                                    EXEC_BIND_PARAM_DATA )
              != IDE_SUCCESS );

    IDE_TEST( qci::executeDCL( &sQciStatement,
                               QC_SMI_STMT( sStatement ),
                               sTrans )
              != IDE_SUCCESS );
    
    sIsAllocStatement = ID_FALSE;
    IDE_TEST( qcg::freeStatement( sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sIsAllocStatement == ID_TRUE )
    {
        (void) qcg::freeStatement( sStatement );
    }
    
    return IDE_FAILURE;
    
}

/* TASK-7219 Non-shard DML */
void qcg::increaseStmtExecSeqForShardTx( qcStatement * aStatement,
                                         idBool      * aIsIncreased )
{
    *aIsIncreased = ID_FALSE;

    if ( aStatement->session->mMmSession != NULL )
    {
        qci::mSessionCallback.mIncreaseStmtExecSeqForShardTx( aStatement->session->mMmSession );
        *aIsIncreased = ID_TRUE;
    }
}

/* TASK-7219 Non-shard DML */
void qcg::decreaseStmtExecSeqForShardTx( qcStatement * aStatement )
{
    if ( aStatement->session->mMmSession != NULL )
    {
        qci::mSessionCallback.mDecreaseStmtExecSeqForShardTx( aStatement->session->mMmSession );
    }
}

/* TASK-7219 Non-shard DML */
UInt qcg::getStmtExecSeqForShardTx( qcStatement * aStatement )
{
    UInt sStmtExecSeqForShardTx = SDI_STMT_EXEC_SEQ_INIT;

    if ( aStatement->session->mMmSession != NULL )
    {
        sStmtExecSeqForShardTx = 
            qci::mSessionCallback.mGetStmtExecSeqForShardTx( aStatement->session->mMmSession );
    }

    return sStmtExecSeqForShardTx;
}

/* TASK-7219 Non-shad DML */
sdiShardPartialExecType qcg::getShardPartialExecType( qcStatement * aStatement )
{
    sdiShardPartialExecType sShardPartialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;

    if ( aStatement->session->mMmSession != NULL )
    {
        sShardPartialExecType =
            qci::mSessionCallback.mGetStatementShardPartialExecType( QC_MM_STMT( aStatement ) );
    }

    return sShardPartialExecType;
}
