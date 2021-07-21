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
 * $Id $
 **********************************************************************/

#include <sdtWorkArea.h>
#include <sdtWAMap.h>
#include <smuProperty.h>
#include <sdpDef.h>
#include <sdtTempPage.h>
#include <sdpTableSpace.h>
#include <smiMisc.h>

UChar               * sdtWorkArea::mArea;
UChar               * sdtWorkArea::mAlignedArea;
ULong                 sdtWorkArea::mWASize;
iduMutex              sdtWorkArea::mMutex;
iduStackMgr           sdtWorkArea::mFreePool;
sdtWASegment        * sdtWorkArea::mWASegListHead;
iduMemPool            sdtWorkArea::mFlushQueuePool;

smuWorkerThreadMgr    sdtWorkArea::mFlusherMgr;
UInt                  sdtWorkArea::mGlobalWAFlusherIdx = 0;
UInt                  sdtWorkArea::mWAFlusherCount;
UInt                  sdtWorkArea::mWAFlushQueueSize;
sdtWAFlusher        * sdtWorkArea::mWAFlusher;
sdtWAState            sdtWorkArea::mWAState = SDT_WASTATE_SHUTDOWN;

/******************************************************************
 * WorkArea
 ******************************************************************/
/**************************************************************************
 * Description :
 * 처음 서버 구동할때 호출된다. 변수값을 초기화하고 Mutex를 만들고
 * createWA를 부른다.
 ***************************************************************************/
