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
 *
 * PROJ-1438 Job Scheduler
 *
 **********************************************************************/

#include <mmtJobManager.h>
#include <qci.h>
#include <mmuProperty.h>
#include <mmErrorCode.h>
#include <mmtSessionManager.h>
#include <mmcStatementManager.h>
#include <mmm.h>

mmtJobScheduler * mmtJobManager::mJobScheduler;

mmtJobThread::mmtJobThread() : idtBaseThread()
{
}

mmtJobScheduler::mmtJobScheduler() : idtBaseThread()
{
}

IDE_RC mmtJobThread::initialize( UInt aIndex, iduQueue * aQueue )
{
    mRun       = ID_FALSE;
    mSession   = NULL;
    mIndex     = aIndex;
    mExecCount = 0;
    mQueue     = aQueue;

    return IDE_SUCCESS;
}

void mmtJobThread::finalize()
{
}

/**
 * mmtJobThread::initialize()
 *
 *  JobThread를 initialize 한다.
 *
 *  Queue Size 는 JobThreadQueueSize 만큼 할당한다.
 */
IDE_RC mmtJobThread::initializeThread()
{
    SChar sBuf[48];

    idlOS::snprintf( sBuf, 48, "MMT_JOB_THREAD_MUTEX_"ID_INT32_FMT, mIndex );

    IDE_TEST( mMutex.initialize((SChar *)sBuf,
                                IDU_MUTEX_KIND_POSIX,
                                IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( mThreadCV.initialize() != IDE_SUCCESS,
                    ContInitError );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ContInitError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_INIT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * mmtJobThread::finalize()
 *
 *  Queue를 finalize 하고 mutex와 cond를 destory 한다.
 */
void mmtJobThread::finalizeThread()
{
    ( void )mMutex.destroy();
    ( void )mThreadCV.destroy();
}

/**
 * mmtJobThread::signalToJobThread()
 *
 *  현재 Thread에 sianl을 보낸다.
 */
void mmtJobThread::signalToJobThread()
{
    idBool sIsLock = ID_FALSE;

    mMutex.trylock( sIsLock );

    if ( sIsLock == ID_TRUE )
    {
        ( void )mThreadCV.signal();
        signalUnlock();
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * mmtJobThread::run()
 *
 *  Job 을 실행한다. 실행할 Job 이 없다면 대기 상태로 대기하다가 Job Scheduler
 *  로 부터 signal을 받으면 Queue를 뒤져서 실행할 JobID를 꺼내고 이를 실행한다.
 */
void mmtJobThread::run()
{
    qciUserInfo    sUserInfo;
    vSLong         sItem  = 0;
    SInt           sJob;
    UInt           sState = 0;
    SInt           sSleep = 1;
    SInt           i;

    // internal userinfo를 생성한다.
    idlOS::memset(&sUserInfo, 0, ID_SIZEOF(sUserInfo));
    mtl::makeNameInSQL( sUserInfo.loginID, (SChar *) "SYS", 3 );
    sUserInfo.loginUserID      = QC_SYS_USER_ID;   /* BUG-41561 */
    sUserInfo.userID           = QC_SYS_USER_ID;
    sUserInfo.loginPassword[0] = '\0';
    sUserInfo.tablespaceID     = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
    sUserInfo.tempTablespaceID = SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP;

    mRun = ID_TRUE;

    signalLock();
    sState = 1;

    while ( mRun == ID_TRUE )
    {
        IDE_CLEAR();

        IDE_TEST_RAISE( mThreadCV.wait(&mMutex) != IDE_SUCCESS, CondWaitError );

        if ( mRun == ID_FALSE )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }

        while ( iduQueueDequeue( mQueue, (void **)&sItem ) == IDE_SUCCESS )
        {
            sJob = (SInt)sItem;

            ideLog::log( IDE_JOB_32, "[JOB SCHEDLER : JOB THREAD %d JOB %d ASSIGNED]",
                         mIndex, sJob );

            sSleep = 1;
            while ( mmtSessionManager::allocInternalSession( &mSession, &sUserInfo )
                    != IDE_SUCCESS )
            {
                for ( i = 0; i < sSleep; i++ )
                {
                    idlOS::sleep(1);
                    if ( mRun == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                if ( mRun == ID_FALSE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                /* Max 1시간( 3600 초 )으로 점점 Sleep 시간이 늘어난다. */
                if ( sSleep < 3600 )
                {
                    sSleep = sSleep << 1;
                }
                else
                {
                    sSleep = 3600;
                }
            }

            if ( mRun == ID_FALSE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            /* 1 session에 1 job만 실행한다. */
            qciMisc::executeJobItem( mIndex, sJob, (void *)mSession );

            mSession->setSessionState( MMC_SESSION_STATE_END );
            IDU_FIT_POINT( "mmtJobThread::run::endSession" );
            ( void )mSession->endSession();
            ( void )mmtSessionManager::freeInternalSession( mSession );

            mSession = NULL;
            mExecCount++;
        }
    }

    sState = 0;
    signalUnlock();

    return;

    IDE_EXCEPTION( CondWaitError );
    {
        IDE_SET( ideSetErrorCode( mmERR_FATAL_THREAD_CONDITION_WAIT ) );
    }
    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        signalUnlock();
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * mmtJobThread::stop()
 *
 *   Job Thread 를 Stop 한다.
 *
 *   START SHUTDOWN IMMEDIATE 와 같은 경우 Session에 CLOSED flag를 세팅해
 *   Job 이 실행중이더라도 Job 을 종료할 수 있도록 한다.
 */
void mmtJobThread::stop()
{
    mRun = ID_FALSE;

    if ( mmm::getServerStatus() == ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE )
    {
        if ( mSession != NULL )
        {
            IDU_SESSION_SET_CLOSED(*mSession->getEventFlag());
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

    signalLock();
    ( void )mThreadCV.signal();
    signalUnlock();
}

IDE_RC mmtJobScheduler::initialize()
{
    mRun         = ID_FALSE;
    mThreadCount = mmuProperty::getJobThreadCount();

    return IDE_SUCCESS;
}

void mmtJobScheduler::finalize()
{
}

/**
 * mmtJobScheduler::initialize()
 *
 * Job Scheduler 를 initialize 한다.
 *
 *  Job JobThreadCount 만큼 Job Thread를 생성한다.
 *
 */
IDE_RC mmtJobScheduler::initializeThread()
{
    UInt           i;
    mmtJobThread * sThread = NULL;
    UInt           sInit = 0;
    idBool         sInitQueue = ID_FALSE;

    IDE_TEST( iduQueueInitialize( &mQueue,
                                  mmuProperty::getJobThreadQueueSize(),
                                  IDU_QUEUE_TYPE_DUALLOCK,
                                  IDU_MEM_MMT )
              != IDE_SUCCESS);
    sInitQueue = ID_TRUE;

    for ( i = 0; i < MMT_JOB_MANAGER_MAX_THREADS; i++ )
    {
        mJobThreads[i] = NULL;
    }

    for ( sInit = 0; sInit < mThreadCount; sInit++ )
    {
        IDU_FIT_POINT_RAISE( "mmtJobScheduler::initialize::malloc::Thread",
                              ERR_INSUFFICIENT_MEMORY );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT,
                                         ID_SIZEOF(mmtJobThread),
                                         (void **)&sThread,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_INSUFFICIENT_MEMORY);

        mJobThreads[sInit] = new ( sThread )mmtJobThread();

        IDE_TEST( mJobThreads[sInit]->initialize( sInit, &mQueue ) != IDE_SUCCESS );
    }

    for ( i = 0; i < mThreadCount; i++ )
    {
        IDE_TEST( mJobThreads[i]->start() != IDE_SUCCESS );
    }

    for ( i = 0; i < mThreadCount; i++ )
    {
        IDE_TEST( mJobThreads[i]->waitToStart() != IDE_SUCCESS );

        ideLog::log( IDE_JOB_0, "[JOB SCHEDLER : JOB THREAD %d STARTED]", i );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }

    IDE_EXCEPTION_END;

    for ( i = 0 ; i < mThreadCount; i++ )
    {
        if ( mJobThreads[i] != NULL )
        {
            if ( mJobThreads[i]->isStarted() == ID_TRUE )
            {
                mJobThreads[i]->stop();

                IDE_ASSERT(mJobThreads[i]->join() == IDE_SUCCESS );

            }
            else
            {
                /* Nothing to do */
            }

            if ( i < sInit )
            {
                mJobThreads[i]->finalize();
            }
            else
            {
                /* Nothing to do */
            }
            ( void )iduMemMgr::free( mJobThreads[i] );
            mJobThreads[i] = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sInitQueue == ID_TRUE )
    {
        ( void )iduQueueFinalize(&mQueue);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/**
 * mmtJobScheduler::finalize()
 *
 *  Job Scheduler 를 종료한다.
 *
 *   Job Thread의 수만큼 Stop하면서 종료될때 까지 대기한다.
 */
void mmtJobScheduler::finalizeThread()
{
    UInt  i;

    for ( i = 0 ; i < mThreadCount; i++ )
    {
        mJobThreads[i]->stop();
    }

    for ( i = 0 ; i < mThreadCount; i++ )
    {
        IDE_ASSERT( mJobThreads[i]->join() == IDE_SUCCESS );
    }

    for ( i = 0 ; i < mThreadCount; i++ )
    {
        mJobThreads[i]->finalize();
        ( void )iduMemMgr::free( mJobThreads[i] );
    }

    ( void )iduQueueFinalize(&mQueue);
}

/**
 * mmtJobScheduler::run()
 *
 *  JobScheduler 는 주기적으로 Meta를 조회해서 Job을 실행한다.
 *
 *  1. JOB_SCHEDULER_ENABLE 프러퍼티가 활성화 되어있다면 Meta를 조회한다.
 *  2. 실행할 JOB이 있다면 이를 JOB_THREAD의 QUEUE에 넣어 준다.
 *  3. 대기중인 Job Thread를 깨운다.
 *  4. 다음 분까지 Sleep 한다.
 */
void mmtJobScheduler::run()
{
    PDL_Time_Value  sSleepTime;
    PDL_Time_Value  sCurTime;

    struct tm   sLocaltime;
    time_t      sTime1;
    time_t      sTime2;
    SInt        sItems[MMT_JOB_MAX_ITEMS];
    vSLong      sJob;
    UInt        sCount;
    UInt        sThreadID;
    SLong       sRemainSeconds;
    idBool      sFirstLog;
    UInt        i;

    mRun      = ID_TRUE;

    sCurTime  = idlOS::gettimeofday();
    sTime1    = (time_t)sCurTime.sec();

    if ( ( qciMisc::isExecuteForNatc() == ID_FALSE ) &&
         ( mmuProperty::getJobSchedulerEnable() == 1 ) )
    {
        idlOS::gmtime_r( &sTime1, &sLocaltime );
        sRemainSeconds = ( 60 - sLocaltime.tm_sec );
        sSleepTime.set( sRemainSeconds, 0 );
    }
    else
    {
        sSleepTime.set( 1 , 0 );
    }

    idlOS::sleep(sSleepTime);

    while ( mRun == ID_TRUE )
    {
        IDE_CLEAR();

        sCurTime = idlOS::gettimeofday();
        sTime1   = (time_t)sCurTime.sec();

        /* 1. JOB_START 프러퍼티가 활성화 되어있다면 Meta를 조회한다. */
        sCount   = 0;
        if ( mmuProperty::getJobSchedulerEnable() == 1 )
        {
            qciMisc::getExecJobItems( sItems,
                                      &sCount,
                                      MMT_JOB_MAX_ITEMS );
        }
        else
        {
            /* Nothing to do */
        }

        /* 2. 실행할 JOB이 있다면 이를 JOB SCHEDULER의 QUEUE에 넣어준다. */
        for ( i = 0; ( i < sCount ) && ( mRun == ID_TRUE ); i++ )
        {
            sJob = (vSLong)sItems[i];
            sFirstLog = ID_TRUE;

            while ( iduQueueEnqueue( &mQueue, (void *)sJob ) != IDE_SUCCESS )
            {
                if ( sFirstLog == ID_TRUE )
                {
                    ideLog::log( IDE_JOB_0, "[JOB SCHEDLER : JOB QUEUE FULL]" );
                    sFirstLog = ID_FALSE;
                }
                else
                {
                    /* Nothing to do */
                }

                /* Shutdown 명령어가 들어오는지 체크한다 */
                if ( mRun == ID_FALSE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                idlOS::thr_yield();
            }
        }

        if ( sCount > 0 )
        {
            /* enqueue가 성공하면 전체 broadcast한다. */
            for ( sThreadID = 0; sThreadID < mThreadCount; sThreadID++ )
            {
                if ( mRun == ID_FALSE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
                mJobThreads[sThreadID]->signalToJobThread();
            }
        }
        else
        {
            /* Nothing to do */
        }

        sCurTime = idlOS::gettimeofday();
        sTime2   = (time_t)sCurTime.sec();

        /* 4. 다음 분까지 Sleep 한다. */
        if ( ( qciMisc::isExecuteForNatc() == ID_FALSE ) &&
             ( mmuProperty::getJobSchedulerEnable() == 1 ) )

        {
            idlOS::gmtime_r( &sTime2, &sLocaltime );

            if ( sTime2 - sTime1 >= 60 )
            {
                continue;
            }
            else
            {
                sRemainSeconds = ( 60 - sLocaltime.tm_sec );
            }

            /* 다음 분이 될때 까지 1초씩 쉬면서 대기한다 */
            for ( i = 0; i < sRemainSeconds; i++ )
            {
                /* Shutdown 명령어가 들어오는지 체크한다 */
                if ( ( mRun == ID_FALSE ) ||
                     ( qciMisc::isExecuteForNatc() == ID_TRUE ) )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
                idlOS::sleep(1);
            }
        }
        else
        {
            /* Natc 수행중에는 1 초 만 Sleep한다 */
            idlOS::sleep(1);
        }
    }
}

void mmtJobScheduler::stop()
{
    mRun = ID_FALSE;

    IDE_ASSERT(join() == IDE_SUCCESS);
}

/**
 * mmtJobManager::initialize()
 *
 * JobThreadCount 가 0 보다 클경우 Job Scheduler 를 생성한다.
 */
IDE_RC mmtJobManager::initialize()
{
    mmtJobScheduler * sScheduler = NULL;
    idBool            sIsInit    = ID_FALSE;

    qciMisc::resetInitialJobState();

    if ( mmuProperty::getJobThreadCount() > 0 )
    {
        IDE_TEST_RAISE( ( mmuProperty::getJobThreadCount() +
                          mmuProperty::getMaxClient() ) > MMC_STMT_ID_SESSION_MAX,
                          ERR_EXCEEDS_MAX_SESSION );

        IDU_FIT_POINT_RAISE( "mmtJobManager::initialize::malloc::Scheduler",
                              ERR_INSUFFICIENT_MEMORY );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT,
                                         ID_SIZEOF(mmtJobScheduler),
                                         (void **)&sScheduler,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_INSUFFICIENT_MEMORY);

        mJobScheduler = new ( sScheduler )mmtJobScheduler();

        IDE_TEST( mJobScheduler->initialize() != IDE_SUCCESS );
        sIsInit = ID_TRUE;
        IDE_TEST( mJobScheduler->start() != IDE_SUCCESS );
        IDE_TEST( mJobScheduler->waitToStart() != IDE_SUCCESS );
    }
    else
    {
        mJobScheduler = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEEDS_MAX_SESSION )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_EXCEEDS_MAX_SESSION ) );
    }
    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;
    {
        if ( sScheduler != NULL )
        {
            if ( mJobScheduler->isStarted() == ID_TRUE )
            {
                mJobScheduler->stop();
            }
            else
            {
                /* Nothing to do */
            }
            if ( sIsInit == ID_TRUE )
            {
                mJobScheduler->finalize();
            }
            else
            {
                /* Nothing to do */
            }

            ( void )iduMemMgr::free( mJobScheduler );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_FAILURE;
}

/**
 * mmtJobManager::finalize()
 *
 *  Job Manager 를 종료한다. JobScheduler를 종료하도록 호출한다.
 */
IDE_RC mmtJobManager::finalize()
{
    if ( mJobScheduler != NULL )
    {
        mJobScheduler->stop();
        mJobScheduler->finalize();

        ( void )iduMemMgr::free( mJobScheduler );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;
}
