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
 * $Id: smrChkptThread.cpp 82426 2018-03-09 05:12:27Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smm.h>
#include <smu.h>
#include <smr.h>

idBool gIsChkptThreadInitialized = ID_FALSE;

smrChkptThread gSmrChkptThread;

smrChkptThread::smrChkptThread() : idtBaseThread()
{

}

smrChkptThread::~smrChkptThread()
{

}


IDE_RC smrChkptThread::initialize()
{

    /* INITIALIZE PROPERTIES */

    mFinish = ID_FALSE;
    mResume = ID_TRUE;
    mChkptType = SMR_CHKPT_TYPE_BOTH;

    IDE_TEST(mMutex.initialize((SChar*)"CHECKPOINT_THREAD_MUTEX",
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(mCV.initialize((SChar *)"CHECKPOINT_THREAD_COND") != IDE_SUCCESS,
                   err_cond_var_init);

    gIsChkptThreadInitialized = ID_TRUE;

    idvManager::initSession(&mOldSess, 0 /* unuse */, NULL /* unuse */);

    // BUG-21155 : current session을 초기화
    idvManager::initSession(&mCurrSess, 0 /* unuse */, NULL /* unuse */);

    idvManager::initSQL( &mStatistics,
                         &mCurrSess,
                         NULL, NULL, NULL, NULL, IDV_OWNER_CHECKPOINTER );

    IDE_TEST( mFlusher.initialize( ID_UINT_MAX,
                                   SD_PAGE_SIZE,
                                   smuProperty::getBufferIOBufferSize(),
                                   sdbBufferMgr::getPool()->getCPListSet() )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smrChkptThread::startThread()
{

    IDE_TEST(start() != IDE_SUCCESS);
    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smrChkptThread::destroy()
{

    IDE_TEST_RAISE(mCV.destroy() != IDE_SUCCESS, err_cond_destroy);

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    gIsChkptThreadInitialized = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*
    Checkpoint가 수행되지 않도록 막는다.

    unblockCheckpoint를 호출해야 Checkpoint가 동작한다.
 */
IDE_RC smrChkptThread::blockCheckpoint()
{
    if ( gIsChkptThreadInitialized == ID_TRUE )
    {
        IDE_TEST( gSmrChkptThread.lock( NULL /* idvSQL* */)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Checkpoint가 다시 수행되도록 Checkpoint를 Unblock한다.
 */
IDE_RC smrChkptThread::unblockCheckpoint()
{
    if ( gIsChkptThreadInitialized == ID_TRUE )
    {
        IDE_TEST( gSmrChkptThread.unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smrChkptThread::run()
{
    IDE_RC    rc;
    UInt      sState = 0;
    time_t    sUntilTime;
    struct tm sUntil;

  startPos:
    sState = 0;

    IDE_TEST( lock( NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    if(smuProperty::getChkptEnabled() != 0)
    {
        IDE_TEST( smrRecoveryMgr::checkpoint( NULL,  /* idvSQL* */
                                              SMR_CHECKPOINT_BY_SYSTEM,
                                              mChkptType )
                  != IDE_SUCCESS );
    }

    // timewait 시간 설정
    sUntilTime = idlOS::time(NULL) + smuProperty::getChkptIntervalInSec();

    while(mFinish == ID_FALSE)
    {
        mChkptType = SMR_CHKPT_TYPE_BOTH;
        mResume = ID_FALSE;

        if (smuProperty::getChkptEnabled() == 0)
        {

            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_CHKP_DISABLE_THREAD);

            rc = mCV.wait(&mMutex);
            IDE_TEST_RAISE(rc != IDE_SUCCESS, err_cond_wait);
        }
        else
        {
            idlOS::localtime_r((time_t*)&sUntilTime, &sUntil);


            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_CHKP_SLEEP_THREAD,
                        sUntil.tm_year + 1900,
                        sUntil.tm_mon + 1,
                        sUntil.tm_mday,
                        sUntil.tm_hour,
                        sUntil.tm_min,
                        sUntil.tm_sec);

            mTV.set(sUntilTime);

            rc = mCV.timedwait(&mMutex, &mTV);
        }

        if(mFinish == ID_TRUE)
        {
            break;
        }

        if ( smuProperty::isRunCheckpointThread() == SMU_THREAD_OFF )
        {
            // To Fix PR-14783
            // System Thread의 작업을 수행하지 않도록 한다.
            sUntilTime
                = idlOS::time(NULL) + 
                  smuProperty::getChkptIntervalInSec();
            continue;
        }
        else
        {
            // Go Go
        }

        if ( rc != IDE_SUCCESS )
        {
            IDE_TEST_RAISE(mCV.isTimedOut() != ID_TRUE, err_cond_wait);
            mReason = SMR_CHECKPOINT_BY_TIME;
            mResume = ID_TRUE;
        }
        else if ( mResume == ID_FALSE )
        {
            // clear checkpoint interval의 경우이므로 time을 reset 한다.
            sUntilTime = idlOS::time(NULL) +
                smuProperty::getChkptIntervalInSec();

            continue;
        }

        // BUG-22060 로 인하여 disk backup중에 disk checkpoint 뿐만
        // 아니라 memory checkpoint 도 문제가 될 수 있습니다. 그래서
        // backup중에는 memory, disk 양쪽 모두 checkpoint를 막습니다.

        if ( ((smrBackupMgr::getBackupState() & SMR_BACKUP_MEMTBS)
              == SMR_BACKUP_MEMTBS) ||
             ((smrBackupMgr::getBackupState() & SMR_BACKUP_DISKTBS)
              == SMR_BACKUP_DISKTBS) )
        {
            sUntilTime = idlOS::time(NULL) +
                smuProperty::getChkptIntervalInSec();

            continue;
        }

        switch ( mChkptType )
        {
            case SMR_CHKPT_TYPE_DRDB :
                {
                    IDE_TEST( smrRecoveryMgr::checkpointDRDB(NULL /* idvSQL* */)
                              != IDE_SUCCESS );
                    break;
                }
            case SMR_CHKPT_TYPE_MRDB :
            case SMR_CHKPT_TYPE_BOTH :
                {
                    if ( mReason == SMR_CHECKPOINT_BY_LOGFILE_SWITCH )
                    {
                        IDE_TEST( smrLogMgr::clearLogSwitchCount()
                                  != IDE_SUCCESS );
                    }

                    IDE_TEST( smrRecoveryMgr::checkpoint( NULL,  /* idvSQL* */
                                                          mReason,
                                                          mChkptType )
                              != IDE_SUCCESS );

                    // 메모리 체크포인트 수행시에만 INTERVAL을
                    // 갱신하여 적용한다.
                    sUntilTime = idlOS::time(NULL)
                                 + smuProperty::getChkptIntervalInSec();

                    break;
                }
            default :
                IDE_ASSERT(0);
                break;
       }
    }

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if(sState != 0)
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }

        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING, SM_TRC_MRECOVERY_CHKP_WARNING);
    }
    IDE_POP();

    goto startPos;
}

IDE_RC smrChkptThread::resumeAndWait( idvSQL * aStatistics )
{

    UInt          sState = 0;

    IDE_TEST(lock( NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    IDE_TEST_RAISE( smrBackupMgr::getBackupState() != SMR_BACKUP_NONE,
                    err_checkpoint_by_backup );
    IDE_TEST( smrRecoveryMgr::checkpoint( aStatistics,
                                          SMR_CHECKPOINT_BY_USER,
                                          SMR_CHKPT_TYPE_BOTH,
                                          ID_TRUE )
              != IDE_SUCCESS );

    // Reset Checkpoint Interval ( Log File and Time )
    IDE_TEST(clearCheckPTInterval(ID_TRUE, ID_TRUE) != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_checkpoint_by_backup );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_BACKUP_GOING ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if(sState != 0)
    {
        IDE_ASSERT ( unlock() == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;

}

IDE_RC smrChkptThread::resumeAndNoWait( SInt         aByWho, 
                                        smrChkptType aChkptType )
{

    SInt    sState = 0;
    idBool  sLock;

    IDE_TEST(mMutex.trylock(sLock) != IDE_SUCCESS);

    if(sLock == ID_TRUE)
    {
        while(1)
        {
            sState = 1;

            if(mResume == ID_TRUE)
            {
                IDE_TEST( unlock() != IDE_SUCCESS);
                sState = 0;
                break;
            }

            mResume    = ID_TRUE;
            mChkptType = aChkptType;
            mReason    = aByWho;

            IDE_TEST_RAISE(mCV.signal() != IDE_SUCCESS, err_cond_signal);

            IDE_TEST( unlock() != IDE_SUCCESS);
            sState = 0;
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( unlock() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

IDE_RC smrChkptThread::setCheckPTLSwitchInterval()
{
    SInt sState = 0;

    IDE_TEST(lock( NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    /* ------------------------------------------------
     * CHECKPOINT_INTERVAL_IN_LOG
     * ----------------------------------------------*/
//    SMU_CHECKPOINT_INTERVAL_IN_LOG = aLogFileCountInterval;

    IDE_TEST(clearCheckPTInterval(ID_TRUE, ID_FALSE) != IDE_SUCCESS);

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;
}

IDE_RC smrChkptThread::setCheckPTTimeInterval()
{

    SInt sState = 0;

    IDE_TEST( lock( NULL /* idvSQL* */ ) != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * CHECKPOINT_INTERVAL_IN_SEC
     * ----------------------------------------------*/
//    SMU_CHECKPOINT_INTERVAL_IN_SEC = aTimeInterval;

    IDE_TEST(clearCheckPTInterval(ID_FALSE, ID_TRUE) != IDE_SUCCESS);

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC smrChkptThread::clearCheckPTInterval(idBool aLog, idBool aTime)
{

    if(aLog == ID_TRUE)
    {
        IDE_TEST(smrLogMgr::clearLogSwitchCount() != IDE_SUCCESS);
    }

    if(aTime == ID_TRUE)
    {
        mResume = ID_FALSE;

        IDE_TEST_RAISE(mCV.signal() != IDE_SUCCESS, err_cond_signal);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smrChkptThread::shutdown()
{
    SInt          sState = 0;

    IDE_TEST( lock( NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    mFinish = ID_TRUE;

    IDE_TEST_RAISE(mCV.signal() != IDE_SUCCESS, err_cond_signal);

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    IDE_TEST_RAISE(join() != IDE_SUCCESS, err_thr_join);

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

    if(sState != 0)
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

void smrChkptThread::applyStatisticsForSystem()
{
    idvManager::applyStatisticsToSystem( &mCurrSess,
                                         &mOldSess );
}

IDE_RC smrChkptThread::flushForCheckpoint(idvSQL       *aStatistics,
                                          ULong         aMinFlushCount,
                                          ULong         aRedoDirtyPageCnt,
                                          UInt          aRedoLogFileCount,
                                          ULong        *aFlushedCount)
{

    IDE_TEST( gSmrChkptThread.mFlusher.flushForCheckpoint( aStatistics,
                                                           aMinFlushCount,
                                                           aRedoDirtyPageCnt,
                                                           aRedoLogFileCount,
                                                           SDB_CHECKPOINT_BY_CHKPT_THREAD,
                                                           aFlushedCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
