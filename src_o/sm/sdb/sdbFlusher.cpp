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
 
/******************************************************************************
 * Description :
 *    sdbFlusher 객체는 시스템 쓰레드로서 버퍼 풀의 dirty 버퍼들을 flush하는
 *    역할을 한다. flush 종류에는 replacement flush와 checkpoint flush 두가지가
 *    있는데 replacement flush는 flush list에 있는 dirty 버퍼들을 flush함으로써
 *    replace 발생시 victim을 빨리 찾도록 해주는 역할을 하고, checkpoint flush는
 *    checkpoint list 상의 dirty 버퍼들을 flush함으로써 redo LSN을 당겨
 *    restart시 빠른 구동을 하게 해주는 역할을 한다.
 *    flusher는 주기적으로 돌면서 이 두가지 flush 작업을 수행한다.
 *    그리고 flusher는 일정시간을 쉰 후 flush 작업을 수행하는데,
 *    timeover로 깨어날 수도 있고 외부 signal에 의해 깨어날 수도 있다.
 *    외부 signal이란 트랜잭션 쓰레드가 victim을 찾다가 실패한 경우
 *    발생된다.
 *
 * Implementation :
 *    flusher는 replacement flush와 checkpoint flush를 sdbFlushMgr로부터
 *    job 형태로 받아서 수행한다. flusher는 일정시간 cond_wait을 하다가
 *    timeover 또는 signal을 받고 깨어나 job을 하나 얻어와서 처리한다.
 *    각 flusher 객체는 내부적으로 IO buffer를 유지하는데 이를 IOB라고 한다.
 *    IOB는 버퍼를 디스크에 기록하기 전에 복사하는 메모리 공간이다.
 *    하나의 버퍼를 flush하는 과정은 다음과 같다.
 *     1. checkpoint list 또는 flush list로부터 flush할 버퍼를 찾는다.
 *     2. flush 조건을 만족하면 page latch를 S-mode로 획득한다.
 *     3. BCBMutex를 잡고 mState를 INIOB 상태로 바꾼 후 BCBMutex를 푼다.
 *        이 순간부터 다른 flusher들은 이 버퍼를 flush 대상으로 삼을 수 없다.
 *        S-latch를 잡고 있기 때문에 redirty도 될 수 없다.
 *     4. 이 버퍼의 recovery LSN을 flusher의 min recovery LSN에 반영한다.
 *     5. checkpoint list에서 이 버퍼를 제거한다.
 *     6. IOB에 버퍼의 내용을 복사한다.
 *     7. page latch를 푼다.
 *        이때부터는 버퍼가 redirty될 수 있다. redirty가 되면
 *        버퍼는 다시 checkpoint list에 달린다. 하지만 redirty인 버퍼는
 *        다른 flusher들이 flush할 수 없다.
 *     8. IOB를 디스크에 기록한다.
 *     9. 버퍼의 상태를 clean 상태로 바꾼다.
 *     10. replacement flush인 경우 이 버퍼를 prepare list로 옮긴다.
 *
 *    여기서 4번에서 언급한 min recovery LSN에 대해 설명할 필요가 있다.
 *    checkpoint가 발생하면 디스크에 기록되지 않은 dirty 버퍼들 중에
 *    가장 작은 recovery LSN을 restart redo LSN으로 설정해야 한다.
 *    하지만 checkpoint list에서 dirty 버퍼를 빼는 순간 이 버퍼의 recovery LSN을
 *    checkpoint에 반영할 수 없게 된다. 그렇다고 이 버퍼가 디스크에 기록된 건
 *    아니다. 따라서 버퍼가 IOB에 존재하는 동안 checkpoint가 checkpoint list
 *    뿐만아니라 IOB에 기록된 버퍼들까지 모두 고려하여 최소값의 recovery LSN을
 *    유지해야 한다. 그러기 위해서 sdbFlusher 객체는 mMinRecoveryLSN이란
 *    맴버 변수를 유지한다. 이 맴버는 IOB에 복사된 버퍼들 중에
 *    가장 작은 recovery LSN을 가지고 있다. IOB에 버퍼를 복사할 때마다
 *    mMinRecoveryLSN과 비교하여 더 작은 값을 만나면 이 맴버를 갱신한다.
 *    그리고 IOB가 디스크에 내려간 후에는 이 값을 최대값으로 세팅한다.
 *    checkpoint가 발생하면 checkpoint list의 minRecovery과 각 flusher들
 *    의 mMinRecoveryLSN 중에 가장 작은 값을 restart redo LSN으로 설정한다.
 ******************************************************************************/
#include <smErrorCode.h>
#include <sdbFlushMgr.h>
#include <sdbFlusher.h>
#include <sdbReq.h>
#include <smuProperty.h>
#include <sddDiskMgr.h>
#include <sdbBufferMgr.h>
#include <smrRecoveryMgr.h>
#include <sdpPhyPage.h>
#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h>
#include <sdsFlusher.h>
#include <sdsFlushMgr.h>

#define IS_USE_DELAYED_FLUSH() \
    ( mDelayedFlushListPct != 0 )
#define IS_DELAYED_FLUSH_LIST( __LIST ) \
    ( (__LIST)->getListType() == SDB_BCB_DELAYED_FLUSH_LIST )

extern "C" SInt
sdbCompareSyncFileInfo( const void* aElem1, const void* aElem2 )
{
    sdbSyncFileInfo sFileInfo1;
    sdbSyncFileInfo sFileInfo2;

    sFileInfo1 = *(sdbSyncFileInfo*)aElem1;
    sFileInfo2 = *(sdbSyncFileInfo*)aElem2;

    if ( sFileInfo1.mSpaceID > sFileInfo2.mSpaceID )
    {
        return 1;
    }
    else
    {
        if ( sFileInfo1.mSpaceID < sFileInfo2.mSpaceID )
        {
            return -1;
        }
    }

    if ( sFileInfo1.mFileID > sFileInfo2.mFileID )
    {
        return 1;
    }
    else
    {
        if ( sFileInfo1.mFileID < sFileInfo2.mFileID )
        {
            return -1;
        }
    }
    return 0;
}

sdbFlusher::sdbFlusher() : idtBaseThread()
{

}

sdbFlusher::~sdbFlusher()
{

}

/******************************************************************************
 * Description :
 *   flusher 객체를 초기화한다. flusher thread를 start하기 전에 반드시
 *   초기화를 수행해야 한다. initialize된 객체는 사용 후 destroy로
 *   자원을 해제해줘야 한다. destroy된 객체는 다시 initialize를 호출하여
 *   재사용할 수 있다.
 *
 *  aFlusherID      - [IN]  이 객체의 고유 ID. flusher들은 각자 고유 ID를
 *                          가진다.
 *  aPageSize       - [IN]  페이지 크기. flusher들마다 page size를
 *                          다르게 줄 수 있다.
 *  aPageCount      - [IN]  flusher의 IOB 크기로서 단위는 page 개수이다.
 *  aCPListSet      - [IN]  플러시 해야 하는 FlushList가 속해있는 buffer pool
 *                          에 속한 checkpoint set
 ******************************************************************************/
