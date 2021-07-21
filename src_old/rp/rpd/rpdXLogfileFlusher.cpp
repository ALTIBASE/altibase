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

/*******************************************************************************
 * $Id: rpdXLogfileFlusher.cpp
 ******************************************************************************/

#include <mtc.h>
#include <rpdXLogfileFlusher.h>
#include <rpdXLogfileMgr.h>
#include <rpuProperty.h>

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::initialize             *
 * ------------------------------------------------------------------*
 * xlogfile flush thread의 초기화를 위해 호출되는 함수
 *
 * aReplName      - [IN] replication의 이름
 * aXLogfileMgr   - [IN] xlogfile manager
 *********************************************************************/
IDE_RC rpdXLogfileFlusher::initialize( SChar *          aReplName,
                                       rpdXLogfileMgr * aXLogfileMgr )
{
    SChar   sBuffer[IDU_MUTEX_NAME_LEN] = {0,};
    UInt    sState = 0;
    SInt    sLengthCheck = 0;

    /* 변수 초기화 */
    mXLogfileMgr = aXLogfileMgr;

    /* flush list 초기화 */
    IDU_LIST_INIT( &mFlushList );

    /* thread 관련 변수*/
    mExitFlag      = ID_FALSE;
    mEndFlag       = ID_FALSE;
    mIsThreadSleep = ID_FALSE;

    /* mutex 및 CV 초기화 */
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_FLUSHLIST_%s",
                                    aReplName);
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mFlushMutex.initialize( sBuffer,
                                      IDU_MUTEX_KIND_POSIX,
                                      IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_FLUSHER_CV_%s",
                                    aReplName);
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mThreadWaitMutex.initialize( sBuffer,
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mThreadWaitCV.initialize() != IDE_SUCCESS );
    sState = 3;

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_mutex_name )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_MUTEX_NAME, sBuffer ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 3:
            (void)mThreadWaitCV.destroy();
        case 2:
            (void)mThreadWaitMutex.destroy();
        case 1:
            (void)mFlushMutex.destroy();
        case 0:
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::finalize               *
 * ------------------------------------------------------------------*
 * xlogfile flush thread를 정지하는 함수
 *********************************************************************/
