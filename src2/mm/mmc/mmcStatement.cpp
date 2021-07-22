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

#include <idtContainer.h>
#include <idvProfile.h>
#include <idvAudit.h>
#include <qci.h>
#include <sdi.h>
#include <sdiStatementManager.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcTask.h>
#include <mmtSessionManager.h>
#include <mmuProperty.h>
#include <mmcPlanCache.h>
#include <mmcParentPCO.h>
#include <mmcChildPCO.h>
#include <mmcPCB.h>
#include <mmtAuditManager.h>
#include <idmSNMP.h>

SChar  mmcStatement::mNoneSQLCacheTextID[] = "NO_SQL_CACHE_STMT";

IDE_RC mmcStatement::initialize(mmcStmtID      aStatementID,
                                mmcSession   * aSession,
                                mmcStatement * aParentStmt)
{
    /*
     * Statement Info 초기화
     */

    mInfo.mStmt                 = this;
    mInfo.mStatementID          = aStatementID;
    mInfo.mSessionID            = aSession->getSessionID();
    mInfo.mTransID              = 0;
    mInfo.mStmtState            = MMC_STMT_STATE_ALLOC;
    mInfo.mResultSetCount       = 0;
    mInfo.mEnableResultSetCount = 0;
    mInfo.mResultSetHWM         = 0;
    mInfo.mResultSet            = NULL;
    mInfo.mFetchFlag            = MMC_FETCH_FLAG_NO_RESULTSET;
    mInfo.mQueryString          = NULL;
    mInfo.mQueryLen             = 0;
    mInfo.mCursorFlag           = 0;
    mInfo.mIsArray              = ID_FALSE;
    mInfo.mIsAtomic             = ID_FALSE;
    mInfo.mRowNumber            = 0;
    mInfo.mTotalRowNumber       = 0;
    mInfo.mIsStmtBegin          = ID_FALSE;
    mInfo.mExecuteFlag          = ID_FALSE;
    mInfo.mParentStmt           = aParentStmt;
    mInfo.mLastQueryStartTime   = 0;
    mInfo.mSQLPlanCacheTextId   = (SChar*)(mmcStatement::mNoneSQLCacheTextID);
    mInfo.mSQLPlanCachePCOId    = 0;
    mInfo.mPreparedTimeExist    = ID_FALSE;

    /* BUG-45823 */
    sdi::shardPinToString( mInfo.mShardPinStr,
                           ID_SIZEOF( mInfo.mShardPinStr ),
                           aSession->getShardPIN() );
    mInfo.mShardSessionType = aSession->getShardSessionType();
    mInfo.mShardQueryType = SDI_QUERY_TYPE_NONE;

    setQueryStartTime(0);
    setFetchStartTime(0);
    setFetchEndTime(0);      /* BUG-19456 */

    idvManager::initSQL(&mInfo.mStatistics,
                        aSession->getStatistics(),
                        aSession->getEventFlag(),
                        &(aSession->getInfo()->mCurrStmtID),
                        aSession->getTask()->getLink(),
                        aSession->getTask()->getLinkCheckTime(),
                        IDV_OWNER_TRANSACTION);
    
    //PROJ-1436 SQL Plan Cache.
    mPCB = NULL;
    /*
     * QCISTATEMENT 초기화
     */

    idlOS::memset(&mQciStmt, 0, ID_SIZEOF(mQciStmt));

    IDE_TEST( qci::initializeStmtInfo( &mQciStmtInfo,
                                       (void*)this )
              != IDE_SUCCESS );

    
    IDE_TEST(qci::initializeStatement(&mQciStmt,
                                      aSession->getQciSession(),
                                      &mQciStmtInfo,
                                      &mInfo.mStatistics) != IDE_SUCCESS);

    /*
     * Statement List 초기화
     */

    IDU_LIST_INIT_OBJ(&mStmtListNodeForParent, this);
    IDU_LIST_INIT_OBJ(&mStmtListNode, this);
    IDU_LIST_INIT_OBJ(&mFetchListNode, this);

    /*
     * Mutex 초기화
     */

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Request a mutex from the mutex pool in mmcSession. */
    IDE_TEST( aSession->getMutexPool()->getMutexFromPool( &mQueryMutex,
                                                          (SChar*)"MMC_STMT_QUERY_MUTEX" )
              != IDE_SUCCESS );

    /*
     * 기타 멤버 변수 초기화
     */

    mSession             = aSession;
    mTrans               = NULL;
    mBindState           = MMC_STMT_BIND_NONE;
    mPlanPrinted         = ID_FALSE;
    mSendBindColumnInfo  = ID_FALSE;
    mTimeoutEventOccured = ID_FALSE;
    mSmiStmtPtr          = &mSmiStmt;

    /* PROJ-2701 Online Data Rebuild: for Statement Serialize */
    mExecutingTrans            = NULL;
    mIsShareTransSmiStmtLocked = ID_FALSE;

    /* BUG-47238 */
    mInfo.mFlag &= ~MMC_STMT_NEED_UNLOCK_MASK;
    mInfo.mFlag |= MMC_STMT_NEED_UNLOCK_FALSE;

    /* BUG-33196 Statement type need to be initialized in MM module */
    mStmtType           = QCI_STMT_MASK_MAX;

    mAtomicInfo.mAtomicExecuteSuccess = ID_TRUE;
    mAtomicInfo.mIsCursorOpen         = ID_FALSE;
    // BUG-23054 mmcStatement::setAtomicLastErrorCode() 에서 Valgrind BUG 발생합니다.
    mAtomicInfo.mAtomicLastErrorCode  = idERR_IGNORE_NoError;

    IDU_LIST_INIT( &mChildStmtList );
    
    /*
     * Plan String 초기화
     */

    IDE_TEST(iduVarStringInitialize(getPlanString(),
                                    mmcStatementManager::getPlanBufferPool(),
                                    MMC_PLAN_BUFFER_SIZE) != IDE_SUCCESS);


    // PROJ-1386 Dynamic-SQL
    // parent statement의 child로 매달기
    if( isRootStmt() == ID_TRUE )
    {        
        // root는 session에 달기
        aSession->lockForStmtList();
        
        IDU_LIST_ADD_LAST(aSession->getStmtList(), getStmtListNode());
        
        aSession->unlockForStmtList();
    }
    else
    {
        IDU_LIST_ADD_LAST( mInfo.mParentStmt->getChildStmtList(), getStmtListNodeForParent() );
    }

    /* PROJ-2177 User Interface - Cancel */
    mInfo.mStmtCID     = MMC_STMT_CID_NONE;
    mInfo.mIsExecuting = ID_FALSE;

    mInfo.mCursorHold = MMC_STMT_CURSOR_HOLD_ON;
    mInfo.mKeysetMode = MMC_STMT_KEYSETMODE_OFF;

    /* TASK-7219 Non-shard DML */
    mInfo.mShardPartialExecType = SDI_SHARD_PARTIAL_EXEC_TYPE_NONE;

    /* BUG-46892 */
    mInfo.mMathTempMem = (ULong)0;

    /* PROJ-2223 Altibase Auditing */
    mAuditObjects     = NULL;
    mAuditObjectCount = 0;

    idlOS::memset( &mAuditTrail, 0x00, ID_SIZEOF(mAuditTrail));
    mAuditTrail.mActionCode    = QCI_AUDIT_OPER_MAX;

    /* BUG-37806 */
#ifdef DEBUG
    getQciStmt()->flag &= ~QCI_STMT_AUDIT_MASK;
    getQciStmt()->flag |= QCI_STMT_AUDIT_TRUE;
#else
    if ( mmtAuditManager::isAuditStarted() == ID_TRUE ) 
    {
        getQciStmt()->flag &= ~QCI_STMT_AUDIT_MASK;
        getQciStmt()->flag |= QCI_STMT_AUDIT_TRUE;
    }
    else
    {
        getQciStmt()->flag &= ~QCI_STMT_AUDIT_MASK;
        getQciStmt()->flag |= QCI_STMT_AUDIT_FALSE;
    }
#endif

    /* PROJ-2616 */
    mIsSimpleQuerySelectExecuted = ID_FALSE;

    /* BUG-46092 */
    sdiStatementManager::initializeStatement( &mSdStmt );

    clearGCTxStmtInfo();
    linkGCTxStmtInfo();

    mInfo.mRebuildCount = 0;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcStatement::finalize()
{
    UShort sResultSetID = 0;

    /* PROJ-2223 Altibase Auditing */
    mmtAuditManager::auditBySession( this );

    if( isRootStmt() == ID_TRUE )
    {
        getSession()->lockForStmtList();
        
        IDU_LIST_REMOVE(getStmtListNode());
        
        getSession()->unlockForStmtList();   
    }
    else
    {
        IDU_LIST_REMOVE(getStmtListNodeForParent());
    }
    
    if (getStmtState() >= MMC_STMT_STATE_EXECUTED)
    {
        // BUG-17580
        IDE_ASSERT(endFetch(MMC_RESULTSET_ALL) == IDE_SUCCESS);
    }

    if (isStmtBegin() == ID_TRUE)
    {
        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditByAccess( this, MMC_EXECUTION_FLAG_FAILURE );
    
        clearStmt(MMC_STMT_BIND_NONE);

        IDE_ASSERT(endStmt(MMC_EXECUTION_FLAG_FAILURE) == IDE_SUCCESS);
    }

    IDE_ASSERT(qci::finalizeStatement(getQciStmt()) == IDE_SUCCESS);

    /* BUG-46092 */
    freeAllRemoteStatement( CMP_DB_FREE_DROP );
    sdiStatementManager::finalizeStatement( &mSdStmt );

    unlinkGCTxStmtInfo();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Free the mutex from the mutex pool in mmcSession. */
    IDE_ASSERT( getSession()->getMutexPool()->freeMutexFromPool(mQueryMutex)
                == IDE_SUCCESS );

    if( mTrans != NULL )
    {    
        if( isRootStmt() == ID_TRUE )
        {
            IDE_ASSERT(mmcTrans::free( NULL, mTrans ) == IDE_SUCCESS);
        }
        mTrans = NULL;
    }

    /* PROJ-2701 Online Data Rebuild: for Statement Serialize */
    IDE_DASSERT( mIsShareTransSmiStmtLocked == ID_FALSE );
    mIsShareTransSmiStmtLocked = ID_FALSE;
    mExecutingTrans = NULL;

    if (mInfo.mQueryString != NULL)
    {
        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(mInfo.mQueryString) == IDE_SUCCESS);
    }

    if (mInfo.mResultSet != NULL)
    {
        // PROJ-2256
        for ( sResultSetID = 0; sResultSetID < mInfo.mResultSetHWM; sResultSetID++ )
        {
            IDE_ASSERT( freeBaseRow( &( mInfo.mResultSet[sResultSetID].mBaseRow ) )
                        == IDE_SUCCESS );
        }

        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(mInfo.mResultSet) == IDE_SUCCESS);
        mInfo.mResultSet = NULL;
    }

    if(mPCB != NULL)
    {
        releasePlanCacheObject();
    }

    setStmtState(MMC_STMT_STATE_ALLOC);

    IDE_ASSERT(iduVarStringFinalize(getPlanString()) == IDE_SUCCESS);

    return IDE_SUCCESS;
}

