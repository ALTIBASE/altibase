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
 * $Id: smuWorkerThread.cpp $
 **********************************************************************/

#include <smuWorkerThread.h>
#include <smErrorCode.h>

IDE_RC smuWorkerThread::initialize( smuWorkerThreadFunc    aThreadFunc, 
                                    UInt                   aThreadCnt,
                                    UInt                   aQueueSize,
                                    smuWorkerThreadMgr   * aThreadMgr )
{
    smuWorkerThread * sThread;
    UInt              sState = 0;
    UInt              i = 0;
    UInt              j;

    IDE_ASSERT( ( aQueueSize % aThreadCnt ) == 0 );
    IDE_ASSERT( aThreadCnt >= 1  );

    aThreadMgr->mThreadFunc      = aThreadFunc;
    aThreadMgr->mThreadCnt       = aThreadCnt;
    aThreadMgr->mDone            = ID_FALSE;
                                 
    aThreadMgr->mQueueSize       = aQueueSize;
    aThreadMgr->mJobTail         = 0;

    if( aThreadCnt == 1 )
    {
        /* Thread가 하나면, Queue를 이용치 않고 직접 수행한다. */
    }
    else
    {
        /* smuWorkerThread_initialize_calloc_JobQueue.tc */
        IDU_FIT_POINT("smuWorkerThread::initialize::calloc::JobQueue");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMU,
                                     aQueueSize,
                                     ID_SIZEOF( void* ),
                                     (void**)&(aThreadMgr->mJobQueue ) )
                  != IDE_SUCCESS );

        idlOS::memset( aThreadMgr->mJobQueue, 0, (size_t)aQueueSize * ID_SIZEOF(void*) );
        sState = 1;

        /* smuWorkerThread_initialize_calloc_ThreadArray.tc */
        IDU_FIT_POINT("smuWorkerThread::initialize::calloc::ThreadArray");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMU, 1,
                                     (ULong)ID_SIZEOF( smuWorkerThread ) * aThreadCnt,
                                     (void**)&(aThreadMgr->mThreadArray) )
                  != IDE_SUCCESS );
        sState = 2;

        for( ; i < aThreadMgr->mThreadCnt ; i ++ )
        {
            sThread = aThreadMgr->mThreadArray + i;
            new (sThread) smuWorkerThread;
            sThread->mThreadMgr    = aThreadMgr;
            sThread->mJobIdx       = i;
            IDU_FIT_POINT_RAISE( "smuWorkerThread::initialize::start", thread_create_failed );
            IDE_TEST( aThreadMgr->mThreadArray[i].start() != IDE_SUCCESS );
        }
        sState = 3;
    }

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( thread_create_failed );
    {
       IDE_SET(ideSetErrorCode(idERR_ABORT_THR_CREATE_FAILED));
    }
