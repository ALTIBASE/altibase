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
 * $$Id:$
 **********************************************************************/
/******************************************************************************
 * Description :
 *    sdbFlushMgr은 static class로서 flusher들을 생성, 관리하며
 *    flush 량, waiting time, flush job scheduling 등을 담당한다.
 *    flush manager는 시스템에서 하나만 존재하며, 모든 flusher들을
 *    생성에서 소멸, 쓰레드 정지, 깨우기 등의 기능을 수행한다.
 *    시스템의 상황에따라 flush량을 조절하며 flusher들의 waiting time도
 *    동적으로 조절한다.
 *    또한 트랜잭션 쓰레드들이 victim을 찾다가 list warp를 하기전에
 *    flush job을 등록시켜주도록하는 인터페이스도 제공한다.
 *    등록된 replacement flush job이 없으면 flush manager는 flush list의
 *    길이를 보고 replacement flush를 할 것인지, checkpoint flush를 할 것인지
 *    결정한 후 flusher에게 flush 작업을 할당한다.
 *
 *    위 처럼 시스템이 주기적으로 수행하는 체크포인트를 system checkpoint라고 하고
 *    사용자의 입력에 의한 체크포인트를 user checkpoint라고 한다.
 *
 *    flush job의 우선 순위는 다음과 같다.
 *      1. 트랜잭션 쓰레드에 의해 등록된 replacement flush job 또는
 *         checkpoint 쓰레드에 의해 등록된 full checkpoint flush job
 *      2. flush list가 일정 길이 이상이 되었을 때 주기적으로 처리되는
 *         replacement flush job
 *      3. checkpoint flush job
 *
 * Implementation :
 *    flush manager는 flusher들에게 sdbFlushJob이라는 구조체를 매개로
 *    flush job을 할당한다.
 *    flush manager는 위의 1번 job들을 요청받기 위해 내부적으로 job queue를
 *    유지한다. 이때 사용되는 맴버가 mReqJobQueue이다.
 *    job queue는 array로 관리되며 mReqJobAddPos 위치에 넣고,
 *    mReqJobGetPos에서 job을 얻는다. job queue는 mReqJobMutex에 의해
 *    동시성이 제어된다.
 *    SDB_FLUSH_JOB_MAX보다 많은 job을 등록하려고 시도하면 등록이 실패한다.
 ******************************************************************************/
#include <sdbFlushMgr.h>
#include <smErrorCode.h>
#include <sdbReq.h>
#include <sdbBufferMgr.h>
#include <smrRecoveryMgr.h>

iduMutex       sdbFlushMgr::mReqJobMutex;
sdbFlushJob    sdbFlushMgr::mReqJobQueue[SDB_FLUSH_JOB_MAX];
UInt           sdbFlushMgr::mReqJobAddPos;
UInt           sdbFlushMgr::mReqJobGetPos;
iduLatch       sdbFlushMgr::mFCLatch;
sdbFlusher*    sdbFlushMgr::mFlushers;
UInt           sdbFlushMgr::mFlusherCount;
idvTime        sdbFlushMgr::mLastFlushedTime;
sdbCPListSet  *sdbFlushMgr::mCPListSet;

/******************************************************************************
 * Description :
 *    flush manager를 초기화한다.
 *    flusher의 개수를 인자로 받는다.
 *    초기화 과정중에 flusher 객체들을 생성한다.
 *
 *  aFlusherCount   - [IN]  구동시킬 플려셔 갯수
 ******************************************************************************/