IDE_RC mmcStatement::makePlanTreeText(idBool aCodeOnly)
{
    if (mPlanPrinted != ID_TRUE)
    {
        IDE_TEST(qci::getPlanTreeText(getQciStmt(),
                                      getPlanString(),
                                      aCodeOnly) != IDE_SUCCESS);

        mPlanPrinted = ID_TRUE;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcStatement::clearPlanTreeText()
{
    mPlanPrinted = ID_FALSE;

    IDE_TEST(iduVarStringTruncate(getPlanString(), ID_FALSE) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-2598 Shard pilot(shard Analyze)
 * shard analyze protocol에 의해 shard query를 분석한다.
 * prepare protocol이 query plan을 생성하듯
 * analyze protocol은 analysis report를 생성한다.
 */
IDE_RC mmcStatement::shardAnalyze(SChar *aQueryString, UInt aQueryLen)
{
    /* BUG-45823 */
    setShardQueryType( SDI_QUERY_TYPE_NONE );
    mSession->setCallByShardAnalyzeProtocol( ID_TRUE );

    /* TASK-7219 Analyzer/Transformer/Executor 성능개선
     *
     * Soft analysis
     *
     * Preparation 과정중에
     * Shard analysis result가 생성되어
     * plan cache에 등록 되도록 변경
     */
    IDE_TEST( prepare( aQueryString, aQueryLen )
              != IDE_SUCCESS );

    /* BUG-45823 */
    setShardQueryType( sdi::getQueryType( getQciStmt() ) );
    mSession->setCallByShardAnalyzeProtocol( ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mSession->setCallByShardAnalyzeProtocol( ID_FALSE );

    return IDE_FAILURE;
}

IDE_RC mmcStatement::prepare(SChar *aQueryString, UInt aQueryLen)
{
    mmcChildPCO            *sChildPCO;
    idBool                  sNeedHardPrepare;
    mmcPCB                 *sPCB = NULL;
    qciSQLPlanCacheContext  sPlanCacheContext;
    idvSQL                 *sStatistics;
    UChar                   sState = 0;

    /* PROJ-2223 Altibase Auditing */
    idvAuditTrail           *sAuditTrail;

    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     * QMP Memory와 Shared Template을 초기화한다.
     */
    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
    sPlanCacheContext.mSharedPlanMemory = NULL;
    sPlanCacheContext.mPrepPrivateTemplate = NULL;

    // BUG-36203 PSM Optimize
    IDE_TEST( freeChildStmt( ID_TRUE,
                             ID_TRUE )
              != IDE_SUCCESS );

    //PROJ-1436 SQL-Plan Cache.
    if(mPCB != NULL)
    {
        releasePlanCacheObject();
    }

    // BUG-21204 : V$SESSTAT 중 QUERY_PARSE, QUERY_VALIDATE, QUERY_OPTIMIZE는
    //             prepare에서 accumTime을 초기화해야 한다
    sStatistics = getStatistics();
    
    idvManager::initPrepareAccumTime(sStatistics );

    // bug-23991: prepared 후 execute 마다 PVO time 값이 그대로 남음
    // TIMED_STATISTICS 속성이 1인 경우만(ON) 수행됨.
    // statement 시작 시점(execute)에서 PVO time 값을 clear시킬지 여부를
    // 결정하기 위해서 mTimedStatistics flag를 새로 추가하고
    // prepaed가 수행되면 true로 세팅하여 beginStmt()에서 PVO time값들은
    // clear하지 않도록 한다
    if (iduProperty::getTimedStatistics() == IDV_TIMED_STATISTICS_ON)
    {
        setPreparedTimeExist(ID_TRUE);
    }

    // fix BUG-18865
    setQueryStartTime(mmtSessionManager::getBaseTime());
    
    setBindState(MMC_STMT_BIND_NONE);
    setQueryString(aQueryString, aQueryLen);

    /* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
    if ( mSession->getCallByShardAnalyzeProtocol() == ID_TRUE )
    {
        setShardQueryType( SDI_QUERY_TYPE_NONE );
    }

    /* TASK-7219 Non-shard DML */
    IDE_TEST( qci::setShardPartialExecType( getQciStmt(),
                                            getShardPartialExecType() )
              != IDE_SUCCESS );

    /*
     * BUG-15658
     */
    IDE_TEST(qci::refineStackSize(getQciStmt()) != IDE_SUCCESS);

    if ( mmcPlanCache::isEnable( mSession, &mQciStmt ) == ID_TRUE )
    {
        IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_QUERY_SOFT_PREPARE);
                             
        IDE_TEST_RAISE(softPrepare((mmcParentPCO*) NULL,
                                   MMC_SOFT_PREPARE_FOR_PREPARE,
                             &sPCB,
                             &sNeedHardPrepare) != IDE_SUCCESS,ErrorSoftPrepare);
        
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_QUERY_SOFT_PREPARE);
        
        if(sNeedHardPrepare == ID_FALSE)
        {
            //plan cache hit sucess!
            IDE_DASSERT(sPCB != NULL);
            sChildPCO = sPCB->getChildPCO();
            sState =1;
            IDE_TEST(qci::clonePrivateTemplate(getQciStmt(),
                                               sChildPCO->getSharedPlan(),
                                               sChildPCO->getPreparedPrivateTemplate(),
                                               &sPlanCacheContext)
                     != IDE_SUCCESS);
            sState = 0;
            mPCB = sPCB;
            mInfo.mSQLPlanCacheTextId = sChildPCO->mSQLTextId;
            mInfo.mSQLPlanCachePCOId  = sChildPCO->mChildID;

            mStmtType = sPlanCacheContext.mStmtType;
            
            if ( ( mSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
                 ( isRootStmt() == ID_TRUE ) )
            {
                (void)allocTrans();
            }
        }
        else
        {
            if(sPCB == NULL)
            {
                sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
            }
            else
            {
                sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_ON;
            }

            IDE_TEST(hardPrepare(sPCB,
                                 &sPlanCacheContext) != IDE_SUCCESS);

        }
    }
    else
    {
        //cache disable
        sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
        IDE_TEST(hardPrepare((mmcPCB*)NULL,
                             &sPlanCacheContext) !=  IDE_SUCCESS);
    }//else

    /* BUG-45899 */
    setShardQueryType( sdi::getQueryType( getQciStmt() ) );

    if( mSession->getExplainPlan() == QCI_EXPLAIN_PLAN_ONLY )
    {
        IDE_TEST(makePlanTreeText(ID_FALSE) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    if( mmtAuditManager::isAuditStarted() == ID_TRUE )
    {
        /* get the new object information */
        qci::getAllRefObjects( getQciStmt(), &mAuditObjects, &mAuditObjectCount );

        sAuditTrail = getAuditTrail();
        if( (sAuditTrail->mSessionID != getSessionID()) )
        {
            setAuditTrailSess( sAuditTrail, getSession() );
        }

        setAuditTrailStmt( sAuditTrail, this );
    }
    
    // fix BUG-18865
    setQueryStartTime(0);
    // BUG-17544
    setSendBindColumnInfo(ID_FALSE);
    setCursorFlag(sPlanCacheContext.mSmiStmtCursorFlag);
    setResultSetCount(qci::getResultSetCount(getQciStmt()));

    /* PROJ-2616 */
    if ( getStmtType() == QCI_STMT_DEQUEUE ||
         getStmtType() == QCI_STMT_ENQUEUE )
    {
        /* BUG-46163 DEQUEUE, ENQUEUE는 simplequery 제외 */
        setSimpleQuery( ID_FALSE );
    }
    else
    {
        setSimpleQuery( qci::isSimpleQuery(getQciStmt()) );
    }

    setStmtState(MMC_STMT_STATE_PREPARED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ErrorSoftPrepare);
    {
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_QUERY_SOFT_PREPARE);
    }
    IDE_EXCEPTION_END;
    {
        // fix BUG-18865
        setQueryStartTime(0);

        /* BUG-38472 Query timeout applies to one statement. */
        setTimeoutEventOccured( ID_FALSE );

        if ((ideGetErrorCode() & E_ACTION_MASK) == E_ACTION_FATAL)
        {
            ideLog::logErrorMsg(IDE_SERVER_0);
            IDE_CALLBACK_FATAL("fatal error code occurred in mmcStatement::prepare()");
        }

        if(sState != 0)
        {
            sPCB->planUnFix( getStatistics());
            IDU_FIT_POINT( "mmcStatement::prepare::SLEEP::AFTER::mmcPCB::planUnFix" );
        }//if
    }

    return IDE_FAILURE;
}



IDE_RC mmcStatement::execute(SLong *aAffectedRowCount, SLong *aFecthedRowCount)
{
    idBool           sIsReplStmt = ID_FALSE;
    qciStatement    *sQciStmt    = NULL;
    idmSNMPTrap      sTrap;
    UInt             sSessionFailureCount = 0;
    ULong            sProcessRow = 0;

    IDV_SQL_OPTIME_BEGIN( getStatistics(), IDV_OPTM_INDEX_QUERY_EXECUTE );

    sQciStmt    = getQciStmt();
    sIsReplStmt = qci::hasReplicationTable( sQciStmt );

    // fix BUG-12452
    IDE_TEST(qci::checkRebuild(sQciStmt) != IDE_SUCCESS);

    setExecuteFlag(ID_TRUE);
    IDE_TEST(mExecuteFunc[qciMisc::getStmtTypeNumber(mStmtType)](
                 this,
                 aAffectedRowCount,
                 aFecthedRowCount) != IDE_SUCCESS);
    IDV_SQL_OPTIME_END( getStatistics(), IDV_OPTM_INDEX_QUERY_EXECUTE );

    IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                        IDV_STAT_INDEX_EXECUTE_SUCCESS_COUNT, 1);
    /*
     * BUG-38430
     */
    mSession->getInfo()->mLastProcessRow = getStatistics()->mProcessRow;

    IDV_SQL_ADD_DIRECT(getStatistics(), mExecuteSuccessCount, 1);

    sProcessRow = (ULong)((*aAffectedRowCount < 0) ? 0 : *aAffectedRowCount);
    IDV_SQL_SET_DIRECT(getStatistics(), mProcessRow, sProcessRow);

    /* >> BUG-39352 Add each execute success count for select, insert, delete, 
       update into V$SYSSTAT */
    switch (mStmtType)
    {
        case QCI_STMT_INSERT:

            if ( sIsReplStmt != ID_TRUE )
            {
                IDV_SQL_ADD_DIRECT( getStatistics(), mExecuteInsertSuccessCount, 1 );
                IDV_SESS_ADD_DIRECT( mSession->getStatistics(), 
                                     IDV_STAT_INDEX_EXECUTE_INSERT_SUCCESS_COUNT, 1 );
            }
            else
            {
                IDV_SQL_ADD_DIRECT( getStatistics(), mExecuteReplInsertSuccessCount, 1 );
                IDV_SESS_ADD_DIRECT( mSession->getStatistics(), 
                                     IDV_STAT_INDEX_EXECUTE_REPL_INSERT_SUCCESS_COUNT, 1 );
            }

            break;

        case QCI_STMT_UPDATE:

            if ( sIsReplStmt != ID_TRUE )
            {
                IDV_SQL_ADD_DIRECT( getStatistics(), mExecuteUpdateSuccessCount, 1 );
                IDV_SESS_ADD_DIRECT( mSession->getStatistics(), 
                                     IDV_STAT_INDEX_EXECUTE_UPDATE_SUCCESS_COUNT, 1 );
            }
            else
            {
                IDV_SQL_ADD_DIRECT( getStatistics(), mExecuteReplUpdateSuccessCount, 1 );
                IDV_SESS_ADD_DIRECT( mSession->getStatistics(), 
                                     IDV_STAT_INDEX_EXECUTE_REPL_UPDATE_SUCCESS_COUNT, 1 );
            }

            break;

        case QCI_STMT_DELETE:

            if ( sIsReplStmt != ID_TRUE )
            {
                IDV_SQL_ADD_DIRECT( getStatistics(), mExecuteDeleteSuccessCount, 1 );
                IDV_SESS_ADD_DIRECT( mSession->getStatistics(), 
                                     IDV_STAT_INDEX_EXECUTE_DELETE_SUCCESS_COUNT, 1 );
            }
            else
            {
                IDV_SQL_ADD_DIRECT( getStatistics(), mExecuteReplDeleteSuccessCount, 1 );
                IDV_SESS_ADD_DIRECT( mSession->getStatistics(), 
                                     IDV_STAT_INDEX_EXECUTE_REPL_DELETE_SUCCESS_COUNT, 1 );
            }

            break;

        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_FIXED_TABLE:
            IDV_SQL_ADD_DIRECT(getStatistics(), mExecuteSelectSuccessCount, 1);
            IDV_SESS_ADD_DIRECT(mSession->getStatistics(), 
                                IDV_STAT_INDEX_EXECUTE_SELECT_SUCCESS_COUNT, 1);
            break;

        default:
            break;
    }
    /* << BUG-39352 */

    /* PROJ-2473 SNMP 지원 */
    mSession->resetSessionFailureCount();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDV_SQL_OPTIME_END( getStatistics(),
                            IDV_OPTM_INDEX_QUERY_EXECUTE );

        // fix BUG-30891
        // retry, rebuild 에러는 execute failure count를 증가시키지 않음
        switch (ideGetErrorCode() & E_ACTION_MASK)
        {
            case E_ACTION_RETRY:
                IDE_DASSERT( ideGetErrorCode() != smERR_RETRY_Row_Retry ); 
            case E_ACTION_REBUILD:
                break;
            default:
                IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                                    IDV_STAT_INDEX_EXECUTE_FAILURE_COUNT, 1);
                IDV_SQL_ADD_DIRECT(getStatistics(), mExecuteFailureCount, 1);

                sSessionFailureCount = iduProperty::getSNMPAlarmSessionFailureCount();

                /* 
                 * PROJ-2473 SNMP 지원
                 *
                 * 연속으로 sSessionFailureCount 실패가 일어나면 trap를 보낸다.
                 */
                if ((sSessionFailureCount > 0) &&
                    (mSession->addSessionFailureCount() >= sSessionFailureCount))
                {
                    idlOS::snprintf((SChar *)sTrap.mAddress, sizeof(sTrap.mAddress),
                                    "%"ID_UINT32_FMT, iduProperty::getPortNo());

                    idlOS::snprintf((SChar *)sTrap.mMessage, sizeof(sTrap.mMessage),
                                    MM_TRC_SNMP_SESSION_FAILURE_COUNT,
                                    mSession->getSessionID(),
                                    mSession->getSessionFailureCount());

                    /* BUG-40283 The third parameter of strncpy() is not correct. */
                    sTrap.mMoreInfo[0] = '\0';

                    sTrap.mLevel = 2;
                    sTrap.mCode  = SNMP_ALARM_SESSION_FAILURE_COUNT;

                    idmSNMP::trap(&sTrap);

                    mSession->resetSessionFailureCount();
                }
                else
                {
                    /* Nothing */
                }
        }

    }

    return IDE_FAILURE;
}

IDE_RC mmcStatement::doHardRebuild(mmcPCB                  *aPCB,
                                   qciSQLPlanCacheContext  *aPlanCacheContext)
{
    cmiProtocolContext *sCtx       = mSession->getTask()->getProtocolContext();
    mmcTransObj        *sTrans;
    smiTrans           *sSmiTrans;
    SChar              *sSQLText;
    UInt sStmtRetryMax    = mSession->getShardStatementRetry();
    UInt sRebuildRetryMax = SDI_SHARD_RETRY_LOOP_MAX;
    smiDistTxInfo sDistTxInfo;

    sTrans = mSession->allocTrans(this);
    sSmiTrans = mmcTrans::getSmiTrans(sTrans);

    if(aPCB != NULL)
    {
        sSQLText = aPCB->getParentPCO()->getSQLString4HardPrepare();
    }
    else
    {
        sSQLText = getQueryString();
    }

    /* BUG-48282 : Shard TX의 HARD REBUILD시에도 분산정보가 설정되어야 한다. */
    buildSmiDistTxInfo( &sDistTxInfo );

    while (qci::hardRebuild(getQciStmt(),
                            getSmiStmt(),
                            sSmiTrans->getStatement(),
                            aPlanCacheContext,
                            sSQLText,
                            getQueryLen(),
                            &sDistTxInfo) != IDE_SUCCESS)
    {
        IDE_TEST( mSession->processShardRetryError( this,
                                                    &sStmtRetryMax,
                                                    &sRebuildRetryMax )
                  != IDE_SUCCESS );

        IDE_TEST(ideIsRebuild() != IDE_SUCCESS);

        if ( ( getQciStmt()->flag & QCI_STMT_REBUILD_EXEC_MASK )
             == QCI_STMT_REBUILD_EXEC_FAILURE )
        {
            IDE_TEST(qci::clearStatement4Reprepare(getQciStmt(),
                                                   sSmiTrans->getStatement()) != IDE_SUCCESS);
        }

        if ((cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA) && (isSimpleQuery() == ID_TRUE))
        {
            /* BUG-46756 IPCDA simpleQuery exeucte 시에는 mmcStatement를 사용하지 않는다.
             * BUG-47029 smiStatement를 직접 begin, end 한다. */
        }
        else
        {
            IDE_TEST(endStmt(MMC_EXECUTION_FLAG_REBUILD) != IDE_SUCCESS);
            IDE_TEST(beginStmt() != IDE_SUCCESS);
        }
        
        IDV_SESS_ADD_DIRECT(mSession->getStatistics(), IDV_STAT_INDEX_REBUILD_COUNT, 1);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::rebuild()
{
    mmcPCB                 *sPCB =NULL;
    idBool                  sNeedHardPrepare;
    idBool                  sSuccess;
    qciSQLPlanCacheContext  sPlanCacheContext;
    mmcChildPCO            *sChildPCO;
    idvSQL                 *sStatistics;
    mmcPCOLatchState        sPCOLatchState = MMC_PCO_LOCK_RELEASED;
    
    sStatistics = getStatistics();
    IDV_SESS_ADD_DIRECT(mSession->getStatistics(), IDV_STAT_INDEX_REBUILD_COUNT, 1);

    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     * QMP Memory와 Shared Template을 초기화한다.
     */
    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
    sPlanCacheContext.mSharedPlanMemory = NULL;
    sPlanCacheContext.mPrepPrivateTemplate = NULL;

    // PROJ-2163
    IDE_TEST(qci::clearStatement4Reprepare(getQciStmt(),
                                           getSmiStmt()) != IDE_SUCCESS);

    /* TASK-7219 Non-shard DML */
    IDE_TEST( qci::setShardPartialExecType( getQciStmt(),
                                            getShardPartialExecType() )
              != IDE_SUCCESS );

    /* PROJ-2701 Sharding online date rebuild
     * Rebuild coordinator수행을 위한 plan은 plan cache를 사용하지 않는다.
     * Shard rebuild plan은 bind 값에 따라서 plan이 각기 다를 수 있기 때문에
     * Statement마다 private하게 사용한다.
     * -> PROJ-2745
     *  더이상 Rebuild Coordinator 를 생성하지 않고
     *  Shard Coordinator 가 아니라면 SMN 차이에 의한 rebuild 를 수행하지 않기 때문에
     *  soft prepare 를 허용한다. */
    if ( mPCB == NULL )
    {
        sPlanCacheContext.mPlanCacheInMode =QCI_SQL_PLAN_CACHE_IN_OFF ;
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
        IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);
        IDE_TEST_RAISE(doHardRebuild(sPCB,
                                     &sPlanCacheContext) != IDE_SUCCESS,
                       ErrorNeedGCPcb);
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);
    }
    else
    {
        //soft-rebuild;
        // 새로운 plan을 recompile해야 하는 경우이다.
        //fix BUG-27360 Code-Sonar UMR sPCB can be uninitialized mememory.
        IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC);
        //fix BUG-31376 Reducing s-latch duration of parent PCO while perform soft prepare .
        IDE_TEST_RAISE(mmcPlanCache::preventDupPlan(sStatistics,
                                                    this,
                                                    mPCB->getParentPCO(),
                                                    mPCB,
                                                    &sPCB,
                                                    &sPCOLatchState,
                                                    MMC_PLAN_RECOMPILE_BY_EXECUTION,0) != IDE_SUCCESS,
                       CreateNewPcoByRebuild);
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC);
        
        IDE_DASSERT( sPCOLatchState == MMC_PCO_LOCK_RELEASED);
        if(sPCB  == NULL)
        {
            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_SOFT_REBUILD);
            // loser,  find new plan from parent pco.
            IDE_TEST_RAISE(softPrepare(mPCB->getParentPCO(),
                                       MMC_SOFT_PREPARE_FOR_REBUILD,
                                       &sPCB,
                                       &sNeedHardPrepare) != IDE_SUCCESS,SoftPrepareError);
            IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_SOFT_REBUILD);
        }//if
        else
        {
            //winner,hard-prepare가 필요
            sNeedHardPrepare = ID_TRUE;
        }//else
        //fix BUG-24364 valgrind direct execute으로 수행시킨 statement의  plan cache정보를 reset시켜야함.
        releasePlanCacheObject();
        if(sNeedHardPrepare == ID_FALSE)
        {
            //plan cache hit.
            IDE_DASSERT(sPCB != NULL);
            sChildPCO = sPCB->getChildPCO();
            IDE_TEST(qci::clonePrivateTemplate(getQciStmt(),
                                               sChildPCO->getSharedPlan(),
                                               sChildPCO->getPreparedPrivateTemplate(),
                                               &sPlanCacheContext)
                     != IDE_SUCCESS);
        }
        else
        {
            //hard-prepare need.
            if(sPCB == NULL)
            {
                // hard-rebuild winner statement가 sql-plan cache check-in이 실패한 경우.
                sPlanCacheContext.mPlanCacheInMode =  QCI_SQL_PLAN_CACHE_IN_OFF;
                //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);
                IDE_TEST_RAISE(doHardRebuild(sPCB,
                                             &sPlanCacheContext) != IDE_SUCCESS,
                               ErrorNeedGCPcb);
                IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);
            }
            else
            {
                //sql-plan cache check-in필요.
                sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_ON;
                //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);
                IDE_TEST_RAISE(doHardRebuild(sPCB,
                                             &sPlanCacheContext) != IDE_SUCCESS,
                               ErrorNeedGCPcb);
                IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);

                /* cache in off 일 수 있다. */
                if ( sPlanCacheContext.mPlanCacheInMode == QCI_SQL_PLAN_CACHE_IN_ON )
                {
                    IDV_SQL_OPTIME_BEGIN( sStatistics, IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_REBUILD );
                    mmcPlanCache::checkIn( getStatistics(),
                                           sPCB,
                                           &sPlanCacheContext,
                                           &sSuccess);
                    IDV_SQL_OPTIME_END( sStatistics, IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_REBUILD );
                    if ( sSuccess == ID_TRUE )
                    {
                        /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
                         * QMP Memory와 Shared Template을 초기화한다.
                         */
                        sPlanCacheContext.mSharedPlanMemory = NULL;
                        sPlanCacheContext.mPrepPrivateTemplate = NULL;

                        /*fix BUG-30434 VAL] mmcPCB::getChildPCO Invalid read of size 8
                          check In 이 성공한 case에 한하여 child PCO를 참조하여야 한다.
                          check In이  실패한 경우는 cache replace에 의하여 victim이 되어
                          child PCO가 해제될수 있기때문이다.*/
                        sChildPCO = sPCB->getChildPCO();
                        IDE_TEST_RAISE(qci::clonePrivateTemplate(getQciStmt(),
                                                                 sChildPCO->getSharedPlan(),
                                                                 sChildPCO->getPreparedPrivateTemplate(),
                                                                 &sPlanCacheContext)
                                       != IDE_SUCCESS,
                                       ErrorNeedPlanUnfix);
                        mInfo.mSQLPlanCacheTextId = sChildPCO->mSQLTextId;
                        mInfo.mSQLPlanCachePCOId  = sChildPCO->mChildID;
                    }
                    else
                    {
                        (void)qci::setPrivateTemplate( getQciStmt(),
                                                       sPlanCacheContext.mSharedPlanMemory,
                                                       QCI_SQL_PLAN_CACHE_IN_FAILURE);

                        /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
                         * qci::makePlanCacheInfo()에서 만든 QMP Memory와 Shared Template을 제거한다.
                         */
                        sPlanCacheContext.mSharedPlanMemory = NULL;

                        (void)qci::freePrepPrivateTemplate( sPlanCacheContext.mPrepPrivateTemplate );
                        sPlanCacheContext.mPrepPrivateTemplate = NULL;

                        sPCB = NULL;
                    }
                }
                else
                {
                    // PROJ-2163
                    // D$table, no_plan_cache, select * from t1 where i1 = ?(mtdUndef)
                    qci::setPrivateTemplate(getQciStmt(),
                                            NULL,
                                            QCI_SQL_PLAN_CACHE_IN_NORMAL);
                    (void)mmcPlanCache::movePCBOfChildToUnUsedLst(sStatistics, sPCB);
                    sPCB = NULL;
                }
            }
        }
    }

    mPCB = sPCB;

    /* PROJ-2701 Sharding online data rebuild
     *
     * Shard rebuild에 의한 coordinating plan의 경우
     * 실제 DML을 수행하지 않고, data node로 전달/결과 취합만 할 것 이기 때문에
     * SMI_STATEMENT_SELF_TRUE가 아니면 smiStatement를 다시 begin해야한다.
     *
     */
    if ( ( sdi::hasShardCoordPlan( &((getQciStmt())->statement) ) == ID_TRUE ) &&
         ( ( mInfo.mFlag & MMC_STMT_NEED_UNLOCK_MASK ) == MMC_STMT_NEED_UNLOCK_FALSE ) )
    {
        IDE_TEST(endStmt(MMC_EXECUTION_FLAG_REBUILD) != IDE_SUCCESS);
        IDE_TEST(beginStmt() != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-45823 */
    setShardQueryType( sdi::getQueryType( getQciStmt() ) );

    // fix BUG-12452
    setBindState(MMC_STMT_BIND_DATA);
    setCursorFlag(sPlanCacheContext.mSmiStmtCursorFlag);
    IDE_TEST( resetCursorFlag() != IDE_SUCCESS );

    // PROJ-2163
    IDE_TEST(qci::setBindTuple(getQciStmt()) != IDE_SUCCESS);

    getQciStmt()->flag &= ~QCI_STMT_REBUILD_EXEC_MASK;
    getQciStmt()->flag |= QCI_STMT_REBUILD_EXEC_SUCCESS;

    return IDE_SUCCESS;
    IDE_EXCEPTION(ErrorNeedGCPcb);
    {
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLAN_HARD_REBUILD);
        if(sPCB != NULL)
        {    
            mmcPlanCache::register4GC(getStatistics(),
                                  sPCB);
        }
    }
    IDE_EXCEPTION(ErrorNeedPlanUnfix);
    {
        if(sPCB != NULL)
        {    
            sPCB->planUnFix(getStatistics());
        }
    }
    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
    IDE_EXCEPTION(SoftPrepareError);
    {

        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_SOFT_REBUILD);
    }
    
    IDE_EXCEPTION(CreateNewPcoByRebuild);
    {
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC);
    }
    
    IDE_EXCEPTION_END;
    
    if(mPCB != NULL)
    {
        //fix BUG-24364 valgrind direct execute으로 수행시킨 statement의  plan cache정보를 reset시켜야함.
        releasePlanCacheObject();

    }    

    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     * qci::makePlanCacheInfo()에서 만든 QMP Memory와 Shared Template을 제거한다.
     */
    if ( sPlanCacheContext.mPlanCacheInMode == QCI_SQL_PLAN_CACHE_IN_ON )
    {
        if ( sPlanCacheContext.mSharedPlanMemory != NULL )
        {
            (void)qci::freeSharedPlan( sPlanCacheContext.mSharedPlanMemory );
            sPlanCacheContext.mSharedPlanMemory = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sPlanCacheContext.mPrepPrivateTemplate != NULL )
        {
            (void)qci::freePrepPrivateTemplate( sPlanCacheContext.mPrepPrivateTemplate );
            sPlanCacheContext.mPrepPrivateTemplate = NULL;
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

    return IDE_FAILURE;
}

IDE_RC mmcStatement::checkShardRebuildStatus( idBool * aRebuildDetected )
{
    qciSQLPlanCacheContext  sPlanCacheContext;

    *aRebuildDetected = ID_FALSE;

    if ( mSession->detectShardMetaChange() == ID_TRUE )
    {
        /* SMN 변경을 감지 했다.
         * HardPrepare 수행해서 재시도 할지 여부를 판단하자.
         * -> continue
         */
        *aRebuildDetected = ID_TRUE;
    }
    else
    {
        if ( mSession->isShardUserSession() == ID_FALSE )
        {
            if ( ( mSession->getInfo()->mShardInternalLocalOperation == SDI_INTERNAL_OP_FAILBACK ) ||
                 ( mSession->getInfo()->mShardInternalLocalOperation == SDI_INTERNAL_OP_FAILOVER ) )
            {
                /* Failback은 허용 */
            }
            else
            {
                IDE_TEST_RAISE( mSession->getShardMetaNumber() > sdi::getSMNForDataNode(),
                                InvalidSMN );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST_CONT( *aRebuildDetected == ID_FALSE, END_OF_FUNC );

    IDE_TEST( qci::clearStatement4Reprepare( getQciStmt(),
                                             getSmiStmt() )
              != IDE_SUCCESS );

    //Plan cache 에서 가져온 PCB 를 해제한다. (unfix)
    if(mPCB != NULL)
    {
        releasePlanCacheObject();
        IDU_FIT_POINT( "mmcStatement::reprepare::SLEEP::AFTER::releasePlanCacheObject" );
    }

    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;

    IDE_TEST_RAISE(hardPrepare((mmcPCB*)NULL,
                               &sPlanCacheContext) !=  IDE_SUCCESS, ErrorHardPrepare);

    IDE_EXCEPTION_CONT( END_OF_FUNC );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ErrorHardPrepare);
    {
        getQciStmt()->flag &= ~QCI_STMT_REHARDPREPARE_EXEC_MASK;
        getQciStmt()->flag |= QCI_STMT_REHARDPREPARE_EXEC_FAILURE;

        qci::setPrivateArea(getQciStmt());

        clearStmt(MMC_STMT_BIND_NONE);
    }
    IDE_EXCEPTION( InvalidSMN )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "Shard meta change is not propagated" ) );
    }
    IDE_EXCEPTION_END;

    setQueryStartTime(0);

    if ((ideGetErrorCode() & E_ACTION_MASK) == E_ACTION_FATAL)
    {
        ideLog::logErrorMsg(IDE_SERVER_0);
        IDE_CALLBACK_FATAL("fatal error code occurred in mmcStatement::checkShardRebuildStatus()");
    }

    mSession->transBeginForGTxEndTran();

    return IDE_FAILURE;
}