IDE_RC sdtWorkArea::initializeStatic()
{
    IDE_TEST( mMutex.initialize( (SChar*)"SDT_WORK_AREA_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);

    IDE_TEST( createWA() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * memory를 할당하고 구조를 만든다.
 ***************************************************************************/
IDE_RC sdtWorkArea::createWA()
{
    SChar     sMutexName[ IDU_MUTEX_NAME_LEN ];
    UInt      sWAExtentCount;
    vULong    sValue;
    UInt      sLockState = 0;
    UInt      sState = 0;
    UInt      i;

    /* WASegment 자료구조에 대한 검증*/
    /* WASegment는 Page에 배치되기 때문에 Page크기보다 작아야 한다. */
    IDE_DASSERT( ID_SIZEOF( sdtWASegment ) < SD_PAGE_SIZE );
    /* WASegment + Pointer*N 이 Page에 비례해야 하며, 그냥도 Page에
     * 비례해야 함 */
    IDE_DASSERT( ( SD_PAGE_SIZE - ID_SIZEOF( sdtWASegment ) )
                 % ( ID_SIZEOF( UChar * ) ) == 0 );
    IDE_DASSERT( ( SD_PAGE_SIZE % ID_SIZEOF( UChar * ) ) == 0 );

    lock();
    sLockState = 1;

    mWAState = SDT_WASTATE_INITIALIZING;

    sWAExtentCount = smuProperty::getTotalWASize() / SDT_WAEXTENT_SIZE + 1;
    /************************* WorkArea ***************************/
    mWASize = (ULong)sWAExtentCount *  SDT_WAEXTENT_SIZE;
    mWASegListHead = NULL;

    IDU_FIT_POINT_RAISE( "sdtWorkArea::createWA::calloc1", memory_allocate_failed );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_TEMP,
                                 1,
                                 mWASize + SDT_WAEXTENT_SIZE,
                                 (void**)&mArea )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( mFreePool.initialize( IDU_MEM_SM_TEMP,
                                    ID_SIZEOF( UChar* ) )
              != IDE_SUCCESS );
    sState = 2;

    mAlignedArea = (UChar*)idlOS::align( mArea, SDT_WAEXTENT_SIZE );

    sValue = (vULong)mAlignedArea;
    for( i = 0 ; i < sWAExtentCount ; i ++ )
    {
        IDE_DASSERT( ( sValue % SDT_WAEXTENT_SIZE ) == 0 );

        IDE_TEST( mFreePool.push( ID_FALSE, /* lock */
                                  (void*) &sValue )
                  != IDE_SUCCESS );
        sValue += SDT_WAEXTENT_SIZE;
    }
    sState = 2;
    /****************************** Flusher *****************************/
    mWAFlusherCount = smuProperty::getTempFlusherCount();
    IDU_FIT_POINT_RAISE( "sdtWorkArea::createWA::calloc2", memory_allocate_failed );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_TEMP,
                                 getWAFlusherCount(),
                                 ID_SIZEOF( sdtWAFlusher ),
                                 (void**)&mWAFlusher )
              != IDE_SUCCESS );
    sState = 3;

    for( i = 0 ; i < getWAFlusherCount() ; i ++ )
    {
        mWAFlusher[ i ].mIdx           = i;
        mWAFlusher[ i ].mTargetHead    = NULL;
        mWAFlusher[ i ].mRun           = ID_FALSE;
        idlOS::snprintf( sMutexName,
                         IDU_MUTEX_NAME_LEN,
                         "SDT_WORKAREA_FLUSHER_%"ID_UINT32_FMT,
                         i );
        IDE_TEST( mWAFlusher[ i ].mMutex.initialize( sMutexName,
                                                     IDU_MUTEX_KIND_NATIVE,
                                                     IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS);
    }
    sState = 4;

    IDE_TEST( smuWorkerThread::initialize( flusherRun,
                                           getWAFlusherCount(),
                                           getWAFlusherCount()*4,
                                           &mFlusherMgr )
              != IDE_SUCCESS );

    /* Flusher 기동시키기 전에 Done을 설정해줘야, Flusher들이 열리자마자
     * 닫히는 현상이 없음*/

    for( i = 0 ; i < getWAFlusherCount() ; i ++ )
    {
        IDE_TEST( smuWorkerThread::addJob( &mFlusherMgr,
                                           (void*)&mWAFlusher[ i ] )
                  != IDE_SUCCESS );
        mWAFlusher[ i ].mRun           = ID_TRUE;
    }
    sState = 5;

    mWAFlushQueueSize = smuProperty::getTempFlushQueueSize();
    IDE_TEST( mFlushQueuePool.initialize(
                  IDU_MEM_SM_TEMP,
                  (SChar*)"SDT_FLUSH_QUEUE_POOL",
                  ID_SCALABILITY_SYS,
                  ID_SIZEOF( sdtWAFlushQueue ) +
                  ID_SIZEOF( scPageID ) * mWAFlushQueueSize,
                  64,
                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                  ID_TRUE,							/* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                  ID_FALSE,							/* ForcePooling */
                  ID_TRUE,							/* GarbageCollection */
                  ID_TRUE,                          /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS);			
    sState = 6;

    mWAState = SDT_WASTATE_RUNNING;
    sLockState = 0;
    unlock();

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( memory_allocate_failed );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
#endif
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 6:
            mFlushQueuePool.destroy();
        case 5:
            smuWorkerThread::finalize( &mFlusherMgr );
        case 4:
            while( i-- )
            {
                mWAFlusher[ i ].mMutex.destroy();
            }
        case 3:
            (void) iduMemMgr::free( mWAFlusher );
        case 2:
            mFreePool.destroy();
        case 1:
            (void) iduMemMgr::free( mArea );
        default:
            break;
    }

    mWAFlusherCount = 0;
    mWASize         = 0;
    mWAState        = SDT_WASTATE_SHUTDOWN;

    switch( sLockState )
    {
        case 1:
            unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WA를 재구축한다.
 ***************************************************************************/
IDE_RC sdtWorkArea::resetWA()
{
    IDE_TEST( dropWA() != IDE_SUCCESS );
    IDE_TEST( createWA() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 서버 내릴때 호출된다. dropWA를 호출하고 Mutex를 free 한다.
 ***************************************************************************/
IDE_RC sdtWorkArea::destroyStatic()
{
    IDE_ASSERT( dropWA() == IDE_SUCCESS );
    IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 * WA와 관련된 객체들을 제거하고 메모리를 해제한다.
 ***************************************************************************/
IDE_RC sdtWorkArea::dropWA()
{
    UInt              sFreeCount;
    UInt              sWAExtentCount = mWASize / SDT_WAEXTENT_SIZE;
    PDL_Time_Value    sTV;
    UInt              i;

    sTV.set(0, smuProperty::getTempSleepInterval() );

    lock();
    /* 다른 녀석에 의해 Shutdown 중이면 무시함 */
    IDE_TEST_CONT( getWAState() != SDT_WASTATE_RUNNING, SKIP );
    mWAState = SDT_WASTATE_FINALIZING;
    unlock();

    sFreeCount = mFreePool.getTotItemCnt();

    /* 모든 Transaction들이 Extent를 전부 사용 완료할때까지 대기함 */
    while( sFreeCount < sWAExtentCount )
    {
        idlOS::sleep( sTV );

        lock();
        sFreeCount = mFreePool.getTotItemCnt();
        unlock();
    }

    lock();
    /****************************** Flusher *****************************/
    IDE_ASSERT( smuWorkerThread::finalize( &mFlusherMgr ) == IDE_SUCCESS );

    for( i = 0 ; i < getWAFlusherCount() ; i ++ )
    {
        if( mWAFlusher[ i ].mRun == ID_TRUE )
        {
            /* 살아 있다면, Target들이 모두 정리되어 있어야 함. */
            IDE_ASSERT( mWAFlusher[ i ].mTargetHead == NULL );
        }
        IDE_ASSERT( mWAFlusher[ i ].mMutex.destroy() == IDE_SUCCESS);
    }
    IDE_ASSERT( iduMemMgr::free( mWAFlusher ) == IDE_SUCCESS );
    IDE_ASSERT( mFreePool.destroy() == IDE_SUCCESS );
    IDE_ASSERT( mFlushQueuePool.destroy() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::free( mArea ) == IDE_SUCCESS );

    mWAState = SDT_WASTATE_SHUTDOWN;

    IDE_EXCEPTION_CONT( SKIP );
    unlock();

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 * WAExtent들을 할당한다. 그리고 WAExtent의 가장 앞쪽에 WASegment를 배치하고
 * 이를 초기화해준다.
 ***************************************************************************/
IDE_RC sdtWorkArea::allocWASegmentAndExtent( idvSQL             *aStatistics,
                                             sdtWASegment      **aWASegment,
                                             smiTempTableStats **aStatsPtr,
                                             UInt                aExtentCount )
{
    sdtWASegment   * sWASeg = NULL;
    UChar          * sExtent;
    idBool           sIsEmpty;
    UInt             sTryCount4AllocWExtent = 0;
    PDL_Time_Value   sTV;
    UInt             sLockState = 0;
    UInt             i;

    sTV.set(0, smuProperty::getTempSleepInterval() );

    IDE_ERROR( aExtentCount >= 1 );
    /* 아예 할당하기에 WA가 부족한 상황 */
    IDE_TEST_RAISE( aExtentCount > (mWASize / SDT_WAEXTENT_SIZE),
                    ERROR_NOT_ENOUGH_WORKAREA );

    lock();
    sLockState = 1;

    /***************************WAExtent 할당****************************/
    while( 1)
    {
        if( getWAState() == SDT_WASTATE_RUNNING ) /* WA가 유효하고 */
        {
            if( getFreeWAExtentCount() >= aExtentCount ) /* Extent도 있고*/
            {
                break; /* 그러면 Ok */
            }
        }

        sLockState = 0;
        unlock();

        idlOS::sleep( sTV );

        (*aStatsPtr)->mAllocWaitCount ++;
        sTryCount4AllocWExtent ++;

        IDE_TEST_RAISE( sTryCount4AllocWExtent >
                        smuProperty::getTempAllocTryCount(),
                        ERROR_NOT_ENOUGH_WORKAREA );
        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS);

        lock();
        sLockState = 1;
    }

    /* WAExtent를 가져오고, 거기서 가장 앞쪽에 WASegment구조체를 배치함 */
    for( i = 0 ; i < aExtentCount ; i ++ )
    {
        IDE_TEST( mFreePool.pop( ID_FALSE, /* lock */
                                 (void*) &sExtent,
                                 &sIsEmpty )
                  != IDE_SUCCESS );
        IDE_ERROR( sIsEmpty == ID_FALSE ); /* 반드시 성공해야함 */

        if( i == 0 )
        {
            /* 첫 Page에는 WASegment를 배치시킴*/
            sWASeg = (sdtWASegment*)sdtWASegment::getFrameInExtent( sExtent, 0 );
            idlOS::memset( sWASeg, 0, ID_SIZEOF( sdtWASegment ) );
        }

        IDE_TEST( sdtWASegment::addWAExtentPtr( sWASeg, sExtent ) != IDE_SUCCESS );
    }
    IDE_ERROR( sWASeg->mWAExtentCount == aExtentCount );

    /*************************** WAList연결  ****************************/
    sWASeg->mPrev = NULL;
    if( mWASegListHead != NULL )
    {
        mWASegListHead->mPrev = sWASeg;
    }
    sWASeg->mNext = mWASegListHead;

    mWASegListHead = sWASeg;

    sLockState = 0;
    unlock();

    *aWASegment = sWASeg;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NOT_ENOUGH_WORKAREA );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_ENOUGH_WORKAREA ) );
    }
    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WAExtent들을 반환한다.
 ***************************************************************************/
IDE_RC sdtWorkArea::freeWASegmentAndExtent( sdtWASegment   * aWASegment )
{
    UChar          * sWAExtent;
    UInt             sLockState = 0;
    UInt             i;

    lock();
    sLockState = 1;

    for( i = 0 ; i < aWASegment->mWAExtentCount ; i ++ )
    {
        sWAExtent = sdtWASegment::getWAExtentPtr( aWASegment, i );
        IDE_TEST( mFreePool.push( ID_FALSE, /* lock */
                                  (void*) &sWAExtent )
                  != IDE_SUCCESS );
    }

    if( aWASegment->mNext != NULL )
    {
        aWASegment->mNext->mPrev = aWASegment->mPrev;
    }
    if( aWASegment->mPrev != NULL )
    {
        aWASegment->mPrev->mNext = aWASegment->mNext;
    }
    else
    {
        mWASegListHead = aWASegment->mNext;
    }

    sLockState = 0;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 현재의 WASegment를 담당할 Flusher를 선택한다.
 ***************************************************************************/
IDE_RC sdtWorkArea::assignWAFlusher( sdtWASegment * aWASegment )
{
    sdtWAFlusher    * sWAFlusher = NULL;
    sdtWAFlushQueue * sWAFlushQueue;
    UInt              sLockState = 0;
    UInt              sState = 0;
    UInt              sWAFlusherIdx;
    UInt              i;

    for( i = 0 ; i < getWAFlusherCount() ; i ++ )
    {
        /* AtomicOperation으로 수행 안해도 LoadBalancing만
         * 불균형해질뿐 문제없음 */
        sWAFlusherIdx = getGlobalWAFlusherIdx();
        sWAFlusher    = getWAFlusher( sWAFlusherIdx );

        IDE_TEST( sWAFlusher->mMutex.lock( NULL ) != IDE_SUCCESS );
        sLockState = 1;

        if( sWAFlusher->mRun == ID_TRUE ) /* 동작중인 Flusher를 찾았음 */
        {
            /* sdtWASegment_assignWAFlusher_alloc_FlushQueue.tc */
            IDU_FIT_POINT("sdtWorkArea::assignWAFlusher::alloc::FlushQueue");
            IDE_TEST( allocWAFlushQueue( &aWASegment->mFlushQueue )
                      != IDE_SUCCESS );
            sState = 1;
            sWAFlushQueue = aWASegment->mFlushQueue;

            sWAFlushQueue->mFQBegin      = 0;
            sWAFlushQueue->mFQEnd        = 0;
            sWAFlushQueue->mFQDone       = ID_FALSE;
            sWAFlushQueue->mWAFlusherIdx = sWAFlusherIdx;
            sWAFlushQueue->mWASegment    = aWASegment;

            sWAFlushQueue->mNextTarget   = sWAFlusher->mTargetHead;
            sWAFlushQueue->mWriteCount   = 0;
            sWAFlushQueue->mWritePageCount = 0;
            sWAFlushQueue->mRedirtyCount = 0;
            sWAFlusher->mTargetHead      = sWAFlushQueue;

            sLockState = 0;
            IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );
            break;
        }
        else
        {
            /* 아니면, 다음 Flusher를 찾아 다시 시도함 */
            sLockState = 0;
            IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );
        }
    }

    /* 동작중인 Flusher가 하나도 없으면, 정상동작 할 수 없음 */
    IDE_ERROR( sWAFlusher != NULL );
    IDE_ERROR( sWAFlusher->mRun == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            sWAFlusher->mMutex.unlock();
            break;
        default:
            break;
    }

    switch( sState )
    {
        case 1:
            freeWAFlushQueue( aWASegment->mFlushQueue );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 현재의 WASegment를 담당한 Flusher를 때어놓는다.
 ***************************************************************************/
IDE_RC sdtWorkArea::releaseWAFlusher( sdtWAFlushQueue   * aWAFlushQueue )
{
    sdtWAFlusher     * sWAFlusher = getWAFlusher( aWAFlushQueue->mWAFlusherIdx );
    sdtWAFlushQueue ** sWAFlushQueuePtr;
    sdtWAFlushQueue  * sWAFlushQueue;
    UInt               sLockState = 0;

    IDE_TEST( sWAFlusher->mMutex.lock( NULL ) != IDE_SUCCESS );
    sLockState = 1;

    sWAFlushQueuePtr = &sWAFlusher->mTargetHead;
    sWAFlushQueue    = sWAFlusher->mTargetHead;

    while( sWAFlushQueue != NULL )
    {
        if( sWAFlushQueue == aWAFlushQueue )
        {
            break;
        }

        sWAFlushQueuePtr = &sWAFlushQueue->mNextTarget;
        sWAFlushQueue    = sWAFlushQueue->mNextTarget;
    }

    IDE_ERROR( sWAFlushQueue != NULL );

    /* 해당 Queue는 사용 종료됨.  Link에서 제거하고 Free함 */
    *sWAFlushQueuePtr = sWAFlushQueue->mNextTarget;
    IDE_TEST( freeWAFlushQueue( sWAFlushQueue )
              != IDE_SUCCESS );

    sLockState = 0;
    IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            sWAFlusher->mMutex.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     Flusher가 동작하는 메인 함수
 *
 * <Warrning>
 * Flusher에서는 반드시 getWCB, getWAPagePtr등의 Hint를 사용하는 함수를
 * 사용하면 안된다.
 * 해당 함수들은 ServiceThread 혼자 접근한다는 전제가 있기 때문에,
 * Flusher는 사용하면 안된다. getWCBInternal등으로 처리해야 한다.
 * <Warrning>
 *
 * <IN>
 * aParam         - 이 Flusher에 대한 정보
 ***************************************************************************/
void sdtWorkArea::flusherRun( void * aParam )
{
    sdtWAFlusher     * sWAFlusher;
    sdtWAFlushQueue  * sWAFlushQueue = NULL;
    sdtWASegment     * sWASegment    = NULL;
    idBool             sDoFlush;
    UInt               sLockState = 0;
    UInt               sDoneState = 0;
    PDL_Time_Value     sTV;

    sTV.set(0, smuProperty::getTempSleepInterval() );

    sWAFlusher = (sdtWAFlusher*)aParam;
    while( getWAState() == SDT_WASTATE_INITIALIZING )
    {
        idlOS::sleep( sTV );
    }

    while( 1 )
    {
        IDE_TEST( sWAFlusher->mMutex.lock( NULL ) != IDE_SUCCESS );
        sLockState = 1;

        if( ( sWAFlusher->mTargetHead == NULL ) &&
            ( getWAState() != SDT_WASTATE_RUNNING ) )
        {
            /*할일 다했으며, 종료해야 하는 상황 */
            sWAFlusher->mRun = ID_FALSE;

            sLockState = 0;
            IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );
            break;
        }

        sDoFlush         = ID_FALSE;
        sWAFlushQueue    = sWAFlusher->mTargetHead;
        while( sWAFlushQueue != NULL )
        {
            if( sWAFlushQueue->mFQDone == ID_TRUE )
            {
                sWASegment = (sdtWASegment*)sWAFlushQueue->mWASegment;
                sDoneState = 1;
                IDE_TEST( sdtWASegment::freeAllNPage( sWASegment )
                          != IDE_SUCCESS );
                sDoneState = 2;
                IDE_TEST( sWASegment->mFreeNPageStack.destroy()
                          != IDE_SUCCESS );
                sDoneState = 3;
                IDE_TEST( freeWASegmentAndExtent( sWASegment )
                          != IDE_SUCCESS );
                sDoneState = 4;
                sLockState = 0;
                IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );

                IDE_TEST( releaseWAFlusher( sWAFlushQueue ) != IDE_SUCCESS );

                IDE_TEST( sWAFlusher->mMutex.lock( NULL ) != IDE_SUCCESS );
                sLockState = 1;

                sWAFlushQueue    = sWAFlusher->mTargetHead;
                continue;
            }

            /* Flush할 작업이 있다면 */
            if( isEmptyQueue( sWAFlushQueue ) == ID_FALSE )
            {
                sLockState = 0;
                IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );

                IDE_TEST( flushTempPages( sWAFlushQueue ) != IDE_SUCCESS );
                sDoFlush = ID_TRUE;

                IDE_TEST( sWAFlusher->mMutex.lock( NULL ) != IDE_SUCCESS );
                sLockState = 1;
            }

            sWAFlushQueue    = sWAFlushQueue->mNextTarget;
        }

        sLockState = 0;
        IDE_TEST( sWAFlusher->mMutex.unlock() != IDE_SUCCESS );

        if( sDoFlush == ID_FALSE )
        {
            /* 할게 없음 */
            idlOS::sleep( sTV );
        }
    }

    IDE_ERROR( sWAFlusher->mRun == ID_FALSE );

    return;

    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            IDE_ASSERT( sWAFlusher->mMutex.unlock() == IDE_SUCCESS );
            break;
        default:
            break;
    }
    switch ( sDoneState )
    {
        /* BUG-42751 종료 단계에서 예외가 발생하면 해당 자원이 정리되기를 기다리면서
           server stop 시 HANG이 발생할수 있으므로 정리해줘야 함. */
        case 1:
            IDE_ASSERT( sWASegment->mFreeNPageStack.destroy() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( freeWASegmentAndExtent( sWASegment ) == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( releaseWAFlusher( sWAFlushQueue ) == IDE_SUCCESS );
            break;
        default:
            break;
    }
    ideLog::log( IDE_DUMP_0,
                 "STOP WAFlusher %u",
                 sWAFlusher->mIdx );
    if( sWAFlushQueue != NULL )
    {
        smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                        sdtWASegment::dumpFlushQueue,
                                        (void*)sWAFlushQueue );
    }

    sWAFlusher->mRun = ID_FALSE;
    IDE_DASSERT( 0 );

    return;
}

/**************************************************************************
 * Description :
 * Flusher가 FlushQueue에서 Flush할 페이지를 가져와 Write한다.
 *
 * <Warrning>
 * Flusher에서는 반드시 getWCB, getWAPagePtr등의 Hint를 사용하는 함수를
 * 사용하면 안된다.
 * 해당 함수들은 ServiceThread 혼자 접근한다는 전제가 있기 때문에,
 * Flusher는 사용하면 안된다. getWCBInternal등으로 처리해야 한다.
 * <Warrning>
 *
 * <IN>
 * aWAFlushQueue  - 대상 FlushQueue
 ***************************************************************************/
IDE_RC sdtWorkArea::flushTempPages( sdtWAFlushQueue * aWAFlushQueue )
{
    UInt             sSiblingPageCount = 0;
    sdtWCB         * sPrevWCBPtr       = NULL;
    sdtWCB         * sCurWCBPtr        = NULL;
    scPageID         sWPID;
    UInt             i;

    for( i = 0 ; i < smuProperty::getTempFlushPageCount() ; i ++ )
    {
        IDE_TEST( popJob( aWAFlushQueue, &sWPID, &sCurWCBPtr )
                  != IDE_SUCCESS );
        if( sCurWCBPtr == NULL )
        {
            break;
        }

        if( aWAFlushQueue->mFQDone == ID_TRUE )
        {
            /* 당장 Flusher 작업을 종료함.
             * 남은 작업들을 Flush할 필요도 없음 */
            break;
        }

        /* WPID가 제대로 설정되어 있는지 검사 */
        IDE_ERROR( sCurWCBPtr->mWPageID == sWPID );

        if( ( sSiblingPageCount > 0 ) &&
            ( isSiblingPage( sPrevWCBPtr, sCurWCBPtr ) == ID_FALSE ) )
        {
            // 연속적이지 못한 페이지를 만나게 되면,
            // 기존의 연속된 페이지들을 Flush함
            IDU_FIT_POINT( "1.BUG-46670@sdbWorkArea::flushTempPages::sleep" );

            IDE_TEST( flushTempPagesInternal( aWAFlushQueue,
                                              sPrevWCBPtr,
                                              sSiblingPageCount )
                      != IDE_SUCCESS );

            sSiblingPageCount = 0;
        }
        sPrevWCBPtr = sCurWCBPtr;

        sSiblingPageCount ++;

    }

    if( ( sSiblingPageCount > 0 ) && ( aWAFlushQueue->mFQDone == ID_FALSE ) )
    {
        IDU_FIT_POINT( "2.BUG-46670@sdbWorkArea::flushTempPages::sleep" );

        IDE_TEST( flushTempPagesInternal( aWAFlushQueue,
                                          sPrevWCBPtr,
                                          sSiblingPageCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Page를 MultiBlockWrite로 기록한다.
 *
 * <Warrning>
 * Flusher에서는 반드시 getWCB, getWAPagePtr등의 Hint를 사용하는 함수를
 * 사용하면 안된다.
 * 해당 함수들은 ServiceThread 혼자 접근한다는 전제가 있기 때문에,
 * Flusher는 사용하면 안된다. getWCBInternal등으로 처리해야 한다.
 * <Warrning>
 *
 * <IN>
 * aStatistics    - 통계정보
 * aStats         - TempTable용 통계정보
 * aWASegment     - 대상 WASegment
 * aWCBPtr        - 대상 WCBPtr
 * aPageCount     - 위 WCB로부터, N개의 Page를 한번에 Flush한다.
 *                  위 WCB의 PID가 10, N의 4이면, 7,8,9,10 이다.
 ***************************************************************************/
IDE_RC sdtWorkArea::flushTempPagesInternal( sdtWAFlushQueue   * aWAFlushQueue,
                                            sdtWCB            * aWCBPtr,
                                            UInt                aPageCount )
{
    UChar          * sWAPagePtr;
    sdtWCB         * sTargetWCBPtr;
    sdtWAPageState   sState;
    scSpaceID        sSpaceID;
    scPageID         sPageID;
    UInt             i;

    aWAFlushQueue->mWriteCount ++;
    aWAFlushQueue->mWritePageCount  += aPageCount;

    sWAPagePtr = aWCBPtr->mWAPagePtr - SD_PAGE_SIZE * ( aPageCount - 1 );
#if defined(DEBUG)
    /* 이웃페이지를 제대로 가져왔는지 검사 */
    IDE_TEST( sdtWASegment::getSiblingWCBPtr( aWCBPtr, aPageCount - 1, &sTargetWCBPtr )
              != IDE_SUCCESS );
    IDE_ASSERT( sWAPagePtr == sTargetWCBPtr->mWAPagePtr );
#endif

    sPageID  = aWCBPtr->mNPageID - aPageCount + 1;
    sSpaceID = aWCBPtr->mNSpaceID;
    IDE_ERROR( sSpaceID == ((sdtWASegment*)aWAFlushQueue->mWASegment)->mSpaceID );

    IDE_TEST( sddDiskMgr::write4DPath( aWAFlushQueue->mStatistics,
                                       sSpaceID,
                                       sPageID,
                                       aPageCount,
                                       sWAPagePtr )
              != IDE_SUCCESS );

    for( i = 0 ; i < aPageCount ; i ++ )
    {
        /* Writing을 다시 Clean으로 변경 */
        IDE_TEST( sdtWASegment::getSiblingWCBPtr( aWCBPtr, i, &sTargetWCBPtr )
                  != IDE_SUCCESS );
        sdtWASegment::checkAndSetWAPageState( sTargetWCBPtr,
                                              SDT_WA_PAGESTATE_WRITING,
                                              SDT_WA_PAGESTATE_CLEAN,
                                              &sState );

        if( sState == SDT_WA_PAGESTATE_IN_FLUSHQUEUE )
        {
            /* ServiceThread가 다시금 Flush를 요청하였다. 그러면서 Queue에
             * Job을 등록하였을 뿐이기 때문에, 현 시점에서는 정리만 한다.
             * 단순히 통계 누적 용이다. */
            aWAFlushQueue->mRedirtyCount ++;
        }

        /* Writing 상태만은 아니어야 한다.
         * Writing으로 변경하는 것은 이 Segment를 담당하는
         * 이 Flusher Thread 뿐이기 때문이다. */
        IDE_DASSERT( sTargetWCBPtr->mWPState != SDT_WA_PAGESTATE_WRITING );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "TEMP Flush fail :\n"
                 "PCount  : %u\n",
                 aPageCount );
    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    sdtWASegment::dumpWCB,
                                    aWCBPtr );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * NPID상으로도 WPID상으로도 이웃인지 판단한다.
 *
 * <IN>
 * aPrevWCBPtr    - 이전 WCBPtr
 * aNextWCBPtr    - 다음 WCBPtr
 ***************************************************************************/
idBool sdtWorkArea::isSiblingPage( sdtWCB * aPrevWCBPtr, sdtWCB * aNextWCBPtr )
{
    UInt sPrevExtentIdx = sdtWASegment::getExtentIdx( aPrevWCBPtr->mWPageID );
    UInt sNextExtentIdx = sdtWASegment::getExtentIdx( aNextWCBPtr->mWPageID );

    if( ( aNextWCBPtr->mNSpaceID    == aPrevWCBPtr->mNSpaceID ) &&
        ( aNextWCBPtr->mNPageID - 1 == aPrevWCBPtr->mNPageID  ) &&
        ( aNextWCBPtr->mWPageID - 1 == aPrevWCBPtr->mWPageID  ) &&
        ( sPrevExtentIdx == sNextExtentIdx ) )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/**************************************************************************
 * Description :
 * Queue의 Sequqence값을 옮긴다.
 * 하나의 Thread만 접근하기 때문에, ABA Problem외 걱정할건 없다.
 * ( Begin은 Flusher가, End는 ServiceThread가 )
 *
 * <IN>
 * aSeq           - 올릴 Seq값
 ***************************************************************************/
void sdtWorkArea::incQueueSeq( SInt * aSeq )
{
    (void)idCore::acpAtomicInc32( aSeq );

    /* Max에 도달했으면 초기화 */
    if( *aSeq == (SInt)mWAFlushQueueSize )
    {
        (void)idCore::acpAtomicSet32( aSeq, 0 );
    }
}

/**************************************************************************
 * Description :
 * Queue가 비어 있는가?
 *
 * <IN>
 * aWAFQ    - 대상 Queue
 ***************************************************************************/
idBool   sdtWorkArea::isEmptyQueue( sdtWAFlushQueue * aWAFQ )
{
    if( ( idCore::acpAtomicGet32( &aWAFQ->mFQBegin )
          == idCore::acpAtomicGet32( &aWAFQ->mFQEnd ) ) ||
        ( aWAFQ->mFQDone == ID_TRUE ) ) /* 종료되었어도, 빈것으로 여김 */
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/**************************************************************************
 * Description :
 * ServiceThread가 Flush할 대상 Page를 Queue에 삽입함
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 ***************************************************************************/
IDE_RC   sdtWorkArea::pushJob( sdtWASegment    * aWASeg,
                               scPageID          aWPID )
{
    sdtWAFlushQueue * sWAFQ = aWASeg->mFlushQueue;
    SInt              sEnd;
    SInt              sNextEnd;
    PDL_Time_Value    sTV;

    sTV.set(0, smuProperty::getTempSleepInterval() );

    sEnd = sNextEnd = idCore::acpAtomicGet32( &sWAFQ->mFQEnd );
    incQueueSeq( &sNextEnd );

    /* Queue가 꽉찼는가?*/
    while( sNextEnd == idCore::acpAtomicGet32( &sWAFQ->mFQBegin ) )
    {
        /* Flusher가 멈췄으면, 동작을 멈춤 */
        IDE_TEST_RAISE( mWAFlusher[ sWAFQ->mWAFlusherIdx ].mRun == ID_FALSE,
                        ERROR_TEMP_FLUSHER_STOPPED );

        /* sleep했다가 다시 시도함 */
        (*aWASeg->mStatsPtr)->mQueueWaitCount ++;
        idlOS::thr_yield();
    }

    // BUG-45811 cache miss occurs in the Numa system.
    (void)idCore::acpAtomicSet32( &sWAFQ->mSlotPtr[ sEnd ], aWPID );

    (void)idCore::acpAtomicSet32( &sWAFQ->mFQEnd, sNextEnd );
    IDE_ERROR( sWAFQ->mFQEnd < (SInt)mWAFlushQueueSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_TEMP_FLUSHER_STOPPED );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_TEMP_FLUSHER_STOPPED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Flusher가 Flush할 대상 Page를 Queue에서 가져옴
 *
 * <Warrning>
 * Flusher에서는 반드시 getWCB, getWAPagePtr등의 Hint를 사용하는 함수를
 * 사용하면 안된다.
 * 해당 함수들은 ServiceThread 혼자 접근한다는 전제가 있기 때문에,
 * Flusher는 사용하면 안된다. getWCBInternal등으로 처리해야 한다.
 * <Warrning>
 *
 * <IN>
 * aWAFQ          - 대상 FlushQueue
 * <OUT>
 * aWPID          - 대상 WPID
 * aWCBPtr        - 대상 WCBPtr
 ***************************************************************************/
IDE_RC     sdtWorkArea::popJob( sdtWAFlushQueue  * aWAFQ,
                                scPageID         * aWPID,
                                sdtWCB          ** aWCBPtr)
{
    sdtWASegment   * sWASeg = (sdtWASegment*)aWAFQ->mWASegment;
    sdtWCB         * sWCBPtr        = NULL;
    sdtWAPageState   sState;
    SInt             sBeginSeq;

    *aWPID   = SC_NULL_PID;
    *aWCBPtr = NULL;

    while( isEmptyQueue( aWAFQ ) == ID_FALSE )
    {
        sBeginSeq = idCore::acpAtomicGet32( &aWAFQ->mFQBegin );

        // BUG-45811 cache miss occurs in the Numa system.
        *aWPID = idCore::acpAtomicGet32( &aWAFQ->mSlotPtr[ sBeginSeq ] );

        sWCBPtr = sdtWASegment::getWCBInternal( sWASeg, *aWPID );

        /* Flusher가 가져갔단 의미로, Writing 상태로 설정해둠 */
        sdtWASegment::checkAndSetWAPageState( sWCBPtr,
                                              SDT_WA_PAGESTATE_IN_FLUSHQUEUE,
                                              SDT_WA_PAGESTATE_WRITING,
                                              &sState );

        incQueueSeq( &sBeginSeq );
        (void)idCore::acpAtomicSet32( &aWAFQ->mFQBegin, sBeginSeq );
        IDE_ERROR( aWAFQ->mFQBegin < (SInt)sdtWorkArea::mWAFlushQueueSize );

        if( sState == SDT_WA_PAGESTATE_IN_FLUSHQUEUE )
        {
            *aWCBPtr = sWCBPtr;
            break;
        }
        else
        {
            /* ServiceThread에서 이 WCB에 대한 Flush를 Queue에 넣은 후
             * 포기하였음 */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * X$Tempinfo를 위해 레코드 구축
 ***************************************************************************/
IDE_RC sdtWorkArea::buildTempInfoRecord( void                * aHeader,
                                         iduFixedTableMemory * aMemory )
{
    smiTempInfo4Perf sInfo;
    SChar            sName[ 32 ];
    SChar            sTrueFalse  [ 2 ][ 16 ] = {"FALSE","TRUE"};
    SChar            sWAStateName[ 4 ][ 16 ] = {
        "SHUTDOWN",
        "INITIALIZING",
        "RUNNING",
        "FINALIZING" };
    UInt             sLockState = 0;
    UInt             i;

    lock();
    sLockState = 1;

    SMI_TT_SET_TEMPINFO_STR( "WASTATE",
                             sWAStateName[ mWAState ],
                             " " );
    SMI_TT_SET_TEMPINFO_ULONG( "TOTAL WORK AREA SIZE", mWASize, "BYTES" );
    SMI_TT_SET_TEMPINFO_UINT( "FREE WA COUNT", getFreeWAExtentCount(),
                              "EXTENT" );
    SMI_TT_SET_TEMPINFO_UINT( "EXTENT SIZE", SDT_WAEXTENT_SIZE, "BYTES" );
    SMI_TT_SET_TEMPINFO_UINT( "FLUSHER COUNT", getWAFlusherCount(), "INTEGER" );
    SMI_TT_SET_TEMPINFO_UINT( "FLUSHER SEQ", mGlobalWAFlusherIdx, "INTEGER" );

    for( i = 0 ; i < getWAFlusherCount() ; i ++ )
    {
        idlOS::snprintf( sName, 32, "FLUSHER %"ID_UINT32_FMT" ENABLE", i );
        SMI_TT_SET_TEMPINFO_STR( sName,
                                 sTrueFalse[ mWAFlusher[ i ].mRun ],
                                 "TRUE/FALSE" );
    }

    sLockState = 0;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sLockState )
    {
        case 1:
            unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}