IDE_RC sdbFlushMgr::initialize(UInt aFlusherCount)
{
    UInt   i = 0;
    UInt   sFlusherIdx = 0;
    idBool sNotStarted; 
    SInt   sState = 0; 

    mReqJobAddPos   = 0;
    mReqJobGetPos   = 0;
    mFlusherCount   = aFlusherCount;
    mCPListSet      = sdbBufferMgr::getPool()->getCPListSet();

    // job queue를 초기화한다.
    for (i = 0; i < SDB_FLUSH_JOB_MAX; i++)
    {
        initJob(&mReqJobQueue[i]);
    }

    IDE_TEST(mReqJobMutex.initialize(
                 (SChar*)"BUFFER_FLUSH_MANAGER_REQJOB_MUTEX",
                 IDU_MUTEX_KIND_NATIVE,
                 IDB_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_FLUSH_MANAGER_REQJOB)
             != IDE_SUCCESS);
    sState = 1;

    IDE_ASSERT( mFCLatch.initialize( (SChar*)"BUFFER_FLUSH_CONTROL_LATCH") 
               == IDE_SUCCESS); 
    sState = 2; 

    // 마지막 flush 시각을 세팅한다.
    IDV_TIME_GET(&mLastFlushedTime);

    /* TC/FIT/Limit/sm/sdb/sdbFlushMgr_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbFlushMgr::initialize::malloc",
                          insufficient_memory );

    // flusher들을 생성한다.
    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                      (ULong)ID_SIZEOF(sdbFlusher) * aFlusherCount,
                                      (void**)&mFlushers) != IDE_SUCCESS,
                    insufficient_memory );
                    
    sState = 3;

    for( sFlusherIdx = 0; sFlusherIdx < aFlusherCount; sFlusherIdx++ )
    {
        new (&mFlushers[sFlusherIdx]) sdbFlusher();

        // 현재는 모든 플러들이 같은 크기의 페이지 사이즈와
        // 같은 크기의 IOB 크기를 가진다.
        IDE_TEST(mFlushers[sFlusherIdx].initialize(
                    sFlusherIdx,
                    SD_PAGE_SIZE,
                    smuProperty::getBufferIOBufferSize(),
                    mCPListSet)
                != IDE_SUCCESS);

        mFlushers[sFlusherIdx].setDelayedFlushProperty( smuProperty::getDelayedFlushListPct(),
                                                        smuProperty::getDelayedFlushProtectionTimeMsec() );

        mFlushers[sFlusherIdx].start();
    }

    sState = 4;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch (sState) 
    { 
        case 4: 
        case 3: 
            for ( ; sFlusherIdx > 0; sFlusherIdx-- ) 
            { 
                IDE_ASSERT(mFlushers[sFlusherIdx-1].finish(NULL, &sNotStarted) 
                        == IDE_SUCCESS); 
                IDE_ASSERT(mFlushers[sFlusherIdx-1].destroy() == IDE_SUCCESS); 
            } 

            IDE_ASSERT(iduMemMgr::free(mFlushers) == IDE_SUCCESS); 
        case 2: 
            IDE_ASSERT(mFCLatch.destroy() == IDE_SUCCESS); 
        case 1: 
            IDE_ASSERT(mReqJobMutex.destroy() == IDE_SUCCESS); 
        case 0: 
        default: 
            break; 
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    모든 flusher들을 구동시킨다.
 ******************************************************************************/
void sdbFlushMgr::startUpFlushers()
{
    UInt i;

    for (i = 0; i < mFlusherCount; i++)
    {
        mFlushers[i].start();
    }
}

/******************************************************************************
 * Description :
 *    구동중인 모든 flusher들을 종료시키고 자원 해제시킨다.
 *
 ******************************************************************************/
IDE_RC sdbFlushMgr::destroy()
{
    UInt   i;
    idBool sNotStarted;

    for (i = 0; i < mFlusherCount; i++)
    {
        IDE_TEST(mFlushers[i].finish(NULL, &sNotStarted) != IDE_SUCCESS);
        IDE_TEST(mFlushers[i].destroy() != IDE_SUCCESS);
    }

    IDE_TEST(iduMemMgr::free(mFlushers) != IDE_SUCCESS);

    IDE_ASSERT(mFCLatch.destroy() == IDE_SUCCESS);

    IDE_ASSERT(mReqJobMutex.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    모든 flusher들을 깨운다. 이미 활동중인 flusher에 대해서는
 *    아무런 작업을 하지 않는다.
 *
 ******************************************************************************/
IDE_RC sdbFlushMgr::wakeUpAllFlushers()
{
    UInt   i;
    idBool sDummy;

    for (i = 0; i < mFlusherCount; i++)
    {
        IDE_TEST(mFlushers[i].wakeUpOnlyIfSleeping(&sDummy) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    Flusher들이 작업중인 작업이 종료될때까지 대기한다. 쉬고 있는 Flusher들은
 *    무시한다.
 *
 ******************************************************************************/
IDE_RC sdbFlushMgr::wait4AllFlusher2Do1JobDone()
{
    UInt   i;

    for (i = 0; i < mFlusherCount; i++)
    {
        mFlushers[i].wait4OneJobDone();
    }

    return IDE_SUCCESS;
}

/******************************************************************************
 * Description :
 *  flusher들중 가장 작은 recoveryLSN을 리턴한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRet        - [OUT] flusher들중 가장 작은 recoveryLSN을 리턴한다.
 ******************************************************************************/
void sdbFlushMgr::getMinRecoveryLSN(idvSQL *aStatistics,
                                    smLSN  *aRet)
{
    UInt   i;
    smLSN  sFlusherMinLSN;

    SM_LSN_MAX(*aRet);
    for (i = 0; i < mFlusherCount; i++)
    {
        mFlushers[i].getMinRecoveryLSN(aStatistics, &sFlusherMinLSN);
        if ( smLayerCallback::isLSNLT( &sFlusherMinLSN, aRet ) == ID_TRUE )
        {
            SM_GET_LSN(*aRet, sFlusherMinLSN);
        }
    }
}

/******************************************************************************
 * Description :
 *    job queue에 등록된 job을 하나 얻는다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴할 Job
 ******************************************************************************/
void sdbFlushMgr::getReqJob(idvSQL      *aStatistics,
                            sdbFlushJob *aRetJob)
{
    sdbFlushJob *sJob = &mReqJobQueue[mReqJobGetPos];

    if (sJob->mType == SDB_FLUSH_JOB_NOTHING)
    {
        aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
    }
    else
    {
        IDE_ASSERT(mReqJobMutex.lock(aStatistics) == IDE_SUCCESS);

        // lock을 잡은 뒤에 다시 받아와야 한다.
        sJob = &mReqJobQueue[mReqJobGetPos];

        if (sJob->mType == SDB_FLUSH_JOB_NOTHING)
        {
            aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
        }
        else
        {
            // req job이 있다.
            *aRetJob    = *sJob;
            sJob->mType = SDB_FLUSH_JOB_NOTHING;

            // get position을 하나 진행시킨다.
            incPos(&mReqJobGetPos);
        }

        IDE_ASSERT(mReqJobMutex.unlock() == IDE_SUCCESS);
    }
}

/*****************************************************************************
 * Description :
 *    flusher에 의해 호출되며 flusher가 할일을 반환한다.
 *    flusher가 할일은 다음과 같으며 번호는 우선순위이다.
 *    1.    requested  object flushjob
 *             이 job은 pageout/flush page 등으로 사용자의 명시적인 요청으로
 *             등록된다. addReqObjectFlushJob()를 통해 등록된다. 
 *         requested checkpoint flush job
 *           : 위의 requested object flush job과 동등한 우선순위를 가지며
 *             먼저 등록된 순으로 처리된다.
 *             이 job은 checkpoint 쓰레드가 checkpoint 시점이 너무 늦어졌다고
 *             판단했을 경우 등록되거나 사용자의 명시적 checkpoint 명령시
 *             등록된다. addReqCPFlushJob()을 통해 등록된다.
 *      2. main replacement flush job
 *           : flusher들이 주기적으로 돌면서 flush list의 길이가
 *             일정 수치를 넘어서면 flush하게 되는데 이 작업을 말한다.
 *             이 작업은 트랜잭션 쓰레드에 의해 등록되는게 아니라
 *             flusher들에 의해 등록되고 처리된다.
 *      3. system checkpoint flush job
 *           : 1, 2번 작업이 없을 경우 system checkpoint flush를 수행한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴할 job
 *****************************************************************************/
void sdbFlushMgr::getJob(idvSQL      *aStatistics,
                         sdbFlushJob *aRetJob)
{
    getReqJob(aStatistics, aRetJob);

    if (aRetJob->mType == SDB_FLUSH_JOB_NOTHING)
    {
        getReplaceFlushJob(aRetJob);

        if (aRetJob->mType == SDB_FLUSH_JOB_NOTHING)
        {
            // checkpoint flush job을 구한다.
            getCPFlushJob(aStatistics, aRetJob);
        }
        else
        {
            // replace flush job을 하나 얻었다.
        }
    }
    else
    {
        // req job을 하나 얻었다.
    }
}

/******************************************************************************
 * Description :
 *    PROJ-2669
 *      Delayed Flush List 에 BCB가 있을 경우 Job을 반환한다.
 *      Flush 대상 Flush list 는 Delayed Flush list 를 설정한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴되는 job
 ******************************************************************************/
void sdbFlushMgr::getDelayedFlushJob( sdbFlushJob *aColdJob,
                                      sdbFlushJob *aRetJob)
{
    sdbFlushList  *sNormalFlushList;
    sdbFlushList  *sDelayedFlushList;
    sdbBufferPool *sPool;
    ULong          sFlushCount;

    initJob(aRetJob);

    sPool             = sdbBufferMgr::getPool();
    sNormalFlushList  = aColdJob->mFlushJobParam.mReplaceFlush.mFlushList;
    sDelayedFlushList = sPool->getDelayedFlushList( sNormalFlushList->getID() );
    sFlushCount       = sDelayedFlushList->getPartialLength();

    if ( sFlushCount != 0 )
    {
        aRetJob->mType          = SDB_FLUSH_JOB_REPLACEMENT_FLUSH;
        aRetJob->mReqFlushCount = SDB_FLUSH_COUNT_UNLIMITED;
        aRetJob->mFlushJobParam.mReplaceFlush.mLRUList
                                = sPool->getMinLengthLRUList();
        aRetJob->mFlushJobParam.mReplaceFlush.mFlushList
                                = sDelayedFlushList;
    }
    else
    {
        /* nothing to do */
    }
}

/*****************************************************************************
 * Description :
 *   이 함수는 checkpoint thread가 recovery LSN이 너무 오랫동안
 *   갱신되지 않아 checkpoint flush를 할 필요가 있다고 판단한 경우
 *   호출된다.
 *   jobDone을 옵션으로 줄 수 있는데 이 값을 세팅하면 job이 완료되었을 때
 *   어떤 처리를 수행할 수 있다. job 완료를 통보받을 필요없으면
 *   이 인자에 NULL을 세팅하면 된다.
 *
 *  aStatistics       - [IN]  통계정보
 *  aReqFlushCount    - [IN]  flush 수행 되길 원하는 최소 페이지 개수
 *  aRedoPageCount    - [IN]  Restart Recovery시에 Redo할 Page 수,
 *                            반대로 말하면 CP List에 남겨둘 Dirty Page의 수
 *  aRedoLogFileCount - [IN]  Restart Recovery시에 Redo할 LogFile 수,
 *                            반대로 BufferPool에 남겨둘 LSN 간격 (LogFile수 기준)
 *  aJobDoneParam     - [IN]  job 완료를 통보 받기를 원할때, 이 파라미터를 설정한다.
 *  aAdded            - [OUT] job 등록 성공 여부. 만약 SDB_FLUSH_JOB_MAX보다 많은
 *                            job을 등록하려고 시도하면 등록이 실패한다.
 *****************************************************************************/
void sdbFlushMgr::addReqCPFlushJob(
        idvSQL                     * aStatistics,
        ULong                        aMinFlushCount,
        ULong                        aRedoPageCount,
        UInt                         aRedoLogFileCount,
        sdbFlushJobDoneNotifyParam * aJobDoneParam,
        idBool                     * aAdded)
{
    sdbFlushJob *sCurr;

    IDE_ASSERT(mReqJobMutex.lock(aStatistics) == IDE_SUCCESS);

    sCurr = &mReqJobQueue[mReqJobAddPos];

    if (sCurr->mType == SDB_FLUSH_JOB_NOTHING)
    {
        if (aJobDoneParam != NULL)
        {
            initJobDoneNotifyParam(aJobDoneParam);
        }

        initJob(sCurr);
        sCurr->mType          = SDB_FLUSH_JOB_CHECKPOINT_FLUSH;
        sCurr->mReqFlushCount = aMinFlushCount;
        sCurr->mJobDoneParam  = aJobDoneParam;
        sCurr->mFlushJobParam.mChkptFlush.mRedoPageCount    = aRedoPageCount;
        sCurr->mFlushJobParam.mChkptFlush.mRedoLogFileCount = aRedoLogFileCount;
        sCurr->mFlushJobParam.mChkptFlush.mCheckpointType   = SDB_CHECKPOINT_BY_CHKPT_THREAD;

        incPos(&mReqJobAddPos);
        *aAdded = ID_TRUE;
    }
    else
    {
        *aAdded = ID_FALSE;
    }

    IDE_ASSERT(mReqJobMutex.unlock() == IDE_SUCCESS);
}

/*****************************************************************************
 * Description :
 *   이 함수는 tablespace 또는 table이 drop 또는 offline되었을 때,
 *   이 tablespace나 table에 대해서 flush 작업을 할 때 사용된다.
 *   jobDone을 옵션으로 줄 수 있는데 이 값을 세팅하면 job이 완료되었을 때
 *   어떤 처리를 수행할 수 있다. job 완료를 통보받을 필요없으면
 *   이 인자에 NULL을 세팅하면 된다.
 *
 * Implementation:
 *
 *  aStatistics - [IN]  통계정보
 *  aBCBQueue   - [IN]  flush를 수행할 BCB포인터가 들어있는 큐
 *  aFiltFunc   - [IN]  aBCBQueue에 있는 BCB중 aFiltFunc조건에 맞는 BCB만 flush
 *  aFiltObj    - [IN]  aFiltFunc에 파라미터로 넣어주는 값
 *  aJobDoneParam-[IN]  job 완료를 통보 받기를 원할때, 이 파라미터를 설정한다.
 *  aAdded      - [OUT] job 등록 성공 여부. 만약 SDB_FLUSH_JOB_MAX보다 많은
 *                      job을 등록하려고 시도하면 등록이 실패한다.
 *****************************************************************************/
void sdbFlushMgr::addReqDBObjectFlushJob(
        idvSQL                     *aStatistics,
        smuQueueMgr                *aBCBQueue,
        sdbFiltFunc                 aFiltFunc,
        void                       *aFiltObj,
        sdbFlushJobDoneNotifyParam *aJobDoneParam,
        idBool                     *aAdded)
{
    sdbFlushJob *sCurr;

    IDE_ASSERT(mReqJobMutex.lock(aStatistics) == IDE_SUCCESS);

    sCurr = &mReqJobQueue[mReqJobAddPos];
    if (sCurr->mType == SDB_FLUSH_JOB_NOTHING)
    {
        if (aJobDoneParam != NULL)
        {
            initJobDoneNotifyParam(aJobDoneParam);
        }

        initJob(sCurr);
        sCurr->mType         = SDB_FLUSH_JOB_DBOBJECT_FLUSH;
        sCurr->mJobDoneParam = aJobDoneParam;
        sCurr->mFlushJobParam.mObjectFlush.mBCBQueue = aBCBQueue;
        sCurr->mFlushJobParam.mObjectFlush.mFiltFunc = aFiltFunc;
        sCurr->mFlushJobParam.mObjectFlush.mFiltObj  = aFiltObj;

        incPos(&mReqJobAddPos);
        *aAdded = ID_TRUE;
    }
    else
    {
        *aAdded = ID_FALSE;
    }

    IDE_ASSERT(mReqJobMutex.unlock() == IDE_SUCCESS);
}

/******************************************************************************
 * Description :
 *    main job을 하나 얻는다. main job이란 flush list가 일정 길이가 초과되면
 *    flush 대상으로 간주하고 replacement flush job을 생성하여 반환한다.
 *
 * Implementation :
 *    pool의 flush list들 중에 가장 긴 list를 얻어와 flush 길이를 초과하는지
 *    검사하여 초과되면 flush job으로 생성한 후 반환한다.
 *    이 알고리즘에 의하면 동시에 여러 flusher들이 한 flush list에 대해서만
 *    flush할 수도 있다. 하지만 flush list의 길이가 줄어들면 flusher들은
 *    다른 flush list에 대해 작업을 수행할 것이다.
 *    한 flush list에 대해서 여러개의 flusher가 flush 작업을 수행해도
 *    성능이 떨어지지 않게끔 설계되어 있다.
 *
 *    aRetJob   - [OUT] 리턴되는 job
 ******************************************************************************/
void sdbFlushMgr::getReplaceFlushJob(sdbFlushJob *aRetJob)
{
    sdbFlushList  *sFlushList;
    sdbBufferPool *sPool;
    
    sPool = sdbBufferMgr::getPool();
    sFlushList = sPool->getMaxLengthFlushList();

    initJob(aRetJob);

    /* BUG-24303 Dirty 페이지가 없어도 Wait없이 Checkpoint Flush Job이 무한 발생
     *
     * Replacement Flush가 발생하는 조건을 다시 정리하자는 것이다.
     *
     * 서버가 Busy하다고 판단하는 경우로 HIGH_FLUSH_PCT 이상인 경우
     * 만 고려하였는데, 이것뿐만 아니라 LOW_FLUSH_PCT 이상 LOW_RREPARE_PCT
     * 이하 인 경우이다. 즉, 이 조건은 Replacement Flush가 발생할 수 있는 조건과
     * 동치이다.
     *
     * 문제는 서버가 Replacement Flush가 발생할 수 있는 조건에서 Busy하다고 판단
     * 하고는 쉬지않고 Flush Job을 발생시키는데 이때, 조건검사 누락으로 
     * Replacement Flush는 발생되지 않고, 계속 Checkpoint List Flush만을 
     * 생성하여 발생하므로 해서 CPU를 최대로 사용하는 문제가 발생한다.
     * ( ChkptList Flush에 의해서 Flush List 길이가 줄어들지 않는 것은
     * Flush이후 Dirty 상태가 해제되어도 Flush List에 존재할 수 있기 때문이다. )
     *
     * 그러므로, getJob시 Replacement Flush Job을 생성하도록 해서 Flush
     * List의 clean BCB가 prepare list로 이동시켜, Busy 조건을 만족하지 않도록
     * 해야한다.
     */
    if ( isCond4ReplaceFlush() == ID_TRUE )
    {
        aRetJob->mType          = SDB_FLUSH_JOB_REPLACEMENT_FLUSH;
        aRetJob->mReqFlushCount = SDB_FLUSH_COUNT_UNLIMITED;
        aRetJob->mFlushJobParam.mReplaceFlush.mFlushList
                                = sFlushList;
        aRetJob->mFlushJobParam.mReplaceFlush.mLRUList
                                = sPool->getMinLengthLRUList();
    }
    else
    {
        aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
    }
}


/******************************************************************************
 * Description :
 *   incremental checkpoint를 수행하는 job을 얻는다.
 *   이 함수를 수행한다고 해서 그냥 job을 주지는 않는다. 즉, 어떤 조건을 만족
 *   해야 하는데, 그조건은 아래 둘중 하나를 만족 할 때이다.
 *   1. 마지막 checkpoint 수행 후 시간이 많이 지났다
 *   2. Recovery LSN이 너무 작을때
 *
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴되는 job
 ******************************************************************************/
void sdbFlushMgr::getCPFlushJob(idvSQL *aStatistics, sdbFlushJob *aRetJob)
{
    smLSN       sMinLSN;
    idvTime     sCurrentTime;
    ULong       sTime;

    mCPListSet->getMinRecoveryLSN(aStatistics, &sMinLSN);

    IDV_TIME_GET(&sCurrentTime);
    sTime = IDV_TIME_DIFF_MICRO(&mLastFlushedTime, &sCurrentTime) / 1000000;

    if ( ( smLayerCallback::isCheckpointFlushNeeded(sMinLSN) == ID_TRUE ) ||
         ( sTime >= smuProperty::getCheckpointFlushMaxWaitSec() ) )
    {
        initJob(aRetJob);
        aRetJob->mType          = SDB_FLUSH_JOB_CHECKPOINT_FLUSH;
        aRetJob->mReqFlushCount = smuProperty::getCheckpointFlushCount();
        aRetJob->mFlushJobParam.mChkptFlush.mRedoPageCount
                                = smuProperty::getFastStartIoTarget();
        aRetJob->mFlushJobParam.mChkptFlush.mRedoLogFileCount
                                = smuProperty::getFastStartLogFileTarget();
        aRetJob->mFlushJobParam.mChkptFlush.mCheckpointType 
                                = SDB_CHECKPOINT_BY_FLUSH_THREAD;
    }
    else
    {
        aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
    }
}

/******************************************************************************
 * Description :
 *    flusher 쓰레드를 종료시킨다. 즉 flusher 하나를 정지시킨다.
 *    쓰레드만 종료할 뿐 자원은 해제하지 않는다.
 *    정지된 flusher는 turnOnFlusher()로 다시 구동시킬 수 있다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFlusherID  - [IN]  flusher id
 ******************************************************************************/
IDE_RC sdbFlushMgr::turnOffFlusher(idvSQL *aStatistics, UInt aFlusherID)
{
    idBool sNotStarted;
    idBool sLatchLocked = ID_FALSE;

    IDE_DASSERT(aFlusherID < mFlusherCount);

    ideLog::log( IDE_SM_0, "[PREPARE] Stop flusher ID(%d)\n", aFlusherID );

    // BUG-26476 
    // checkpoint가 발생해서 checkpoint flush job이 수행되기를 
    // 대기 하고 있는 쓰레드가 있을 수 있다. 그런 경우 
    // flusher를 끄면 대기하던 쓰레드가 무한정 대기할 수 
    // 있기 때문에 이땐 대기하는 쓰레드가 깨어나기까지 기대려야 한다. 

    IDE_ASSERT(mFCLatch.lockWrite(NULL, 
                                  NULL) 
               == IDE_SUCCESS); 
    sLatchLocked = ID_TRUE;

    IDE_TEST(mFlushers[aFlusherID].finish(aStatistics,
                                          &sNotStarted) != IDE_SUCCESS);

    sLatchLocked = ID_FALSE;
    IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS);

    ideLog::log( IDE_SM_0, "[SUCCESS] Stop flusher ID(%d)\n", aFlusherID );

    IDE_TEST_RAISE(sNotStarted == ID_TRUE,
                   ERROR_FLUSHER_NOT_STARTED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERROR_FLUSHER_NOT_STARTED);
    {
        ideLog::log( IDE_SM_0, 
                "[WARNING] flusher ID(%d) already stopped.\n", aFlusherID );

        IDE_SET(ideSetErrorCode(smERR_ABORT_sdbFlusherNotStarted,
                                aFlusherID));
    }
    IDE_EXCEPTION_END;

    if (sLatchLocked == ID_TRUE)
    {
        sLatchLocked = ID_FALSE;
        IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS);
    }

    ideLog::log( IDE_SM_0, "[FAILURE] Stop flusher ID(%d)\n", aFlusherID );

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    종료된 flusher를 다시 재구동시킨다.
 *
 *  aFlusherID  - [IN]  flusher id
 ******************************************************************************/
IDE_RC sdbFlushMgr::turnOnFlusher(UInt aFlusherID)
{
    IDE_DASSERT(aFlusherID < mFlusherCount);

    ideLog::log( IDE_SM_0, "[PREPARE] Start flusher ID(%d)\n", aFlusherID );

    IDE_ASSERT(mFCLatch.lockWrite(NULL,
                                  NULL)
               == IDE_SUCCESS);

    IDE_TEST_RAISE(mFlushers[aFlusherID].isRunning() == ID_TRUE,
                   ERROR_FLUSHER_RUNNING);

    IDE_TEST(mFlushers[aFlusherID].start() != IDE_SUCCESS);

    IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS);

    ideLog::log( IDE_SM_0, "[SUCCESS] Start flusher ID(%d)\n", aFlusherID );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERROR_FLUSHER_RUNNING);
    {
        ideLog::log( IDE_SM_0, 
                "[WARNING] flusher ID(%d) already started\n", aFlusherID );

        IDE_SET(ideSetErrorCode(smERR_ABORT_sdbFlusherRunning, aFlusherID));
    }
    IDE_EXCEPTION_END;

    IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS);

    ideLog::log( IDE_SM_0, "[FAILURE] Start flusher ID(%d)\n", aFlusherID );

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    종료된모든 flusher를 다시 재구동시킨다.
 *
 ******************************************************************************/
IDE_RC sdbFlushMgr::turnOnAllflushers()
{
    UInt i;

    IDE_ASSERT(mFCLatch.lockWrite(NULL, 
                                  NULL) 
               == IDE_SUCCESS); 

    for (i = 0; i < mFlusherCount; i++)
    {
        if( mFlushers[i].isRunning() != ID_TRUE )
	{
	   IDE_TEST(mFlushers[i].start() != IDE_SUCCESS);
	}
    }

    IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS);

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  사용자가 요청한 flush작업 요청이 끝났는지 확인하는 수단인
 *  sdbFlushJobDoneNotifyParam 을 초기화 한다.
 *
 *  aParam  - [IN]  초기화할 변수
 ******************************************************************************/
IDE_RC sdbFlushMgr::initJobDoneNotifyParam(sdbFlushJobDoneNotifyParam *aParam)
{
    IDE_TEST(aParam->mMutex.initialize((SChar*)"BUFFER_FLUSH_JOB_COND_WAIT_MUTEX",
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_LATCH_FREE_OTHERS)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(aParam->mCondVar.initialize((SChar*)"BUFFER_FLUSH_JOB_COND_WAIT_COND") != IDE_SUCCESS, err_cond_init);

    aParam->mJobDone = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  초기화된 sdbFlushJobDoneNotifyParam을 바탕으로
 *  요청된 작업이 종료하기를 대기한다.
 *
 *  aParam  - [IN]  대기를 위해 필요한 변수
 ******************************************************************************/
IDE_RC sdbFlushMgr::waitForJobDone(sdbFlushJobDoneNotifyParam *aParam)
{
    IDE_ASSERT(aParam->mMutex.lock(NULL) == IDE_SUCCESS);

    /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                 wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
    while( aParam->mJobDone == ID_FALSE )
    {
        /* 등록한 job이 처리 완료될때까지 대기한다.
         * cond_wait을 수행함과 동시에 mMutex를 푼다. */
        IDE_TEST_RAISE(aParam->mCondVar.wait(&(aParam->mMutex))
                       != IDE_SUCCESS, err_cond_wait);
    }

    IDE_ASSERT(aParam->mMutex.unlock() == IDE_SUCCESS);

    IDE_TEST_RAISE(aParam->mCondVar.destroy() != IDE_SUCCESS,
                   err_cond_destroy);

    IDE_TEST(aParam->mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION(err_cond_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    CPListSet의 dirty 버퍼들을 flush한다.
 *    flush가 완료될 때까지 이 함수를 빠져나가지 않는다.
 *
 * Implementation :
 *    이 함수는 의외로 복잡하다. checkpoint flush job을 등록하고
 *    이 job이 처리 완료될 때까지 기다리기 위해, mutex를 생성하고
 *    condition variable을 생성하여 cond_wait을 하게된다.
 *    이 대기 상태에 있다가 flusher가 이때 요청된 job을 처리 완료하면
 *    notifyJobDone()을 호출하게 되는데 그러면 cond_signal()을 호출하게 되어
 *    cond_wait에서 풀려나게 되고 그러면 비로소 이 함수를 빠져나가게 된다.
 *
 *  aStatistics     - [IN]  통계정보
 *  aFlushAll       - [IN]  check point list내의 모든 BCB를 flush 하려면
 *                          이 변수를 ID_TRUE로 설정한다.
 ******************************************************************************/
IDE_RC sdbFlushMgr::flushDirtyPagesInCPList(idvSQL    * aStatistics,
                                            idBool      aFlushAll,
                                            idBool      aIsChkptThread )
{
    idBool                      sAdded          = ID_FALSE;
    sdbFlushJobDoneNotifyParam  sJobDoneParam;
    PDL_Time_Value              sTV;
    ULong                       sMinFlushCount;
    ULong                       sRedoPageCount;
    UInt                        sRedoLogFileCount;
    ULong                       sRetFlushedCount;
    idBool                      sLatchLocked    = ID_FALSE; 

    /* BUG-26476 
     * checkpoint 수행시 flusher들이 stop되어 있으면 skip을 해야 한다. 
     * 하지만 checkpoint에 의해 checkpoint flush가 수행되고 있는 동안 
     * flush가 stop되면 안된다. */
    IDE_ASSERT(mFCLatch.lockRead(NULL, 
                                 NULL) 
               == IDE_SUCCESS); 
    sLatchLocked = ID_TRUE; 

    if ( getActiveFlusherCount() == 0 )
    { 
        IDE_CONT(SKIP_FLUSH); 
    } 

    if (aFlushAll == ID_FALSE)
    {
        sMinFlushCount    = smuProperty::getCheckpointFlushCount();
        sRedoPageCount    = smuProperty::getFastStartIoTarget();
        sRedoLogFileCount = smuProperty::getFastStartLogFileTarget();
    }
    else
    {
        sMinFlushCount    = mCPListSet->getTotalBCBCnt();
        sRedoPageCount    = 0; // DirtyPage를 BufferPool에 남겨두지 않는다.
        sRedoLogFileCount = 0;
    }

    if ( (aIsChkptThread == ID_TRUE) &&
         (smuProperty::getCheckpointFlushResponsibility() == 1) )
    {
        IDE_TEST( smLayerCallback::flushForCheckpoint( aStatistics,
                                                       sMinFlushCount,
                                                       sRedoPageCount,
                                                       sRedoLogFileCount,
                                                       &sRetFlushedCount )
                  != IDE_SUCCESS );

    }
    else
    {
        while (sAdded == ID_FALSE)
        {
            addReqCPFlushJob(aStatistics,
                             sMinFlushCount,
                             sRedoPageCount,
                             sRedoLogFileCount,
                             &sJobDoneParam,
                             &sAdded);
            IDE_TEST(wakeUpAllFlushers() != IDE_SUCCESS);
            sTV.set(0, 50000);
            idlOS::sleep(sTV);
        }
 
        IDE_TEST(waitForJobDone(&sJobDoneParam) != IDE_SUCCESS);
    }
    IDE_EXCEPTION_CONT(SKIP_FLUSH); 

    sLatchLocked = ID_FALSE; 
    IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sLatchLocked == ID_TRUE) 
    { 
        IDE_ASSERT(mFCLatch.unlock() == IDE_SUCCESS); 
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * BCB 포인터가 들어있는 queue와 그 BCB들을 flush 해야 할지 말지
 * 결정할 수 있는 aFiltFunc 및 aFiltObj를 바탕으로
 * 플러시를 수행한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCBQueue   - [IN]  flush할 대상이 들어 있는 BCB의 포인터가 있는 queue
 *  aFiltFunc   - [IN]  aBCBQueue에 있는 BCB중 aFiltFunc조건에 맞는 BCB만 flush
 *  aFiltObj    - [IN]  aFiltFunc에 파라미터로 넣어주는 값
 ******************************************************************************/
IDE_RC sdbFlushMgr::flushObjectDirtyPages(idvSQL       *aStatistics,
                                          smuQueueMgr  *aBCBQueue,
                                          sdbFiltFunc   aFiltFunc,
                                          void         *aFiltObj)
{
    idBool                     sAdded = ID_FALSE;
    sdbFlushJobDoneNotifyParam sJobDoneParam;
    PDL_Time_Value             sTV;

    while (sAdded == ID_FALSE)
    {
        addReqDBObjectFlushJob(aStatistics,
                               aBCBQueue,
                               aFiltFunc,
                               aFiltObj,
                               &sJobDoneParam,
                               &sAdded);
        IDE_TEST(wakeUpAllFlushers() != IDE_SUCCESS);
        sTV.set(0, 50000);
        idlOS::sleep(sTV);
    }

    IDE_TEST(waitForJobDone(&sJobDoneParam) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    job을 처리했음을 알린다. 만약 jobDone을 세팅하지 않았으면
 *    이 함수안에서 아무런 작업도 하지 않는다.
 *    jobDone은 job을 등록할 때 옵션으로 줄 수 있다.
 *
 * aParam   - [IN]  이 변수에 대기하고 있는 쓰레드들에게 job이 종료 되었음을
 *                  알린다.
 ******************************************************************************/
void sdbFlushMgr::notifyJobDone(sdbFlushJobDoneNotifyParam *aParam)
{
    if (aParam != NULL)
    {
        IDE_ASSERT(aParam->mMutex.lock(NULL) == IDE_SUCCESS);

        IDE_ASSERT(aParam->mCondVar.signal() == 0);

        aParam->mJobDone = ID_TRUE;

        IDE_ASSERT(aParam->mMutex.unlock() == IDE_SUCCESS);
    }
}

/******************************************************************************
 * Description :
 *    flush list의 길이가 현 함수의 리턴값 이상이되면, 많은 양의 BCB에 대해
 *    flush를 수행한다.
 *    자세한 용도는 sdbFlushMgr::isCond4ReplaceFlush 을 보면 알수 있다.
 ******************************************************************************/
ULong sdbFlushMgr::getLowFlushLstLen4FF()
{
    sdbBufferPool *sPool;
    ULong          sPoolSizePerFlushList;
    ULong          sRet;

    sPool = sdbBufferMgr::getPool();

    sPoolSizePerFlushList = sPool->getPoolSize() / sPool->getFlushListCount();
    // LOW_FLUSH_PCT 1%
    sRet = (sPoolSizePerFlushList * smuProperty::getLowFlushPCT()) / 100;

    return (sRet == 0) ? 1 : sRet;
}

/******************************************************************************
 * Description :
 *    flush list의 길이가 현 함수의 리턴값 이상이되면 많은 양의 BCB에 대해
 *    flush를 수행한다.
 *    자세한 용도는 sdbFlushMgr::isCond4ReplaceFlush을 보면 알수 있다.
 ******************************************************************************/
ULong sdbFlushMgr::getHighFlushLstLen4FF()
{
    sdbBufferPool  *sPool;
    ULong           sPoolSizePerFlushList;
    ULong           sRet;

    sPool = sdbBufferMgr::getPool();
    sPoolSizePerFlushList = sPool->getPoolSize() / sPool->getFlushListCount();
    // HIGH_FLUSH_PCT 5%
    sRet = (sPoolSizePerFlushList * smuProperty::getHighFlushPCT()) / 100;

    return (sRet == 0) ? 1 : sRet;
}

/******************************************************************************
 * Description :
 *    prepare list의 길이가 이 길이 이하가 되면 많은 양의 BCB에 대해
 *    flush를 수행한다.
 *    자세한 용도는 sdbFlushMgr::isCond4ReplaceFlush을 보면 알 수 있다.
 ******************************************************************************/
ULong sdbFlushMgr:: getLowPrepareLstLen4FF()
{
    sdbBufferPool  *sPool;
    ULong           sPoolSizePerPrepareList;

    sPool = sdbBufferMgr::getPool();
    sPoolSizePerPrepareList = sPool->getPoolSize() / sPool->getPrepareListCount();
    // LOW_PREPARE_PCT 1%
    return (sPoolSizePerPrepareList * smuProperty::getLowPreparePCT()) / 100;
}

/******************************************************************************
 *
 * Description : Replacement Flush 의 조건
 *
 * RF가 발생할 수 있는 조건은 다음과 같다.
 *
 * 첫번째, 최대길이의 Flush List의 길이가 전체버퍼 대비 HIGH_FLUSH_PCT(기본값5%)
 *        이상인 경우를 만족한다.
 *
 * 두번째, 최소길이의 Prepared List의 길이가 전체버퍼 대비 LOW_PREPARE_PCT(기본값1%) 이하
 *        이면서 최대길이의 Flush List의 길이가 전체버퍼 대비 LOW_FLUSH_PCT(기본값1%) 이상
 *        인 경우를 만족한다.
 *
 ******************************************************************************/
idBool sdbFlushMgr::isCond4ReplaceFlush()
{
    sdbBufferPool  *sPool;
    ULong           sPrepareLength;
    ULong           sTotalLength;
    ULong           sNormalFlushLength;
    sdbFlushList   *sFlushList;

    sPool = sdbBufferMgr::getPool();

    /* PROJ-2669
     * 기존 Flush List 에 Delayed Flush List 가 추가되었으므로
     * Delayed/Normal 갯수를 합쳐주는 함수를 사용해야 한다.      */
    sFlushList = sPool->getMaxLengthFlushList();

    sNormalFlushLength = sFlushList->getPartialLength();
    sTotalLength = sPool->getFlushListLength( sFlushList->getID() );

    /* PROJ-2669
     * Normal Flush List 의 Length==0 이라면
     * 현재 유입되고 있는 Dirty Page가 없으므로
     * Replacement Flush Condition 으로 판정하지 않아도 된다. */
    if ( sNormalFlushLength != 0 )
    {
        if ( sTotalLength > getHighFlushLstLen4FF() )
        {
            return ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        sPrepareLength = sPool->getMinLengthPrepareList()->getLength();

        if ( ( sPrepareLength < getLowPrepareLstLen4FF() ) &&
             ( sTotalLength > getLowFlushLstLen4FF() ) )
        {
            return ID_TRUE;
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

    return ID_FALSE;
}

/******************************************************************************
 * Description :
 *    flusher가 job을 처리하고 다음 job을 처리할 때까지 대기할 시간을
 *    반환한다. 시스템의 상황에 따라 가변적이다. flush할게 많으면
 *    대기 시간은 짧고 할게 별로 없으면 대기 시간은 길어진다.
 *
 ******************************************************************************/
idBool sdbFlushMgr::isBusyCondition()
{
    idBool sRet = ID_FALSE;

    if (mReqJobQueue[mReqJobGetPos].mType != SDB_FLUSH_JOB_NOTHING)
    {
        sRet = ID_TRUE;
    }

    if ( isCond4ReplaceFlush() == ID_TRUE )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

/******************************************************************************
 * Abstraction : 현재 가동중인 flusher의 수를 반환한다.
 ******************************************************************************/
UInt sdbFlushMgr::getActiveFlusherCount()
{
    UInt i;
    UInt sActiveCount = 0;

    for( i = 0 ; i < mFlusherCount ; i++ )
    {
        if( mFlushers[ i ].isRunning() == ID_TRUE )
        {
            sActiveCount++;
        }
    }
    return sActiveCount;
}

/******************************************************************************
 * Description :
 *  PROJ-2669
 *   Delayed Flush 기능에 관련된 프로퍼티 설정 함수
 ******************************************************************************/
void sdbFlushMgr::setDelayedFlushProperty( UInt aDelayedFlushListPct,
                                           UInt aDelayedFlushProtectionTimeMsec )
{
    UInt i;

    for ( i = 0 ; i < mFlusherCount ; i++ )
    {
        mFlushers[ i ].setDelayedFlushProperty( aDelayedFlushListPct,
                                                aDelayedFlushProtectionTimeMsec );
    }
}

