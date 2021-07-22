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
#include <idrLogMgr.h>
#include <idrRecThread.h>
#include <iduShmMgr.h>
#include <idwPMMgr.h>

idrRecThread::idrRecThread()
    : idtBaseThread()
{

}

idrRecThread::~idrRecThread()
{

}


IDE_RC idrRecThread::initialize( iduShmTxInfo  * aDeadTxInfo )
{
    mDeadTxInfo = aDeadTxInfo;

    return IDE_SUCCESS;
}

IDE_RC idrRecThread::destroy()
{
    IDE_TEST( shutdown() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Latches that are found in the latch stack must, by definition, have
 * been locked.  But sometimes a thread may have crashed after pushing
 * it in the latch stack, but before actually acquiring it.  Therefore
 * inspect the top of the latch stack, and see if the latch at the top
 * of the stack is indeed locked by the dead thread.  If it's not, pop
 * it out of the latch stack and discard it.
 *
 * The function does nothing if the latch stack is empty.  In debug
 * mode, it checks the states of the latches in the entire latch
 * stack as well.
 */
void idrRecThread::recoverLatchStack()
{
    iduShmLatch      *sLatch     = NULL;
    iduShmLatchInfo  *sLatchInfo = NULL;
    iduShmLatchStack *sStack     = &mDeadTxInfo->mLatchStack;

    if ( !IDU_SHM_LATCH_STACK_IS_EMPTY( sStack ) )
    {
        /* Get the real address for the latch. */
        sLatchInfo = &IDU_SHM_LATCH_STACK_TOP( sStack );
        sLatch = (iduShmLatch *)IDU_SHM_GET_ADDR_PTR_CHECK( sLatchInfo->mAddr4Latch );
        IDE_DASSERT( sLatchInfo->mAddr4Latch == sLatch->mAddrSelf );

        if ( ( sLatchInfo->mMode == IDU_SHM_LATCH_MODE_EXCLUSIVE ) &&
             ( IDL_LIKELY_FALSE( iduShmLatchIsLockedByThread( sLatch,
                                                              mDeadTxInfo->mThrID )
                                 == ID_FALSE ) ) )
        {
            IDU_SHM_LATCH_STACK_POP_DISCARD( sStack );
        }
        else
        {
            /* The latch state seems okay. */
        }

#ifdef DEBUG
        for ( UInt i = 0; i < sStack->mCurSize; i++ )
        {
            sLatchInfo = &IDU_SHM_LATCH_STACK_GET( sStack, i );
            sLatch = (iduShmLatch *)IDU_SHM_GET_ADDR_PTR_CHECK( sLatchInfo->mAddr4Latch );
            IDE_DASSERT( sLatchInfo->mAddr4Latch == sLatch->mAddrSelf );

            /* This is obviously an error condition, but some of the
             * current code relies on a latch being released without
             * popping it off the stack, so we let it off with just a
             * warning. */
            if ( ( sLatchInfo->mMode == IDU_SHM_LATCH_MODE_EXCLUSIVE ) &&
                 ( iduShmLatchIsLockedByThread( sLatch, mDeadTxInfo->mThrID ) == ID_FALSE ) )
            {
                ideLog::log( IDE_ERR_0,
                             "Latch (" IDU_SHM_ADDR_FMT ") in the latch stack "
                             "(Owner GblThrID: %d) is expected to be locked, "
                             "but is not.\n",
                             IDU_SHM_ADDR_ARGS( sLatch->mAddrSelf ),
                             mDeadTxInfo->mThrID );
            }
            else
            {
                /* Normal condition */
            }
        }
#endif
    }
    else
    {
        /* The stack is empty - nothing to recover. */
    }
}

void idrRecThread::run()
{
    idvSQL            sStatistics;
    iduShmProcInfo  * sProcInfo;

    idvManager::initSQL( &sStatistics, NULL );

    sProcInfo = idwPMMgr::getProcInfo( mDeadTxInfo->mLPID );

    IDV_WP_SET_PROC_INFO( &sStatistics, sProcInfo );
    IDV_WP_SET_THR_INFO( &sStatistics, mDeadTxInfo );

    recoverLatchStack();

    IDE_DASSERT( iduShmMgr::validateShmMgr( &sStatistics, mDeadTxInfo )
                 == IDE_SUCCESS );

    IDE_TEST_RAISE( idrLogMgr::abort( &sStatistics, mDeadTxInfo ) != IDE_SUCCESS,
                    err_undo_thread );

    IDE_DASSERT( iduShmMgr::validateShmMgr( &sStatistics, mDeadTxInfo )
                 == IDE_SUCCESS );

    return;

    IDE_EXCEPTION( err_undo_thread );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_UNDO_THREAD_BY_WATCH_DOG ) );
    }
    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL( "Runtime InMemory Thread Recovery Error " );

    return;
}

IDE_RC idrRecThread::startThread()
{
    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idrRecThread::shutdown()
{
    if( isStarted() == ID_TRUE )
    {
        return join();
    }

    return IDE_SUCCESS;
}