IDE_RC mmcStatement::shardRetryRebuild()
{
    if ( ( getQciStmt()->flag & QCI_STMT_SHARD_RETRY_REBUILD_MASK )
         == QCI_STMT_SHARD_RETRY_REBUILD_TRUE )
    {
        /* Sharding
         *  GLOBAL_TRANSACTION_LEVEL=3(GCTx) 이면서
         *  Execute statement 의 Request SCN != 0 인 경우 (Multi node execute OR Partial statement)
         *  Rebuild 에러가 발생 시 참여 노드는 Rebuild 를 수행하지 않고
         *  Coordinator 로 에러 응답 후
         *  Coordinator 에서 shard statement retry 를 수행한다. 
         *  (여러 노드에서 수행되는 구문인 경우 참여 노드 혼자 Rebuild 할 경우 정합성을 맞출수 없다)
         *  이러한 경우 execute 재시도 (doExecute) 할때 Plan 을 rebuild 해야 한다.
         *
         * PROJ-2745
         *  참여 노드에서 Rebuild event 발생 시 GCTx 와 마찬가지로
         *  참여 노드에서 rebuild 하지 않고
         *  Coordinator 로 에러 응답 후 shard statement retry 를 수행한다.
         *
         *  참여 노드에서 rebuild 에러로 인해 
         *  shard statement retry 를 통해 재수행 되는 경우
         *  QCI_STMT_SHARD_RETRY_REBUILD_TRUE 플래그가 설정되어 있으며
         *  plan rebuild 를 수행한다.
         */
        getQciStmt()->flag &= ~QCI_STMT_SHARD_RETRY_REBUILD_MASK;
        getQciStmt()->flag |= QCI_STMT_SHARD_RETRY_REBUILD_FALSE;

        IDE_TEST( rebuild() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::closeCursor(idBool aSuccess)
{
    mmcExecutionFlag sExecutionFlag;
    mmcStmtBindState sStmtBindState;

    if (getStmtState() >= MMC_STMT_STATE_EXECUTED)
    {
        // BUG-17580
        IDE_TEST(endFetch(MMC_RESULTSET_ALL) != IDE_SUCCESS);
    }

    if (isStmtBegin() == ID_TRUE)
    {
        if (aSuccess == ID_TRUE)
        {
            sExecutionFlag = MMC_EXECUTION_FLAG_SUCCESS;
            sStmtBindState = MMC_STMT_BIND_NONE;
        }
        else
        {
            sExecutionFlag = MMC_EXECUTION_FLAG_FAILURE;
            sStmtBindState = MMC_STMT_BIND_INFO;
        }

        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditByAccess( this, sExecutionFlag );
        
        clearStmt(sStmtBindState);

        IDE_TEST(endStmt(sExecutionFlag) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::beginStmt()
{
    ULong *sEventFlag = mSession->getEventFlag();

    // BUG-21204 : V$SESSTAT의 accumTime을 초기화
    idvManager::initBeginStmtAccumTime( getStatistics() );
    
    setQueryStartTime(mmtSessionManager::getBaseTime());

    /* BUG-38472 Query timeout applies to one statement. */
    setTimeoutEventOccured( ID_FALSE );

    IDU_SESSION_CLR_CANCELED(*sEventFlag); /* PROJ-2177 User Interface - Cancel */

    /* PROJ-2733 Statement SCN Sync (statement begin) */
    IDE_TEST( updateSystemSCN() != IDE_SUCCESS );

    IDE_TEST(mBeginFunc[qciMisc::getStmtTypeNumber(mStmtType)](this) != IDE_SUCCESS);

    //==========================================================
    // bug-23991: prepared 후 execute 마다 PVO time 값이 그대로 남음
    // TIMED_STATISTICS 속성값이 1(on)인 경우만 수행.
    // time 값들을 계산하기 전에 time 항목들을 0으로 초기화
    if (iduProperty::getTimedStatistics() == IDV_TIMED_STATISTICS_ON)
    {
        clearStatisticsTime();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ((ideGetErrorCode() & E_ACTION_MASK) == E_ACTION_FATAL)
        {
            ideLog::logErrorMsg(IDE_SERVER_0);
            IDE_CALLBACK_FATAL("fatal error occurred in mmcStatement::beginStmt()");
        }

        setQueryStartTime(0);
    }

    return IDE_FAILURE;
}

//==============================================================
// bug-23991: prepared 후 execute 마다 PVO time 값이 그대로 남음
// 다음 함수는 beginStmt(execute 시작시점) 에서 호출되며
// TIMED_STATISTICS 속성값이 1 (on)인 경우만 수행이 된다
// 시간 통계정보를 구하기 전에 PVO time 값을 초기화시키는 용도임
// 깔끔하게 하기 위해 idvSQL에서 time 관련 변수들을 전부 초기화 시킨다
// time 관련 항목: parse, validate, optimize, execute, fetch, soft_prepare
// ex)
// 1. prepared: PVO time 계산
// 2. execute : execute time 계산. statement 종료시 PVO + execute 전부 출력
// 3. execute : execute time 게산. statement 종료시 PVO time은 0이어야 함
//==============================================================
IDE_RC mmcStatement::clearStatisticsTime()
{
    int     sOpIdx;
    int     sIdxStart, sIdxEnd;
    idBool  sPlanCacheReplace = ID_FALSE;

    idvSQL* sStatSQL = getStatistics();

    // 혹시 NULL인 경우에도 그냥 무시(caller가 return값 검사 안함)
    IDE_TEST(sStatSQL == NULL);

    // 바로 이전에 prepared가 수행되어 PVO time이 존재하는 경우
    // PVO time을 clear하지 않고 execute, fetch time만 clear
    if (getPreparedTimeExist() == ID_TRUE)
    {
        sIdxStart = IDV_OPTM_INDEX_QUERY_EXECUTE;       // 3
        sIdxEnd   = IDV_OPTM_INDEX_QUERY_FETCH;         // 4
        setPreparedTimeExist(ID_FALSE);
    }
    // 바로 이전에 prepared가 수행되지 않고 바로 execute가
    // 수행된 경우 모든 time 항목을 초기화
    else
    {
        sIdxStart = IDV_OPTM_INDEX_QUERY_PARSE;         // 0
        sIdxEnd   = IDV_OPTM_INDEX_QUERY_SOFT_PREPARE;  // 5
        sPlanCacheReplace = ID_TRUE;
    }

    // time array index order (0 ~ 5)
    // parse, validate, optimize, execute, fetch, soft_prepare
    for (sOpIdx = sIdxStart; sOpIdx <= sIdxEnd; sOpIdx++)
    {
        idlOS::memset(&(sStatSQL->mOpTime[sOpIdx]), 0, ID_SIZEOF(idvTimeBox));
    }

    if (sPlanCacheReplace == ID_TRUE)
    {
        idlOS::memset(&(sStatSQL->mOpTime[IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE]),
                      0,
                      ID_SIZEOF(idvTimeBox));
        idlOS::memset(&(sStatSQL->mOpTime[IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE]),
                      0,
                      ID_SIZEOF(idvTimeBox));
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


// fix BUG-30990
// clearStmt()시 bind 상태를 명확하게 설정한다.
IDE_RC mmcStatement::clearStmt(mmcStmtBindState aBindState)
{
    mmcAtomicInfo   *sAtomicInfo = getAtomicInfo();

    // Atomic 관련 속성을 초기화 시킨다.
    setAtomicExecSuccess(ID_TRUE);
    setAtomic(ID_FALSE);

    qci::atomicFinalize(getQciStmt(),
                        &(sAtomicInfo->mIsCursorOpen),
                        getSmiStmt());

    // PROJ-1386 Dynamic-SQL
    IDE_TEST( freeChildStmt( (aBindState == MMC_STMT_BIND_NONE) ? ID_TRUE : ID_FALSE,
                             ID_FALSE )
              != IDE_SUCCESS );

    if ((aBindState == MMC_STMT_BIND_NONE) &&
        (getStmtExecMode() == MMC_STMT_EXEC_DIRECT) &&
        (isArray() != ID_TRUE))
    {
        setStmtState(MMC_STMT_STATE_ALLOC);

        IDE_TEST(qci::closeCursor(getQciStmt(),
                                  getSmiStmt()) != IDE_SUCCESS);

        IDE_TEST(qci::clearStatement(getQciStmt(),
                                     getSmiStmt(),
                                     QCI_STMT_STATE_INITIALIZED) != IDE_SUCCESS);
        /* BUG-22826 PLAN CACHE를 unfix 하는 시점이 잘못되었습니다.
           direct execute는 clear statement시마다 unfix를 수행한다. */
        if(mPCB != NULL)
        {

            //fix BUG-24364 valgrind direct execute으로 수행시킨 statement의  plan cache정보를 reset시켜야함.
            releasePlanCacheObject();
        }

        setBindState(MMC_STMT_BIND_NONE);

        // BUG-17544
        setSendBindColumnInfo(ID_FALSE);
    }
    else
    {
        setStmtState(MMC_STMT_STATE_PREPARED);

        IDE_TEST(qci::closeCursor(getQciStmt(),
                                  getSmiStmt()) != IDE_SUCCESS);

        // To Fix BUG-19842
        // SQLExecute() 과정중에 Binding 실패가 발생할 수 있으므로
        // BINDING 상태에 따라 적절히 제어하여야 함.
        switch ( mBindState )
        {
            case MMC_STMT_BIND_NONE:
                // Nothing To Do
                // MM 은 BIND_INFO 성공전 에러가 발생한 것임
                // QP는 QCI_STMT_STATE_PREPARED 상태임.
                break;
            case MMC_STMT_BIND_INFO:
                // MM 은 BIND_INFO 성공후 에러가 발생한 것임
                // QP는 QCI_STMT_STATE_PREPARED 상태임.
                IDE_TEST(qci::clearStatement(getQciStmt(),
                                             getSmiStmt(),
                                             QCI_STMT_STATE_PREPARED)
                         != IDE_SUCCESS);

                setBindState(MMC_STMT_BIND_INFO);
                break;
            case MMC_STMT_BIND_DATA:
                // fix BUG-30990
                // DEQUEUE 대기시 Bind된 Data는 유지하도록 함
                if (aBindState == MMC_STMT_BIND_DATA)
                {
                    IDE_TEST(qci::clearStatement(getQciStmt(),
                                                 getSmiStmt(),
                                                 QCI_STMT_STATE_PARAM_DATA_BOUND)
                             != IDE_SUCCESS);

                    setBindState(MMC_STMT_BIND_DATA);
                }
                else
                {
                    IDE_TEST(qci::clearStatement(getQciStmt(),
                                                 getSmiStmt(),
                                                 QCI_STMT_STATE_PREPARED)
                             != IDE_SUCCESS);

                    setBindState(MMC_STMT_BIND_INFO);
                }
                break;
            default :
                // 잘못된 상태임.
                IDE_DASSERT(0);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcStatement::endStmt(mmcExecutionFlag aExecutionFlag)
{
    idvSQL           *sStatSQL;
    mmcSession       *sSession;
    mmcSessionInfo   *sSessInfo;
    UInt              sTransID; // autocommit일때 TxID가 사라짐. so, keep it.
    ULong            *sEventFlag;
    idvProfStmtInfo   sProfInfo;
    idBool            sSuccess;
    UShort            sResultSetCount;
    mmcFetchFlag      sFetchFlag = MMC_FETCH_FLAG_NO_RESULTSET;

    if (aExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS)
    {
        sSuccess = ID_TRUE;
    }
    else
    {
        sSuccess = ID_FALSE;
    }

    sStatSQL   = getStatistics();
    sSession   = getSession();
    sSessInfo  = sSession->getInfo();
    sTransID   = getTransID();

    sEventFlag = mSession->getEventFlag();

    sResultSetCount = getResultSetCount();

    setQueryStartTime(0);
    
    IDU_SESSION_SET_BLOCK(*sEventFlag);
    /* BUG-47244 if use limit syntax in non-auto commit mode then it occurs session hang.
     * to prevent self statement local loop back deadlock
     */
    if ( sdi::isShardEnable() == ID_TRUE )
    {
        freeAllRemoteStatement( CMP_DB_FREE_CLOSE );
    }

    IDE_TEST(mEndFunc[qciMisc::getStmtTypeNumber(mStmtType)](this, sSuccess)
             != IDE_SUCCESS);

    /* BUG-47238 */
    mInfo.mFlag &= ~MMC_STMT_NEED_UNLOCK_MASK;
    mInfo.mFlag |= MMC_STMT_NEED_UNLOCK_FALSE;

    if (sResultSetCount > 0)
    {
        if (getFetchFlag() == MMC_FETCH_FLAG_CLOSE)
        {
            IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                                IDV_STAT_INDEX_FETCH_SUCCESS_COUNT, 1);
            IDV_SQL_ADD_DIRECT(getStatistics(), mFetchSuccessCount, 1);
        }
        else
        {
            IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                                IDV_STAT_INDEX_FETCH_FAILURE_COUNT, 1);
            IDV_SQL_ADD_DIRECT(getStatistics(), mFetchFailureCount, 1);
        }
    }

    /* BUG-38472 Query timeout applies to one statement. */
    setTimeoutEventOccured( ID_FALSE );

    IDU_SESSION_CLR_BLOCK(*sEventFlag);
    IDU_SESSION_CLR_CANCELED(*sEventFlag); /* PROJ-2177 User Interface - Cancel */

    // Stmt에서 측정해야할 경과시간을 모두 계산하여 저장하고,
    // 이를 Session에 반영한다.
 
    // 문제점: Array Execute 수행시 Stmt 성능저하를 가져올 수 있다.
    applyOpTimeToSession();

    if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_STMT_FLAG) 
        == IDV_PROF_TYPE_STMT_FLAG)
    {
        if (sResultSetCount == 0)
        {
            sFetchFlag = MMC_FETCH_FLAG_NO_RESULTSET;
        }
        else
        {
            sFetchFlag = getFetchFlag();
        }

        sProfInfo.mExecutionFlag = (UInt)aExecutionFlag;
        sProfInfo.mFetchFlag     = (UInt)sFetchFlag;
        sProfInfo.mSID           = getSessionID();
        sProfInfo.mSSID          = getStmtID();
        sProfInfo.mTxID          = sTransID;
        sProfInfo.mUserID        = sSessInfo->mUserInfo.userID;
        sProfInfo.mClientPID     = sSessInfo->mClientPID;
        sProfInfo.mXaSessionFlag = sSessInfo->mXaSessionFlag;


        idlOS::memcpy(sProfInfo.mClientType,
                sSessInfo->mClientType,
                IDV_PROF_MAX_CLIENT_TYPE_LEN);
        sProfInfo.mClientType[IDV_PROF_MAX_CLIENT_TYPE_LEN] = '\0';

        idlOS::memcpy(sProfInfo.mClientAppInfo,
                sSessInfo->mClientAppInfo,
                IDV_PROF_MAX_CLIENT_APP_LEN);
        sProfInfo.mClientAppInfo[IDV_PROF_MAX_CLIENT_APP_LEN] = '\0';

        IDE_ASSERT( sStatSQL != NULL );

        // applyStmtOetStat.. 에서 계산되어진 경과시간값을 프로파일링한다.
        sProfInfo.mParseTime    =
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL,
                        IDV_OPTM_INDEX_QUERY_PARSE));

        sProfInfo.mValidateTime = 
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL,
                        IDV_OPTM_INDEX_QUERY_VALIDATE));

        sProfInfo.mOptimizeTime =
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL,
                        IDV_OPTM_INDEX_QUERY_OPTIMIZE));

        sProfInfo.mExecuteTime  =
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL,
                        IDV_OPTM_INDEX_QUERY_EXECUTE));

        sProfInfo.mFetchTime    =
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL,
                        IDV_OPTM_INDEX_QUERY_FETCH));
        
        sProfInfo.mSoftPrepareTime    = 
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL,
                        IDV_OPTM_INDEX_QUERY_SOFT_PREPARE));
        
        // BUG-21751
        sProfInfo.mTotalTime =
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(sStatSQL,
                  IDV_OPTM_INDEX_QUERY_PARSE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(sStatSQL,
                  IDV_OPTM_INDEX_QUERY_VALIDATE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(sStatSQL,
                  IDV_OPTM_INDEX_QUERY_OPTIMIZE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(sStatSQL,
                  IDV_OPTM_INDEX_QUERY_EXECUTE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(sStatSQL,
                  IDV_OPTM_INDEX_QUERY_FETCH)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(sStatSQL,
                  IDV_OPTM_INDEX_QUERY_SOFT_PREPARE));

        idvProfile::writeStatement(sStatSQL,
                &sProfInfo,
                getQueryString(),
                getQueryLen());
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        /* BUG-38472 Query timeout applies to one statement. */
        setTimeoutEventOccured( ID_FALSE );

        IDU_SESSION_CLR_BLOCK(*sEventFlag);
    }

    return IDE_FAILURE;
}

IDE_RC mmcStatement::beginFetch(UShort aResultSetID)
{
    if( isRootStmt() == ID_TRUE )
    {
        /* PROJ-1381 FAC : FetchList를 바꿀 때도 lock으로 보호 */
        mSession->lockForFetchList();
        IDU_LIST_ADD_LAST(mSession->getFetchList(), getFetchListNode());
        mSession->unlockForFetchList();
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST(setResultSetState(aResultSetID, MMC_RESULTSET_STATE_FETCH_READY) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::endFetch(UShort aResultSetID)
{
    UShort sID;
    UShort sResultSetCount;

    if( isRootStmt() == ID_TRUE )
    {
        /* PROJ-1381 FAC : FetchList를 바꿀 때도 lock으로 보호 */
        mSession->lockForFetchList();
        IDU_LIST_REMOVE(getFetchListNode());
        mSession->unlockForFetchList();
    }
    else
    {
        // Nothing to do.
    }
    
    setFetchStartTime(0);
    /* BUG-19456 */
    setFetchEndTime(0);

    IDV_SQL_OPTIME_END( getStatistics(),
                        IDV_OPTM_INDEX_QUERY_FETCH );

    sResultSetCount = getResultSetCount();

    if (isSimpleQuerySelectExecuted() == ID_TRUE)
    {
        setExecuteFlag(ID_FALSE);
    }
    else
    {
        if (aResultSetID == MMC_RESULTSET_ALL)
        {
            // BUG-17580
            for (sID = 0; sID < sResultSetCount; sID++)
            {
                IDE_TEST(setResultSetState(sID, MMC_RESULTSET_STATE_FETCH_CLOSE) != IDE_SUCCESS);
            }
            // fix BUG-21968
            setExecuteFlag(ID_FALSE);
        }
        else
        {
            IDE_TEST(setResultSetState(aResultSetID, MMC_RESULTSET_STATE_FETCH_CLOSE) != IDE_SUCCESS);

            if (aResultSetID == sResultSetCount - 1)
            {
                // fix BUG-21968
                setExecuteFlag(ID_FALSE);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::parse(SChar* aSQLText)
{
    idBool isFixedTable;
    idBool sLogQuery = ID_FALSE;
    SChar  sBuf[IDL_IP_ADDR_MAX_LEN];
    
    IDE_TEST(qci::parse(getQciStmt(), aSQLText, getQueryLen()) != IDE_SUCCESS);


    
    IDE_TEST(qci::getStmtType(getQciStmt(), &mStmtType) != IDE_SUCCESS);

    if( isRootStmt() == ID_FALSE )
    {
        IDE_TEST( qci::checkInternalSqlType(mStmtType)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    if (mmm::getCurrentPhase() != MMM_STARTUP_SERVICE)
    {
        switch (qciMisc::getStmtType(mStmtType))
        {
            case QCI_STMT_MASK_DDL:
            case QCI_STMT_MASK_SP:
                IDE_RAISE( InvalidServerPhaseError );
                break;

            case QCI_STMT_MASK_DML:
                /*
                 * BUG-11031: Fixed Table이나 Performance View 에 대한 SELECT 만 허용
                 */
                if (mStmtType == QCI_STMT_SELECT)
                {
                    IDE_TEST(qci::hasFixedTableView(getQciStmt(), &isFixedTable) != IDE_SUCCESS);
                    IDE_TEST_RAISE( isFixedTable == ID_FALSE, InvalidServerPhaseError );
                }
                else
                {
                    /* BUG-44387 SERVICE 이전 단계에서 INSERT, DELETE, UPDATE 구문 실행시 FATAL */
                    IDE_RAISE( InvalidServerPhaseError );
                }
                break;

            case QCI_STMT_MASK_DCL:
            case QCI_STMT_MASK_DB:
                break;

            default:
                break;
        }
    }
    else
    {
        /* BUG-20832 */
        /* service 가 시작된 후에는 xa session 이라면 DCL, DDL 수행을 방지해야 한다. */
        switch (qciMisc::getStmtType(mStmtType))
        {
            case QCI_STMT_MASK_DCL:
                break;

            case QCI_STMT_MASK_DDL:
            case QCI_STMT_MASK_DB:
                //fix BUG-20850
                IDE_TEST_RAISE(getSession()->getXaAssocState() == MMD_XA_ASSOC_STATE_ASSOCIATED, DDLNotAllowedError);
                break;

            case QCI_STMT_MASK_SP:
            case QCI_STMT_MASK_DML:
            default:
                break;
        }

        switch( mmuProperty::getQueryLoggingLevel() )
        {
            case 0:
                break;
            case 1:
                if( mStmtType == QCI_STMT_SELECT )
                {
                    sLogQuery = ID_TRUE;
                }
                break;
            case 2:
                if( ( qciMisc::getStmtType(mStmtType) == QCI_STMT_MASK_DML ) ||
                    ( qciMisc::getStmtType(mStmtType) == QCI_STMT_MASK_SP ) )
                {
                    sLogQuery = ID_TRUE;
                }
                break;
            case 3:
                sLogQuery = ID_TRUE;
                break;
                
            default:
                break;
        }

        if( sLogQuery == ID_TRUE )
        {
            (void)cmiGetLinkInfo(getSession()->getTask()->getLink(),
                                 sBuf,
                                 ID_SIZEOF(sBuf),
                                 CMI_LINK_INFO_ALL);
            ideLog::log(IDE_QP_0, "THR ID:%d, %s\nQUERY:%s\n",
                        (UInt)idlOS::getThreadID(),
                        sBuf,
                        getQueryString());
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidServerPhaseError );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SERVER_PHASE_MISMATCHES_QUERY_TYPE ) );
    }
    IDE_EXCEPTION(DDLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DDL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::doHardPrepare(SChar                   *aSQLText,             
                                   qciSQLPlanCacheContext  *aPlanCacheContext,
                                   smiStatement            *aSmiStmt)
{
    UInt sStmtRetryMax    = mSession->getShardStatementRetry();
    UInt sRebuildRetryMax = SDI_SHARD_RETRY_LOOP_MAX;
    smiDistTxInfo sDistTxInfo;

    /* BUG-37806 */
#ifdef DEBUG
    getQciStmt()->flag &= ~QCI_STMT_AUDIT_MASK;
    getQciStmt()->flag |= QCI_STMT_AUDIT_TRUE;
#else
    /* release mode */
    if ( mmtAuditManager::isAuditStarted() == ID_TRUE ) 
    {
        getQciStmt()->flag &= ~QCI_STMT_AUDIT_MASK;
        getQciStmt()->flag |= QCI_STMT_AUDIT_TRUE;
    }    
    else 
    {    
        getQciStmt()->flag &= ~QCI_STMT_AUDIT_MASK;
        getQciStmt()->flag |= QCI_STMT_AUDIT_FALSE;
    }    
#endif

    getQciStmt()->flag &= ~QCI_STMT_SHARD_RETRY_REBUILD_MASK;
    getQciStmt()->flag |= QCI_STMT_SHARD_RETRY_REBUILD_FALSE;

    /* BUG-48282 : Shard TX의 HARD PREPARE시에도 분산정보가 설정되어야 한다.  */
    buildSmiDistTxInfo( &sDistTxInfo );

    while (qci::hardPrepare(getQciStmt(),
                            aSmiStmt,
                            aPlanCacheContext,
                            &sDistTxInfo) != IDE_SUCCESS)
    {
        IDE_TEST( mSession->processShardRetryError( this,
                                                    &sStmtRetryMax,
                                                    &sRebuildRetryMax )
                  != IDE_SUCCESS );

        IDE_TEST(ideIsRebuild() != IDE_SUCCESS);

        /* BUG-40170 */
        IDE_TEST(qci::clearStatement4Reprepare(getQciStmt(),
                                               aSmiStmt) != IDE_SUCCESS);

        IDE_TEST(parse(aSQLText) != IDE_SUCCESS);

        // PROJ-2163
        IDE_TEST(qci::bindParamInfo(getQciStmt(), aPlanCacheContext) != IDE_SUCCESS);

        IDV_SESS_ADD_DIRECT(mSession->getStatistics(), IDV_STAT_INDEX_REBUILD_COUNT, 1);
    }

    /* BUG-42639 Monitoring query */
    IDE_TEST(qci::getStmtType(getQciStmt(), &mStmtType) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcStatement::beginSmiStmtInternal(mmcTransObj *aTrans, UInt aFlag, UChar *aSessionType)
{
    smSCN            sSCN;
    smiDistTxInfo    sDistTxInfo;
    smiDistTxInfo  * sDistTxInfoPtr = NULL;

    /* TASK-7219 Non-shard DML */
    idBool           sIsShardPartialExecStmt = ID_FALSE;
    UInt             sStmtExecSeq = 0;

    SM_INIT_SCN( &sSCN ); 

    ACP_UNUSED( aSessionType );

    if ( ( mSession->isGCTx() == ID_TRUE ) &&
         ( isNeedRequestSCN() == ID_TRUE ) )
    {
        getRequestSCN( &sSCN );
        aFlag |= SMI_STATEMENT_VIEWSCN_REQUESTED;
    }

    buildSmiDistTxInfo( &sDistTxInfo );

    if ( SMI_DIST_LEVEL_IS_VALID( sDistTxInfo.mDistLevel ) )
    {
        #if defined(DEBUG)
        /* PROJ-2733-DistTxInfo DistTxInfo 전달 */
        ideLog::log( IDE_SD_18,
                     "= [%s] beginSmiStmtInternal"
                     ", VIEWSCN_REQUESTED : %s"
                     ", SCN : %"ID_UINT64_FMT
                     ", TxFirstStmtViewSCN : %"ID_UINT64_FMT
                     ", TxFirstStmtTime : [%"ID_UINT64_FMT"][%"ID_UINT64_FMT"]"
                     ", DistLevel : %"ID_UINT32_FMT
                     "\nQuery : %s",
                     aSessionType,
                     SMI_STATEMENT_VIEWSCN_IS_REQUESTED(aFlag) ? "True" : "False",
                     sSCN,
                     sDistTxInfo.mFirstStmtViewSCN,
                     sDistTxInfo.mFirstStmtTime.tv_.tv_sec, sDistTxInfo.mFirstStmtTime.tv_.tv_usec,
                     sDistTxInfo.mDistLevel,
                     mInfo.mQueryString );
        #endif

        sDistTxInfoPtr = &sDistTxInfo;
    }
    else
    {
        sDistTxInfoPtr = NULL;
    }

    /* TASK-7219 Non-shard DML
     * SM statement의 cursor SCN 관리를 위해
     * Partial execution query 여부와 execution exquence를 전달한다.
     */
    if ( SDU_SHARD_ENABLE == 1 )
    {
        sIsShardPartialExecStmt = ( mInfo.mShardPartialExecType !=
                                    SDI_SHARD_PARTIAL_EXEC_TYPE_NONE ) ? ID_TRUE : ID_FALSE;
        sStmtExecSeq = mSession->getStmtExecSeqForShardTx();
    }

    if( isRootStmt() == ID_TRUE )
    {
        /* PROJ-2446 */
        IDE_TEST( mSmiStmt.begin( getStatistics(),
                                  mmcTrans::getSmiStatement(aTrans),
                                  ( getCursorFlag() | aFlag ),
                                  sSCN,
                                  sDistTxInfoPtr,
                                  sIsShardPartialExecStmt,
                                  sStmtExecSeq )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2446 */
        IDE_TEST( mSmiStmt.begin( getStatistics(),
                                  mInfo.mParentStmt->getSmiStmt(),
                                  ( getCursorFlag() | aFlag ),
                                  sSCN,
                                  sDistTxInfoPtr,
                                  sIsShardPartialExecStmt,
                                  sStmtExecSeq )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-48115
   smiDistTxInfo 구조체에 분산정보를 기록한다. */
void mmcStatement::buildSmiDistTxInfo( smiDistTxInfo * aDistTxInfo )
{
    smSCN            sTxFirstStmtSCN;
    PDL_Time_Value   sTxFirstStmtTime;
    sTxFirstStmtTime.initialize(0);
    smiDistTxInfo    sDistTxInfo = { SM_SCN_INIT,
                                     sTxFirstStmtTime,
                                     SMI_SHARD_PIN_INVALID,
                                     SMI_DIST_LEVEL_INIT };

    if ( mSession->isShardTrans() == ID_TRUE ) 
    {
        if ( getDistLevel() > SDI_DIST_LEVEL_INIT )
        {
            getTxFirstStmtSCN( &sTxFirstStmtSCN );
            sTxFirstStmtTime.msec( getTxFirstStmtTime() );

            if ( SM_SCN_IS_NOT_INIT( sTxFirstStmtSCN ) &&
                 SM_SCN_IS_NOT_NON_GCTX_TX_FIRST_STMT_SCN( sTxFirstStmtSCN ) )
            {
                SM_SET_SCN_VIEW_BIT(&sTxFirstStmtSCN); /* to ViewSCN */
            }

            /* set DistTxInfo */
            SMI_SET_SMI_DIST_TX_INFO( &sDistTxInfo,
                                      sTxFirstStmtSCN,
                                      sTxFirstStmtTime,
                                      mSession->getShardPIN(),
                                      (smiDistLevel)getDistLevel() );
        }
        else 
        {
            if ( mSession->isShardUserSession() == ID_FALSE )
            {
                /* SHARD TX인데 분산정보가 없다면 Dummy값을 넣어준다.
                   (예) TX OPEN후 첫 HARD PREPARE 시

                   < DUMMY >
                   mFisrtStmtViewSCN : 0 (SM_SCN_INIT)
                   mFirstStmtTime    : 현재시간
                   mShardPin         : SHARD-PIN
                   mDistLevel        : SMI_DIST_LEVEL_SINGLE
                 */
                SMI_SET_SMI_DIST_TX_INFO( &sDistTxInfo,
                                          SM_SCN_INIT,            /* mFirstStmtViewSCN */
                                          idlOS::gettimeofday(),  /* mFirstStmtTime */
                                          mSession->getShardPIN(),/* mShardPin */
                                          SMI_DIST_LEVEL_SINGLE );/* mDistLevel */

                #ifdef DEBUG
                SChar sPinStr[SDI_MAX_SHARD_PIN_STR_LEN + 1]; 
                sdi::shardPinToString( sPinStr,
                                       ID_SIZEOF( sPinStr ),
                                       sDistTxInfo.mShardPin );

                ideLog::log( IDE_SD_18,
                             "Dummy Distributed Info"
                             ", TxFirstStmtViewSCN : %"ID_UINT64_FMT
                             ", TxFirstStmtTime : [%"ID_UINT64_FMT"][%"ID_UINT64_FMT"]"
                             ", DistLevel : %"ID_UINT32_FMT
                             ", Shard-PIN : %s",
                             sDistTxInfo.mFirstStmtViewSCN,
                             sDistTxInfo.mFirstStmtTime.tv_.tv_sec, sDistTxInfo.mFirstStmtTime.tv_.tv_usec,
                             sDistTxInfo.mDistLevel,
                             sPinStr );
                #endif
            }
        }
    }

    *aDistTxInfo = sDistTxInfo;
}

IDE_RC mmcStatement::beginSmiStmt(mmcTransObj *aTrans, UInt aFlag)
{
    IDE_DASSERT(mIsShareTransSmiStmtLocked == ID_FALSE);

    /* PROJ-2701 Online Data Rebuild: multi-node parallel statement serialize */
    if( mmcTrans::isShareableTrans(aTrans) == ID_TRUE )
    {
        /* for multi-node parallel statement serialize
         * this statement is executing with transaction lock acquired.
         */
        IDE_TEST( mmcTrans::fixSharedTrans4Statement( aTrans,
                                                      getSession(),
                                                      MMC_TRANS_FIX_RECURSIVE )
                  != IDE_SUCCESS );
        mIsShareTransSmiStmtLocked = ID_TRUE;

        IDE_TEST( beginSmiStmtInternal(aTrans, aFlag, mSession->getSessionTypeString()) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( beginSmiStmtInternal(aTrans, aFlag, mSession->getSessionTypeString()) != IDE_SUCCESS );
    }

    mSmiStmtPtr = &mSmiStmt;
    setTransID(mmcTrans::getTransID(aTrans));
    mExecutingTrans = aTrans;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( mIsShareTransSmiStmtLocked == ID_TRUE )
    {
        mIsShareTransSmiStmtLocked = ID_FALSE;
        mmcTrans::unfixSharedTrans( aTrans, getSessionID() );
    }

    return IDE_FAILURE;
}

void mmcStatement::setSmiStmt(smiStatement * aSmiStmt)
{
    if( isRootStmt() == ID_TRUE )
    {
        mSmiStmtPtr = aSmiStmt;
    }
    else
    {
        mSmiStmtPtr = mInfo.mParentStmt->getSmiStmt();
    }
}

IDE_RC mmcStatement::endSmiStmt(UInt aFlag)
{
    IDE_DASSERT(mExecutingTrans != NULL);

    /* PROJ-2701 Online Data Rebuild: multi-node parallel statement serialize */
    if( mmcTrans::isShareableTrans(mExecutingTrans) == ID_TRUE )
    {
        if ( mIsShareTransSmiStmtLocked == ID_FALSE )
        {
            mmcTrans::fixSharedTrans( mExecutingTrans, getSessionID() );
            mIsShareTransSmiStmtLocked = ID_TRUE;
        }

        IDE_TEST(mSmiStmt.end(aFlag) != IDE_SUCCESS);

        mIsShareTransSmiStmtLocked = ID_FALSE;
        mmcTrans::unfixSharedTrans( mExecutingTrans, getSessionID() );
    }
    else
    {
        IDE_TEST(mSmiStmt.end(aFlag) != IDE_SUCCESS);
    }

    mExecutingTrans = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( mIsShareTransSmiStmtLocked == ID_TRUE )
    {
        mIsShareTransSmiStmtLocked = ID_FALSE;
        mmcTrans::unfixSharedTrans( mExecutingTrans, getSessionID() );
    }

    return IDE_FAILURE;
}

/**
 * PROJ-2701 Online Data Rebuild
 * getShareTransForSmiStmtLock() for multi-node parallel statement serialize
 * 1. for using smiStmt at prepare
 * 2. for fetching after transaction end at fetch across commit
 * return:
 * if share transaction lock was needed then return the share transaction,
 * otherwise return NULL.
 */
mmcTransObj* mmcStatement::getShareTransForSmiStmtLock(mmcTransObj *aTrans)
{
    mmcTransObj *sTrans = NULL;

    if( aTrans != NULL )
    {
        sTrans = aTrans;
    }
    else
    {
        sTrans = mExecutingTrans;
    }

    if( sTrans != NULL )
    {
        if( mmcTrans::isShareableTrans(sTrans) == ID_TRUE )
        {
            /* Nothing to do */
        }
        else
        {
            sTrans = NULL;
        }
    }
    else
    {
        /* fixed table fetch is dcl, no need to lock share transaction, set null for skip */
    }

    return sTrans;
}

/**
 * PROJ-2701 Online Data Rebuild
 * getExecutingTrans() for multi-node parallel statement serialize
 * 1. for transaction commit(closeAllCursor) at fetch across commit
 */
mmcTransObj* mmcStatement::getExecutingTrans()
{
    return mExecutingTrans;
}

/**
 * PROJ-2701 Online Data Rebuild
 * acquireShareTransSmiStmtLock() for multi-node parallel statement serialize
 * 1. for using smiStmt at prepare
 * 2. for fetching after transaction end at fetch across commit
 */
IDE_RC mmcStatement::acquireShareTransSmiStmtLock(mmcTransObj *aTrans)
{
    IDE_TEST( mmcTrans::fixSharedTrans4Statement( aTrans,
                                                  getSession(),
                                                  MMC_TRANS_FIX_RECURSIVE )
              != IDE_SUCCESS );

    mIsShareTransSmiStmtLocked = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-2701 Online Data Rebuild
 * releaseShareTransSmiStmtLock() for multi-node parallel statement serialize
 * 1. for using smiStmt at prepare
 * 2. for fetching after transaction end at fetch across commit
 */
void mmcStatement::releaseShareTransSmiStmtLock(mmcTransObj *aTrans)
{
    if ( mIsShareTransSmiStmtLocked == ID_TRUE )
    {
        mIsShareTransSmiStmtLocked = ID_FALSE;
        mmcTrans::unfixSharedTrans( aTrans, getSessionID() );
    }
}

IDE_RC mmcStatement::resetCursorFlag()
{
    if ((qciMisc::isStmtDCL(mStmtType) != ID_TRUE) &&
        (qciMisc::isStmtSP(mStmtType)  != ID_TRUE) &&
        (qciMisc::isStmtDB(mStmtType)  != ID_TRUE))
    {
        /* Always Success */
        (void)mSmiStmt.resetCursorFlag(getCursorFlag());
    }
    return IDE_SUCCESS;
}

IDE_RC mmcStatement::initializeResultSet(UShort aResultSetCount)
{
    UShort sResultSetID;

    if (mInfo.mResultSetHWM < aResultSetCount)
    {
        if (mInfo.mResultSet == NULL)
        {
            IDU_FIT_POINT_RAISE( "mmcStatement::initializeResultSet::malloc::ResultSet",
                                 InsufficientMemory );

            IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMM,
                                             ID_SIZEOF(mmcResultSet) * aResultSetCount,
                                             (void **)&(mInfo.mResultSet),
                                             IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );
        }
        else
        {
            IDU_FIT_POINT_RAISE( "mmcStatement::initializeResultSet::realloc::ResultSet",
                                 InsufficientMemory );

            IDE_TEST_RAISE(iduMemMgr::realloc(IDU_MEM_CMM,
                                        ID_SIZEOF(mmcResultSet) * aResultSetCount,
                                        (void **)&(mInfo.mResultSet),
                                        IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );
        }

        // PROJ-2256
        for ( sResultSetID = mInfo.mResultSetHWM; sResultSetID < aResultSetCount; sResultSetID++ )
        {
            initBaseRow( &( mInfo.mResultSet[sResultSetID].mBaseRow ) );
        }

        mInfo.mResultSetHWM = aResultSetCount;
    }

    for ( sResultSetID = 0; sResultSetID < aResultSetCount; sResultSetID++ )
    {
        IDE_TEST( qci::getResultSet( getQciStmt(),
                                     sResultSetID,
                                     &mInfo.mResultSet[sResultSetID].mResultSetStmt,
                                     &mInfo.mResultSet[sResultSetID].mInterResultSet,
                                     &mInfo.mResultSet[sResultSetID].mRecordExist )
                  != IDE_SUCCESS );

        if ( mInfo.mResultSet[sResultSetID].mRecordExist == ID_TRUE )
        {
            mInfo.mResultSet[sResultSetID].mResultSetState = MMC_RESULTSET_STATE_FETCH_READY;

            // PROJ-2256
            mInfo.mResultSet[sResultSetID].mBaseRow.mIsFirstRow = ID_TRUE;
        }
        else
        {
            mInfo.mResultSet[sResultSetID].mResultSetState = MMC_RESULTSET_STATE_INITIALIZE;
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mmcStatement::freeChildStmt( idBool aSuccess,
                                    idBool aIsFree )
{
    mmcStatement * sStmt;
    iduListNode  * sIterator;
    iduListNode  * sNodeNext;
    UShort sResultSetID;

    IDU_LIST_ITERATE_SAFE(getChildStmtList(), sIterator, sNodeNext)
    {
        sStmt = (mmcStatement *)sIterator->mObj;

        IDE_TEST(sStmt->closeCursor(aSuccess) != IDE_SUCCESS);

        // BUG-36203 PSM Optimize
        if( aIsFree == ID_TRUE )
        {
            IDE_TEST(mmcStatementManager::freeStatement(sStmt) != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-20756 
    // ChildStmtList 에 ResultSet 도 포함되므로 ResultSet도 초기화 시켜야 합니다.
    if( mInfo.mResultSet != NULL)
    {
        /* BUG-42903 한계값 보정 */
        for (sResultSetID = 0; sResultSetID < IDL_MIN(mInfo.mResultSetCount, mInfo.mResultSetHWM); sResultSetID++)
        {
            mInfo.mResultSet[sResultSetID].mResultSetState = MMC_RESULTSET_STATE_INITIALIZE;
            mInfo.mResultSet[sResultSetID].mResultSetStmt  = NULL;
            mInfo.mResultSet[sResultSetID].mRecordExist    = ID_FALSE;
            mInfo.mResultSet[sResultSetID].mInterResultSet = ID_FALSE;
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* BUG-46090 Meta Node SMN 전파 */
void mmcStatement::clearShardDataInfo()
{
    mmcStatement * sStmt     = NULL;
    iduListNode  * sIterator = NULL;

    IDU_LIST_ITERATE( getChildStmtList(), sIterator )
    {
        sStmt = (mmcStatement *)sIterator->mObj;

        sStmt->clearShardDataInfo();
    }

    qci::clearShardDataInfo( getQciStmt() );
}

void mmcStatement::clearShardDataInfoForRebuild()
{
    mmcStatement * sStmt     = NULL;
    iduListNode  * sIterator = NULL;

    IDU_LIST_ITERATE( getChildStmtList(), sIterator )
    {
        sStmt = (mmcStatement *)sIterator->mObj;

        sStmt->clearShardDataInfoForRebuild();
    }

    qci::clearShardDataInfoForRebuild( getQciStmt() );
}

/* BUG-46092 */
void mmcStatement::freeAllRemoteStatement( UChar aMode )
{
    mmcStatement * sStmt     = NULL;
    iduListNode  * sIterator = NULL;

    IDU_LIST_ITERATE( getChildStmtList(), sIterator )
    {
        sStmt = (mmcStatement *)sIterator->mObj;

        sStmt->freeAllRemoteStatement( aMode );
    }

    sdiStatementManager::freeAllRemoteStatement( &mSdStmt,
                                                 mSession->getQciSession()->mQPSpecific.mClientInfo,
                                                 aMode );
}

/* BUG-46092 */
void mmcStatement::freeRemoteStatement( UInt aNodeId, UChar aMode )
{
    mmcStatement * sStmt     = NULL;
    iduListNode  * sIterator = NULL;

    IDU_LIST_ITERATE( getChildStmtList(), sIterator )
    {
        sStmt = (mmcStatement *)sIterator->mObj;

        sStmt->freeRemoteStatement( aNodeId, aMode );
    }

    sdiStatementManager::freeRemoteStatement( &mSdStmt,
                                              mSession->getQciSession()->mQPSpecific.mClientInfo,
                                              aNodeId,
                                              aMode );
}

void mmcStatement::applyOpTimeToSession()
{
   /* statistics event 관련 통계정보만 업뎃한다.
    * 왜냐하면, wait event 관련 통계정보는 측정직후에
    * 바로 Session에 반영되기 때문이다. */
   idvManager::applyOpTimeToSession( 
       getSession()->getStatistics(),     // idvSession *
       getStatistics());                  // idvSQL *
}
//PROJ-1436 SQL-Plan Cache.
IDE_RC  mmcStatement::getSmiStatement4PrepareCallback(void          *aGetSmiStmt4PrepareContext,
                                                      smiStatement  **aSmiStatement4Prepare)
{
    mmcGetSmiStmt4PrepareContext   *sGetSmiStmt4PrepareContext = (mmcGetSmiStmt4PrepareContext*)aGetSmiStmt4PrepareContext;
    smiStatement *sRootStmt;
    mmcStatement * sStmt = NULL;
    idBool         sIsBegin = ID_FALSE;


    if(sGetSmiStmt4PrepareContext->mSoftPrepareReason  ==  MMC_SOFT_PREPARE_FOR_PREPARE)
    {
        if(sGetSmiStmt4PrepareContext->mTrans == NULL)
        {    
            IDE_TEST( sGetSmiStmt4PrepareContext->mStatement->beginTransAndLock4Prepare(
                            &(sGetSmiStmt4PrepareContext->mTrans),
                            &(sGetSmiStmt4PrepareContext->mEndTransAction) )
                      != IDE_SUCCESS );
            sIsBegin = ID_TRUE;

            sGetSmiStmt4PrepareContext->mNeedCommit = ID_TRUE;
            sRootStmt = mmcTrans::getSmiStatement(sGetSmiStmt4PrepareContext->mTrans);

            IDE_TEST( sGetSmiStmt4PrepareContext->mPrepareStmt.begin( NULL,
                                                                      sRootStmt, 
                                                                      SMI_STATEMENT_UNTOUCHABLE | 
                                                                      SMI_STATEMENT_MEMORY_CURSOR ) 
                      != IDE_SUCCESS );
        }
        else
        {
            //nothing to do.
        }
    
        *aSmiStatement4Prepare =  &(sGetSmiStmt4PrepareContext->mPrepareStmt);
    }
    else
    {
        //rebuild
        IDE_DASSERT(sGetSmiStmt4PrepareContext->mSoftPrepareReason == MMC_SOFT_PREPARE_FOR_REBUILD);
        *aSmiStatement4Prepare =  sGetSmiStmt4PrepareContext->mStatement->getSmiStmt();
    }
    
 
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sIsBegin == ID_TRUE )
    {
        sStmt = sGetSmiStmt4PrepareContext->mStatement;

        IDE_ASSERT( sStmt->endTransAndUnlock4Prepare( sGetSmiStmt4PrepareContext->mTrans,
                                                      &sGetSmiStmt4PrepareContext->mEndTransAction,
                                                      MMC_END_TRANS_FAILURE )
                    == IDE_SUCCESS );
        sIsBegin = ID_FALSE;
        sGetSmiStmt4PrepareContext->mNeedCommit = ID_FALSE;
    }

    return IDE_FAILURE;
}

//PROJ-1436 SQL-Plan Cache.
IDE_RC mmcStatement::softPrepare(mmcParentPCO          *aParentPCO,
                                 mmcSoftPrepareReason  aSoftPrepareReason,
                                 mmcPCB                **aPCB,
                                 idBool                *aNeedHardPrepare)
{
    vULong                       sHashKeyVal;
    mmcParentPCO                *sParentPCO;
    mmcChildPCO                 *sChildPCO;
    mmcPCB                      *sPlanPCB;
    mmcPCB                      *sPCB4NewPlan;
    idBool                       sValidPlan;
    mmcChildPCOPlanState         sChildPCOPlanState;
    idBool                       sCacheAbleSQLText;
    idBool                       sSuccess;
    mmcGetSmiStmt4PrepareContext sGetSmit4PrepareContext;
    mmcPCOLatchState             sPCOLatchState = MMC_PCO_LOCK_RELEASED;
    idvSQL                      *sStatistics;
    //fix BUG-31376 Reducing s-latch duration of parent PCO while perform soft prepare .
    idBool                       sIsFixPcb =  ID_FALSE;;
    UInt                         sCurChildCreateCnt;
    mmcStatement               * sStmt = NULL;

    IDU_FIT_POINT( "mmcStatement::softPrepare" );

    sStatistics = getStatistics();
    //parent PCO에 prepare-latch가 s-latch로 걸려 있는 상태.
    *aNeedHardPrepare = ID_FALSE;
    if(aParentPCO == NULL)
    {
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.  
        IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLANCACHE_SEARCH_PPCO);
        mmcPlanCache::searchSQLText(this,&sParentPCO,&sHashKeyVal,&sPCOLatchState);
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_SEARCH_PPCO);
    }
    else
    {
        //rebuild에 의하여 Parent PCO으로부터 soft-prepare를 하는 경우.
        sParentPCO = aParentPCO;
        //TASK-3873 Static Analysis Code-Sonar false alarm remove.
        sHashKeyVal = aParentPCO->getHashKeyValue();
        sParentPCO->latchPrepareAsShared(sStatistics);
        sPCOLatchState = MMC_PCO_LOCK_ACQUIRED_SHARED;
    }
  retraverse_from_parent:

    if(sParentPCO == NULL)
    {
        IDE_DASSERT( sPCOLatchState == MMC_PCO_LOCK_RELEASED);
        //SQLText를 Plan Cache에 등록 시도(parent PCO 생성시도를 의미).
        sCacheAbleSQLText = mmcPlanCache::isCacheAbleSQLText(getQueryString());
        if(sCacheAbleSQLText == ID_TRUE)
        {
            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CREATE_PPCO);
            IDE_TEST_RAISE(mmcPlanCache::tryInsertSQLText(sStatistics,
                                                          getQueryString(),
                                                          getQueryLen(),
                                                          sHashKeyVal,
                                                          &sParentPCO,
                                                          &sPlanPCB,
                                                          &sSuccess,
                                                          &sPCOLatchState) != IDE_SUCCESS,
                           InsertSQLTextError);
            IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CREATE_PPCO);
            
            if(sSuccess == ID_FALSE)
            {
                IDE_DASSERT(sPCOLatchState == MMC_PCO_LOCK_ACQUIRED_SHARED);
                IDE_DASSERT(sParentPCO != NULL);
                goto  retraverse_from_parent;
            }
            else
            {
                IDE_DASSERT( sPCOLatchState == MMC_PCO_LOCK_RELEASED);
                // plan cache 대상이며,hard-prepare를 수행해야 함(sql plan check-in이 필요).
                *aNeedHardPrepare = ID_TRUE;
                *aPCB = sPlanPCB;
            }
            mmcPlanCache::incCacheMissCnt(sStatistics);
        }
        else
        {
            // plan cache 비대상이며,hard-prepare를   수행해야 됨.
            mmcPlanCache::incNoneCacheSQLTryCnt(sStatistics);
            *aNeedHardPrepare = ID_TRUE;
            *aPCB = NULL;
        }
    }
    else
    {
        IDE_DASSERT(sPCOLatchState == MMC_PCO_LOCK_ACQUIRED_SHARED);
        //SQL Text는 Plan Cache에 있는 상태
        // /statement의 environement와 match되는 plan을 찾는다.
        // fix BUG-23005 soft-prepare와 cachde replace간의 동시성 문제로 parent PCO가 free될수 있음.
        if(sParentPCO->getChildCnt() == 0 )
        {
            *aNeedHardPrepare = ID_TRUE;
            *aPCB = NULL;
        }
        else
        {
            // compile code optimizing때문에
            // else 블럭으로 보호한다.
            IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLANCACHE_SEARCH_CHILD_PCO);
            sParentPCO->searchChildPCO(this,&sPlanPCB,&sChildPCOPlanState,&sPCOLatchState);
            IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_SEARCH_CHILD_PCO);
            switch(sChildPCOPlanState)
            {
                case MMC_CHILD_PCO_PLAN_NEED_HARD_PREPARE:
                    IDE_DASSERT( sPCOLatchState == MMC_PCO_LOCK_RELEASED);
                    *aNeedHardPrepare = ID_TRUE;
                    *aPCB = NULL;
                    break;
                case  MMC_CHILD_PCO_PLAN_IS_NOT_READY:
                    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                    IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CREATE_CHILD_PCO);
                    IDE_DASSERT(sPCOLatchState == MMC_PCO_LOCK_ACQUIRED_SHARED);
                    sPlanPCB = sParentPCO->getSafeGuardPCB();
                    IDE_ASSERT(sPlanPCB != NULL);
                    //statement의 environement와 match되는 plan이 cache에 없는 경우.
                    // child PCO 생성시도.
                    //preventDupPlan에서 parent PCO의 s-latch를 품.
                    IDE_TEST_RAISE(mmcPlanCache::preventDupPlan(sStatistics,
                                                          sParentPCO,
                                                          sPlanPCB,
                                                          &sPCB4NewPlan,
                                                          &sPCOLatchState) != IDE_SUCCESS,
                             CreateNewPcoError);
                
                    IDE_DASSERT( sPCOLatchState == MMC_PCO_LOCK_RELEASED);
                    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                    IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CREATE_CHILD_PCO);
                
                    if(sPCB4NewPlan == NULL)
                    {
                        sParentPCO->latchPrepareAsShared(sStatistics);
                        sPCOLatchState = MMC_PCO_LOCK_ACQUIRED_SHARED;
                        goto  retraverse_from_parent;
                    }
                    else
                    {
                        //winner,hard-prepare가 필요
                        mmcPlanCache::incCacheMissCnt(sStatistics);
                        *aNeedHardPrepare = ID_TRUE;
                        *aPCB = sPCB4NewPlan;
                    }
                    break;
                case    MMC_CHILD_PCO_PLAN_IS_READY:
                    IDE_DASSERT(sPCOLatchState == MMC_PCO_LOCK_ACQUIRED_SHARED);
                    // Plan이 있다.
                    sChildPCO = sPlanPCB->getChildPCO();
                    IDE_ASSERT(sChildPCO != NULL);
                    //fix BUG-31376 Reducing s-latch duration of parent PCO while perform soft prepare .
                    /* parent PCO s-latch release하기전에 old PCB에 대하여 plan fix를 하여,
                       parent PCO가  victim되어 해제되는 case를 막는다.
                     */
                    
                    sParentPCO->getChildCreateCnt(&sCurChildCreateCnt);
                    sIsFixPcb = ID_TRUE;
                    sPlanPCB->planFix(sStatistics);
                    sPCOLatchState = MMC_PCO_LOCK_RELEASED;
                    sParentPCO->releasePrepareLatch();
                    
                    sGetSmit4PrepareContext.mStatement = this;
                    sGetSmit4PrepareContext.mNeedCommit = ID_FALSE;
                    sGetSmit4PrepareContext.mTrans = NULL;
                    sGetSmit4PrepareContext.mSoftPrepareReason = aSoftPrepareReason;
                    //Plan Validation.
                    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                    IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_VALIDATE_PCO);
                    IDE_TEST_RAISE(qci::validatePlan(mmcStatement::getSmiStatement4PrepareCallback,
                                                     &sGetSmit4PrepareContext,
                                                     getQciStmt(),
                                                     sChildPCO->getSharedPlan(),
                                                     &sValidPlan) != IDE_SUCCESS,
                                   PlanValidationError);
                    IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_VALIDATE_PCO);
                    if(sValidPlan == ID_TRUE)
                    {
                        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                        IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_CHECK_PRIVILEGE_PCO);
                        IDE_TEST_RAISE(qci::checkPrivilege(mmcStatement::getSmiStatement4PrepareCallback,
                                                           &sGetSmit4PrepareContext,            
                                                           getQciStmt(),
                                                           sChildPCO->getSharedPlan()) != IDE_SUCCESS,
                                       PrivilegeError);
                        //soft-prepare success.
                        *aPCB = sPlanPCB;
                        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_CHECK_PRIVILEGE_PCO);
                    }            
                    else
                    {
                        // fix BUG-30903
                        // V$SYSSTATdml rebuild count를 증가시킨다.
                        IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                                            IDV_STAT_INDEX_REBUILD_COUNT,
                                            1);
                        // new plan생성을 위하여 중복 plan 방지 절차를 수행한다.
                        //fix BUG-27360 Code-Sonar UMR, sPCB4NewPlan can be uninitialized mememory.
                        IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE);
                        IDE_TEST_RAISE(mmcPlanCache::preventDupPlan(sStatistics,
                                                                    this,
                                                                    sParentPCO,
                                                                    sPlanPCB,
                                                                    &sPCB4NewPlan,
                                                                    &sPCOLatchState,
                                                                    MMC_PLAN_RECOMPILE_BY_SOFT_PREPARE,
                                                                    sCurChildCreateCnt) != IDE_SUCCESS,
                                       CreateNewPcoByRebuild);
                        //fix BUG-31376 Reducing s-latch duration of parent PCO while perform soft prepare .
                        sIsFixPcb = ID_FALSE;
                        sPlanPCB->planUnFix(sStatistics);
                        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
                        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE);
               
                        
                        IDE_DASSERT( sPCOLatchState == MMC_PCO_LOCK_RELEASED);
                        if(sPCB4NewPlan == NULL)
                        {
                            sParentPCO->latchPrepareAsShared(sStatistics);
                            sPCOLatchState = MMC_PCO_LOCK_ACQUIRED_SHARED;
                            goto  retraverse_from_parent;
                    
                        }//if
                        else
                        {
                            //winner,hard-prepare가 필요
                            *aNeedHardPrepare = ID_TRUE;
                            *aPCB = sPCB4NewPlan;
                        }//else
                    }//else
                
                    if(sGetSmit4PrepareContext.mNeedCommit == ID_TRUE)
                    {
                        IDE_TEST_RAISE(sGetSmit4PrepareContext.mPrepareStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS,
                                       ErrorSmiStmtEnd);

                        sStmt = sGetSmit4PrepareContext.mStatement;

                        IDE_TEST( sStmt->endTransAndUnlock4Prepare( sGetSmit4PrepareContext.mTrans,
                                                                    &sGetSmit4PrepareContext.mEndTransAction,
                                                                    MMC_END_TRANS_SUCCESS )
                                  != IDE_SUCCESS );
                    }
                    break;
                default:
                    IDE_ASSERT(0);
                    break;
            }//switch
        }//else sParentPCO->getChildCnt() 
    }//else sParentPCO
    
    
    //Parent PCO 의 prepare-latch의  release.
    if(sPCOLatchState !=  MMC_PCO_LOCK_RELEASED)
    {
        sParentPCO->releasePrepareLatch();
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ErrorSmiStmtEnd);
    {
        sStmt = sGetSmit4PrepareContext.mStatement;

        IDE_ASSERT( sStmt->endTransAndUnlock4Prepare( sGetSmit4PrepareContext.mTrans,
                                                      &sGetSmit4PrepareContext.mEndTransAction,
                                                      MMC_END_TRANS_FAILURE )
                    == IDE_SUCCESS );
        sGetSmit4PrepareContext.mNeedCommit = ID_FALSE;
    }
    IDE_EXCEPTION(PlanValidationError);
    {
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_VALIDATE_PCO);
    }
    IDE_EXCEPTION(PrivilegeError);
    {
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_CHECK_PRIVILEGE_PCO);
    }
    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
    IDE_EXCEPTION(InsertSQLTextError);
    {
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CREATE_PPCO);
    }
    IDE_EXCEPTION(CreateNewPcoError);
    {
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CREATE_CHILD_PCO);
    }
    IDE_EXCEPTION(CreateNewPcoByRebuild);
    {
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE);
    }
    IDE_EXCEPTION_END;

    //fix BUG-31376 Reducing s-latch duration of parent PCO while perform soft prepare .
    if(sIsFixPcb == ID_TRUE)
    {
        sPlanPCB->planUnFix(sStatistics);
    }
    if(sPCOLatchState !=  MMC_PCO_LOCK_RELEASED)
    {
        sParentPCO->releasePrepareLatch();
    }

    if ( sGetSmit4PrepareContext.mNeedCommit == ID_TRUE )
    {
        IDE_ASSERT( sGetSmit4PrepareContext.mPrepareStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) 
                    == IDE_SUCCESS );

        sStmt = sGetSmit4PrepareContext.mStatement;

        IDE_ASSERT( sStmt->endTransAndUnlock4Prepare(  sGetSmit4PrepareContext.mTrans,
                                                       &sGetSmit4PrepareContext.mEndTransAction,
                                                       MMC_END_TRANS_FAILURE )
                    == IDE_SUCCESS );
        sGetSmit4PrepareContext.mNeedCommit = ID_FALSE;
    }

    return IDE_FAILURE;
}
//PROJ-1436 SQL-Plan Cache.
// commit mode와 root statement에 따라
// 적절한 mmcTrans object를 return한다.
IDE_RC mmcStatement::beginTransAndLock4Prepare( mmcTransObj                  ** aTrans,
                                                mmcEndTransAction4Prepare     * aEndTransAction )
{
    mmcTransObj           * sTrans = NULL;
    idBool                  sIsDummyBegin = ID_TRUE;

    mmcTransEndAction       sTransEndAction = MMC_TRANS_END;
    idBool                  sIsLock = ID_FALSE;

    if ( mSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT )
    {
        if ( isRootStmt() == ID_TRUE )
        {
            sTrans = allocTrans();

            mmcTrans::begin( sTrans, 
                             getStatistics(), 
                             (SMI_TRANSACTION_NORMAL | SMI_ISOLATION_CONSISTENT | SMI_COMMIT_WRITE_NOWAIT),
                             getSession(), 
                             &sIsDummyBegin );
            sTransEndAction = MMC_TRANS_END;
        }
        else
        {
            sTrans = getParentStmt()->getTransPtr();
            sTransEndAction = MMC_TRANS_DO_NOTHING;
        }

        IDE_DASSERT( mmcTrans::isShareableTrans( sTrans ) == ID_FALSE );

    }//if
    else
    {
        IDE_DASSERT( mSession->isTransAlloc() == ID_TRUE );

        sTrans = mSession->getTransPtr();

        if( mmcTrans::isShareableTrans( sTrans ) == ID_TRUE )
        {
            IDE_TEST( acquireShareTransSmiStmtLock( sTrans ) != IDE_SUCCESS );
            sIsLock = ID_TRUE;
        }

        if ( mSession->getTransBegin() == ID_FALSE )
        {
            mmcTrans::begin( sTrans,
                             mSession->getStatSQL(),
                             mSession->getSessionInfoFlagForTx(),
                             mSession,
                             &sIsDummyBegin );

            /* fix BUG-46913 */
            mmcTrans::clearAndSetSessionInfoAfterBegin( mSession,
                                                        sTrans );

            if ( sIsDummyBegin == ID_TRUE )
            {
                sTransEndAction = MMC_TRANS_SESSION_ONLY_END;
            }
            else
            {
                sTransEndAction = MMC_TRANS_END;
            }
        }
        else
        {
            sTransEndAction = MMC_TRANS_DO_NOTHING;
        }
    }//else

    IDE_DASSERT( sTrans != NULL );

    mmcStatement::setTransID(mmcTrans::getTransID(sTrans));

    *aTrans = sTrans;
    aEndTransAction->mTransEndAction = sTransEndAction;
    aEndTransAction->mNeedReleaseLock = sIsLock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        releaseShareTransSmiStmtLock( sTrans );
    }

    return IDE_FAILURE;
}

IDE_RC mmcStatement::endTransAndUnlock4Prepare( mmcTransObj                * aTrans,
                                                mmcEndTransAction4Prepare  * aEndTransAction,
                                                mmcEndTransResult4Prepare    aResult )
{
    switch ( aResult )
    {
        case MMC_END_TRANS_SUCCESS:
            IDE_TEST( mmcTrans::commit4Prepare( aTrans,
                                                mSession,
                                                aEndTransAction->mTransEndAction )
                      != IDE_SUCCESS );
            break;

        case MMC_END_TRANS_FAILURE:
            IDE_TEST( mmcTrans::rollback4Prepare( aTrans,
                                                  mSession,
                                                  aEndTransAction->mTransEndAction )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_DASSERT( 0 );
            break;
    }

    if ( aEndTransAction->mNeedReleaseLock == ID_TRUE )
    {
        releaseShareTransSmiStmtLock( aTrans );
        aEndTransAction->mNeedReleaseLock = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//PROJ-1436 SQL-Plan Cache.
IDE_RC mmcStatement::hardPrepare(mmcPCB                 *aPCB,
                                 qciSQLPlanCacheContext *aPlanCacheContext)
{
    idBool                     sSuccess;
    mmcTransObj               *sTrans;
    mmcEndTransAction4Prepare  sEndTransAction;
    SChar                     *sSQLText;
    mmcChildPCO               *sChildPCO;
    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
    idvSQL                    *sStatistics = getStatistics();

    if(aPCB != NULL)
    {
        sSQLText = aPCB->getParentPCO()->getSQLString4HardPrepare();
    }
    else
    {
        sSQLText = getQueryString();
    }
    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
    IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_HARD_PREPARE);

    IDE_TEST_RAISE(parse(sSQLText) != IDE_SUCCESS, parse_error);

    /* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
    if ( mSession->getCallByShardAnalyzeProtocol() == ID_TRUE )
    {
        IDE_TEST( sdi::checkStmtTypeBeforeAnalysis( &getQciStmt()->statement )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    IDE_TEST( beginTransAndLock4Prepare( &sTrans, &sEndTransAction ) != IDE_SUCCESS );

    // PROJ-2163
    IDE_TEST_RAISE(qci::bindParamInfo(getQciStmt(),
                                      aPlanCacheContext) != IDE_SUCCESS,
                   HardPrepareError);

    IDU_FIT_POINT_RAISE( "mmcStatement::hardPrepare::doHardPrepare", 
                         HardPrepareError, 
                         mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, 
                         "doHardPrepare Error" );
    IDE_TEST_RAISE(doHardPrepare(sSQLText,
                                 aPlanCacheContext,
                                 mmcTrans::getSmiStatement(sTrans)) 
                   != IDE_SUCCESS, HardPrepareError);

    //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
    IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_HARD_PREPARE);

    IDU_FIT_POINT_RAISE( "mmcStatement::hardPrepare::endTransAndUnlock4Prepare", 
                         CommitError, 
                         mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, 
                         "CommitError" );

    /* BUG-47650 BUG-38585 IDE_ASSERT remove */
    IDE_TEST_RAISE( endTransAndUnlock4Prepare( sTrans,
                                               &sEndTransAction,
                                               MMC_END_TRANS_SUCCESS )
                    != IDE_SUCCESS, CommitError );

    if( aPCB != NULL)
    {
        if(aPlanCacheContext->mPlanCacheInMode ==  QCI_SQL_PLAN_CACHE_IN_ON)
        {
            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_PREPARE);
            mmcPlanCache::checkIn(getStatistics(),
                                  aPCB,
                                  aPlanCacheContext,
                                  &sSuccess);
            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_PREPARE);

            if(sSuccess == ID_FALSE)
            {
                mmcPlanCache::incCacheInFailCnt(getStatistics());
                qci::setPrivateTemplate(getQciStmt(),
                                        aPlanCacheContext->mSharedPlanMemory,
                                        QCI_SQL_PLAN_CACHE_IN_FAILURE);

                /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
                 * qci::makePlanCacheInfo()에서 만든 QMP Memory와 Shared Template을 제거한다.
                 */
                aPlanCacheContext->mSharedPlanMemory = NULL;

                (void)qci::freePrepPrivateTemplate( aPlanCacheContext->mPrepPrivateTemplate );
                aPlanCacheContext->mPrepPrivateTemplate = NULL;

                mPCB = NULL;
            }
            else
            {
                /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
                 * QMP Memory와 Shared Template을 초기화한다.
                 */
                aPlanCacheContext->mSharedPlanMemory = NULL;
                aPlanCacheContext->mPrepPrivateTemplate = NULL;

                //success;
                /*fix BUG-30434 VAL] mmcPCB::getChildPCO Invalid read of size 8
                  check In 이 성공한 case에 한하여 child PCO를 참조하여야 한다.
                  check In이  실패한 경우는 cache replace에 의하여 victim이 되어
                  child PCO가 해제될수 있기때문이다.*/
                sChildPCO = aPCB->getChildPCO();
                IDE_TEST(qci::clonePrivateTemplate(getQciStmt(),
                                                   sChildPCO->getSharedPlan(),
                                                   sChildPCO->getPreparedPrivateTemplate(),
                                                   aPlanCacheContext)
                         != IDE_SUCCESS);
                mInfo.mSQLPlanCacheTextId = sChildPCO->mSQLTextId;
                mInfo.mSQLPlanCachePCOId  = sChildPCO->mChildID;
                mPCB = aPCB;

                /* BUG-46158 */
                if (aPlanCacheContext->mPlanCacheKeep == ID_TRUE)
                {
                    aPCB->getParentPCO()->setPlanCacheKeep(MMC_PCO_PLAN_CACHE_KEEP);
                }
            }
        }
        else
        {
            // PROJ-2163
            // D$table, no_plan_cache, select * from t1 where i1 = ?(mtdUndef)
            qci::setPrivateTemplate(getQciStmt(),
                                    NULL,
                                    QCI_SQL_PLAN_CACHE_IN_NORMAL);
            mPCB = NULL;
            mmcPlanCache::movePCBOfChildToUnUsedLst(sStatistics, aPCB);
        }
    }

    // fix BUG-23161
    // prepare success의 의미는  hard prepare가 성공했다는 것임.
    IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                        IDV_STAT_INDEX_PREPARE_SUCCESS_COUNT, 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION(HardPrepareError);
    {
        IDE_PUSH();
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_HARD_PREPARE);

        IDE_ASSERT( endTransAndUnlock4Prepare( sTrans,
                                               &sEndTransAction,
                                               MMC_END_TRANS_FAILURE )
                    == IDE_SUCCESS );

        /* fix BUG-27952
           Non-Autocommit 세션에서 DDL prepare 중 에러가 발생했을 경우에는
           이전 연산을 모두 commit 시킨다.
           BUG-48592 Sharding에서는 에러가 발생해도 정합성 보장을 위해 commit하지 않는다. */
        if ((sdi::isShardEnable() != ID_TRUE) &&
            (mSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT) &&
            (isRootStmt() == ID_TRUE))
        {
            if (qciMisc::getStmtType(mStmtType) == QCI_STMT_MASK_DDL)
            {
                if ( mSession->isDDLAutoCommit() == ID_TRUE ) 
                {
                    // fix BUG-30411
                    // hardPrepare 실패 후 commit시 에러가 발생하면
                    // 부트로그에 에러 로그를 기록한다. 
                    if (mSession->commit(ID_FALSE) != IDE_SUCCESS)
                    {
                        ideLog::log(IDE_SERVER_0, "[hardPrepare Failure : %s]", ideGetErrorMsg(ideGetErrorCode()));
                    }
                }
                else
                {
                    /* Transactional DDL, Global DDL 도중 실패할 경우에는 Commit/Rollback 어느 작업도 하지 않고 실패 */
                    /* nothing to do */
                }
            }
        }

        IDE_POP();

        if(aPCB != NULL)
        {
            mmcPlanCache::register4GC(getStatistics(),
                                      aPCB);
        }
    }

    IDE_EXCEPTION( parse_error);
    {
        //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_HARD_PREPARE);
        if(aPCB != NULL)
        {
            mmcPlanCache::register4GC(getStatistics(),
                                      aPCB);
        }
    }
    IDE_EXCEPTION( CommitError )
    {
        IDE_ASSERT( endTransAndUnlock4Prepare( sTrans,
                                               &sEndTransAction,
                                               MMC_END_TRANS_FAILURE )
                    == IDE_SUCCESS );

        if ( aPCB != NULL )
        {
            mmcPlanCache::register4GC( getStatistics(), aPCB );
        }
    }
    IDE_EXCEPTION_END;

    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     * qci::makePlanCacheInfo()에서 만든 QMP Memory와 Shared Template을 제거한다.
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

    // fix BUG-23161
    // prepare failure의 의미는  hard prepare가 실패했다는 것임.
    IDV_SESS_ADD_DIRECT(mSession->getStatistics(),
                        IDV_STAT_INDEX_PREPARE_FAILURE_COUNT, 1);

    return IDE_FAILURE;
}

void mmcStatement::makePlanTreeBeforeCloseCursor( mmcStatement * aStatement,
                                                  mmcStatement * aResultSetStmt )
{
    // fix BUG- 22175
    // execution끝부분 또는 fetch 끝부분에 불려지는 함수로,
    // planTree text를 생성한다.
    // 호출하는 부분은 다음과 같음.
    // 1. dml execution end전에 호출
    // 2. no rows select end전에 호출
    // 3. fetchEnd에서 cursor close전에 호출
    
    IDE_RC         sRet;
    iduVarString * sPlanString;
    mmcTransObj  * sTrans = NULL;
    smTID          sTransID;

    // BUG-47831 View plan of query in PSM
    iduListNode  * sPieceIterator;

    //profiling에서 필요로 하거나
    //plan on이면 planTreeText 생성
    if ( ((idvProfile::getProfFlag() & IDV_PROF_TYPE_PLAN_FLAG) ==
          IDV_PROF_TYPE_PLAN_FLAG) ||
         (aStatement->getSession()->getExplainPlan() ==
          QCI_EXPLAIN_PLAN_ON) )
    {
        sRet = aStatement->clearPlanTreeText();
        if (sRet == IDE_SUCCESS)
        {
            sRet = aStatement->makePlanTreeText(ID_FALSE);
        }

        //profiling에서 필요로 한 경우 profile에 plan write
        if ( ((idvProfile::getProfFlag() & IDV_PROF_TYPE_PLAN_FLAG) ==
              IDV_PROF_TYPE_PLAN_FLAG) ||
             (IDE_TRC_QP_6 != 0) )
        {
            if( sRet == IDE_SUCCESS )
            {
                sPlanString = aResultSetStmt->getPlanString();
            }
            else
            {
                sPlanString = NULL;
            }
        }
            
        if ( (idvProfile::getProfFlag() & IDV_PROF_TYPE_PLAN_FLAG) ==
             IDV_PROF_TYPE_PLAN_FLAG)
        {
            /* BUG-46713 codesonar */
            sTrans = aResultSetStmt->getSession()->getTransPtr( aResultSetStmt );

            if ( sTrans != NULL )
            {
                sTransID = mmcTrans::getTransID( sTrans );
            }
            else
            {
                sTransID = 0;
            }

            idvProfile::writePlan(aResultSetStmt->getSessionID(),
                                  aResultSetStmt->getStmtID(),
                                  sTransID,
                                  sPlanString);
        }

        // BUG-47831 IDE_TRC_QP_5 = Logging query in PSM
        if ( (IDE_TRC_QP_5 != 0) &&
             (aResultSetStmt->getParentStmt() != NULL) )
        {
            ideLog::log(IDE_QP_5,
                        "[Query in PSM][SID %"ID_UINT32_FMT"][STMT_ID %"ID_UINT32_FMT"]\n%s",
                        aResultSetStmt->getSessionID(),
                        aResultSetStmt->getStmtID(),
                        aResultSetStmt->getQueryString());
        }

        // BUG-47831 IDE_TRC_QP_6 = Logging plan in PSM
        if ( (IDE_TRC_QP_6 != 0) &&
             (sPlanString != NULL) &&
             (aResultSetStmt->getParentStmt() != NULL) )
        {
            if ( sPlanString->mLength != 0 )
            {
                ideLog::log(IDE_QP_6,
                            "[Plan in PSM][SID %"ID_UINT32_FMT"][STMT_ID %"ID_UINT32_FMT"]",
                            aResultSetStmt->getSessionID(),
                            aResultSetStmt->getStmtID());

                // BUG-47831
                // trc log에는 한번에 2K정도만 기록 가능하여 piece 단위(1024)로 기록한다.
                IDU_LIST_ITERATE(&sPlanString->mPieceList, sPieceIterator)
                {
                    ideLog::log(IDE_QP_6, "%s", ((iduVarStringPiece *)sPieceIterator->mObj)->mData);
                }
            }
        }
    }
}

// BUG-36203 PSM Optimize
IDE_RC mmcStatement::changeStmtState()
{
    switch (getBindState())
    {
        case MMC_STMT_BIND_NONE:
            setBindState(MMC_STMT_BIND_INFO);

        case MMC_STMT_BIND_INFO:
            IDE_TEST(qci::setParamDataState(getQciStmt()) != IDE_SUCCESS);
            setBindState(MMC_STMT_BIND_DATA);
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::reprepare()
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      Execute 전에 plan 을 재생성(reprepare) 하는 함수
 *
 *      호스트 변수가 포함된 쿼리는 실행 시점에
 *      사용자가 바인드한 type 과 plan 의 type 이 서로 다를 수 있다.
 *      이 경우 plan 을 재생성 해야 한다.
 *
 * Implementation :
 *      1. reprepare skip 여부 체크
 *        1. Rebuild failure flag 체크
 *        2. Bind state 체크 및 상태 전이
 *        3. Re hard prepare failure flag 체크 (바인드 쿼리)
 *      2. reprepare
 *        a. (Plan cache 사용 불가 혹은 non-cacheable 쿼리 인경우)
 *           CACHE_IN_OFF 모드로 hard prepare 수행
 *        b. (Soft prepare 에서 plan hit 시)
 *           clone private template 수행
 *        c. (Soft prepare 에서 plan miss 시)
 *           PCB 획득 여부에 따라 CACHE_IN_ON/OFF 로 hard prepare 수행
 *
 ***********************************************************************/

    mmcChildPCO            *sChildPCO;
    idBool                  sNeedHardPrepare;
    mmcPCB                 *sPCB = NULL;
    qciSQLPlanCacheContext  sPlanCacheContext;
    idvSQL                 *sStatistics;
    UChar                   sState =0;
    idBool                  sRebuildDetected = ID_FALSE;

    /* PROJ-2223 Altibase Auditing */
    idvAuditTrail          *sAuditTrail;
    
    /* BUG-44853 Plan Cache 예외 처리가 부족하여, 비정상 종료가 발생할 수 있습니다.
     * QMP Memory와 Shared Template을 초기화한다.
     */
    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
    sPlanCacheContext.mSharedPlanMemory = NULL;
    sPlanCacheContext.mPrepPrivateTemplate = NULL;

    sStatistics = getStatistics();

    if((getQciStmt()->flag & QCI_STMT_REBUILD_EXEC_MASK)
       == QCI_STMT_REBUILD_EXEC_FAILURE )
    {
        IDE_CONT( pass );
    }

    // BUG-36203 PSM Optimize
    IDE_TEST( changeStmtState() != IDE_SUCCESS );

    if((getQciStmt()->flag & QCI_STMT_REHARDPREPARE_EXEC_MASK)
       == QCI_STMT_REHARDPREPARE_EXEC_SUCCESS )
    {
        if(qci::isBindChanged(getQciStmt()) == ID_TRUE)
        {
            // continue
        }
        else
        {
            IDE_TEST( checkShardRebuildStatus( &sRebuildDetected ) != IDE_SUCCESS );

            if ( sRebuildDetected == ID_TRUE )
            {
                // continue
            }
            else
            {
                IDE_CONT( pass );
            }
        }
    }

    /* BUG-44874 Plan Cache가 가득차면, 비정상 종료가 발생할 수 있습니다.
     *  1. Plan Cache가 qcPrepTemplateHeader를 관리하는데,
     *     Plan Cache를 사용하는 qcStatement가 qcPrepTemplateHeader를 포인터로 참조합니다.
     *  2. qcStatement를 정상적으로 재활용하는 경우, qcg::clearStatement() -> qcg::freePrepTemplate()
     *     -> qci::freePrepTemplate() 경로로 qcPrepTemplateHeader 참조를 종료합니다.
     *  3. 이후에 mmcStatement::releasePlanCacheObject()를 호출하면, Plan Cache와 qcStatement의 관계를 종료합니다.
     *  따라서, releasePlanCacheObject()를 호출하기 전에, qci::clearStatement4Reprepare()를 호출해야 합니다.
     */
    IDE_TEST( qci::clearStatement4Reprepare( getQciStmt(),
                                             getSmiStmt() )
              != IDE_SUCCESS );

    /* TASK-7219 Non-shard DML */
    IDE_TEST( qci::setShardPartialExecType( getQciStmt(),
                                            getShardPartialExecType() )
              != IDE_SUCCESS );

    //Plan cache 에서 가져온 PCB 를 해제한다. (unfix)
    if(mPCB != NULL)
    {
        releasePlanCacheObject();
        IDU_FIT_POINT( "mmcStatement::reprepare::SLEEP::AFTER::releasePlanCacheObject" );
    }

    if ( mmcPlanCache::isEnable( mSession, &mQciStmt ) == ID_FALSE )
    {
        //cache disable
        sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;

        IDE_TEST_RAISE(hardPrepare((mmcPCB*)NULL,
                                   &sPlanCacheContext) !=  IDE_SUCCESS, ErrorHardPrepare);
    }
    else
    {
        //no_plan_cache, d$
        if(qci::isCacheAbleStatement(getQciStmt()) == ID_FALSE)
        {
            //cache disable
            sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;

            IDE_TEST_RAISE(hardPrepare((mmcPCB*)NULL,
                                       &sPlanCacheContext) !=  IDE_SUCCESS, ErrorHardPrepare);
        }
        else
        {
            IDV_SQL_OPTIME_BEGIN(sStatistics,IDV_OPTM_INDEX_QUERY_SOFT_PREPARE);

            IDE_TEST_RAISE(softPrepare((mmcParentPCO*) NULL,
                                       MMC_SOFT_PREPARE_FOR_PREPARE,
                                 &sPCB,
                                 &sNeedHardPrepare) != IDE_SUCCESS,ErrorSoftPrepare);

            IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_QUERY_SOFT_PREPARE);

            if(sNeedHardPrepare == ID_FALSE)
            {
                //plan cache hit sucess!
                IDE_DASSERT(sPCB != NULL);
                sChildPCO = sPCB->getChildPCO();
                sState =1;
                IDE_TEST(qci::clonePrivateTemplate(getQciStmt(),
                                                   sChildPCO->getSharedPlan(),
                                                   sChildPCO->getPreparedPrivateTemplate(),
                                                   &sPlanCacheContext)
                         != IDE_SUCCESS);
                sState = 0;
                mPCB = sPCB;
                mInfo.mSQLPlanCacheTextId = sChildPCO->mSQLTextId;
                mInfo.mSQLPlanCachePCOId  = sChildPCO->mChildID;

                mStmtType = sPlanCacheContext.mStmtType;

                if ( ( mSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
                     ( isRootStmt() == ID_TRUE ) )
                {
                    (void)allocTrans();
                }
            }
            else
            {
                // Soft prepare 에서 PCB 를 가져왔다면 CACHE_IN_ON 모드로
                // hard prepare 해서  plan cache 에 check in 하고
                // 가져오지 못했다면 CACHE_IN_OFF 모드로 hard prepare 한다.
                if(sPCB == NULL)
                {
                    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
                }
                else
                {
                    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_ON;
                }

                IDE_TEST_RAISE(hardPrepare(sPCB,
                                           &sPlanCacheContext) != IDE_SUCCESS, ErrorHardPrepare);
            }
        }
    }

    if( mmtAuditManager::isAuditStarted() == ID_TRUE )
    {
        /* get the new object information */
        qci::getAllRefObjects( getQciStmt(), &mAuditObjects, &mAuditObjectCount );

        sAuditTrail = getAuditTrail();
        if( sAuditTrail->mSessionID != getSessionID() )
        {
            setAuditTrailSess( sAuditTrail, getSession() );
        }

        setAuditTrailStmt( sAuditTrail, this );
    }

    /* BUG-45823 */
    setShardQueryType( sdi::getQueryType( getQciStmt() ) );

    IDE_TEST(qci::setBindTuple(getQciStmt()) != IDE_SUCCESS);

    setBindState(MMC_STMT_BIND_DATA);

    setCursorFlag(sPlanCacheContext.mSmiStmtCursorFlag); 

    getQciStmt()->flag &= ~QCI_STMT_REHARDPREPARE_EXEC_MASK;
    getQciStmt()->flag |= QCI_STMT_REHARDPREPARE_EXEC_SUCCESS;

    IDE_EXCEPTION_CONT( pass );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ErrorSoftPrepare);
    {
        IDV_SQL_OPTIME_END(sStatistics,IDV_OPTM_INDEX_QUERY_SOFT_PREPARE);
    }
    IDE_EXCEPTION(ErrorHardPrepare);
    {
        getQciStmt()->flag &= ~QCI_STMT_REHARDPREPARE_EXEC_MASK;
        getQciStmt()->flag |= QCI_STMT_REHARDPREPARE_EXEC_FAILURE;

        qci::setPrivateArea(getQciStmt());

        clearStmt(MMC_STMT_BIND_NONE);
    }
    IDE_EXCEPTION_END;
    {
        // fix BUG-18865
        setQueryStartTime(0);

        if ((ideGetErrorCode() & E_ACTION_MASK) == E_ACTION_FATAL)
        {
            ideLog::logErrorMsg(IDE_SERVER_0);
            IDE_CALLBACK_FATAL("fatal error code occurred in mmcStatement::prepare()");
        }

        if(sState != 0)
        {
            sPCB->planUnFix( getStatistics());
        }

    }

    mSession->transBeginForGTxEndTran();

    return IDE_FAILURE;
}

void mmcStatement::setAuditTrailSess( idvAuditTrail *aAuditTrail, mmcSession *aSession )
{
    mmcSessionInfo *sSessInfo = aSession->getInfo();

    /* set session info */
    aAuditTrail->mClientPID       = sSessInfo->mClientPID;
    aAuditTrail->mSessionID       = sSessInfo->mSessionID;
    aAuditTrail->mXaSessionFlag   = sSessInfo->mXaSessionFlag;
    aAuditTrail->mAutoCommitFlag  = aSession->getCommitMode();


    idlOS::memcpy( aAuditTrail->mClientType,
                   sSessInfo->mClientType,
                   IDV_MAX_CLIENT_TYPE_LEN );
    aAuditTrail->mClientType[IDV_MAX_CLIENT_TYPE_LEN] = '\0';

    idlOS::memcpy( aAuditTrail->mClientAppInfo,
                   sSessInfo->mClientAppInfo,
                   IDV_MAX_CLIENT_APP_LEN );
    aAuditTrail->mClientAppInfo[IDV_MAX_CLIENT_APP_LEN] = '\0';

    idlOS::memcpy( aAuditTrail->mLoginIP,
                   sSessInfo->mUserInfo.loginIP,
                   IDV_MAX_IP_LEN );
    aAuditTrail->mLoginIP[IDV_MAX_IP_LEN] = '\0';

    idlOS::memcpy( aAuditTrail->mLoginID,
                   sSessInfo->mUserInfo.loginID,
                   IDV_MAX_NAME_LEN );
    aAuditTrail->mLoginID[IDV_MAX_NAME_LEN] = '\0';
}

void mmcStatement::setAuditTrailStmt( idvAuditTrail *aAuditTrail, mmcStatement *aStmt )
{
    SChar *sOperation = NULL;
    qciAuditOperIdx sOperIdx;

    aAuditTrail->mStatementID   = aStmt->getStmtID();
    aAuditTrail->mTransactionID = aStmt->getTransID();

    qci::getAuditOperation( aStmt->getQciStmt(), 
                            &sOperIdx, 
                            (const SChar **)&sOperation );

    if( sOperation != NULL )
    {
        aAuditTrail->mActionCode = sOperIdx; 

        idlOS::strncpy( aAuditTrail->mAction, 
                        sOperation, 
                        IDV_MAX_ACTION_LEN );
        aAuditTrail->mAction[IDV_MAX_ACTION_LEN] = '\0';
    }
    else
    {
        aAuditTrail->mActionCode = QCI_AUDIT_OPER_MAX;
    }
}

void mmcStatement::setAuditTrailStatSess( idvAuditTrail *aAuditTrail, idvSQL *aStatSQL )
{
    
    /* Stat Info */
    aAuditTrail->mUseMemory           = aStatSQL->mUseMemory;
    aAuditTrail->mExecuteSuccessCount = aStatSQL->mExecuteSuccessCount;
    aAuditTrail->mExecuteFailureCount = aStatSQL->mExecuteFailureCount;
    aAuditTrail->mProcessRow          = aStatSQL->mProcessRow;

    /* Elapse time Info */
    setAuditTrailStatElapsedTime( aAuditTrail, aStatSQL );
}

void mmcStatement::setAuditTrailStatStmt( idvAuditTrail   *aAuditTrail, 
                                          qciStatement    *aQciStmt,
                                          mmcExecutionFlag aExecutionFlag, 
                                          idvSQL          *aStatSQL )
{
    SLong sAffectedRowCount;
    SLong sFetchedRowCount;

    /* Stat Info */
    aAuditTrail->mUseMemory = aStatSQL->mUseMemory;

    if( aExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS )
    {
        aAuditTrail->mExecuteSuccessCount = 1;
        aAuditTrail->mExecuteFailureCount = 0;
    }
    else if( aExecutionFlag == MMC_EXECUTION_FLAG_FAILURE )
    {
        aAuditTrail->mExecuteSuccessCount = 0;
        aAuditTrail->mExecuteFailureCount = 1;
    }

    qci::getRowCount(aQciStmt, &sAffectedRowCount, &sFetchedRowCount);
    aAuditTrail->mProcessRow = (ULong)((sAffectedRowCount < 0) ? 0 : sAffectedRowCount);

    /* Elapse time Info */
    setAuditTrailStatElapsedTime( aAuditTrail, aStatSQL );
}


void mmcStatement::setAuditTrailStatElapsedTime( idvAuditTrail *aAuditTrail, idvSQL *aStatSQL )
{
    /* Elapse time Info */
    aAuditTrail->mParseTime =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL, IDV_OPTM_INDEX_QUERY_PARSE));

    aAuditTrail->mValidateTime =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL, IDV_OPTM_INDEX_QUERY_VALIDATE));

    aAuditTrail->mOptimizeTime =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL, IDV_OPTM_INDEX_QUERY_OPTIMIZE));

    aAuditTrail->mExecuteTime  =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL, IDV_OPTM_INDEX_QUERY_EXECUTE));

    aAuditTrail->mFetchTime =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL,  IDV_OPTM_INDEX_QUERY_FETCH));

    aAuditTrail->mSoftPrepareTime =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL, IDV_OPTM_INDEX_QUERY_SOFT_PREPARE));

    aAuditTrail->mTotalTime =
        IDV_TIMEBOX_GET_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT(aStatSQL,
                IDV_OPTM_INDEX_QUERY_PARSE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(aStatSQL,
                  IDV_OPTM_INDEX_QUERY_VALIDATE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(aStatSQL,
                  IDV_OPTM_INDEX_QUERY_OPTIMIZE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(aStatSQL,
                  IDV_OPTM_INDEX_QUERY_EXECUTE)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(aStatSQL,
                  IDV_OPTM_INDEX_QUERY_FETCH)) +
            IDV_TIMEBOX_GET_ACCUM_TIME(
               IDV_SQL_OPTIME_DIRECT(aStatSQL,
                  IDV_OPTM_INDEX_QUERY_SOFT_PREPARE));

}

/* BUG-47029 smiStatement.begin시에 전달하는 sFlag(smiStatement 속성) 값을 구한다. */
UInt mmcStatement::getSmiStatementFlag(mmcSession *aSession, mmcStatement *aStmt, mmcTransObj *aTrans)
{
    qciStmtType sQciStmtType = aStmt->getStmtType();
    UInt        sFlag = 0;

    if ( qciMisc::isStmtDML(sQciStmtType) == ID_TRUE )
    {
        if ( (sQciStmtType == QCI_STMT_SELECT) &&
             (aSession->getTxIsolationLevel(aTrans) != SMI_ISOLATION_REPEATABLE) )
        {
            sFlag = SMI_STATEMENT_UNTOUCHABLE;
        }
        else
        {
            // PROJ-2199 SELECT func() FOR UPDATE 지원
            // SMI_STATEMENT_FORUPDATE 추가
            if( sQciStmtType == QCI_STMT_SELECT_FOR_UPDATE )
            {
                sFlag = SMI_STATEMENT_FORUPDATE;
            }
            else
            {
                sFlag = SMI_STATEMENT_NORMAL;
            }
        }
    }
    else if ( qciMisc::isStmtDDL(sQciStmtType) == ID_TRUE )
    {
        sFlag = SMI_STATEMENT_NORMAL;
    }

    return sFlag;
}

inline IDE_RC mmcStatement::updateSystemSCN()
{
    smSCN sRequestSCN;

    getRequestSCN( &sRequestSCN );

    if ( SM_SCN_IS_NOT_INIT( sRequestSCN ) )
    {
        IDE_DASSERT( getSession()->isGCTx() == ID_TRUE );

        if ( getSession()->isGCTx() == ID_TRUE )
        {
            IDU_FIT_POINT( "mmcStatement::updateSystemSCN::statementBeginSync" );

            IDE_TEST( sdi::syncSystemSCN4GCTx( &sRequestSCN, NULL ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

