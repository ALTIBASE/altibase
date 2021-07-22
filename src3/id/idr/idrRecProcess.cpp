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
#include <ide.h>
#include <idl.h>
#include <idrLogMgr.h>
#include <idrRecProcess.h>
#include <idrRecThread.h>
#include <idw.h>

idrRecProcess::idrRecProcess()
        : idtBaseThread()
{

}

idrRecProcess::~idrRecProcess()
{

}

IDE_RC idrRecProcess::initialize( iduShmProcInfo  * aDeadProcInfo )
{
    mDeadProcInfo = aDeadProcInfo;
    return IDE_SUCCESS;
}

IDE_RC idrRecProcess::destroy()
{
    IDE_TEST( shutdown() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idrRecProcess::run()
{
    UInt              i;
    iduShmTxInfo    * sCurDeadTxInfo;
    idrRecThread    * sArrThread;
    idShmAddr         sHeadAddr;
    idvSQL            sStatistics;
    iduShmTxInfo    * sTxInfo;
    iduShmListNode  * sThrListNode;
    UInt              sDeadThrCount;
    iduShmListNode  * sCurNode;

    idvManager::initSQL( &sStatistics, NULL /* Session */ );

    IDV_WP_SET_PROC_INFO( &sStatistics, mDeadProcInfo );
    IDV_WP_SET_THR_INFO( &sStatistics,  &mDeadProcInfo->mMainTxInfo );

    /* Main Thread에 대해서 Recovery를 맨먼저 수행한다. 왜냐하면 ProcInfo에 대한
     * Thread List 정보에 대한 갱신이 발생하였을 수 있기 때문이다. */
    IDE_TEST( idrLogMgr::abort( &sStatistics,
                                &mDeadProcInfo->mMainTxInfo )
              != IDE_SUCCESS );

    /* ThrInfo시 ProcessInfo Latch만 잡고 죽은 경우, Log가 없기때문에 무조건
     * Latch를 풀어준다. */
    IDE_TEST( idwPMMgr::unlockProcess( &sStatistics, mDeadProcInfo ) != IDE_SUCCESS );

    sDeadThrCount = mDeadProcInfo->mThreadCnt;

    if( sDeadThrCount != 0 )
    {
        IDE_TEST( iduMemMgr::malloc(
                      IDU_MEM_SM_IN_MEMORY_RECOVERY,
                      ID_SIZEOF(idrRecThread) * mDeadProcInfo->mThreadCnt,
                      (void**)&sArrThread )
                  != IDE_SUCCESS );

        sThrListNode     = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR(
            mDeadProcInfo->mMainTxInfo.mNode.mAddrNext );
        sCurDeadTxInfo  = (iduShmTxInfo*)iduShmList::getDataPtr( sThrListNode );

        sHeadAddr        = mDeadProcInfo->mMainTxInfo.mSelfAddr;

        /* 죽은 Process에 대해서 Recovery Thread들을 수행한다. */
        for( i = 0; i < sDeadThrCount; i++ )
        {
            IDE_ASSERT( sCurDeadTxInfo->mSelfAddr != sHeadAddr );

            new ( sArrThread + i ) idrRecThread;

            /* Process Log Buffer에 있는 Log들에 대해서 Undo를 수행한다. */
            /* 모든 Thread Info를 Free한다. */

            IDE_ASSERT( sCurDeadTxInfo != &mDeadProcInfo->mMainTxInfo );

            IDE_TEST( sArrThread[i].initialize( sCurDeadTxInfo )
                      != IDE_SUCCESS );

            sThrListNode    = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR(
                sCurDeadTxInfo->mNode.mAddrNext );

            sCurDeadTxInfo  = (iduShmTxInfo*)iduShmList::getDataPtr( sThrListNode );

            IDE_TEST( sArrThread[i].startThread() != IDE_SUCCESS );
        }

        sThrListNode    = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR(
            mDeadProcInfo->mMainTxInfo.mNode.mAddrNext );
        sCurDeadTxInfo  = (iduShmTxInfo*)iduShmList::getDataPtr( sThrListNode );

        for( i = 0; i < sDeadThrCount; i++ )
        {
            IDV_WP_SET_THR_INFO( &sStatistics,  sCurDeadTxInfo );

            IDE_ASSERT( sCurDeadTxInfo->mSelfAddr != sHeadAddr );

            /* Process Log Buffer에 있는 Log들에 대해서 Undo를 수행한다. */
            /* 모든 Thread Info를 Free한다. */
            IDE_TEST( sArrThread[i].shutdown() != IDE_SUCCESS );
            IDE_TEST( sArrThread[i].destroy() != IDE_SUCCESS );

            IDE_DASSERT( iduShmMgr::validateShmMgr( &sStatistics, &mDeadProcInfo->mMainTxInfo )
                         == IDE_SUCCESS );

            sThrListNode = (iduShmListNode*)IDU_SHM_GET_ADDR_PTR(
                sCurDeadTxInfo->mNode.mAddrNext );

            /* ThrInfo를 ProcessInfo에서 제거한다. */
            IDE_TEST( idwPMMgr::freeThrInfo( &sStatistics, sCurDeadTxInfo )
                      != IDE_SUCCESS );

            sCurDeadTxInfo  = (iduShmTxInfo*)iduShmList::getDataPtr( sThrListNode );
        }
    }

    IDV_WP_SET_THR_INFO( &sStatistics, &mDeadProcInfo->mMainTxInfo );

    /* PROCINFO에 StStmtInfo가 등록되어 있는 경우에,
     * mmcStatementManager::removeStStmtInfo를 통하여 해당 메모리를
     * 메모리 리스트에서 UNLINK를 하고 POOL에서 제거하여 재할당이 가능하도록 한다.
     *
     * 참조 NOK 주소 입니다.
     *     http://nok.altibase.com/display/rnd/BUG-41760+Working+Report
     */
    if (mDeadProcInfo->mLPID >= IDU_PROC_TYPE_USER)
    {
        if ((idwWatchDog::mStStmtRmFunc != NULL) &&
                    (idwWatchDog::mWatchDogStatus == ID_WATCHDOG_RUN))
        {
            (void)idwWatchDog::mStStmtRmFunc( &sStatistics,
                                              &mDeadProcInfo->mStStatementList,
                                              (void *)&idwWatchDog::mWatchDogStatus );
        }
        else
        {
            /* do nothing.*/
        }
    }
    else
    {
        /* do nothing. */
    }

    IDE_TEST( idwPMMgr::unRegisterProcByWatchDog( &sStatistics,
                                                  mDeadProcInfo )
              != IDE_SUCCESS );

    if( mDeadProcInfo->mRestartProcFunc != NULL )
    {
        IDV_WP_SET_MAIN_PROC_INFO( &sStatistics );

        IDE_TEST( idwPMMgr::allocThrInfo( &sStatistics,
                                          IDU_SHM_THR_LOGGING_ON,
                                          &sTxInfo )
                        != IDE_SUCCESS );

        IDE_TEST( mDeadProcInfo->mRestartProcFunc( &sStatistics )
                  != IDE_SUCCESS );

        IDE_TEST( idwPMMgr::freeThrInfo( &sStatistics,
                                         sTxInfo )
                  != IDE_SUCCESS );
    }

    return;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( idERR_FATAL_UNDO_PROCESS_BY_WATCH_DOG ) );

    IDE_CALLBACK_FATAL( "Runtime InMemory Process Recovery Error " );

    return;
}