void rpdXLogfileFlusher::finalize()
{
    mEndFlag = ID_TRUE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::destroy                *
 * ------------------------------------------------------------------*
 * xlogfile flush thread를 정리하는 함수
 *********************************************************************/
void rpdXLogfileFlusher::destroy()
{
    /* flush thread에서 사용하는 CV와 mutex를 정리한다. */
    if( mThreadWaitCV.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if( mThreadWaitMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if( mFlushMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    return;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::run                    *
 * ------------------------------------------------------------------*
 * xlogfile flusher가 실제 작업을 수행하는 함수
 *********************************************************************/
void rpdXLogfileFlusher::run()
{
    idBool      sIsListEmpty = ID_TRUE;

    while( mExitFlag == ID_FALSE )
    {
        IDU_FIT_POINT( "rpdXLogfileFlusher::run" );

        sIsListEmpty = checkListEmpty(); 

        if( sIsListEmpty == ID_TRUE )
        {
            /* flush 할 XLogfile이 없을 경우 대기한다. */
            sleepFlusher();
        }
        else
        {
            /* flush 할 XLogfile이 있을 경우 flush를 수행한다. */
            IDE_TEST( flushXLogfiles() != IDE_SUCCESS );
        }
    }

    /* wake와 exitflag setting의 타이밍이 꼬일 경우 flush 대상이 있는 상태로 finalize를 수행할 수 있다.   *
     * 이 경우를 대비해 finalize시 한번 더 list를 체크해 list가 비어있지 않다면 flush를 수행하도록 한다. */
    sIsListEmpty = checkListEmpty(); 

    if( sIsListEmpty == ID_FALSE )
    {
        IDE_TEST( flushXLogfiles() != IDE_SUCCESS );
    }

    IDE_EXCEPTION_END;

    (void)finalize();

    return;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::flushXLogfile          *
 * ------------------------------------------------------------------*
 * 해당 함수는 write에 파일 하나에 쓰기가 끝날때 flush thread에서만 사용한다.
 * write가 끝나 flush를 기다리는 파일들에 대해 sync 및 close를 수행한다.
 * sync이후 해당 파일을 read에서 사용중인지 체크하고 사용중이 아닐 경우 close도 한다.
 *********************************************************************/
IDE_RC rpdXLogfileFlusher::flushXLogfiles()
{
    rpdXLogfile *   sFlushFile = NULL;
    iduListNode *   sNode = NULL;

    sNode = getFileFromFlushList();

    while( sNode != NULL )
    {
        sFlushFile = (rpdXLogfile*)sNode->mObj;

        /* sync를 수행한다. */
        IDE_TEST( sFlushFile->mFile.syncUntilSuccess( rpdXLogfileMgr::rpdXLogfileEmergency ) != IDE_SUCCESS );

        IDU_FIT_POINT( "rpdXLogfileFlusher::flushXLogfiles" );
        mXLogfileMgr->decFileRefCnt( sFlushFile );

        /* 사용이 끝난 노드를 메모리에서 제거한다. */
        (void)iduMemMgr::free( sNode ); 

        sNode = getFileFromFlushList();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::addFileToFlushList     *
 * ------------------------------------------------------------------*
 * flush할 파일을 flush list에 등록하는 함수
 * 등록된 파일은 flush가 종료된 후 close된다.
 * 주의 : 해당 함수는 write에서만 호출된다.(flush에서 호출하지 않는다.)
 *
 * aXLogfile    - [IN] flush를 수행할 xlogfile
 *********************************************************************/
IDE_RC rpdXLogfileFlusher::addFileToFlushList( void * aXLogfile )
{
    iduListNode *   sNode;
    UInt            sState = 0;

    IDE_ASSERT( aXLogfile != NULL );

    /* flush list에 추가할 node를 생성한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                 ID_SIZEOF( iduListNode ),
                                 (void**)&(sNode),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    sState = 1;

    IDU_LIST_INIT_OBJ( sNode, (void*)aXLogfile );

    /* flush mutex를 잡고 flush list에 삽입한다. */
    IDE_ASSERT( mFlushMutex.lock( NULL ) == IDE_SUCCESS );
    IDU_LIST_ADD_LAST( &mFlushList, sNode );
    IDE_ASSERT( mFlushMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free( sNode );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::getFileFromFlushList   *
 * ------------------------------------------------------------------*
 * flush list에서 flush할 file을 꺼내는 함수
 * flush list의 맨 앞 노드를 가져오며, 가져온 노드는 리스트에서 제거된다.
 * write(transfer)의 flush list 추가 작업과 동시성 문제가 발생하지 않도록 mutex를 잡는다.
 *********************************************************************/
iduListNode * rpdXLogfileFlusher::getFileFromFlushList()
{
    iduListNode *   sNode = NULL;

    /* flush mutex를 잡고 list에서 node를 꺼낸다. */
    IDE_ASSERT( mFlushMutex.lock( NULL ) == IDE_SUCCESS );

    if( IDU_LIST_IS_EMPTY( &mFlushList ) == ID_TRUE )
    {
        /* flush list가 비어있다면 null을 리턴한다. */
    }
    else
    {
        /* list에서 node를 꺼내고 제거한다. */
        sNode = IDU_LIST_GET_FIRST( &mFlushList );

        IDU_LIST_REMOVE( sNode );
    }

    IDE_ASSERT( mFlushMutex.unlock() == IDE_SUCCESS );

    return sNode;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::checkListEmpty         *
 * ------------------------------------------------------------------*
 * flush list가 비어있는지 확인하는 함수
 * 동시성 제어를 위해 lock을 잡고 확인한다.
 *********************************************************************/
idBool rpdXLogfileFlusher::checkListEmpty()
{
    idBool  sIsListEmpty = ID_FALSE;

    IDE_ASSERT( mFlushMutex.lock( NULL ) == IDE_SUCCESS );

    if( IDU_LIST_IS_EMPTY( &mFlushList ) == ID_TRUE )
    {
        sIsListEmpty = ID_TRUE;
    }
    else
    {
        sIsListEmpty = ID_FALSE;
    }

    IDE_ASSERT( mFlushMutex.unlock() == IDE_SUCCESS );

    return sIsListEmpty;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::wakeupFlusher      *
 * ------------------------------------------------------------------*
 * xlogfile용 flush thread를 깨우는 함수
 * 주의 : 해당 함수는 write에서만 호출된다.(flush에서 호출하지 않는다.)
 *********************************************************************/
void rpdXLogfileFlusher::wakeupFlusher()
{
    IDE_ASSERT( mThreadWaitMutex.lock( NULL ) == IDE_SUCCESS );

    IDU_FIT_POINT( "rpdXLogfileFlusher::wakeupFlusher" );

    if( mIsThreadSleep == ID_TRUE )
    {
        IDE_ASSERT( mThreadWaitCV.signal() == IDE_SUCCESS );
    }

    IDE_ASSERT( mThreadWaitMutex.unlock() == IDE_SUCCESS );

    IDE_EXCEPTION_END;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::sleepFlusher       *
 * ------------------------------------------------------------------*
 * 작업이 없을때 flush thread를 대기 상태로 변경한다.
 * 따로 signal이 오지 않아도 1초에 한번씩 thread가 깨어난다.
 * 주의 : 해당 함수는 flusher에서만 호출된다.
 *********************************************************************/
void rpdXLogfileFlusher::sleepFlusher()
{
    PDL_Time_Value    sCheckTime;
    PDL_Time_Value    sSleepSec;

    sCheckTime.initialize();
    sSleepSec.initialize();

    sSleepSec.set( 1 );

    IDE_ASSERT( mThreadWaitMutex.lock( NULL ) == IDE_SUCCESS );
    sCheckTime = idlOS::gettimeofday() + sSleepSec;

    mIsThreadSleep = ID_TRUE; 
    (void)mThreadWaitCV.timedwait( &mThreadWaitMutex, &sCheckTime );
    mIsThreadSleep = ID_FALSE;

    IDE_ASSERT( mThreadWaitMutex.unlock() == IDE_SUCCESS );

    return;
}