#endif

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 3:
    case 2: /* State=2 이후 for loop 수행 도중 Thread start() 실패 할 경우
             * State=2 이면서 일부 Thread 가 start 상태 일 수 있다
             */
        aThreadMgr->mDone = ID_TRUE;

        for( j = 0 ; j < i ; j ++ )
        {
            IDE_ASSERT( aThreadMgr->mThreadArray[j].join() == IDE_SUCCESS );
        }
        IDE_ASSERT( iduMemMgr::free( aThreadMgr->mThreadArray ) 
                    == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( iduMemMgr::free( aThreadMgr->mJobQueue ) 
                    == IDE_SUCCESS );
        break;
    default:
        break;
    }

    return IDE_FAILURE;
}
IDE_RC smuWorkerThread::finalize( smuWorkerThreadMgr * aThreadMgr )
{
    UInt           sState = 3;
    UInt           i = 0;

    if( aThreadMgr->mThreadCnt == 1 )
    {
        /* Thread가 하나면, 별다른 객체를 생성하지 않는다. */
    }
    else
    {
        wait( aThreadMgr );

        sState = 2;
        aThreadMgr->mDone = ID_TRUE;
        for ( ; i < aThreadMgr->mThreadCnt ; i ++ )
        {
            IDE_TEST_RAISE(
                aThreadMgr->mThreadArray[i].join() != IDE_SUCCESS,
                thr_join_error );
        }

        sState = 1;
        IDE_TEST( iduMemMgr::free( aThreadMgr->mThreadArray ) != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( iduMemMgr::free( aThreadMgr->mJobQueue ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(thr_join_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 3:
    case 2: /* State=2 이후 for loop 수행 도중 Thread join() 실패 할 경우
             * State=2 이면서 일부 Thread 가 run 상태 일 수 있다
             * i 번째 Thread join 이 실패했으므로 i 다음부터 다시 join 한다
             */
        aThreadMgr->mDone = ID_TRUE;
        for ( ++i ; i < aThreadMgr->mThreadCnt ; i ++ )
        {
            IDE_ASSERT( aThreadMgr->mThreadArray[i].join() == IDE_SUCCESS );
        }
        IDE_ASSERT( iduMemMgr::free( aThreadMgr->mThreadArray ) 
                    == IDE_SUCCESS );
    case 1:
        IDE_ASSERT( iduMemMgr::free( aThreadMgr->mJobQueue ) == IDE_SUCCESS );
        break;
    default:
        break;
    }

    return IDE_FAILURE;
}

IDE_RC smuWorkerThread::addJob( smuWorkerThreadMgr * aThreadMgr, void * aParam )
{
    volatile void   * sJob;
    PDL_Time_Value    sTV;

    if( aThreadMgr->mThreadCnt == 1 )
    {
        /* Thread가 1개이면 직접 수행한다. */
        aThreadMgr->mThreadFunc( (void*)aParam );
    }
    else
    {
        sTV.set(0, 100 );

        sJob = aThreadMgr->mJobQueue[ aThreadMgr->mJobTail ];
        while( sJob != NULL )
        {
            idlOS::sleep( sTV );
            sJob = aThreadMgr->mJobQueue[ aThreadMgr->mJobTail ];
        }

        aThreadMgr->mJobQueue[ aThreadMgr->mJobTail ] = aParam;

        aThreadMgr->mJobTail = ( aThreadMgr->mJobTail + 1 ) % aThreadMgr->mQueueSize ;
    }

    return IDE_SUCCESS;
}

void   smuWorkerThread::wait( smuWorkerThreadMgr * aThreadMgr )
{
    idBool           sRemainJob;
    volatile void ** sJob;
    PDL_Time_Value   sTV;
    UInt             i;

    if( aThreadMgr->mThreadCnt == 1 )
    {
        /* Thread가 1개면, 무조건 일 다했다. 자신이 직접하기 때문에.  */
    }
    else
    {
        /* Queue가 모두 비웠으면, 해야할 일이 없는 것.
         * 왜냐하면 addJob을 하는 MainThread가 이 함수를 호출하니까 */
        sTV.set(0, 100 );

        sRemainJob = ID_TRUE;
        while( sRemainJob == ID_TRUE )
        {
            sRemainJob = ID_FALSE;
            sJob = (volatile void **)aThreadMgr->mJobQueue;
            for( i = 0 ; i < aThreadMgr->mQueueSize ; i ++ )
            {
                if( sJob[ i ] != NULL )
                {
                    sRemainJob = ID_TRUE;
                    break;
                }
                else
                {
                    /* nothing to do... */
                }
            }
            idlOS::sleep( sTV );
        }
    }
}

void smuWorkerThread::run()
{
    volatile void ** sJob;
    volatile void  * sParam;
    PDL_Time_Value   sTV;

    sTV.set(0, 100 );

    do
    {
        sJob   = (volatile void **)mThreadMgr->mJobQueue;
        sParam = (volatile void *)sJob [ mJobIdx ];
        if( sParam != NULL )
        {
            mThreadMgr->mThreadFunc( (void*)sParam );
            sJob[ mJobIdx ] = NULL;
            mJobIdx = ( mJobIdx + mThreadMgr->mThreadCnt )
                                  % mThreadMgr->mQueueSize;
        }
        else
        {
            idlOS::sleep( sTV );
        }
    }
    while( ( mThreadMgr->mDone == ID_FALSE ) || 
           ( sJob[ mJobIdx ] != NULL ) );
}

