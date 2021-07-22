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
#include <idu.h>
#include <idv.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideMsgLog.h>
#include <iduShmMsgMgr.h>

#define IDU_SHM_MSG_POOL_CHUNK_LIST_COUNT 1
#define IDU_SHM_MSG_POOL_CHUNK_ITEM_COUNT 10
#define IDU_SHM_MSG_THREAD_MAX_SLEEP_USEC 1000000
#define IDU_SHM_MSG_THREAD_MIN_SLEEP_USEC 100000

iduShmMsgMgr::iduShmMsgMgr()
    : idtBaseThread()
{

}

iduShmMsgMgr::~iduShmMsgMgr()
{

}

IDE_RC iduShmMsgMgr::initialize( idvSQL               * aStatistics,
                                 iduShmTxInfo         * aShmTxInfo,
                                 iduMemoryClientIndex   aIndex,
                                 SChar                * aStrName,
                                 UInt                   aMsgSize,
                                 iduShmMsgProcessFunc   aMsgProcessFunc,
                                 idShmAddr              aAddr4StMsgMgr,
                                 iduStShmMsgMgr       * aStShmMsgMgr )
{
    idShmAddr sAddr4Obj;

    sAddr4Obj = IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4StMsgMgr,
                                            iduStShmMsgMgr,
                                            mMsgMemPool );
    mMsgSize = aMsgSize;

    IDE_TEST( iduShmMemPool::initialize( aStatistics,
                                         aShmTxInfo,
                                         aIndex,
                                         aStrName,
                                         IDU_SHM_MSG_POOL_CHUNK_LIST_COUNT,
                                         ID_SIZEOF(iduStShmMsg) + mMsgSize,
                                         IDU_SHM_MSG_POOL_CHUNK_ITEM_COUNT,
                                         IDU_AUTOFREE_CHUNK_LIMIT,
                                         ID_TRUE, /* aUseLatch */
                                         IDU_SHM_MEM_POOL_DEFAULT_ALIGN_SIZE,
                                         ID_TRUE, /* aGarbageCollection */
                                         sAddr4Obj,
                                         &aStShmMsgMgr->mMsgMemPool )
              != IDE_SUCCESS );

    sAddr4Obj = IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4StMsgMgr,
                                            iduStShmMsgMgr,
                                            mLatch );

    iduShmLatchInit( sAddr4Obj,
                     IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                     &aStShmMsgMgr->mLatch );

    sAddr4Obj = IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4StMsgMgr,
                                            iduStShmMsgMgr,
                                            mBaseNode4MsgLst );

    iduShmList::initBase( &aStShmMsgMgr->mBaseNode4MsgLst,
                          sAddr4Obj,
                          IDU_SHM_NULL_ADDR );

    IDE_TEST_RAISE( mCV.initialize( aStrName ) != IDE_SUCCESS,
                    err_cond_var_init );

    IDE_TEST( mMutex.initialize( (SChar*)"SHM_MSGMGR_THREAD_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);

    mIndex          = aIndex;
    mMsgProcessFunc = aMsgProcessFunc;
    mStShmMsgMgr    = aStShmMsgMgr;
    mStProcInfo     = (iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics );
    mFinish         = ID_FALSE;

    mStShmMsgMgr->mUserMsgSize = mMsgSize;

    IDE_TEST( startThread() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_var_init );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMsgMgr::destroy( idvSQL        * aStatistics,
                              iduShmTxInfo  * aShmTxInfo )
{
    IDE_TEST( shutdown( aStatistics ) != IDE_SUCCESS );

    IDE_TEST_RAISE( mCV.destroy() != IDE_SUCCESS, err_cond_destroy );

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    if ( iduShmMgr::isShmAreaFreeAtShutdown() == ID_TRUE )
    {
        iduShmLatchDest( &mStShmMsgMgr->mLatch );

        IDE_ASSERT( iduShmMemPool::destroy( aStatistics,
                                            aShmTxInfo,
                                            NULL, /* aSavepoint */
                                            &mStShmMsgMgr->mMsgMemPool,
                                            ID_TRUE /* aCheck */ )
                    == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMsgMgr::attach( idvSQL             * aStatistics,
                             iduStShmMsgMgr     * aStShmMsgMgr,
                             iduMemoryClientIndex aIndex,
                             SChar              * aStrName,
                             iduShmMsgProcessFunc aProcessFunc )
{
    mStProcInfo  = (iduShmProcInfo *)aStatistics->mProcInfo;
    mStShmMsgMgr = aStShmMsgMgr;
    mMsgSize     = mStShmMsgMgr->mUserMsgSize;
    mIndex       = aIndex;

    if ( mStProcInfo->mType == IDU_PROC_TYPE_DAEMON )
    {
        mMsgProcessFunc = aProcessFunc;
        mFinish         = ID_FALSE;

        IDE_TEST_RAISE( mCV.initialize( aStrName ) != IDE_SUCCESS,
                        err_cond_var_init );

        IDE_TEST( mMutex.initialize( (SChar*)"SHM_MSGMGR_THREAD_MUTEX",
                                     IDU_MUTEX_KIND_POSIX,
                                     IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS);

        IDE_TEST( startThread() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_var_init );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMsgMgr::detach( idvSQL * aStatistics )
{
    if ( mStProcInfo->mType == IDU_PROC_TYPE_DAEMON )
    {
        IDE_TEST( shutdown( aStatistics ) != IDE_SUCCESS );

        IDE_TEST_RAISE( mCV.destroy() != IDE_SUCCESS, err_cond_destroy );

        IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    }

    mStShmMsgMgr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMsgMgr::sendRequest( idvSQL             * aStatistics,
                                  iduShmTxInfo       * aShmTxInfo,
                                  void               * aMsg,
                                  iduShmMsgReqMode     aReqMode )
{
    idShmAddr      sAddr4Msg;
    iduStShmMsg  * sNewShmMsg;
    idrSVP         sVSavepoint1;
    idrSVP         sVSavepoint2;
    idBool         sIsTgtProcAlive = ID_TRUE;

    idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint1 );

    IDE_TEST( iduShmMemPool::alloc( aStatistics,
                                    aShmTxInfo,
                                    &mStShmMsgMgr->mMsgMemPool,
                                    &sAddr4Msg,
                                    (void**)&sNewShmMsg ) != IDE_SUCCESS );

    initShmMsg( aShmTxInfo,
                sNewShmMsg->mAddrSelf,
                mTgtThrID,
                aReqMode,
                sNewShmMsg );

    idwPMMgr::validateThrID( IDW_THRID_LPID( sNewShmMsg->mReqThrID ) );

    idlOS::memcpy( sNewShmMsg + 1, aMsg, mMsgSize );

    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                  aShmTxInfo,
                                  &mStShmMsgMgr->mLatch )
              != IDE_SUCCESS );

    iduShmList::addLast( aShmTxInfo,
                         &mStShmMsgMgr->mBaseNode4MsgLst,
                         (iduShmListNode*)sNewShmMsg );

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     aShmTxInfo,
                                     &sVSavepoint1 ) != IDE_SUCCESS );

    if ( iduGetShmProcType() == IDU_PROC_TYPE_WSERVER )
    {
        IDU_FIT_POINT( "iduShmMsgMgr::sendRequest" );
    }

    if( aReqMode == IDU_SHM_MSG_SYNC )
    {
        /* 요청한 작업이 완료될 때 까지 대기 한다. */
        while(1)
        {
            IDE_TEST( idwPMMgr::checkProcAliveByLPID( IDW_THRID_LPID( mTgtThrID ),
                                                      &sIsTgtProcAlive )
                      != IDE_SUCCESS );

            if( sIsTgtProcAlive == ID_FALSE )
            {
                idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint2 );

                IDE_TEST( iduShmLatchAcquire( aStatistics,
                                              aShmTxInfo,
                                              &mStShmMsgMgr->mLatch )
                          != IDE_SUCCESS );

                iduShmList::remove( aShmTxInfo,
                                    (iduShmListNode*)sNewShmMsg );

                IDE_TEST( iduShmMemPool::memfree( aStatistics,
                                                  aShmTxInfo,
                                                  &sVSavepoint2,
                                                  &mStShmMsgMgr->mMsgMemPool,
                                                  sNewShmMsg->mAddrSelf,
                                                  sNewShmMsg ) != IDE_SUCCESS );

                IDE_RAISE( err_msg_process_shutdown );
            }

            if( sNewShmMsg->mState == IDU_SHM_MSG_STATE_FINISH )
            {
                IDL_MEM_BARRIER;
                sNewShmMsg->mState = IDU_SHM_MSG_STATE_CHECKED_BY_REQUESTOR;

                break;
            }

            IDE_TEST_RAISE( sNewShmMsg->mState == IDU_SHM_MSG_STATE_ERROR,
                            err_request_fail );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_request_fail )
    {
        IDE_SET( ideSetErrorCode( sNewShmMsg->mErrorCode ) );
    }
    IDE_EXCEPTION( err_msg_process_shutdown )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_REQUEST_PROCESS_FAILURE ) );
    }
    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sVSavepoint1 )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC iduShmMsgMgr::recvRequest( idvSQL          * aStatistics,
                                  iduShmTxInfo    * aShmTxInfo,
                                  void           ** aReqMsg )
{
    iduStShmMsg  * sCurShmMsg;
    iduStShmMsg  * sNxtShmMsg;
    idrSVP         sVSavepoint1;
    idrSVP         sVSavepoint2;
    idBool         sIsReqProcAlive;

    idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint1 );

    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                  aShmTxInfo,
                                  &mStShmMsgMgr->mLatch )
              != IDE_SUCCESS );

    sCurShmMsg =
        (iduStShmMsg*)iduShmList::getFstPtr( &mStShmMsgMgr->mBaseNode4MsgLst );

    while( sCurShmMsg != (iduStShmMsg*)&mStShmMsgMgr->mBaseNode4MsgLst )
    {
        idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint2 );

        sNxtShmMsg = (iduStShmMsg*)iduShmList::getNxtPtr( (iduShmListNode*)sCurShmMsg );

        idwPMMgr::validateThrID( IDW_THRID_LPID( sCurShmMsg->mReqThrID ) );

        if( sCurShmMsg->mState == IDU_SHM_MSG_STATE_REQUEST_BY_DAEMON )
        {
            break;
        }

        IDE_TEST( idwPMMgr::checkProcAliveByLPID( IDW_THRID_LPID( sCurShmMsg->mReqThrID ),
                                                  &sIsReqProcAlive )
                  != IDE_SUCCESS );

        if( sIsReqProcAlive == ID_TRUE )
        {
            if( sCurShmMsg->mState == IDU_SHM_MSG_STATE_REQUEST )
            {
                break;
            }

            if( sCurShmMsg->mState == IDU_SHM_MSG_STATE_CHECKED_BY_REQUESTOR )
            {
                /* Request에 의해 처리된것이 확인되었기 때문에 삭제 */
            }
            else
            {
                if( sCurShmMsg->mState   == IDU_SHM_MSG_STATE_FINISH &&
                    sCurShmMsg->mReqMode == IDU_SHM_MSG_ASYNC )
                {
                    /* Request에 의해 처리되었고 Requestor에 의해서 처리된것을
                     * 확인하지 않는 Async Mode이기 때문에 Msg Node를 삭제한다. */
                }
                else
                {
                    sCurShmMsg = sNxtShmMsg;
                    continue;
                }
            }
        }

        iduShmList::remove( aShmTxInfo,
                            (iduShmListNode*)sCurShmMsg );

        IDE_TEST( iduShmMemPool::memfree( aStatistics,
                                          aShmTxInfo,
                                          &sVSavepoint2,
                                          &mStShmMsgMgr->mMsgMemPool,
                                          sCurShmMsg->mAddrSelf,
                                          sCurShmMsg ) != IDE_SUCCESS );

        sCurShmMsg = sNxtShmMsg;
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     aShmTxInfo,
                                     &sVSavepoint1 ) != IDE_SUCCESS );

    if( sCurShmMsg != (iduStShmMsg*)&mStShmMsgMgr->mBaseNode4MsgLst )
    {
        *aReqMsg = sCurShmMsg + 1;
    }
    else
    {
        *aReqMsg = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sVSavepoint1 )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

