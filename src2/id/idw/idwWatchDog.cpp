/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#include <idl.h>
#include <idv.h>
#include <idErrorCode.h>
#include <idu.h>
#include <idwWatchDog.h>
#include <idwPMMgr.h>

idrRecProcess     * idwWatchDog::mArrRecProcess;
UInt                idwWatchDog::mMaxRecProcessCnt;

iduCond             idwWatchDog::mCV;
iduMutex            idwWatchDog::mMutex;
idBool              idwWatchDog::mFinish;

iduShmStStmtRemoveFunc idwWatchDog::mStStmtRmFunc;
idWatchDogStatus       idwWatchDog::mWatchDogStatus;

idwStWatchDogInfo * idwWatchDog::mStWatchDogInfo;

idwWatchDog gIdwWatchDog;

idwWatchDog::idwWatchDog()
        : idtBaseThread()
{

}

idwWatchDog::~idwWatchDog()
{

}

IDE_RC idwWatchDog::initialize( UInt aMaxRecProcessCnt )
{
    UInt            i, j;
    UInt            sState1  = 0;
    UInt            sState2  = 0;
    iduShmTxInfo  * sTxInfo = NULL;
    idvSQL          sStatistics;
    idrSVP          sVSavepoint;

    mFinish = ID_FALSE;

    idvManager::initSQL( &sStatistics, NULL /* Session */ );

    IDV_WP_SET_PROC_INFO( &sStatistics, idwPMMgr::getMainShmProcInfo() );

    IDE_TEST( idwPMMgr::allocThrInfo( &sStatistics,
                                      IDU_SHM_THR_DEFAULT_ATTR,
                                      &sTxInfo )
              != IDE_SUCCESS );
    sState1 = 1;

    idrLogMgr::setSavepoint( sTxInfo, &sVSavepoint );

    mMaxRecProcessCnt = aMaxRecProcessCnt;

    IDE_TEST( iduShmMgr::allocMem( &sStatistics,
                                   sTxInfo,
                                   IDU_MEM_ID_WATCHDOG,
                                   ID_SIZEOF(idwStWatchDogInfo),
                                   (void**)&mStWatchDogInfo )
              != IDE_SUCCESS );

    idrLogMgr::commit2Svp( &sStatistics, sTxInfo, &sVSavepoint );

    mStWatchDogInfo->mAddrSelf       = iduShmMgr::getAddr( mStWatchDogInfo );
    mStWatchDogInfo->mDeadProcessCnt = 0;

    idwWatchDog::mStStmtRmFunc = NULL;

    iduShmMgr::setMetaBlockAddr( IDU_SHM_META_WATCHDOG_BLOCK,
                                 mStWatchDogInfo->mAddrSelf );

    sState1 = 0;
    IDE_TEST( idwPMMgr::freeThrInfo( &sStatistics,
                                     sTxInfo )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_WATCHDOG,
                                 ID_SIZEOF(idrRecProcess) * aMaxRecProcessCnt,
                                 (void**)&mArrRecProcess )
              != IDE_SUCCESS );
    sState2 = 1;

    for( i = 0; i < mMaxRecProcessCnt; i++ )
    {
        new ( mArrRecProcess + i ) idrRecProcess;

        IDE_TEST( mArrRecProcess[i].initialize( idwPMMgr::getProcInfo( i ) )
                  != IDE_SUCCESS );
        sState2 = 2;
    }

    IDE_TEST( mMutex.initialize( (SChar*)"WATCHDOG_THREAD_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );
    sState2 = 3;

    IDE_TEST_RAISE( mCV.initialize( (SChar *)"WATCHDOG_THREAD_COND" ) != IDE_SUCCESS,
                    err_cond_var_init );
    sState2 = 4;

    mFinish = ID_FALSE;

    mWatchDogStatus = ID_WATCHDOG_BEGIN;

    IDE_TEST( gIdwWatchDog.startThread() != IDE_SUCCESS );
    sState2 = 5;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_var_init );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondInit ) );
    }
    IDE_EXCEPTION_END;

    switch( sState2 )
    {
    case 5:
        (void)gIdwWatchDog.shutdown();
    case 4:
        (void)mCV.destroy();
    case 3:
        IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
    case 2:
        if( i != 0 )
        {
            for( j = i - 1; j != 0; j-- )
            {
                delete ( mArrRecProcess + j );

                IDE_ASSERT( mArrRecProcess[i].destroy() == IDE_SUCCESS );
            }
        }
    case 1:
        IDE_ASSERT( iduMemMgr::free( mArrRecProcess ) == IDE_SUCCESS );
    default:
        break;
    }

    if( sState1 != 0 )
    {
        idrLogMgr::abort2Svp( &sStatistics, sTxInfo, &sVSavepoint );

        IDE_ASSERT( idwPMMgr::freeThrInfo( &sStatistics,
                                           sTxInfo )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC idwWatchDog::destroy()
{
    IDE_ASSERT( gIdwWatchDog.shutdown() == IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::free( mArrRecProcess ) == IDE_SUCCESS );

    IDE_TEST_RAISE( mCV.destroy() != IDE_SUCCESS, err_cond_destroy );

    IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondDestroy ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idwWatchDog::attach()
{
    idShmAddr sAddr4WatchDog;

    sAddr4WatchDog = iduShmMgr::getMetaBlockAddr( IDU_SHM_META_WATCHDOG_BLOCK );

    mStWatchDogInfo = (idwStWatchDogInfo*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4WatchDog );

    IDE_ASSERT( mStWatchDogInfo->mAddrSelf == sAddr4WatchDog );
}

void idwWatchDog::detach()
{
    mStWatchDogInfo = NULL;
}

void idwWatchDog::run()
{
    iduShmProcInfo     * sDeadProcInfo;
    iduShmProcInfo     * sDaemonProcInfo;
    time_t               sUntilTime;
    PDL_Time_Value       sWaitTV;
    idvSQL               sStatistics;
    SInt                 sRC;
    idBool               sAcquired = ID_FALSE;

    idvManager::initSQL( &sStatistics, NULL /* Session */ );

    IDE_TEST( lock( &sStatistics ) != IDE_SUCCESS);

    sDeadProcInfo = NULL;

    sDaemonProcInfo = idwPMMgr::getDaemonProcInfo();

    mWatchDogStatus = ID_WATCHDOG_RUN;

    /* Process register table을 계속 Monitoring한다. */
    while( mFinish == ID_FALSE )
    {
        /* BUG-41127 */
        /* 아래의 자기 자신에 대한 검사는 세마포어의 무결성을 검사하기 위한 것이다.
         * 와치도그는 세마포어가 내려가서 죽은 경우
         *  - W-Server와 R-Server가 먼저 죽은 경우, 해당 프로세스에 대하여
         *    checkProcAliveByLPID를 하는 과정에 tryAcquireSem4Proc를 하지 않는다.
         *    이로 인하여 와치-독 자체에서 세마포어가 내려간 것을 인지하지 못하고
         *    자신을 죽이지 않는다.
         * 와 같은 현상이 발생하여 세마포어가 비정상적인 데몬이 지속적으로 존재하게 된다.
         * 이를 방지하기 위하여 자신의 프로세스에 대한 정보를 감시하면서 세마포어의 정상
         * 여부를 모니터링(tryAcquireSem4Proc)하게 한다.
         */
        IDE_TEST( idwPMMgr::tryAcquireSem4Proc( sDaemonProcInfo, &sAcquired)
                  != IDE_SUCCESS );
        if ( sAcquired == ID_TRUE )
        {
            IDE_TEST( idwPMMgr::releaseSem4Proc( sDaemonProcInfo ) != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( idwPMMgr::getNxtDeadProc( &sDeadProcInfo )
                  != IDE_SUCCESS );

        if( sDeadProcInfo != NULL )
        {
            if( sDeadProcInfo->mState == IDU_SHM_PROC_STATE_DEAD )
            {
                mStWatchDogInfo->mDeadProcessCnt++;

                idwPMMgr::setProcState( sDeadProcInfo, IDU_SHM_PROC_STATE_RECOVERY );
                IDE_TEST( mArrRecProcess[ sDeadProcInfo->mLPID ].startThread()
                          != IDE_SUCCESS );
                continue;
            }
        }

        sUntilTime = idlOS::time(NULL) + IDW_MAX_SLEEP_SEC;

        sWaitTV.set( sUntilTime );

        sRC = mCV.timedwait( &mMutex, &sWaitTV, IDU_IGNORE_TIMEDOUT );

        IDE_TEST_RAISE( sRC != IDE_SUCCESS, err_cond_wait );
    }

    IDE_TEST( idwPMMgr::getNxtDeadProc( &sDeadProcInfo )
              != IDE_SUCCESS );

    if( sDeadProcInfo != NULL )
    {
        if( sDeadProcInfo->mState == IDU_SHM_PROC_STATE_DEAD )
        {
            idwPMMgr::setProcState( sDeadProcInfo, IDU_SHM_PROC_STATE_RECOVERY );
            IDE_TEST( mArrRecProcess[ sDeadProcInfo->mLPID ].startThread()
                      != IDE_SUCCESS );
        }
    }

    IDE_TEST( unlock() != IDE_SUCCESS );

    return;

    IDE_EXCEPTION( err_cond_wait );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondWait ) );
    }
    IDE_EXCEPTION_END;

    mWatchDogStatus = ID_WATCHDOG_END;

    /* Watch Dog Process의 종료에 대한 Fatal Error를 설정한다. */

    IDE_SET( ideSetErrorCode( idERR_FATAL_WATCHDOG_THEAD_ABNORMAL_EXIT ) );

    IDE_CALLBACK_FATAL( "Watch Dog Thread Error" );

    return;
}

IDE_RC idwWatchDog::startThread()
{
    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwWatchDog::shutdown()
{
    SInt          sState = 0;

    IDE_TEST( lock( NULL /* idvSQL* */ ) != IDE_SUCCESS );
    sState = 1;

    if( mFinish != ID_TRUE )
    {
        mFinish = ID_TRUE;

        mWatchDogStatus = ID_WATCHDOG_END;

        IDE_TEST_RAISE( mCV.signal() != IDE_SUCCESS, err_cond_signal );

        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );

        IDE_TEST_RAISE( this->join() != IDE_SUCCESS, err_thr_join );
    }
    else
    {
        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );

        (void)this->join();
    }

    IDE_TEST( shutdownAllRecProc() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_thr_join );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_Systhrjoin ) );
    }
    IDE_EXCEPTION( err_cond_signal );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ThrCondSignal ) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC idwWatchDog::shutdownAllRecProc()
{
    UInt i;

    for( i = 0; i < mMaxRecProcessCnt; i++ )
    {
        IDE_TEST( mArrRecProcess[i].shutdown() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