IDE_RC sdbFlusher::initialize(UInt          aFlusherID,
                              UInt          aPageSize,
                              UInt          aPageCount,
                              sdbCPListSet *aCPListSet)
{
    SChar   sMutexName[128];
    UInt    i;
    SInt    sState = 0;

    mFlusherID    = aFlusherID;
    mPageSize     = aPageSize;
    mIOBPageCount = aPageCount;
    mCPListSet    = aCPListSet;
    mIOBPos       = 0;
    mFinish       = ID_FALSE;
    mStarted      = ID_FALSE;
    mWaitTime     = smuProperty::getDefaultFlusherWaitSec();
    mPool         = sdbBufferMgr::getPool();
    mServiceable  = sdsBufferMgr::isServiceable();

    SM_LSN_MAX(mMinRecoveryLSN);
    SM_LSN_INIT(mMaxPageLSN);

    // 통계 정보를 위한 private session 정보 초기화
    idvManager::initSession(&mOldSess, 0 /* unused */, NULL /* unused */);

    // BUG-21155 : current session을 초기화
    idvManager::initSession(&mCurrSess, 0 /* unused */, NULL /* unused */);

    idvManager::initSQL(&mStatistics,
                        &mCurrSess,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        IDV_OWNER_FLUSHER);

    // mutex 초기화
    idlOS::memset(sMutexName, 0, 128);
    idlOS::snprintf(sMutexName,
                    128,
                    "BUFFER_FLUSHER_MIN_RECOVERY_LSN_MUTEX_%"ID_UINT32_FMT,
                    aFlusherID);

    IDE_TEST(mMinRecoveryLSNMutex.initialize(sMutexName,
                                  IDU_MUTEX_KIND_NATIVE,
                                  IDB_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_FLUSHER_MIN_RECOVERY_LSN)
             != IDE_SUCCESS);
    sState = 1;

    idlOS::memset(sMutexName, 0, 128);
    idlOS::snprintf(sMutexName,
                    128,
                    "BUFFER_FLUSHER_COND_WAIT_MUTEX_%"ID_UINT32_FMT,
                    aFlusherID);

    IDE_TEST(mRunningMutex.initialize(sMutexName,
                                    IDU_MUTEX_KIND_POSIX,
                                    IDV_WAIT_INDEX_LATCH_FREE_OTHERS)
             != IDE_SUCCESS);
    sState = 2;

    /* TC/FIT/Limit/sm/sdbFlusher_initialize_malloc1.sql */
    IDU_FIT_POINT_RAISE( "sdbFlusher::initialize::malloc1",
                          insufficient_memory );

    // IOB 초기화
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)mPageSize * (mIOBPageCount + 1),
                                     (void**)&mIOBSpace) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 3;

    idlOS::memset((void*)mIOBSpace, 0, mPageSize * (mIOBPageCount + 1));

    mIOB = (UChar*)idlOS::align((void*)mIOBSpace, mPageSize);

    /* TC/FIT/Limit/sm/sdbFlusher_initialize_malloc2.sql */
    IDU_FIT_POINT_RAISE( "sdbFlusher::initialize::malloc2",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)ID_SIZEOF(UChar*) * mIOBPageCount,
                                     (void**)&mIOBPtr) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 4;

    /* TC/FIT/Limit/sm/sdbFlusher_initialize_malloc3.sql */
    IDU_FIT_POINT_RAISE( "sdbFlusher::initialize::malloc3",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)ID_SIZEOF(sdbBCB*) * mIOBPageCount,
                                     (void**)&mIOBBCBArray) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 5;

    /* TC/FIT/Limit/sm/sdbFlusher_initialize_malloc4.sql */
    IDU_FIT_POINT_RAISE( "sdbFlusher::initialize::malloc4",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDB,
                                       (ULong)ID_SIZEOF(sdbSyncFileInfo) * mIOBPageCount,
                                       (void**)&mArrSyncFileInfo ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 6;

    // condition variable 초기화
    // flusher가 쉬는 동안 여기에 대기를 한다.
    idlOS::snprintf(sMutexName,
                    ID_SIZEOF(sMutexName),
                    "BUFFER_FLUSHER_COND_%"ID_UINT32_FMT,
                    aFlusherID);

    IDE_TEST_RAISE(mCondVar.initialize(sMutexName) != IDE_SUCCESS,
                   err_cond_var_init);

    for (i = 0; i < mIOBPageCount; i++)
    {
        mIOBPtr[i] = mIOB + mPageSize * i;
    }

    IDE_TEST(mDWFile.create(mFlusherID, 
                            mPageSize, 
                            mIOBPageCount,
                            SD_LAYER_BUFFER_POOL)
             != IDE_SUCCESS);

    mStat.initialize(mFlusherID);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 6:
            IDE_ASSERT( iduMemMgr::free( mArrSyncFileInfo ) == IDE_SUCCESS );
        case 5:
            IDE_ASSERT(iduMemMgr::free(mIOBBCBArray) == IDE_SUCCESS);
        case 4:
            IDE_ASSERT(iduMemMgr::free(mIOBPtr) == IDE_SUCCESS);
        case 3:
            IDE_ASSERT(iduMemMgr::free(mIOBSpace) == IDE_SUCCESS);
        case 2:
            IDE_ASSERT(mRunningMutex.destroy() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(mMinRecoveryLSNMutex.destroy() == IDE_SUCCESS);
        default:
           break;
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    flusher 객체가 가지고 있는 메모리, mutex, condition variable들을
 *    해제한다. destroy된 객체는 다시 initialize를 호출함으로써 재사용할 수
 *    있다.
 ******************************************************************************/
IDE_RC sdbFlusher::destroy()
{
    IDE_ASSERT(mDWFile.destroy() == IDE_SUCCESS);

    IDE_ASSERT( iduMemMgr::free( mArrSyncFileInfo ) == IDE_SUCCESS );
    mArrSyncFileInfo = NULL;

    IDE_ASSERT(iduMemMgr::free(mIOBBCBArray) == IDE_SUCCESS);
    mIOBBCBArray = NULL;

    IDE_ASSERT(iduMemMgr::free(mIOBPtr) == IDE_SUCCESS);
    mIOBPtr = NULL;

    IDE_ASSERT(iduMemMgr::free(mIOBSpace) == IDE_SUCCESS);
    mIOBSpace = NULL;

    IDE_ASSERT(mRunningMutex.destroy() == IDE_SUCCESS);

    IDE_ASSERT(mMinRecoveryLSNMutex.destroy() == IDE_SUCCESS);

    IDE_TEST_RAISE(mCondVar.destroy() != IDE_SUCCESS, err_cond_destroy);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    flusher 쓰레드의 실행을 동기적으로 종료시킨다. thread가 종료될 때까지
 *    리턴되지 않는다.
 *
 *  aStatistics - [IN]  통계정보
 *  aNotStarted - [OUT] flusher가 아직 시작되지 않았다면 ID_TRUE를 리턴. 그 외엔
 *                      ID_FALSE를 리턴
 ******************************************************************************/
IDE_RC sdbFlusher::finish(idvSQL *aStatistics, idBool *aNotStarted)
{
    IDE_ASSERT(mRunningMutex.lock(aStatistics) == IDE_SUCCESS);
    if (mStarted == ID_FALSE)
    {
        IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);
        *aNotStarted = ID_TRUE;
    }
    else
    {
        mFinish = ID_TRUE;
        mStarted = ID_FALSE;
        *aNotStarted = ID_FALSE;

        IDE_TEST_RAISE(mCondVar.signal() != IDE_SUCCESS, err_cond_signal);

        IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);

        IDE_TEST_RAISE(join() != 0, err_thr_join);

        mStat.applyFinish();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : 현재 하고 있는 작업이 있다면 작업중인 작업이 종료되는 것을 기다
 *               린다. 만약 없다면 바로 리턴한다.
 ******************************************************************************/
void sdbFlusher::wait4OneJobDone()
{
    IDE_ASSERT( mRunningMutex.lock( NULL ) == IDE_SUCCESS );
    IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );
}

/******************************************************************************
 * Description :
 *    이 flusher가 쉬고 있으면 깨운다. 작업중이면 아무런 영향을 주지 않는다.
 *    깨웠는지의 유무는 aWaken으로 반환된다.
 * Implementation :
 *    flusher가 작업중이었으면 mRunningMutex를 잡는데 실패할 것이다.
 *
 *  aWaken  - [OUT] flusher가 쉬고 있어서 깨웠다면 ID_TRUE를,
 *                  flusher가 작업중이었으면 ID_FALSE를 반환한다.
 ******************************************************************************/
IDE_RC sdbFlusher::wakeUpOnlyIfSleeping(idBool *aWaken)
{
    idBool sLocked;

    IDE_ASSERT(mRunningMutex.trylock(sLocked) == IDE_SUCCESS);

    if (sLocked == ID_TRUE)
    {
        IDE_TEST_RAISE(mCondVar.signal() != 0,
                       err_cond_signal);

        IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);

        *aWaken = ID_TRUE;
    }
    else
    {
        *aWaken = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
        IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbFlusher::start()
{
    mFinish = ID_FALSE;
    IDE_TEST(idtBaseThread::start() != IDE_SUCCESS);

    IDE_TEST(idtBaseThread::waitToStart() != IDE_SUCCESS);

    mStat.applyStart();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    flusher 쓰레드가 start되면 불리는 함수이다. flusher는 run에서
 *    무한 루프를 돌며 flusher를 finish()하기 전까지 이 작업은 계속된다.
 *    sdbFlushMgr로부터 job을 얻어와 처리하고 일정시간 cond_wait한다.
 *
 * Implementation :
 *    run()이 호출되면 맨먼저 mFinish를 ID_FALSE로 세팅한다. mFinish는
 *    finish()가 호출되었을 때만 ID_TRUE로 세팅되며, 다시 flusher를 start하면
 *    run()이 호출되면서 mFinish가 ID_FALSE로 세팅된다.
 ******************************************************************************/
void sdbFlusher::run()
{
    PDL_Time_Value           sTimeValue;
    sdbFlushJob              sJob;
    IDE_RC                   sRC;
    time_t                   sBeforeWait;
    time_t                   sAfterWait;
    UInt                     sWaitSec;
    ULong                    sFlushedCount = 0;
    idBool                   sMutexLocked  = ID_FALSE;
    sdbReplaceFlushJobParam *sReplaceJobParam;
    sdbObjectFlushJobParam  *sObjJobParam;
    sdbChkptFlushJobParam   *sChkptJobParam;

    IDE_ASSERT(mRunningMutex.lock(NULL) == IDE_SUCCESS);
    sMutexLocked = ID_TRUE;

    mStarted = ID_TRUE;
    while (mFinish == ID_FALSE)
    {
        sFlushedCount = 0;
        sdbFlushMgr::getJob(&mStatistics, &sJob);
        switch (sJob.mType)
        {
            case SDB_FLUSH_JOB_REPLACEMENT_FLUSH:
                mStat.applyReplaceFlushJob();
                if ( IS_USE_DELAYED_FLUSH() == ID_TRUE )
                {
                    IDE_TEST( delayedFlushForReplacement( &mStatistics,
                                                          &sJob,
                                                          &sFlushedCount )
                              != IDE_SUCCESS );
                }
                else
                {
                    sReplaceJobParam = &sJob.mFlushJobParam.mReplaceFlush;
                    IDE_TEST( flushForReplacement( &mStatistics,
                                                   sJob.mReqFlushCount,
                                                   sReplaceJobParam->mFlushList,
                                                   sReplaceJobParam->mLRUList,
                                                   &sFlushedCount )
                              != IDE_SUCCESS );
                }
                mStat.applyReplaceFlushJobDone();
                break;
            case SDB_FLUSH_JOB_CHECKPOINT_FLUSH:
                mStat.applyCheckpointFlushJob();
                sChkptJobParam = &sJob.mFlushJobParam.mChkptFlush;
                IDE_TEST(flushForCheckpoint( &mStatistics,
                                             sJob.mReqFlushCount,
                                             sChkptJobParam->mRedoPageCount,
                                             sChkptJobParam->mRedoLogFileCount,
                                             sChkptJobParam->mCheckpointType, 
                                             &sFlushedCount )
                         != IDE_SUCCESS);
                mStat.applyCheckpointFlushJobDone();
                break;
            case SDB_FLUSH_JOB_DBOBJECT_FLUSH:
                mStat.applyObjectFlushJob();
                sObjJobParam = &sJob.mFlushJobParam.mObjectFlush;
                IDE_TEST(flushDBObjectBCB(&mStatistics,
                                          sObjJobParam->mFiltFunc,
                                          sObjJobParam->mFiltObj,
                                          sObjJobParam->mBCBQueue,
                                          &sFlushedCount)
                         != IDE_SUCCESS);
                mStat.applyObjectFlushJobDone();
                break;
            default:
                break;
        }

        if (sJob.mType != SDB_FLUSH_JOB_NOTHING)
        {
            sdbFlushMgr::updateLastFlushedTime();
            sdbFlushMgr::notifyJobDone(sJob.mJobDoneParam);
        }

        sWaitSec = getWaitInterval(sFlushedCount);
        if (sWaitSec == 0)
        {
            IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);
            IDE_ASSERT(mRunningMutex.lock(NULL) == IDE_SUCCESS);
            continue;
        }

        sBeforeWait = idlOS::time(NULL);
        // sdbFlushMgr로부터 얼마나 쉴지 알아와서 time value에 설정한다.
        sTimeValue.set(sBeforeWait + sWaitSec);
        /* Timed out 무시 */
        sRC = mCondVar.timedwait(&mRunningMutex, &sTimeValue, IDU_IGNORE_TIMEDOUT);
        sAfterWait = idlOS::time(NULL);

        mStat.applyTotalSleepSec(sAfterWait - sBeforeWait);

        if (sRC == IDE_SUCCESS)
        {
            if(mCondVar.isTimedOut() == ID_TRUE)
            {
                mStat.applyWakeUpsByTimeout();
            }
            else
            {
                // cond_signal을 받고 깨어난 경우
                mStat.applyWakeUpsBySignal();
            }
        }
        else
        {
            // 그외의 경우에는 에러로 처리를 한다.
            ideLog::log( IDE_SM_0, "Flusher Dead [RC:%d, ERRNO:%d]", sRC, errno );
            IDE_RAISE(err_cond_wait);
        }
    }

    sMutexLocked = ID_FALSE;
    IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);

    return;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    if (sMutexLocked == ID_TRUE)
    {
        IDE_ASSERT(mRunningMutex.unlock() == IDE_SUCCESS);
    }

    mStarted = ID_FALSE;
    mFinish = ID_TRUE;

    IDE_ASSERT( 0 );

    return;
}

/******************************************************************************
 * Description :
 *   PROJ-2669
 *    Delayed Flush 기능을 위해 Normal/Delayed flush list를 번갈아 가며 flush 한다.
 *    Normal/Delayed Flush Job 의 정보를 확인하고
 *    Normal Flush -> Delayed Flush 순서로 Flush 동작을 수행한다.
 *
 *  aStatistics     - [IN]  통계정보
 *  aNormalFlushJob - [IN]  Normal flush 에 대한 정보
 *  aRetFlushedCount- [OUT] 실제 flush한 page 갯수
 ******************************************************************************/
IDE_RC sdbFlusher::delayedFlushForReplacement( idvSQL                  *aStatistics,
                                               sdbFlushJob             *aNormalFlushJob,
                                               ULong                   *aRetFlushedCount)
{
    sdbFlushJob              sDelayedFlushJob;
    ULong                    sNormalFlushedCount;
    ULong                    sDelayedFlushedCount;
    sdbReplaceFlushJobParam *sNormalJobParam;
    sdbReplaceFlushJobParam *sDelayedJobParam;
    UInt                     sFlushListLength;

    *aRetFlushedCount = 0;
    sNormalFlushedCount = 0;
    sDelayedFlushedCount = 0;

    sNormalJobParam = &aNormalFlushJob->mFlushJobParam.mReplaceFlush;

    /* PROJ-2669
     * 기존 Flusher의 Flush 갯수 산정 방식과 동일하게 계산한다.
     * (AS-IS: Flush List의 길이를 최대 Flush 수로 설정한다. - BUG-22386 참고) */

    /* 1. Flush 대상 Flush List의 Total Length(Normal + Delayed) 를 확인한다.  */
    sFlushListLength = ( aNormalFlushJob->mReqFlushCount == SDB_FLUSH_COUNT_UNLIMITED )
        ? mPool->getFlushListLength( sNormalJobParam->mFlushList->getID() )
        : aNormalFlushJob->mReqFlushCount;

    /* 2. Delayed Flush를 수행한다.                                            */
    sdbFlushMgr::getDelayedFlushJob( aNormalFlushJob, &sDelayedFlushJob );
    sDelayedJobParam = &sDelayedFlushJob.mFlushJobParam.mReplaceFlush;

    if ( sDelayedFlushJob.mType != SDB_FLUSH_JOB_NOTHING )
    {
        /* TC/FIT/Limit/sm/sdb/sdbFlusher_delayedFlushForReplacement_delayedFlush.sql */
        IDU_FIT_POINT( "sdbFlusher::delayedFlushForReplacement::delayedFlush" );

        IDE_TEST( flushForReplacement( aStatistics,
                                       sDelayedFlushJob.mReqFlushCount,
                                       sDelayedJobParam->mFlushList,
                                       sDelayedJobParam->mLRUList,
                                       &sDelayedFlushedCount)
                  != IDE_SUCCESS );

        *aRetFlushedCount += sDelayedFlushedCount;
    }
    else
    {
        /* nothing to do */
    }

    /* 3. 1에서 계산한 Total Length가 Normal Flush를 수행한 값보다 크다면
     * Total Length - Delayed Flush 개수 만큼을 더 Flush 해야 한다.             */
    if ( sFlushListLength > sDelayedFlushedCount )
    {
        aNormalFlushJob->mReqFlushCount = sFlushListLength - sDelayedFlushedCount;
    }
    else
    {
        aNormalFlushJob->mType = SDB_FLUSH_JOB_NOTHING;
    }


    /* 4. Normal Flush 를 수행한다.
     * flushForReplacement() 함수 내부에서 Flush List 개수만큼 Flush 한다.     */
    if ( aNormalFlushJob->mType != SDB_FLUSH_JOB_NOTHING )
    {
        IDE_TEST( flushForReplacement( aStatistics,
                    aNormalFlushJob->mReqFlushCount,
                    sNormalJobParam->mFlushList,
                    sNormalJobParam->mLRUList,
                    &sNormalFlushedCount)
                != IDE_SUCCESS );

        *aRetFlushedCount  += sNormalFlushedCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    flush list에 대해 flush작업을 수행한다.
 *    flush list의 BCB들 중에 다음 조건을 만족하면 flush를 수행한다.
 *     1. touch count가 BUFFER_HOT_TOUCH_COUNT를 넘지 않는것
 *        -> 만족 못하면 LRU mid point에 넣는다.
 *     2. S-latch를 획득할 수 잇어야 한다.
 *        -> 만족 못하면 LRU mid point에 넣는다.
 *     3. state가 dirty이어야 한다.
 *        -> INIOB, REDIRTY인 것들은 skip, CLEAN인 것들은 prepare list로 옮긴다.
 *    이하 flush 과정은 이 파일 상단에 있는 Implementation 주석을 참고할 것.
 *    flush한 버퍼들은 모두 prepare list로 옮긴다.
 *
 *  aStatistics     - [IN]  각 list 이동시 mutex 획득이 필요하기 때문에
 *                          통계정보가 필요하다.
 *  aReqFlushCount  - [IN]  사용자가 요청한 flush개수
 *  aFlushList      - [IN]  flush할 flush list
 *  aLRUList        - [IN]  touch count가 BUFFER_HOT_TOUCH_COUNT 이상이거나
 *                          X-latch가 잡혀있는 버퍼들을 옮길 LRU list
 *  aRetFlushedCount- [OUT] 실제 flush한 page 갯수
 ******************************************************************************/
IDE_RC sdbFlusher::flushForReplacement(idvSQL         * aStatistics,
                                       ULong            aReqFlushCount,
                                       sdbFlushList   * aFlushList,
                                       sdbLRUList     * aLRUList,
                                       ULong          * aRetFlushedCount)
{
    idBool          sLocked;
    sdbFlushList  * sDelayedFlushList   = mPool->getDelayedFlushList( aFlushList->getID() );
    sdbBCB        * sBCB;
    sdsBCB        * sSBCB;
    ULong           sFlushCount         = 0;
    idBool          sMoveToSBuffer      = ID_FALSE;
    idBool          sIsUnderMaxLength;
    idBool          sIsHotBCB;
    idBool          sIsDelayedFlushList;

    IDE_DASSERT(aFlushList != NULL);
    IDE_DASSERT(aReqFlushCount > 0);

    *aRetFlushedCount = 0;

    sIsDelayedFlushList = (idBool)IS_DELAYED_FLUSH_LIST( aFlushList );

    // flush list를 탐색하기 위해서 beginExploring()을 한다.
    aFlushList->beginExploring(aStatistics);

    // BUG-22386 flush list에 dirty page가 추가되는 속도가 list의
    // dirty page를 flush하는 속도보다 더 빨라서, flusher가 하나의
    // flush list를 붙잡고 다음 작업으로 넘어가지 못하는 현상이
    // 있습니다. 그래서 현재의 길이 만큼만 flush하고 flush도중에
    // 추가된 page는 flush하지 않습니다.
    if ( aReqFlushCount > aFlushList->getPartialLength() )
    {
        aReqFlushCount = aFlushList->getPartialLength();
    }
    else
    {
        /* nothing to do */
    }

     
    // aReqFlushCount만큼 flush한다.
    while (sFlushCount < aReqFlushCount)
    {
        // flush list에서 하나씩 BCB를 본다.
        // 이때 리스트에서 제거하진 않는다.
        sBCB = aFlushList->getNext(aStatistics);

        if (sBCB == NULL)
        {
            // 더이상 flush할 BCB가 없으므로 루프를 빠져나간다.
            break;
        }


        if ( ( sIsDelayedFlushList == ID_FALSE ) &&
             ( mPool->isHotBCB( sBCB ) == ID_TRUE ) )
        {
            // touch count가 BUFFER_HOT_TOUCH_COUNT 이상이거나
            // X-latch가 잡혀있는 버퍼들을 실제로 LRU mid point로 옮긴다.
            aFlushList->remove(aStatistics, sBCB);
            aLRUList->insertBCB2BehindMid(aStatistics, sBCB);

            mStat.applyReplaceSkipPages();
            continue;
        }
        else
        {
            /* Nothing to do */
        }


        if ( IS_USE_DELAYED_FLUSH() == ID_TRUE )
        {
            /* PROJ-2669 Flsuh now or Delay Case
             *        | sIsUnderMaxLength | sIsHotBCB | Doing what
             * -------+-------------------+-----------+------------------------
             * CASE 1 |         1         |     1     | Delayed Flush List Add
             * CASE 2 |         1         |     0     | Flush
             * CASE 3 |         0         |     1     | Flush
             * CASE 4 |         0         |     0     | Flush
             */

            sIsUnderMaxLength = sDelayedFlushList->isUnderMaxLength();
            sIsHotBCB = isHotBCBByFlusher( sBCB );

            /* CASE 3 only for statistics */
            if ( ( sIsUnderMaxLength == ID_FALSE ) && ( sIsHotBCB == ID_TRUE ) )
            {
                if ( sIsDelayedFlushList == ID_FALSE )
                {
                    mStat.applyReplacementOverflowDelayedPages();
                }
                else
                {
                    /* No statistics */
                }
            }
            else
            {
                /* Nothing to do */
            }

            /* CASE 1 */
            if ( ( sIsUnderMaxLength == ID_TRUE ) &&
                 ( sIsHotBCB == ID_TRUE ) )
            {
                /* Touch 정보를 초기화 하여
                 * 다음번 flush 에서 실제로 Touch가 있는 경우에만
                 * 다시 Delayed flush 되도록 한다.                  */
                sBCB->mTouchCnt = 1;
                IDV_TIME_INIT( &sBCB->mLastTouchedTime );

                if ( sIsDelayedFlushList == ID_FALSE )
                {
                    aFlushList->remove( aStatistics, sBCB );
                    sDelayedFlushList->add( aStatistics, sBCB );
                    mStat.applyReplaceAddDelayedPages();
                }
                else
                {
                    mStat.applyReplaceSkipDelayedPages();
                }
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            /* CASE 2, 4
             * 더이상 Delayed Flush 를 위해서 해야 할 일이 없다.
             * 다음 작업을 계속 진행한다.                        */
        }
        else
        {
            /* Nothing to do */
        }

        // S-latch를 try해본다.
        // 실패하면 X-latch가 이미 잡혀있다는 의미이다.
        // 이경우에도 LRU mid point에 넣는다.
        sBCB->tryLockPageSLatch( &sLocked );
        if ( sLocked == ID_FALSE )
        {
            aFlushList->remove( aStatistics, sBCB );
            aLRUList->insertBCB2BehindMid( aStatistics, sBCB );

            mStat.applyReplaceSkipPages();
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        // BCB state 검사를 하기 위해 BCBMutex를 획득한다.
        sBCB->lockBCBMutex(aStatistics);

        if ((sBCB->mState == SDB_BCB_REDIRTY) ||
            (sBCB->mState == SDB_BCB_INIOB)   ||
            (sBCB->mState == SDB_BCB_FREE))
        {
            // REDIRTY, INIOB인 버퍼는 skip, prepare list로
            // 옮긴다.

            sBCB->unlockBCBMutex();
            sBCB->unlockPageLatch();

            if( sBCB->mState == SDB_BCB_FREE )
            {
                aFlushList->remove(aStatistics, sBCB);
                mPool->addBCB2PrepareLst(aStatistics, sBCB);
            }
            mStat.applyReplaceSkipPages();
            continue;
        }
        else
        {
           IDE_ASSERT( ( sBCB->mState == SDB_BCB_DIRTY ) ||
                       ( sBCB->mState == SDB_BCB_CLEAN ) );

            if( needToSkipBCB( sBCB ) == ID_TRUE )
            {
                /* Secondary Buffer에 movedown 대상이 아닌
                   CLEAN page를  prepare list로  옮긴다. */
                sBCB->unlockBCBMutex();
                sBCB->unlockPageLatch();

                aFlushList->remove(aStatistics, sBCB);
                mPool->addBCB2PrepareLst(aStatistics, sBCB);

                mStat.applyReplaceSkipPages();
                continue;
            }
            else 
            {
                /* nothing to do */
            }
        }

        // flush 조건을 모두 만족했다.
        // 이제 flush를 하자.
        sBCB->mPrevState = sBCB->mState;
        sBCB->mState     = SDB_BCB_INIOB;
        //연결된 SBCB가 있다면 delink
        sSBCB = sBCB->mSBCB;
        if( sSBCB != NULL )
        { 
            /* delink 작업이 lock없이 수행되므로
               victim 발생으로 인한 상태 변경이 있을수 있다.
               pageID 등이 다르다면 이미 free 된 상황일수 있다. */
            if (( sBCB->mSpaceID == sSBCB->mSpaceID ) &&
                ( sBCB->mPageID  == sSBCB->mPageID ))
            {
                (void)sdsBufferMgr::removeBCB( aStatistics,
                                               sSBCB );
            }
            else
            {
                /* 여기서 sSBCB 를 삭제하는 이유는
                   secondary buffer에 있을수도 있는 old page를 미리삭제하기 위함인데
                   pageID 등이 다르다면 (victim 등으로 인한) 다른 페이지이므로
                   삭제대상이 아니다. */
            }
        }
        else
        {
            /* nothing to do */
        }

        sBCB->unlockBCBMutex();

        aFlushList->remove(aStatistics, sBCB);
    
        if( mServiceable == ID_TRUE )
        {
            sMoveToSBuffer = ID_TRUE;
        }

        if( SM_IS_LSN_INIT(sBCB->mRecoveryLSN) ) 
        {
            if( sMoveToSBuffer == ID_TRUE ) 
            {
                /* secondary Buffer 를 사용할때는
                   temppage도 secondary Buffer 로 저장한다. */
                IDE_TEST( copyTempPage( aStatistics,
                                        sBCB,
                                        ID_TRUE,         // move to prepare
                                        sMoveToSBuffer ) // move to Secondary
                          != IDE_SUCCESS);
            }
            else 
            {
                // temp page인 경우 double write를 할 필요가 없기때문에
                // IOB에 복사한 후 바로 그 페이지만 disk에 내린다.
                IDE_TEST( flushTempPage( aStatistics,
                                         sBCB,
                                         ID_TRUE )  // move to prepare
                          != IDE_SUCCESS);
            }
        }
        else
        {
            IDE_TEST( copyToIOB( aStatistics,
                                 sBCB,
                                 ID_TRUE,         // move to prepare
                                 sMoveToSBuffer ) // move to Secondary
                      != IDE_SUCCESS);
        }
        mStat.applyReplaceFlushPages();

        sFlushCount++;
    }

    // IOB에 남은 페이지들을 디스크에 내린다.
    IDE_TEST( writeIOB( aStatistics,
                        ID_TRUE, // move to prepare
                        sMoveToSBuffer ) // move to Secondary buffer     
             != IDE_SUCCESS);

    // flush 탐색을 마쳤음을 알린다.
    aFlushList->endExploring(aStatistics);

    *aRetFlushedCount = sFlushCount;

    /* PROJ-2669 Delayed Flush Statistics */
    if ( sIsDelayedFlushList == ID_TRUE )
    {
        mStat.applyTotalFlushDelayedPages( sFlushCount );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    CPListSet의 버퍼들을 recovery LSN이 작은 순으로 flush를 수행한다.
 *    flush 조건은 DIRTY 상태이기만 하면 된다.
 *    DIRTY가 아닌 BCB는 skip한다.
 *    replacement flush에서는 S-latch를 try했지만 여기서는
 *    잡힐때까지 대기를 한다. checkpoint flush는 급할게 없고,
 *    순서대로 flush하는게 효율적이기 때문이다.
 *
 *    aReqFlushCount 이상, aReqFlushLSN 까지,
 *    Chkpt list pages 수 aMaxSkipPageCnt 이하,
 *    세가지 조건을 모두 만족할때까지 flush하지만
 *    작업 시작시 CPList의 마지막 Page의 LSN을 미리 확인하여
 *    그 이상은 flush하지 않는다.
 *
 *  aStatistics         - [IN]  통계정보
 *  aMinFlushCount      - [IN]  flush할 최소 Page 수
 *  aRedoDirtyPageCnt   - [IN]  Restart Recovery시에 Redo할 Page 수,
 *                              반대로 말하면 CP List에 남겨둘 Dirty Page의 수
 *  aRedoLogFileCount   - [IN]  Restart Recovery시에 redo할 LogFile 수
 *  aRetFlushedCount    - [OUT] 실제 flush한 page 갯수
 ******************************************************************************/
IDE_RC sdbFlusher::flushForCheckpoint( idvSQL          * aStatistics,
                                       ULong             aMinFlushCount,
                                       ULong             aRedoPageCount,
                                       UInt              aRedoLogFileCount,
                                       sdbCheckpointType aCheckpointType,
                                       ULong           * aRetFlushedCount )
{
    sdbBCB        * sBCB;
    sdsBCB        * sSBCB;
    ULong           sFlushCount     = 0;
    ULong           sReqFlushCount  = 0;
    ULong           sNotDirtyCount  = 0;
    ULong           sLockFailCount  = 0;
    smLSN           sReqFlushLSN;
    smLSN           sMaxFlushLSN;
    idBool          sLocked;
    UShort          sActiveFlusherCount;

    IDE_ASSERT( aRetFlushedCount != NULL );

    *aRetFlushedCount = 0;

    /* checkpoint flush시 최대 flush할 수 있는 LSN을 얻는다.
     * 이 LSN이상은 flush하지 않는다. */
    mCPListSet->getMaxRecoveryLSN(aStatistics, &sMaxFlushLSN);

    /* last LSN을 가져온다, Disk뿐만 아니라 Memory 포함해서 가장
     * 마지막 LSN이다. 그래야 LogFile을 삭제 할 수 있다. */
    smLayerCallback::getLstLSN( &sReqFlushLSN );

    if( sReqFlushLSN.mFileNo >= aRedoLogFileCount )
    {
        sReqFlushLSN.mFileNo -= aRedoLogFileCount ;
    }
    else
    {
        SM_LSN_INIT( sReqFlushLSN );
    }

    /* flush 요청할 page의 수를 계산한다. */
    sReqFlushCount = mCPListSet->getTotalBCBCnt();

    if( sReqFlushCount > aRedoPageCount )
    {
        /* CP list의 Dirty Page수가 Restart Recovery시에
         * Redo 할 Page 보다 많아졌으므로 그 차이를 flush한다. */
        sReqFlushCount -= aRedoPageCount ;

        /* 여러 flusher가 동시에 작동 중이므로
         * flush할 page를 작동중인 flusher수로 1/n로 나눈다. */
        sActiveFlusherCount = sdbFlushMgr::getActiveFlusherCount();
        IDE_ASSERT( sActiveFlusherCount > 0 );
        sReqFlushCount /= sActiveFlusherCount;

        if( aMinFlushCount > sReqFlushCount )
        {
            sReqFlushCount = aMinFlushCount;
        }
    }
    else
    {
        /* CP list의 Dirty Page수가 Restart Recovery시에
         * Redo 할 Page 보다 작다. MinFlushCount 만큼만 flush한다. */
        sReqFlushCount = aMinFlushCount;
    }

    /* CPListSet에서 recovery LSN이 가장 작은 BCB를 본다.
     * 하지만 이 BCB를 보는 동안 CPListSet에서 빠질 수도 있다. (!!!) */
    sBCB = (sdbBCB *)mCPListSet->getMin();

    while (sBCB != NULL)
    {
        if ( smLayerCallback::isLSNLT( &sMaxFlushLSN,
                                       &sBCB->mRecoveryLSN )
             == ID_TRUE )
        {
            break;
        }

        /* BUG-40138
         * flusher에 의한 checkpoint flush가 수행되는 도중
         * 다른 우선 순위가 높은 job을 수행해야하는 조건을 검사한다.
         * checkpoint thread에 의해 요청된 checkpoint flush는 해당 되지 않는다.*/
        if ( ((sFlushCount % smuProperty::getFlusherBusyConditionCheckInterval()) == 0) &&
             (aCheckpointType == SDB_CHECKPOINT_BY_FLUSH_THREAD) &&
             (sdbFlushMgr::isBusyCondition() == ID_TRUE) )
        {
            break;
        }

        if ( ( sFlushCount >= sReqFlushCount ) &&
             ( smLayerCallback::isLSNGT( &sBCB->mRecoveryLSN,
                                         &sReqFlushLSN ) == ID_TRUE) )
        {
            /* sReqFlushCount 이상 flush하였고
             * sReqFlushLSN까지 flush하였으면 flush 작업을 중단한다. */
            break;
        }

        /* 실제 Flush 부분 */
        sBCB->tryLockPageSLatch(&sLocked);

        if (sLocked == ID_FALSE)
        {
            sLockFailCount++;
            if (sLockFailCount > mCPListSet->getListCount())
            {
                break; /* checkpoint flush를 중단한다. */
            }
            sBCB = (sdbBCB *)mCPListSet->getNextOf( sBCB );
            mStat.applyCheckpointSkipPages();
            continue;
        }
        sLockFailCount = 0;

        sBCB->lockBCBMutex(aStatistics);

        if (sBCB->mState != SDB_BCB_DIRTY)
        {
            /* DIRTY가 아닌 버퍼는 skip한다.
             * 다음 BCB를 참조한다. */
            sBCB->unlockBCBMutex();
            sBCB->unlockPageLatch();

            /* 모든 checkpoint list의 첫번째 BCB가 dirty가 아닌 경우
             * 무한루프에 빠질 수 있다.
             * 플러셔 자신이 InIOB 로가지고 있는 페이지가 아직 디스크에
             * 반영되지 않은 상태에서, redirty가 되는 상황을 가정해 보면,
             * 계속 여기서 무한 루프를 돌게 된다. 이 상황을 방지하기 위해
             * list 순회를 list개수만큼 수행하였으면, 자신이 가지고 있는 IOB의
             * 모든 버퍼를 write한 후 계속 flush를 수행한다.
             * 하지만 위의 상황은 현재 발생하지 않도록 되어있다.
             * 왜냐하면, flush대상이 되는 BCB는 flush가 시작되었을때의 LSN보다
             * mRecvLSN이 작은 BCB이기 때문이다.
             * 만약 InIOB되어있는 이후에 redirty되었다면, 그 BCB의 mRecvLSN은
             * flush가 시작되었을때의 LSN보다 크기때문이다. */
            sNotDirtyCount++;
            if (sNotDirtyCount > mCPListSet->getListCount())
            {
                IDE_TEST( writeIOB( aStatistics,
                                    ID_FALSE,  /* MOVE TO PREPARE */
                                    ID_FALSE ) /* move to Secondary buffer */
                         != IDE_SUCCESS);
                sNotDirtyCount = 0;
            }
            /* nextBCB()를 통해서 얻은 BCB역시 참조하는 동안
             * CPListSet에서 빠질 수 있다.
             * 또한 nextBCB()의 인자로 사용된 sBCB역시
             * 이 시점에서 CPList에서 빠질 수 있다.
             * 만약 그런 경우라면 nextBCB는 minBCB를 리턴할 것이다. */
            sBCB = (sdbBCB *)mCPListSet->getNextOf( sBCB );
            mStat.applyCheckpointSkipPages();
            continue;
        }
        sNotDirtyCount = 0;

        /* 다른 flusher들이 flush를 못하게 하기 위해
         * INIOB상태로 변경한다. */
        sBCB->mState = SDB_BCB_INIOB;
        //연결된 SBCB가 있다면 delink
        sSBCB = sBCB->mSBCB;
        if( sSBCB != NULL )
        { 
            /* delink 작업이 lock없이 수행되므로
               victim 발생으로 인한 상태 변경이 있을수 있다.
               pageID 등이 다르다면 이미 free 된 상황일수 있다. */
            if( (sBCB->mSpaceID == sSBCB->mSpaceID ) &&
                (sBCB->mPageID == sSBCB->mPageID ) )
            {
                (void)sdsBufferMgr::removeBCB( aStatistics,
                                               sSBCB );
            }
            else
            {
                /* 여기서 sSBCB 를 삭제하는 이유는
                   checkpoint로 인해 secondary buffer를 통하지 않고 disk로 내려간 이미지를
                   sSBCB에 있는 페이지가 덮어 쓰지 못하게 하기 위함 인데
                   pageID 등이 다르다면 (victim 등으로 인한) 다른 페이지이므로
                   삭제대상이 아니다. */
            }
        }
        else
        {
            /* nothing to do */
        }

        sBCB->unlockBCBMutex();

        if (SM_IS_LSN_INIT(sBCB->mRecoveryLSN))
        {
            /* TEMP PAGE가 check point 리스트에 달려 있으면 안된다.
             * 그렇지만 문제가 되지는 않기 때문에, release시에 발견되더라도
             * 죽이지 않도록 한다.
             * TEMP PAGE가 아니더라도, mRecoveryLSN이 init값이라는 것은
             * recovery와 상관없는 페이지라는 뜻이다.
             * recovery와 상관없는 페이지는 checkpoint를 하지 않아도 된다. */

            IDE_DASSERT( 0 );
            IDE_TEST( flushTempPage( aStatistics,
                                     sBCB,
                                     ID_FALSE )  /* move to prepare */
                     != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST( copyToIOB( aStatistics,
                                 sBCB,
                                 ID_FALSE,   /* prepare list에 반환하지 않는다. */
                                 ID_FALSE )  /* move to Secondary buffer */ 
                     != IDE_SUCCESS);

        }
        mStat.applyCheckpointFlushPages();

        sFlushCount++;

        /* 다음 flush할 BCB를 구한다. */
        sBCB = (sdbBCB*)mCPListSet->getMin();
    }

    *aRetFlushedCount = sFlushCount;

    /* IOB에 남아 있는 버퍼들을 모두 disk에 기록한다. */
    IDE_TEST( writeIOB( aStatistics,
                        ID_FALSE,  /* move to prepare */
                        ID_FALSE ) /* move to Secondary buffer */
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  aBCBQueue에 들어있는 BCB를 대상으로 flush를 수행한다.
 *  이때, Queue에 속한 모든 BCB중에 aFiltFunc조건을 만족하는 BCB에 대해서만
 *  flush를 수행한다.
 *
 *  주의 사항! aBCBQueue에 있는 BCB는 버퍼 풀의 어떤 위치에도 있을 수 있다.
 *
 *  aStatistics     - [IN]  통계정보.
 *  aFiltFunc       - [IN]  플러시 할지 말지 조건 함수.
 *  aFiltObj        - [IN]  aFiltFunc에 넘겨줄 인자.
 *  aBCBQueue       - [IN]  플러시 해야할 BCB들의 포인터가 들어있는 큐.
 *  aRetFlushedCount- [OUT] 실제 플러시 한 페이지 개수.
 ******************************************************************************/
IDE_RC sdbFlusher::flushDBObjectBCB( idvSQL           * aStatistics,
                                     sdbFiltFunc        aFiltFunc,
                                     void             * aFiltObj,
                                     smuQueueMgr      * aBCBQueue,
                                     ULong            * aRetFlushedCount )
{
    sdbBCB        * sBCB        = NULL;
    sdsBCB        * sSBCB       = NULL;
    idBool          sEmpty      = ID_FALSE;
    idBool          sIsSuccess  = ID_FALSE;
    PDL_Time_Value  sTV;
    ULong           sRetFlushedCount = 0;

    while (1)
    {
        IDE_ASSERT( aBCBQueue->dequeue( ID_FALSE, //mutex 잡지 않는다.
                                        (void*)&sBCB,
                                        &sEmpty)
                    == IDE_SUCCESS );

        if ( sEmpty == ID_TRUE )
        {
            // 더이상 flush할 BCB가 없으므로 루프를 빠져나간다.
            break;
        }
retry:
        sBCB->tryLockPageSLatch(&sIsSuccess);
        if( sIsSuccess == ID_FALSE )
        {
            // lock을 잡지 못했다는 것은 누군가 접근하고 있다는것!!
            // sBCB가 aFiltFunc을 만족시키지 못하면 플러시를 하지 않는다.
            if( aFiltFunc( sBCB, aFiltObj ) == ID_TRUE )
            {
                // 여전히 조건을 만족한다면 latch를 잡을 수 있을때 까지 대기한다.
                sBCB->lockPageSLatch(aStatistics);
            }
            else
            {
                mStat.applyObjectSkipPages();
                continue;
            }
        }

        // BCB state 검사를 하기 위해 BCBMutex를 획득한다.
        sBCB->lockBCBMutex(aStatistics);

        if ((sBCB->mState == SDB_BCB_CLEAN) ||
            (sBCB->mState == SDB_BCB_FREE))
        {
            sBCB->unlockBCBMutex();
            sBCB->unlockPageLatch();
            mStat.applyObjectSkipPages();
            continue;
        }

        if( aFiltFunc( sBCB, aFiltObj ) == ID_FALSE )
        {
            // BCB가 aFiltFunc조건을 만족하지 못하면 플러시 하지 않는다.
            sBCB->unlockBCBMutex();
            sBCB->unlockPageLatch();
            mStat.applyObjectSkipPages();
            continue;
        }

        // BUG-21135 flusher가 flush작업을 완료하기 위해서는
        // INIOB상태의 BCB상태가 변경되길 기다려야 합니다.
        if( (sBCB->mState == SDB_BCB_REDIRTY ) ||
            (sBCB->mState == SDB_BCB_INIOB))
        {
            sBCB->unlockBCBMutex();
            sBCB->unlockPageLatch();
            // redirty인 경우 dirty가 될때까지 기다렸다가해야함
            // dirty가 되면 flush해야함
            // 50000은 임의의 값임.. 너무 작으면 cpu시간을 많이 잡아먹고,
            // 너무 크면 대기시간이 길어질 수 있다.
            sTV.set(0, 50000);
            idlOS::sleep(sTV);
            goto retry;
        }
        // flush 조건을 모두 만족했다.
        // 이제 flush를 하자.
        sBCB->mState = SDB_BCB_INIOB;
        //연결된 SBCB가 있다면 delink
        sSBCB = sBCB->mSBCB;
        if( sSBCB != NULL )
        { 
            /* delink 작업이 lock없이 수행되므로
               victim 발생으로 인한 상태 변경이 있을수 있다.
               pageID 등이 다르다면 이미 free 된 상황일수 있다. */
            if( (sBCB->mSpaceID == sSBCB->mSpaceID ) &&
                (sBCB->mPageID == sSBCB->mPageID ) )
            {
                (void)sdsBufferMgr::removeBCB( aStatistics,
                                               sSBCB );
            }
            else
            {
                /* 여기서 sSBCB 를 삭제하는 이유는
                   flush작업으로 인해 secondary buffer를 통하지 않고 disk로 내려간 이미지를
                   sSBCB에 있는 페이지가 덮어 쓰지 못하게 하기 위함 인데
                   pageID 등이 다르다면 (victim 등으로 인한) 다른 페이지이므로
                   삭제대상이 아니다. */
            }
        }
        else
        {
            /* nothing to do */
        }

        sBCB->unlockBCBMutex();

        if (SM_IS_LSN_INIT(sBCB->mRecoveryLSN))
        {
            // temp page인 경우 double write를 할 필요가 없기때문에
            // IOB에 복사한 후 바로 그 페이지만 disk에 내린다.
            IDE_TEST( flushTempPage( aStatistics,
                                     sBCB,
                                     ID_FALSE ) // move to prepare
                     != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST( copyToIOB( aStatistics,
                                 sBCB,
                                 ID_FALSE,  // prepare list에 반환하지 않는다.
                                 ID_FALSE ) // move to Secondary buffer     
                     != IDE_SUCCESS);
        }
        mStat.applyObjectFlushPages();
        sRetFlushedCount++;
    }

    // IOB를 디스크에 기록한다.
    IDE_TEST( writeIOB( aStatistics,
                        ID_FALSE,  // move to prepare
                        ID_FALSE ) // move to Secondary buffer                
             != IDE_SUCCESS);

    *aRetFlushedCount = sRetFlushedCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aRetFlushedCount = sRetFlushedCount;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    temp page인 경우 flushTempPage 대신 이 함수를 사용한다.
 *    flushTempPage는 하나의 IOB를 바로 내리지만
      copyTempPage는 copyTOIOB와 비긋하게 동작
 *    no logging 이기 때문에 log flush가 필요없다.
 *
 *  aStatistics     - [IN]  통계정보
 *  aBCB            - [IN]  flush할 BCB
 *  aMoveToPrepare  - [IN]  flush이후에 BCB를 prepare list로 옮길지 여부
 *                          ID_TRUE이면 flush 이후 BCB를 prepare list로 옮긴다.
 ******************************************************************************/
IDE_RC sdbFlusher::copyTempPage( idvSQL           * aStatistics,
                                  sdbBCB          * aBCB,
                                  idBool            aMoveToPrepare,
                                  idBool            aMoveToSBuffer )
{
    smLSN      sDummyLSN;
    UChar     *sIOBPtr;
    idvTime    sBeginTime;
    idvTime    sEndTime;
    ULong      sCalcChecksumTime;

    IDE_ASSERT( aBCB->mCPListNo == SDB_CP_LIST_NONE );

    sIOBPtr = mIOBPtr[mIOBPos];

    idlOS::memcpy(sIOBPtr, aBCB->mFrame, mPageSize);
    mIOBBCBArray[mIOBPos] = aBCB;

    aBCB->unlockPageLatch();

    SM_LSN_INIT( sDummyLSN );

    /* BUG-22271: Flusher가 Page를 Disk에 내릴때 Page의 Chechsum을 Page
     *            Latch를 잡고 하고 있습니다. */
    IDV_TIME_GET(&sBeginTime);
    smLayerCallback::setPageLSN( sIOBPtr, &sDummyLSN );
    smLayerCallback::calcAndSetCheckSum( sIOBPtr );
    IDV_TIME_GET(&sEndTime);
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * Checksum 계산하는데 걸린 시간 저장함. */
    mStat.applyTotalCalcChecksumTimeUSec( sCalcChecksumTime );

    mIOBPos++;
    mStat.applyINIOBCount( mIOBPos );

    // IOB가 가득찼으면 IOB를 디스크에 내린다.
    if( mIOBPos == mIOBPageCount )
    {
        IDE_TEST( writeIOB( aStatistics, aMoveToPrepare, aMoveToSBuffer )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    temp page인 경우 copyToIOB()대신 이 함수를 사용한다.
 *    이 함수는 IOB에 버퍼를 복사한 후 그 IOB를 바로 디스크에 기록한다.
 *    copyToIOB()는 IOB가 꽉찼을 때만 disk에 기록되지만
 *    flushPage()는 하나의 IOB를 바로 disk에 기록한다.
 *    no logging 이기 때문에 log flush가 필요없다.
 *
 *  aStatistics     - [IN]  통계정보
 *  aBCB            - [IN]  flush할 BCB
 *  aMoveToPrepare  - [IN]  flush이후에 BCB를 prepare list로 옮길지 여부
 *                          ID_TRUE이면 flush 이후 BCB를 prepare list로 옮긴다.
 ******************************************************************************/
IDE_RC sdbFlusher::flushTempPage( idvSQL           * aStatistics,
                                  sdbBCB           * aBCB,
                                  idBool             aMoveToPrepare )
{
    SInt       sWaitEventState = 0;
    idvWeArgs  sWeArgs;
    smLSN      sDummyLSN;
    UChar     *sIOBPtr;
    idvTime    sBeginTime;
    idvTime    sEndTime;
    ULong      sTempWriteTime;
    ULong      sCalcChecksumTime;

    IDE_ASSERT( aBCB->mCPListNo == SDB_CP_LIST_NONE );
    IDE_ASSERT( smLayerCallback::isTempTableSpace( aBCB->mSpaceID )
                == ID_TRUE );

    sIOBPtr = mIOBPtr[mIOBPos];

    idlOS::memcpy(sIOBPtr, aBCB->mFrame, mPageSize);

    aBCB->unlockPageLatch();

    SM_LSN_INIT( sDummyLSN );

    /* BUG-22271: Flusher가 Page를 Disk에 내릴때 Page의 Chechsum을 Page
     *            Latch를 잡고 하고 있습니다. */
    IDV_TIME_GET(&sBeginTime);
    smLayerCallback::setPageLSN( sIOBPtr, &sDummyLSN );
    smLayerCallback::calcAndSetCheckSum( sIOBPtr );

    IDV_TIME_GET(&sEndTime);
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    IDV_WEARGS_SET( &sWeArgs, IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_WRITE,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );

    sWaitEventState = 1;

    IDV_TIME_GET(&sBeginTime);

    mStat.applyIOBegin();

    IDE_TEST( sddDiskMgr::write( aStatistics,
                                 aBCB->mSpaceID,
                                 aBCB->mPageID,
                                 mIOBPtr[mIOBPos] )
              != IDE_SUCCESS);

    mStat.applyIODone();

    IDV_TIME_GET(&sEndTime);
    sTempWriteTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    aBCB->mWriteCount++;

    sWaitEventState = 0;
    IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );

    aBCB->lockBCBMutex(aStatistics);

    if (aBCB->mState == SDB_BCB_INIOB)
    {
        aBCB->mState = SDB_BCB_CLEAN;
    }
    else
    {
        IDE_DASSERT(aBCB->mState == SDB_BCB_REDIRTY);
        aBCB->mState = SDB_BCB_DIRTY;
    }
    aBCB->unlockBCBMutex();

    if (aMoveToPrepare == ID_TRUE)
    {
        mPool->addBCB2PrepareLst(aStatistics, aBCB);
    }

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * 현재 TempPage는 한번에 한 Page씩 Write함. */
    mStat.applyTotalFlushTempPages( 1 );
    mStat.applyTotalTempWriteTimeUSec( sTempWriteTime );
    mStat.applyTotalCalcChecksumTimeUSec( sCalcChecksumTime );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sWaitEventState == 1)
    {
        IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    BCB의 frame 내용을 IOB에 복사한다. IOB가 가득차면 IOB를 disk에 내린다.
 *    BCB의 frame이 IOB에 복사될 때 IOB의 minRecoveryLSN이 갱신된다.
 *    또한 복사될 때 CPListSet에서도 제거된다.
 *
 * 주의!!
 *    CPListSet에서 제거하기 전에 반드시 IOB의 minRecoveryLSN을 먼저
 *    갱신해야 한다.
 *    만약 CPListSet에서 제거하고 IOB의 minRecoveryLSN을 갱신한다면, 그동안
 *    체크포인트 쓰레드가 잘못된 minRecoveryLSN을 가져갈 수 있다.
 *    그리고, 체크포인트 쓰레드가 minRecoveryLSN을 가져가는 순서도 잘 지켜야
 *    하는데, 먼저 CPListSset에서 minRecoveryLSN을 가져오고, 그다음에
 *    flusher의 InIOB에서 minRecoveryLSN을 가져와야 한다.
 *    만약 순서를 거꾸로 한다면, minRecoveryLSN을 놓칠 수도 있다.
 *
 *  aStatistics     - [IN]  통계정보
 *  aBCB            - [IN]  BCB
 *  aMoveToPrepare  - [IN]  flush이후에 BCB를 prepare list로 옮길지 여부
 *                          ID_TRUE이면 flush 이후 BCB를 prepare list로 옮긴다.
 ******************************************************************************/
IDE_RC sdbFlusher::copyToIOB( idvSQL          * aStatistics,
                              sdbBCB          * aBCB,
                              idBool            aMoveToPrepare,
                              idBool            aMoveToSBuffer )
{
    UChar    * sIOBPtr;
    smLSN      sPageLSN;
    scGRID     sPageGRID;
    idvTime    sBeginTime;
    idvTime    sEndTime;
    ULong      sCalcChecksumTime;

    IDE_ASSERT( mIOBPos < mIOBPageCount );
    IDE_ASSERT( aBCB->mCPListNo != SDB_CP_LIST_NONE );

    /* PROJ-2162 RestartRiskReduction
     * Consistency 하지 않으면, Flush를 막는다.  */
    if( ( smrRecoveryMgr::getConsistency() == ID_FALSE ) &&
        ( smuProperty::getCrashTolerance() != 2 ) )
    {
        /* Checkpoint Linst에서 BCB를 제거하고 Mutex를
         * 풀어주기만 한다. Flush나 IOB 사용은 안함 */
        mCPListSet->remove( aStatistics, aBCB );
        aBCB->unlockPageLatch();
    }
    else
    {
        // IOB의 minRecoveryLSN을 갱신하기 위해서 mMinRecoveryLSNMutex를 획득한다.
        IDE_ASSERT( mMinRecoveryLSNMutex.lock(aStatistics) == IDE_SUCCESS );
    
        if ( smLayerCallback::isLSNLT( &aBCB->mRecoveryLSN, 
                                       &mMinRecoveryLSN )
             == ID_TRUE)
        {
            SM_GET_LSN( mMinRecoveryLSN, aBCB->mRecoveryLSN );
        }

        IDE_ASSERT( mMinRecoveryLSNMutex.unlock() == IDE_SUCCESS );

        mCPListSet->remove( aStatistics, aBCB );

        sPageLSN = smLayerCallback::getPageLSN( aBCB->mFrame );

        sIOBPtr = mIOBPtr[ mIOBPos ];

        idlOS::memcpy( sIOBPtr, aBCB->mFrame, mPageSize );

        mIOBBCBArray[mIOBPos] = aBCB;

        // IOB에 기록된 BCB들의 pageLSN중에 가장 큰 LSN을 설정한다.
        // 이 값은 나중에 WAL을 지키기 위해 사용된다.
        if ( smLayerCallback::isLSNGT( &sPageLSN, &mMaxPageLSN )
             == ID_TRUE )
        {
            SM_GET_LSN( mMaxPageLSN, sPageLSN );
        }
        
        if( aMoveToSBuffer != ID_TRUE )
        {   
            SM_LSN_INIT( aBCB->mRecoveryLSN );
        }

        smLayerCallback::setPageLSN( sIOBPtr, &sPageLSN );

        /* BUG-23269 [5.3.1 SD] backup에서 page image log 시점이 잘못되어
         *           Restart Recovery 실패
         * Disk TBS가 Backup 상태라면 DWB에 복사하는 과정에서 Latch 풀기전에
         * 페이지 Image 로그를 기록한다. 왜냐하면, DWB에 Copy된 이후에도 해당
         * Page는 계속 변경되어지지만, copy 된 시점의 페이지 로깅은 한참 이후에
         * 발생할 수도 있기 때문에 계속되는 변경사항을 엎어쳐버릴수 있다. */
        if ( smLayerCallback::isBackupingTBS( aBCB->mSpaceID ) 
             == ID_TRUE )
        {
            SC_MAKE_GRID( sPageGRID, 
                          aBCB->mSpaceID, 
                          aBCB->mPageID, 
                          0 );
            IDE_TEST( smLayerCallback::writeDiskPILogRec( aStatistics,
                                                          sIOBPtr,
                                                          sPageGRID )
                      != IDE_SUCCESS );
        }

        // IOB에 복사하면 S-latch를 푼다.
        // 이 시점부터 이 버퍼는 다시 DIRTY가 될 수 있다.
        // 즉 다시 CPListSet에 매달릴 수 있다.
        aBCB->unlockPageLatch();

        /* BUG-22271: Flusher가 Page를 Disk에 내릴때 Page의 Checksum을 Page
         * Latch를 잡고 하고 있습니다. 동시성 향상을 위해서 Latch를 풀고한다. */
        IDV_TIME_GET(&sBeginTime);
        smLayerCallback::calcAndSetCheckSum( sIOBPtr );
        IDV_TIME_GET(&sEndTime);
        sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

        /* BUG-32670    [sm-disk-resource] add IO Stat information 
         * for analyzing storage performance.
         * Checksum 계산하는데 걸린 시간 저장함. */
        mStat.applyTotalCalcChecksumTimeUSec( sCalcChecksumTime );

        mIOBPos++;
        mStat.applyINIOBCount( mIOBPos );

        // IOB가 가득찼으면 IOB를 디스크에 내린다.
        if( mIOBPos == mIOBPageCount )
        {
            IDE_TEST( writeIOB( aStatistics, aMoveToPrepare, aMoveToSBuffer )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    IOB를 디스크에 내린다. 내부적으로 double write 기법을 사용할 수도 있고
 *    안할 수도 있다. 그리고 disk에 내리기전에 반드시 WAL을 지키기 위해
 *    log를 먼저 flush한다.
 *    IOB가 비어있으면 아무런 작업도 하지 않는다.
 *    그리고 IOB를 모두 내린 후에는 minRecoveryLSN을 MAX값으로 초기화한다.
 *
 *  aStatistics     - [IN]  통계정보
 *  aMoveToPrepare  - [IN]  flush이후에 BCB를 prepare list로 옮길지 여부
 *                          ID_TRUE이면 flush 이후 BCB를 prepare list로 옮긴다.
 *  aMoveToSBuffer  - [IN]  flushForReplacement에서 호출된경우 TRUE.
 ******************************************************************************/
IDE_RC sdbFlusher::writeIOB( idvSQL           * aStatistics, 
                             idBool             aMoveToPrepare,
                             idBool             aMoveToSBuffer )
{
    UInt            i;
    UInt            sDummySyncedLFCnt; // 나중에 지우자.
    idvWeArgs       sWeArgs;
    SInt            sWaitEventState = 0;
    sdbBCB        * sBCB;
    idvTime         sBeginTime;
    idvTime         sEndTime;
    ULong           sLogSyncTime;
    ULong           sDWTime;
    ULong           sWriteTime;
    ULong           sSyncTime;
    /* PROJ-2102 Secondary Buffer */
    UInt            sExtentIndex = 0;
    sdsBufferArea * sBufferArea;
    sdsBCB        * sSBCB = NULL;    

    if (mIOBPos > 0)
    {
        IDE_DASSERT( ( smrRecoveryMgr::getConsistency() == ID_TRUE ) ||
                     ( smuProperty::getCrashTolerance() == 2 ) );

        if( needToUseDWBuffer( aMoveToSBuffer ) == ID_TRUE ) 
        {                                                  
            IDV_TIME_GET(&sBeginTime);

            mStat.applyIOBegin();
            // double write 기법을 사용하는 경우
            // IOB 전체를 DWFile에 한번 기록한다.
            IDE_TEST( mDWFile.write( aStatistics,
                                     mIOB,
                                     mIOBPos )
                      != IDE_SUCCESS);
            mStat.applyIODone();

            IDV_TIME_GET(&sEndTime);
            sDWTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
        }
        else
        {
            // double write를 사용하지 않을 경우에만
            // 처리해야하는 특별한 작업은 없다.
            sDWTime = 0;
        }

        if( aMoveToSBuffer == ID_TRUE )
        {
            sBufferArea = sdsBufferMgr::getSBufferArea();

            if( sBufferArea->getTargetMoveDownIndex( aStatistics, &sExtentIndex )
                != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0,
                        "CAN NOT FIND SECONDARY BUFFER EXTENT \n ");
                sdsBufferMgr::setUnserviceable();
            }
            else 
            {
                /* nothing to do */   
            }
        }
        else 
        {
            /* */
        }

        // WAL
        /* BUG-45148 DW 사용시 Log Wait 시점 변경
         * DWFile을 이용하는 경우는 DataFile이 Inconsistent 할 때 뿐이다.
         * WAL은 DataFile에 Flush가 일어나기 전에만 지켜지면된다.
         * DWFile이 쓰여지는 동안 Log Flusher가 이미 필요한 Log를 Flush 할수 있으므로
         * Log Wait 시간이 줄어들 수 있다. */
        IDV_TIME_GET(&sBeginTime);
        IDE_TEST( smLayerCallback::sync4BufferFlush( &mMaxPageLSN,
                                                     &sDummySyncedLFCnt )
                  != IDE_SUCCESS );
        IDV_TIME_GET(&sEndTime);
        sLogSyncTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);


        IDV_WEARGS_SET( &sWeArgs, IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_WRITE,
                        0, /* WaitParam1 */
                        0, /* WaitParam2 */
                        0  /* WaitParam3 */ );

        sWriteTime = 0;

        for (i = 0; i < mIOBPos; i++)
        {
            IDV_TIME_GET(&sBeginTime);
            IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );

            sWaitEventState = 1;

            mStat.applyIOBegin();

            sBCB = mIOBBCBArray[i];

            if( needToMovedownSBuffer( aMoveToSBuffer, sBCB ) == ID_TRUE )
            { 
                IDE_TEST( sdsBufferMgr::moveDownPage( aStatistics,
                                                      sBCB,  
                                                      mIOBPtr[i],
                                                      sExtentIndex,/*Extent Idx */
                                                      i,           /*frame Idx */  
                                                      &sSBCB )
                          != IDE_SUCCESS ); 

                IDE_ASSERT( sSBCB != NULL );
                sBCB->mSBCB = sSBCB;
            }
            else   
            {
                IDE_TEST( sddDiskMgr::write( aStatistics,
                                             sBCB->mSpaceID,
                                             sBCB->mPageID,
                                             mIOBPtr[i])
                          != IDE_SUCCESS );

                sBCB->mSBCB = NULL;
            }

            mStat.applyIODone();

            sBCB->mWriteCount++;

            mArrSyncFileInfo[i].mSpaceID = sBCB->mSpaceID;
            mArrSyncFileInfo[i].mFileID  = SD_MAKE_FID( sBCB->mPageID );

            sWaitEventState = 0;
            IDV_END_WAIT_EVENT(aStatistics, &sWeArgs);
            IDV_TIME_GET(&sEndTime);
            sWriteTime += IDV_TIME_DIFF_MICRO( &sBeginTime, 
                                               &sEndTime);

            // BCB 상태를 clean로 변경한다.
            sBCB->lockBCBMutex( aStatistics );

            if( sBCB->mState == SDB_BCB_INIOB )
            {
                sBCB->mState = SDB_BCB_CLEAN;
                sBCB->mPrevState = SDB_BCB_INIOB;

                /* checkpoint flush 등이면 SecondaryBuffer 를 정리해준다. */
                if( aMoveToSBuffer != ID_TRUE )
                { 
                    sSBCB = sBCB->mSBCB;
                    if( sSBCB != NULL )
                    { 
#ifdef DEBUG
                        /* IOB 복사 전에 지워졌어야하지만 지우면됨
                           디버그에서만 사망 처리 */
                        IDE_RAISE( ERROR_INVALID_BCD )
#endif
                        sdsBufferMgr::removeBCB( aStatistics, 
                                                 sSBCB ); 
                        sBCB->mSBCB = NULL;
                    }
                }
            }  
            else 
            {
                IDE_DASSERT( sBCB->mState == SDB_BCB_REDIRTY );

                sBCB->mState = SDB_BCB_DIRTY;
                sBCB->mPrevState = SDB_BCB_REDIRTY;

                /* checkpoint flush 등이면 SecondaryBuffer 를 정리해준다. */
                if( aMoveToSBuffer != ID_TRUE )
                { 
                    sSBCB = sBCB->mSBCB;
                    if( sSBCB != NULL )
                    { 
#ifdef DEBUG
                        /* IOB 복사 전에 지워졌어야하지만 지우면됨
                           디버그에서만 사망 처리 */
                        IDE_RAISE( ERROR_INVALID_BCD )
#endif
                        sdsBufferMgr::removeBCB( aStatistics, 
                                                 sBCB->mSBCB ); 

                        sBCB->mSBCB = NULL;
                    }
                }
            }

            sBCB->unlockBCBMutex();

            if( aMoveToPrepare == ID_TRUE )
            {
                mPool->addBCB2PrepareLst( aStatistics, sBCB );
            }
        }

        if( aMoveToSBuffer == ID_TRUE )
        {
            /* movedown 한 Extent를 sdsFlusher가 flush할수있도록 상태를 변경한다.*/
            sBufferArea->changeStateMovedownDone( sExtentIndex );
        }

        /* BUG-23752: [SD] Buffer에서 Buffer Page를 Disk에 내릴때 Write하고 나서
         * fsync를 호출하고 있지 않습니다.
         *
         * WritePage가 발생한 TBS에 대해서 fsync를 호출한다. */
        if( needToSyncTBS( aMoveToSBuffer ) == ID_TRUE )
        {
            IDV_TIME_GET(&sBeginTime);
            IDE_TEST( syncAllFile4Flush( aStatistics ) != IDE_SUCCESS );
            IDV_TIME_GET(&sEndTime);

            sSyncTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
        }
        else 
        {
            sSyncTime = 0;
        }
        
        mStat.applyTotalFlushPages( mIOBPos );

        // IOB를 모두 disk에 내렸으므로 IOB를 초기화한다.
        mIOBPos = 0;
        mStat.applyINIOBCount( mIOBPos );

        SM_LSN_INIT( mMaxPageLSN );

        IDE_ASSERT( mMinRecoveryLSNMutex.lock(aStatistics) == IDE_SUCCESS );
        SM_LSN_MAX( mMinRecoveryLSN );
        IDE_ASSERT( mMinRecoveryLSNMutex.unlock() == IDE_SUCCESS );

        /* BUG-32670    [sm-disk-resource] add IO Stat information 
         * for analyzing storage performance.
         * Flush에 걸린 시간들을 저장함. */
        mStat.applyTotalLogSyncTimeUSec( sLogSyncTime );
        mStat.applyTotalWriteTimeUSec( sWriteTime );
        mStat.applyTotalSyncTimeUSec( sSyncTime );
        mStat.applyTotalDWTimeUSec( sDWTime );
    }
    else
    {
        // IOB가 비어있으므로 할게 없다.
    }

    return IDE_SUCCESS;

#ifdef DEBUG
    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                "invalid bcb :"
                "spaseID :%u\n"
                "pageID  :%u\n"
                "state   :%u\n"
                "CPListNo:%u\n"
                "HashTableNo:%u\n",
                sSBCB->mSpaceID,
                sSBCB->mPageID,
                sSBCB->mState,
                sSBCB->mCPListNo,
                sSBCB->mHashBucketNo );

        /* IOB에 복사전에 지워졌어야 한다. */
        IDE_DASSERT( 0 );
    }
#endif
    IDE_EXCEPTION_END;

    if (sWaitEventState == 1)
    {
        IDV_END_WAIT_EVENT(aStatistics, &sWeArgs);
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : mArrSyncFileInfo에 있는 File들을 Sort한후에
 *               File 당 한번만 Sync한다.
 *
 *  aStatistics     - [IN] 통계정보
 ******************************************************************************/
IDE_RC sdbFlusher::syncAllFile4Flush( idvSQL * aStatistics )
{
    UInt            i;
    sdbSyncFileInfo sLstSyncFileInfo = { SC_NULL_SPACEID, 0 };

    idlOS::qsort( (void*)mArrSyncFileInfo,
                  mIOBPos,
                  ID_SIZEOF( sdbSyncFileInfo ),
                  sdbCompareSyncFileInfo );

    for( i = 1 ; i < mIOBPos ; i++ )
    {
        /* 동일 File에 대해서 두번 Sync하지 않고 한번만 한다. */
        /* drop file의 경우 flusher job을 대기한다.
         * 그러므로 flusher에서는 drop file을 고려하지 않아도 된다.*/
        if (( sLstSyncFileInfo.mSpaceID != mArrSyncFileInfo[i].mSpaceID ) ||
            ( sLstSyncFileInfo.mFileID  != mArrSyncFileInfo[i].mFileID ))
        {
            IDE_ASSERT( mArrSyncFileInfo[i].mSpaceID != SC_NULL_SPACEID );

            sLstSyncFileInfo = mArrSyncFileInfo[i];

            IDE_TEST( sddDiskMgr::syncFile( aStatistics,
                                            sLstSyncFileInfo.mSpaceID,
                                            sLstSyncFileInfo.mFileID )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    IOB에 복사된 BCB들 중 recoveryLSN이 가장 작은 것을 얻는다.
 *    이 값은 checkpoint시 사용된다.
 *    restart redo LSN은 이 값과 CPListSet의 minRecoveryLSN중 작은 값으로
 *    결정된다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRet        - [OUT] 최소 recoveryLSN
 ******************************************************************************/
void sdbFlusher::getMinRecoveryLSN(idvSQL *aStatistics,
                                   smLSN  *aRet)
{
    IDE_ASSERT(mMinRecoveryLSNMutex.lock(aStatistics) == IDE_SUCCESS);

    SM_GET_LSN(*aRet, mMinRecoveryLSN);

    IDE_ASSERT(mMinRecoveryLSNMutex.unlock() == IDE_SUCCESS);
}

/******************************************************************************
 * Description :
 *    통계 정보를 시스템에 반영한다.
 ******************************************************************************/
void sdbFlusher::applyStatisticsToSystem()
{
    idvManager::applyStatisticsToSystem(&mCurrSess, &mOldSess);
}

/******************************************************************************
 * Description :
 *  하나의 작업을 끝마친 flusher가 쉬어야 할 시간을 지정해 준다.
 *
 *  aFlushedCount   - [IN]  flusher가 바로전 flush한 페이지 개수
 ******************************************************************************/
UInt sdbFlusher::getWaitInterval(ULong aFlushedCount)
{
    if (sdbFlushMgr::isBusyCondition() == ID_TRUE)
    {
        mWaitTime = 0;
    }
    else
    {
        if (aFlushedCount > 0)
        {
            mWaitTime = smuProperty::getDefaultFlusherWaitSec();
        }
        else
        {
            // 기존에 한건도 flush하지 않았다면, 푹 쉰다.
            if (mWaitTime < smuProperty::getMaxFlusherWaitSec())
            {
                mWaitTime++;
            }
        }
    }
    mStat.applyLastSleepSec(mWaitTime);

    return mWaitTime;
}

/* PROJ-2102 Secondary Buffer */
/******************************************************************************
 * Description :  SecondaryBuffer를 사용할때는 DW를 사용하지 않을수 있다.
 * DW를 사용하는지 판단하는 조건
 * 1. DW property가 설정되어야 하고
 * 2. Secondary Buffer를 사용 안함
 * 3. Secondary Buffer사용해도 cache type이 clean page 면
 * 4. Secondary Buffer사용해도 flush for replacement 가 아니면 Disk에 기록
 *    dirty page는 DW후 disk에 써야함.
 ******************************************************************************/
idBool sdbFlusher::needToUseDWBuffer( idBool aMoveToSBuffer )
{
    sdsSBufferType sSBufferType; 
    idBool sRet = ID_FALSE;
         
    sSBufferType = (sdsSBufferType)sdsBufferMgr::getSBufferType();

    /* case 1 */
    if( smuProperty::getUseDWBuffer() == ID_TRUE )
    {
        /* case 2  */
        if( mServiceable == ID_FALSE )
        {
            sRet = ID_TRUE;
        }
        else
        {   
            if( aMoveToSBuffer == ID_TRUE )
            {    
                 /* BUGBUG 
                  * CLEAN PAGE도 같이 DW에 써진다.
                  * 이문제랑 Secondary Buffer 에 한페이지씩 쓰는 문제를 해결하기위해
                  * IOB를 두벌 두는 방법을 고려해야 한다.
                  */

                 /* case 3 */
                if( sSBufferType == SDS_FLUSH_PAGE_CLEAN )
                {
                    sRet = ID_TRUE;
                }
                else 
                {
                    /* nothing to do */
                }
            }         
            else 
            {   /* case 4 */
                sRet = ID_TRUE;
            }
        }
    }
    else 
    {
        /* DW를 사용하지 않는다면 고려할것 없음.
         * nothing to do */
    }
    return sRet;
}

/******************************************************************************
 * Description : 해당 페이지가 Secondary Buffer에 써야 하는 페이지인지 판단
 ******************************************************************************/
idBool sdbFlusher::needToMovedownSBuffer( idBool aMoveToSBuffer, sdbBCB  * aBCB )
{
    sdsSBufferType sSBufferType; 

    idBool sRet = ID_FALSE;

    sSBufferType = (sdsSBufferType)sdsBufferMgr::getSBufferType();

    if( (mServiceable == ID_TRUE) && (aMoveToSBuffer == ID_TRUE) )
    {
        switch( sSBufferType )
        {   
            case SDS_FLUSH_PAGE_ALL:
                if( (aBCB->mPrevState == SDB_BCB_DIRTY) || 
                    (aBCB->mPrevState == SDB_BCB_CLEAN) )
                {
                    sRet = ID_TRUE;
                }
                else 
                {
                    /* nothing to do */
                }
                break;

            case SDS_FLUSH_PAGE_DIRTY:
                if( aBCB->mPrevState == SDB_BCB_DIRTY )
                {
                    sRet = ID_TRUE;
                }
                else 
                {
                    /* nothing to do */
                }
                break;

            case SDS_FLUSH_PAGE_CLEAN:
                if( aBCB->mPrevState == SDB_BCB_CLEAN )
                {
                    sRet = ID_TRUE;
                }
                else 
                {
                    /* nothing to do */
                }
                break;  

            default:
                ideLog::log( IDE_ERR_0,
                        "Unknown  Secondary Buffer Type:%u\n",
                        sSBufferType );
                IDE_DASSERT( 0 );
                break;
        }
    }
    else 
    {
        /* nothing to do */
    }
    return sRet;
}

idBool sdbFlusher::needToSkipBCB( sdbBCB  * aBCB )
{
    idBool sRet = ID_FALSE;

   // case 1: Secondary Buffer 를 사용하지 않는 경우 clean page 리턴..
   if( mServiceable != ID_TRUE )  
    {
        if( aBCB->mState == SDB_BCB_CLEAN )
        {
             sRet = ID_TRUE;
        }
        else 
        {
            /* nothing to do */
        }
    }
    else // Secondary Buffer 사용
    {  
    /*                             FSB               HDD
     * cacheType 이 ALL 일때 CLEAN + DIRTY   /        -
     *              CLEAN    CLEAN           /      DIRTY
     *              DIRTY    DIRTY           /        -      <--
     * case 2:  cacheType 이  DIRTY 면 clean page는  리턴 시킨다.
     */
        if( (sdsBufferMgr::getSBufferType() == SDS_FLUSH_PAGE_DIRTY) &&   
            (aBCB->mState == SDB_BCB_CLEAN) ) 
        {
             sRet = ID_TRUE;
        }
        else 
        {
            /* nothing to do */
        }
    } 
    return sRet;
}

idBool sdbFlusher::needToSyncTBS( idBool aMoveToSBuffer )
{
    sdsSBufferType sSBufferType; 

    idBool sRet = ID_TRUE;

    sSBufferType = (sdsSBufferType)sdsBufferMgr::getSBufferType();

    if( ( mServiceable == ID_TRUE ) &&
        ( aMoveToSBuffer == ID_TRUE ) )
    {
        switch( sSBufferType )
        {   
            case SDS_FLUSH_PAGE_ALL:
            case SDS_FLUSH_PAGE_DIRTY:
                sRet = ID_FALSE;
                break;

            case SDS_FLUSH_PAGE_CLEAN:
                break;  

            default:
                ideLog::log( IDE_ERR_0,
                        "Unknown  Secondary Buffer Type:%u\n",
                        sSBufferType );
                IDE_DASSERT( 0 );
                break;
        }
    }
    return sRet;
} 

void sdbFlusher::setDelayedFlushProperty( UInt aDelayedFlushListPct,
                                          UInt aDelayedFlushProtectionTimeMsec )
{
    mDelayedFlushListPct            = aDelayedFlushListPct;
    mDelayedFlushProtectionTimeUsec = aDelayedFlushProtectionTimeMsec * 1000;
}