void iduShmMsgMgr::run()
{
    SInt             rc;
    UInt             sState = 0;
    iduShmTxInfo   * sTxInfo;
    idvSQL           sStatistics;
    idrSVP           sVSavepoint1;
    UInt             sSleepUSec;
    void           * sUserMsg;
    iduStShmMsg    * sCurShmMsg;
    PDL_Time_Value   sTV;
    PDL_Time_Value   sWaitTime;

    idvManager::initSQL( &sStatistics, NULL /* Session */ );

    IDV_WP_SET_PROC_INFO( &sStatistics, mStProcInfo );

    IDE_TEST_RAISE( idwPMMgr::allocThrInfo( &sStatistics,
                                            IDU_SHM_THR_DEFAULT_ATTR,
                                            &sTxInfo )
                    != IDE_SUCCESS, err_alloc_thrInfo );
    sState = 1;

    mTgtThrID = sTxInfo->mThrID;

    idrLogMgr::setSavepoint( sTxInfo, &sVSavepoint1 );

    IDE_TEST( lock( &sStatistics ) != IDE_SUCCESS );
    sState = 2;

    sSleepUSec = IDU_SHM_MSG_THREAD_MAX_SLEEP_USEC;

    while( mFinish == ID_FALSE )
    {
        sWaitTime.set( 0, sSleepUSec );

        sTV = idlOS::gettimeofday();
        sTV += sWaitTime;

        rc = mCV.timedwait( &mMutex, &sTV, IDU_IGNORE_TIMEDOUT );

        IDE_TEST_RAISE( rc != IDE_SUCCESS, err_cond_wait );

        IDE_TEST( recvRequest( &sStatistics, sTxInfo, &sUserMsg )
                  != IDE_SUCCESS );

        if( sUserMsg != NULL )
        {
            if( mMsgProcessFunc( &sStatistics, sTxInfo, sUserMsg ) == IDE_SUCCESS )
            {
                setMsgState( IDU_SHM_MSG_STATE_FINISH, sUserMsg );
            }
            else
            {
                ID_SERIAL_BEGIN( setMsgErrCode( ideGetErrorCode(), sUserMsg ) );
                ID_SERIAL_END( setMsgState( IDU_SHM_MSG_STATE_ERROR, sUserMsg ) );
            }

            sSleepUSec = 0;
        }
        else
        {
            if( sSleepUSec == 0 )
            {
                sSleepUSec = IDU_SHM_MSG_THREAD_MIN_SLEEP_USEC;
            }
            else
            {
                sSleepUSec = sSleepUSec * 2;
                sSleepUSec = sSleepUSec < IDU_SHM_MSG_THREAD_MAX_SLEEP_USEC ?
                    sSleepUSec : IDU_SHM_MSG_THREAD_MAX_SLEEP_USEC;
            }
        }
    }

    while(1)
    {
        IDE_TEST( recvRequest( &sStatistics, sTxInfo, (void**)&sCurShmMsg )
                  != IDE_SUCCESS );

        if( sCurShmMsg == NULL )
        {
            break;
        }

        IDE_TEST( iduShmLatchAcquire( &sStatistics,
                                      sTxInfo,
                                      &mStShmMsgMgr->mLatch )
                  != IDE_SUCCESS );

        ideLog::log( IDE_SERVER_0, " remove: %p", sCurShmMsg );

        iduShmList::remove( sTxInfo,
                            (iduShmListNode*)sCurShmMsg );

        IDE_TEST( iduShmMemPool::memfree( &sStatistics,
                                          sTxInfo,
                                          &sVSavepoint1,
                                          &mStShmMsgMgr->mMsgMemPool,
                                          sCurShmMsg->mAddrSelf,
                                          sCurShmMsg ) != IDE_SUCCESS );

    }

    sState = 1;
    IDE_TEST( unlock() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( idwPMMgr::freeThrInfo( &sStatistics,
                                     sTxInfo )
              != IDE_SUCCESS );

    return;

    IDE_EXCEPTION( err_cond_wait );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_ThrCondWait) );
    }
    IDE_EXCEPTION( err_alloc_thrInfo )
    {
        IDE_CALLBACK_FATAL( "Can't Alloc Thread space for Message Processing Thread" );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
    case 2:
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    default:
        break;
    }

    if( sState != 0 )
    {
        IDE_ASSERT( idrLogMgr::abort2Svp( &sStatistics, sTxInfo, &sVSavepoint1 )
                    == IDE_SUCCESS );
    }

    IDE_POP();
}

IDE_RC iduShmMsgMgr::startThread()
{
    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMsgMgr::shutdown( idvSQL * aStatistics )
{
    SInt          sState = 0;

    IDE_TEST( lock( aStatistics ) != IDE_SUCCESS);
    sState = 1;

    mFinish = ID_TRUE;

    IDE_TEST_RAISE( mCV.signal() != IDE_SUCCESS, err_cond_signal );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    IDE_TEST_RAISE( this->join() != IDE_SUCCESS, err_thr_join );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_thr_join );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_Systhrjoin) );
    }
    IDE_EXCEPTION( err_cond_signal );
    {
        IDE_SET( ideSetErrorCode(idERR_FATAL_ThrCondSignal) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}
