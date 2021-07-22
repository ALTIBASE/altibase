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
 * $Id:$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer 
 **********************************************************************/

#include <smDef.h>
#include <smErrorCode.h>
#include <sdb.h>
#include <sds.h>
#include <sdsReq.h>


sdbFlushJob   sdsFlushMgr::mReqJobQueue[SDB_FLUSH_JOB_MAX];
UInt          sdsFlushMgr::mReqJobAddPos;
UInt          sdsFlushMgr::mReqJobGetPos;
iduMutex      sdsFlushMgr::mReqJobMutex;
iduLatch      sdsFlushMgr::mFCLatch; // flusher control latch
sdsFlusher*   sdsFlushMgr::mFlushers;
UInt          sdsFlushMgr::mMaxFlusherCount;
UInt          sdsFlushMgr::mActiveCPFlusherCount;
sdbCPListSet* sdsFlushMgr::mCPListSet;
idvTime       sdsFlushMgr::mLastFlushedTime;
idBool        sdsFlushMgr::mServiceable;
idBool        sdsFlushMgr::mInitialized;

/***********************************************************************
 * Description :  flush manager를 초기화한다.
 *  aFlusherCount   - [IN]  구동시킬 플려셔 갯수
 ***********************************************************************/
IDE_RC sdsFlushMgr::initialize( UInt aFlusherCount )
{
    UInt    i;
    SInt    sFlusherIdx = 0;
    UInt    sIOBPageCount = smuProperty::getBufferIOBufferSize();
    idBool  sNotStarted;
    SInt    sState = 0;

    /* identify 이후에 flusher가 생성이 되므로 serviceable을 볼수 있음 */    
    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE, 
                    SKIP_SUCCESS );
    mInitialized = ID_TRUE;
    mServiceable = ID_TRUE;

    mActiveCPFlusherCount = 0;
    mReqJobAddPos         = 0;
    mReqJobGetPos         = 0;
    mMaxFlusherCount      = aFlusherCount;
    mCPListSet            = sdsBufferMgr::getCPListSet();

    // job queue를 초기화한다.
    for (i = 0; i < SDB_FLUSH_JOB_MAX; i++)
    {
        initJob( &mReqJobQueue[i] );
    }

    IDE_TEST( mReqJobMutex.initialize(
              (SChar*)"SECONDARY_BUFFER_FLUSH_MANAGER_REQJOB_MUTEX",
              IDU_MUTEX_KIND_NATIVE,
              IDB_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FLUSH_MANAGER_REQJOB )
             != IDE_SUCCESS);
    sState = 1;

    IDE_ASSERT( mFCLatch.initialize( (SChar*)"SECONDARY_BUFFER_FLUSH_LATCH" ) 
                == IDE_SUCCESS);
    sState = 2;

    // 마지막 flush 시각을 세팅한다.
    IDV_TIME_GET( &mLastFlushedTime );

    /* TC/FIT/Limit/sm/sdb/sdsFlushMgr_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsFlushMgr::initialize::malloc",
                          ERR_INSUFFICIENT_MEMORY );

    // flusher들을 생성한다.
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF(sdsFlusher) * mMaxFlusherCount,
                                       (void**)&mFlushers ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );

    sState = 3;

    for( sFlusherIdx = 0 ; sFlusherIdx < (SInt)mMaxFlusherCount ; sFlusherIdx++ )
    {
        new ( &mFlushers[sFlusherIdx]) sdsFlusher();

        // 현재는 모든 플러들이 같은 크기의 페이지 사이즈와
        // 같은 크기의 IOB 크기를 가진다.
        IDE_TEST( mFlushers[sFlusherIdx].initialize( sFlusherIdx,    /* ID */ 
                                                     SD_PAGE_SIZE,   /* pageSize  */
                                                     sIOBPageCount ) /* PageCount */
                  != IDE_SUCCESS);

        mFlushers[sFlusherIdx].start();
    }

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;

    sFlusherIdx--;
    for( ; sFlusherIdx >= 0 ; sFlusherIdx-- )
    {
        IDE_ASSERT( mFlushers[sFlusherIdx].finish( NULL, &sNotStarted ) 
                    == IDE_SUCCESS );
        IDE_ASSERT( mFlushers[sFlusherIdx].destroy() == IDE_SUCCESS );
    }

    switch (sState)
    {
        case 3:
            IDE_ASSERT( iduMemMgr::free( mFlushers ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( mFCLatch.destroy( ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mReqJobMutex.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/*************************************************************************
 * Description : 구동중인 모든 flusher들을 종료시키고 자원 해제시킨다.
 *************************************************************************/
IDE_RC sdsFlushMgr::destroy()
{
    UInt   i;
    idBool sNotStarted;

    IDE_TEST_CONT( mInitialized != ID_TRUE, SKIP_SUCCESS );

    for ( i = 0; i < mMaxFlusherCount; i++ )
    {
        IDE_ASSERT( mFlushers[i].finish( NULL, &sNotStarted ) 
                   == IDE_SUCCESS);
        IDE_ASSERT( mFlushers[i].destroy() == IDE_SUCCESS);
    }

    IDE_ASSERT( iduMemMgr::free( mFlushers ) == IDE_SUCCESS );
    IDE_ASSERT( mFCLatch.destroy( ) == IDE_SUCCESS );
    IDE_ASSERT( mReqJobMutex.destroy() == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;
}

/*************************************************************************
 * Description : 종료된 flusher를 다시 재구동시킨다.
 *  aFlusherID  - [IN]  flusher id
 *************************************************************************/
IDE_RC sdsFlushMgr::turnOnFlusher( UInt aFlusherID )
{
    IDE_TEST_CONT( mServiceable != ID_TRUE, SKIP_SUCCESS );

    IDE_DASSERT( aFlusherID < mMaxFlusherCount );

    ideLog::log( IDE_SM_0, "[PREPARE] Start flusher ID(%d)\n", aFlusherID );

    IDE_TEST( mFCLatch.lockWrite( NULL,
                                  NULL )
              != IDE_SUCCESS);

    IDE_TEST_RAISE( mFlushers[aFlusherID].isRunning() == ID_TRUE,
                    ERR_FLUSHER_RUNNING );

    IDE_TEST( mFlushers[aFlusherID].start() != IDE_SUCCESS );

    IDE_TEST( mFCLatch.unlock() != IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "[SUCCESS] Start flusher ID(%d)\n", aFlusherID );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FLUSHER_RUNNING );
    {
        ideLog::log( IDE_SM_0,
                "[WARNING] flusher ID(%d) already started\n", aFlusherID );

        IDE_SET( ideSetErrorCode(smERR_ABORT_sdsFlusherRunning, aFlusherID) );
    }
    IDE_EXCEPTION_END;

    IDE_ASSERT( mFCLatch.unlock() == IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "[FAILURE] Start flusher ID(%d)\n", aFlusherID );

    return IDE_FAILURE;
}

/*************************************************************************
 * Description : flusher 쓰레드를 종료시킨다.
 *  aStatistics - [IN]  통계정보
 *  aFlusherID  - [IN]  flusher id
 *************************************************************************/
IDE_RC sdsFlushMgr::turnOffFlusher( idvSQL *aStatistics, UInt aFlusherID )
{
    idBool sNotStarted;
    idBool sLatchLocked = ID_FALSE;

    IDE_TEST_CONT( mServiceable != ID_TRUE, SKIP_SUCCESS );

    IDE_DASSERT( aFlusherID < mMaxFlusherCount );

    ideLog::log( IDE_SM_0, "[PREPARE] Stop flusher ID(%d)\n", aFlusherID );

    // BUG-26476
    // checkpoint가 발생해서 checkpoint flush job이 수행되기를 
    // 대기 하고 있는 쓰레드가 있을 수 있다. 그런 경우 
    // flusher를 끄면 대기하던 쓰레드가 무한정 대기할 수 
    // 있기 때문에 이땐 대기하는 쓰레드가 깨어나기까지 기대려야 한다. 
    IDE_ASSERT( mFCLatch.lockWrite( NULL,
                                    NULL )
                == IDE_SUCCESS);
    sLatchLocked = ID_TRUE;

    IDE_TEST( mFlushers[aFlusherID].finish( aStatistics,
                                           &sNotStarted ) 
              != IDE_SUCCESS);

    sLatchLocked = ID_FALSE;
    IDE_ASSERT( mFCLatch.unlock() == IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "[SUCCESS] Stop flusher ID(%d)\n", aFlusherID );

    IDE_TEST_RAISE( sNotStarted == ID_TRUE,
                    ERROR_FLUSHER_NOT_STARTED);

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_FLUSHER_NOT_STARTED );
    {
        ideLog::log( IDE_SM_0,
                "[WARNING] flusher ID(%d) already stopped.\n", aFlusherID );

        IDE_SET( ideSetErrorCode( smERR_ABORT_sdsFlusherNotStarted,
                                  aFlusherID) );
    }
    IDE_EXCEPTION_END;

    if( sLatchLocked == ID_TRUE )
    {
        sLatchLocked = ID_FALSE;
        IDE_ASSERT( mFCLatch.unlock() == IDE_SUCCESS );
    }

    ideLog::log( IDE_SM_0, "[FAILURE] Stop flusher ID(%d)\n", aFlusherID );

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    종료된모든 flusher를 다시 재구동시킨다.
 ******************************************************************************/
IDE_RC sdsFlushMgr::turnOnAllflushers()
{
    UInt i;

    IDE_TEST_CONT( mServiceable != ID_TRUE, SKIP_SUCCESS );

    IDE_ASSERT( mFCLatch.lockWrite( NULL, NULL ) 
                == IDE_SUCCESS); 

    for (i = 0; i < mMaxFlusherCount ; i++)
    {
        if( mFlushers[i].isRunning() != ID_TRUE )
        {
            IDE_TEST( mFlushers[i].start() != IDE_SUCCESS );
        }
    }

    IDE_ASSERT( mFCLatch.unlock( ) == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( mFCLatch.unlock(  ) == IDE_SUCCESS );

    return IDE_FAILURE;
}

/*************************************************************************
 * Description :
 *    모든 flusher들을 깨운다. 이미 활동중인 flusher에 대해서는
 *    아무런 작업을 하지 않는다.
 *************************************************************************/
IDE_RC sdsFlushMgr::wakeUpAllFlushers()
{
    UInt   i;
    idBool sDummy;

    for( i = 0; i < mMaxFlusherCount; i++ )
    {
        IDE_TEST( mFlushers[i].wakeUpSleeping( &sDummy ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    Flusher들이 작업중인 작업이 종료될때까지 대기한다.
 ******************************************************************************/
IDE_RC sdsFlushMgr::wait4AllFlusher2Do1JobDone()
{
    UInt   i;

    for( i = 0; i < mMaxFlusherCount; i++ )
    {
        mFlushers[i].wait4OneJobDone();
    }

    return IDE_SUCCESS;
}

/************************************************************************
 * Description :
 *  aStatistics       - [IN]  통계정보
 *  aReqFlushCount    - [IN]  flush 수행 되길 원하는 최소 페이지 개수
 *  aRedoPageCount    - [IN]  Restart Recovery시에 Redo할 Page 수,
 *                            반대로 말하면 CP List에 남겨둘 Dirty Page의 수
 *  aRedoLogFileCount - [IN]  Restart Recovery시에 Redo할 LogFile 수,
 *                            반대로 BufferPool에 남겨둘 LSN 간격 (LogFile수 기준)
 *  aJobDoneParam     - [IN]  job 완료를 통보 받기를 원할때, 이 파라미터를 설정한다.
 *  aAdded            - [OUT] job 등록 성공 여부. 만약 SDB_FLUSH_JOB_MAX보다 많은
 *                            job을 등록하려고 시도하면 등록이 실패한다.
 ************************************************************************/
void sdsFlushMgr::addReqReplaceFlushJob( 
                            idvSQL                     * aStatistics,
                            sdbFlushJobDoneNotifyParam * aJobDoneParam,
                            idBool                     * aAdded,
                            idBool                     * aSkip )
{
    sdbFlushJob * sCurr;
    UInt          sExtIndex;

    IDE_ASSERT( mReqJobMutex.lock( aStatistics ) == IDE_SUCCESS );

    *aAdded = ID_FALSE;
    *aSkip = ID_FALSE;

    if( isCond4ReplaceFlush() != ID_TRUE )
    {
        *aSkip = ID_TRUE;
        IDE_CONT( SKIP_ADDJOB );
    }

    sCurr = &mReqJobQueue[mReqJobAddPos];

    if( sCurr->mType == SDB_FLUSH_JOB_NOTHING )
    {
        if( sdsBufferMgr::getTargetFlushExtentIndex( &sExtIndex ) 
            == ID_TRUE )
        {
            if( aJobDoneParam != NULL )
            {
                initJobDoneNotifyParam( aJobDoneParam );
            }

            initJob( sCurr );

            sCurr->mType          = SDB_FLUSH_JOB_REPLACEMENT_FLUSH;
            sCurr->mReqFlushCount = 1;
            sCurr->mJobDoneParam  = aJobDoneParam;
            sCurr->mFlushJobParam.mSBufferReplaceFlush.mExtentIndex = sExtIndex;

            incPos( &mReqJobAddPos );
            *aAdded = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    IDE_EXCEPTION_CONT( SKIP_ADDJOB );

    IDE_ASSERT( mReqJobMutex.unlock() == IDE_SUCCESS );
}

/************************************************************************
 * Description :
 *  aStatistics       - [IN]  통계정보
 *  aReqFlushCount    - [IN]  flush 수행 되길 원하는 최소 페이지 개수
 *  aRedoPageCount    - [IN]  Restart Recovery시에 Redo할 Page 수,
 *                            반대로 말하면 CP List에 남겨둘 Dirty Page의 수
 *  aRedoLogFileCount - [IN]  Restart Recovery시에 Redo할 LogFile 수,
 *                            반대로 BufferPool에 남겨둘 LSN 간격 (LogFile수 기준)
 *  aJobDoneParam     - [IN]  job 완료를 통보 받기를 원할때, 이 파라미터를 설정한다.
 *  aAdded            - [OUT] job 등록 성공 여부. 만약 SDB_FLUSH_JOB_MAX보다 많은
 *                            job을 등록하려고 시도하면 등록이 실패한다.
 ************************************************************************/
void sdsFlushMgr::addReqCPFlushJob( 
                            idvSQL                     * aStatistics,
                            ULong                        aMinFlushCount,
                            ULong                        aRedoPageCount,
                            UInt                         aRedoLogFileCount,
                            sdbFlushJobDoneNotifyParam * aJobDoneParam,
                            idBool                     * aAdded)
{
    sdbFlushJob *sCurr;

    IDE_ASSERT( mReqJobMutex.lock( aStatistics ) == IDE_SUCCESS );

    sCurr = &mReqJobQueue[mReqJobAddPos];

    if( sCurr->mType == SDB_FLUSH_JOB_NOTHING )
    {
        if( aJobDoneParam != NULL )
        {
            initJobDoneNotifyParam( aJobDoneParam );
        }

        initJob( sCurr );

        sCurr->mType          = SDB_FLUSH_JOB_CHECKPOINT_FLUSH;
        sCurr->mReqFlushCount = aMinFlushCount;
        sCurr->mJobDoneParam  = aJobDoneParam;
        sCurr->mFlushJobParam.mChkptFlush.mRedoPageCount    = aRedoPageCount;
        sCurr->mFlushJobParam.mChkptFlush.mRedoLogFileCount = aRedoLogFileCount;

        incPos( &mReqJobAddPos );
        *aAdded = ID_TRUE;
    }
    else
    {
        *aAdded = ID_FALSE;
    }

    IDE_ASSERT( mReqJobMutex.unlock() == IDE_SUCCESS );
}

/*************************************************************************
 * Description :
 *  aStatistics - [IN]  통계정보
 *  aBCBQueue   - [IN]  flush를 수행할 BCB포인터가 들어있는 큐
 *  aFiltFunc   - [IN]  aBCBQueue에 있는 BCB중 aFiltFunc조건에 맞는 BCB만 flush
 *  aFiltObj    - [IN]  aFiltFunc에 파라미터로 넣어주는 값
 *  aJobDoneParam-[IN]  job 완료를 통보 받기를 원할때, 이 파라미터를 설정한다.
 *  aAdded      - [OUT] job 등록 성공 여부. 만약 SDB_FLUSH_JOB_MAX보다 많은
 *                      job을 등록하려고 시도하면 등록이 실패한다.
 ************************************************************************/
void sdsFlushMgr::addReqObjectFlushJob(
                                    idvSQL                     * aStatistics,
                                    smuQueueMgr                * aBCBQueue,
                                    sdbFiltFunc                  aFiltFunc,
                                    void                       * aFiltObj,
                                    sdbFlushJobDoneNotifyParam * aJobDoneParam,
                                    idBool                     * aAdded )
{
    sdbFlushJob *sCurr;

    IDE_ASSERT( mReqJobMutex.lock( aStatistics ) == IDE_SUCCESS );

    sCurr = &mReqJobQueue[mReqJobAddPos];

    if( sCurr->mType == SDB_FLUSH_JOB_NOTHING )
    {
        if( aJobDoneParam != NULL )
        {
            initJobDoneNotifyParam( aJobDoneParam );
        }

        initJob( sCurr );
        sCurr->mType         = SDB_FLUSH_JOB_DBOBJECT_FLUSH;
        sCurr->mJobDoneParam = aJobDoneParam;
        sCurr->mFlushJobParam.mObjectFlush.mBCBQueue = aBCBQueue;
        sCurr->mFlushJobParam.mObjectFlush.mFiltFunc = aFiltFunc;
        sCurr->mFlushJobParam.mObjectFlush.mFiltObj  = aFiltObj;

        incPos( &mReqJobAddPos );
        *aAdded = ID_TRUE;
    }
    else
    {
        *aAdded = ID_FALSE;
    }

    IDE_ASSERT( mReqJobMutex.unlock() == IDE_SUCCESS );
}

/**************************************************************************
 * Description : CPListSet의 dirty 버퍼들을 flush한다.
 *  aStatistics - [IN]  통계정보
 *  aFlushAll   - [IN]  check point list내의 모든 BCB를 flush 하려면 이 변수를
 *                      ID_TRUE로 설정한다.
 *************************************************************************/
IDE_RC sdsFlushMgr::flushDirtyPagesInCPList( idvSQL *aStatistics,
                                             idBool  aFlushAll )
{
    idBool                          sAdded = ID_FALSE;
    sdbFlushJobDoneNotifyParam      sJobDoneParam;
    PDL_Time_Value                  sTV;
    ULong                           sMinFlushCount;
    ULong                           sRedoPageCount;
    UInt                            sRedoLogFileCount;
    idBool                          sLatchLocked = ID_FALSE;

    // BUG-26476
    // checkpoint 수행시 flusher들이 stop되어 있으면 skip을 해야 한다.
    // 하지만 checkpoint에 의해 checkpoint flush가 수행되고 있는 동안
    // flush가 stop되면 안된다.
    IDE_ASSERT( mFCLatch.lockRead( NULL,        /* idvSQL */
                                   NULL)        /* Wait Event */
                == IDE_SUCCESS );

    sLatchLocked = ID_TRUE;

    if( getActiveFlusherCount() == 0 )
    {
        IDE_CONT( SKIP_FLUSH );
    }

    if( aFlushAll == ID_FALSE )
    {
         sMinFlushCount    = smuProperty::getCheckpointFlushCount();
         sRedoPageCount    = smuProperty::getFastStartIoTarget();
         sRedoLogFileCount = smuProperty::getFastStartLogFileTarget();
    }
    else
    {
        sMinFlushCount    = mCPListSet->getTotalBCBCnt();
        sRedoPageCount    = 0; // DirtyPage를 BufferPool에 남겨두지 않는다.
        sRedoLogFileCount = 0;
    }

    while( sAdded == ID_FALSE )
    {
        addReqCPFlushJob( aStatistics,
                          sMinFlushCount,
                          sRedoPageCount,
                          sRedoLogFileCount,
                          &sJobDoneParam,
                          &sAdded );
        IDE_TEST( wakeUpAllFlushers() != IDE_SUCCESS );
        sTV.set( 0, 50000 );
        idlOS::sleep(sTV);
    }

    IDE_TEST( waitForJobDone( &sJobDoneParam ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_FLUSH );

    sLatchLocked = ID_FALSE;

    IDE_ASSERT( mFCLatch.unlock( ) == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLatchLocked == ID_TRUE )
    {
       IDE_ASSERT( mFCLatch.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*************************************************************************
 * BCB 포인터가 들어있는 queue와 그 BCB들을 flush 해야 할지 말지
 * 결정할 수 있는 aFiltFunc 및 aFiltObj를 바탕으로
 * 플러시를 수행한다.
 *************************************************************************/
IDE_RC sdsFlushMgr::flushPagesInExtent( idvSQL  * aStatistics )
{
    sdbFlushJobDoneNotifyParam sJobDoneParam;
    PDL_Time_Value   sTV;

    idBool sAdded = ID_FALSE;
    idBool sSkip  = ID_FALSE;

    while( sAdded == ID_FALSE )
    {
        addReqReplaceFlushJob( aStatistics,
                               &sJobDoneParam,
                               &sAdded,
                               &sSkip );

        IDE_TEST_CONT( sSkip == ID_TRUE, SKIP_FLUSH )

        IDE_TEST( wakeUpAllFlushers() != IDE_SUCCESS );

        sTV.set( 0, 50000 );
        idlOS::sleep(sTV);
    }

    IDE_TEST( waitForJobDone( &sJobDoneParam ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_FLUSH );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * BCB 포인터가 들어있는 queue와 그 BCB들을 flush 해야 할지 말지
 * 결정할 수 있는 aFiltFunc 및 aFiltObj를 바탕으로
 * 플러시를 수행한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCBQueue   - [IN]  flush할 대상이 들어 있는 BCB의 포인터가 있는 queue
 *  aFiltFunc   - [IN]  aBCBQueue에 있는 BCB중 aFiltFunc조건에 맞는 BCB만 flush
 *  aFiltObj    - [IN]  aFiltFunc에 파라미터로 넣어주는 값
 *************************************************************************/
IDE_RC sdsFlushMgr::flushObjectDirtyPages( idvSQL       *aStatistics,
                                           smuQueueMgr  *aBCBQueue,
                                           sdsFiltFunc   aFiltFunc,
                                           void         *aFiltObj )
{
    idBool                     sAdded = ID_FALSE;
    sdbFlushJobDoneNotifyParam sJobDoneParam;
    PDL_Time_Value             sTV;

    while( sAdded == ID_FALSE )
    {
        addReqObjectFlushJob( aStatistics,
                              aBCBQueue,
                              aFiltFunc,
                              aFiltObj,
                              &sJobDoneParam,
                              &sAdded );

        IDE_TEST( wakeUpAllFlushers() != IDE_SUCCESS );

        sTV.set( 0, 50000 );
        idlOS::sleep(sTV);
    }

    IDE_TEST( waitForJobDone(&sJobDoneParam) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : flusher에 의해 호출되며 flusher가 할일을 반환한다.
 *    flusher가 할일은 다음과 같으며 번호는 우선순위이다.
 *      1. requested replacement flush job
 *           : 트랜잭션 쓰레드가 victim 찾기를 실패했을 때,
 *             해당 flush list를 flush할 것을 작업요청한다. 이때
 *             등록되는 작업이며 addReqFlushJob()을 통해서 등록된다.
 *         requested checkpoint flush job
 *           : 위의 requested replacement flush job과 동등한 우선순위를 가지며
 *             먼저 등록된 순으로 처리된다.
 *             이 job은 checkpoint 쓰레드가 checkpoint 시점이 너무 늦어졌다고
 *             판단했을 경우 등록되거나 사용자의 명시적 checkpoint 명령시
 *             등록된다. addReqCPFlushJob()을 통해 등록된다.
 *         requested  object flushjob
 *           : 위에 2가지  flush job과 동등한 우선 순위를 가지며 
 *             먼저 등록된 순으로 처리 된다. 
 *             이 job은 pageout/flush page 등으로 사용자의 명시적인 요청으로
 *             등록된다. addReqObjectFlushJob()를 통해 등록된다. 
 *      2. main replacement flush job
 *           : flusher들이 주기적으로 돌면서 flush list의 길이가
 *             일정 수치를 넘어서면 flush하게 되는데 이 작업을 말한다.
 *             이 작업은 트랜잭션 쓰레드에 의해 등록되는게 아니라
 *             flusher들에 의해 등록되고 처리된다.
 *      3. system checkpoint flush job
 *           : 1, 2번 작업이 없을 경우 system checkpoint flush를 수행한다.
 
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴할 job
 ************************************************************************/
void sdsFlushMgr::getJob( idvSQL      * aStatistics,
                          sdbFlushJob * aRetJob )
{
    initJob( aRetJob );

    getReqJob( aStatistics, aRetJob );

    if( aRetJob->mType == SDB_FLUSH_JOB_NOTHING )
    {
        getReplaceFlushJob( aRetJob );

        if( aRetJob->mType == SDB_FLUSH_JOB_NOTHING )
        {
            // checkpoint flush job을 구한다.
            getCPFlushJob( aStatistics, aRetJob );
        }
        else
        {
            // replace flush job을 하나 얻었다.
        }
    }
    else
    {
        // req job을 하나 얻었다.
    }
}

/*************************************************************************
 * Description : job queue에 등록된 job을 하나 얻는다.
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴할 Job
 *************************************************************************/
void sdsFlushMgr::getReqJob( idvSQL      *aStatistics,
                             sdbFlushJob *aRetJob )
{
    sdbFlushJob *sJob = &mReqJobQueue[mReqJobGetPos];

    if( sJob->mType == SDB_FLUSH_JOB_NOTHING )
    {
        aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
    }
    else
    {
        IDE_ASSERT( mReqJobMutex.lock( aStatistics ) == IDE_SUCCESS );

        // lock을 잡은 뒤에 다시 받아와야 한다.
        sJob = &mReqJobQueue[mReqJobGetPos];

        if( sJob->mType == SDB_FLUSH_JOB_NOTHING )
        {
            aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
        }
        else
        {
            // req job이 있다.
            *aRetJob    = *sJob;
            sJob->mType = SDB_FLUSH_JOB_NOTHING;

            // get position을 하나 진행시킨다.
            incPos( &mReqJobGetPos );

            /* CheckpointFlusher 한계에 도달했는지 검사 */
            limitCPFlusherCount( aStatistics,
                                 aRetJob,
                                 ID_TRUE ); /* already locked */
        }

        IDE_ASSERT( mReqJobMutex.unlock() == IDE_SUCCESS );
    }
}

/*************************************************************************
 * Description : main job을 하나 얻는다. 
 *  aRetJob   - [OUT] 리턴되는 job
 *************************************************************************/
void sdsFlushMgr::getReplaceFlushJob( sdbFlushJob *aRetJob )
{
    UInt sExtIndex = 0;
    initJob( aRetJob );
    
    if( isCond4ReplaceFlush() == ID_TRUE )
    {   
        if( sdsBufferMgr::getTargetFlushExtentIndex( &sExtIndex ) 
            == ID_TRUE )
        {
            aRetJob->mType = SDB_FLUSH_JOB_REPLACEMENT_FLUSH;
            /* 한 Extent씩 flush 한다 */
            aRetJob->mReqFlushCount = 1; 
            aRetJob->mFlushJobParam.mSBufferReplaceFlush.mExtentIndex = sExtIndex;
        }
        else
        {
            /* nothing to do */
        }
    }
    return;
}

/*************************************************************************
 * Description :
 *   incremental checkpoint를 수행하는 job을 얻는다.
 *   이 함수를 수행한다고 해서 그냥 job을 주지는 않는다. 즉, 어떤 조건을 만족
 *   해야 하는데, 그조건은 아래 둘중 하나를 만족 할 때이다.
 *   1. 마지막 checkpoint 수행 후 시간이 많이 지났다
 *   2. Recovery LSN이 너무 작을때
 *
 *  aStatistics - [IN]  통계정보
 *  aRetJob     - [OUT] 리턴되는 job
 *************************************************************************/
void sdsFlushMgr::getCPFlushJob( idvSQL *aStatistics, sdbFlushJob *aRetJob )
{
    smLSN          sMinLSN;
    idvTime        sCurrentTime;
    ULong          sTime;

    mCPListSet->getMinRecoveryLSN( aStatistics, &sMinLSN );

    IDV_TIME_GET( &sCurrentTime );
    sTime = IDV_TIME_DIFF_MICRO( &mLastFlushedTime, &sCurrentTime ) / 1000000;

    if ( ( smLayerCallback::isCheckpointFlushNeeded( sMinLSN ) == ID_TRUE ) ||
         ( sTime >= smuProperty::getCheckpointFlushMaxWaitSec() ) )
    {
        initJob( aRetJob );
        aRetJob->mType          = SDB_FLUSH_JOB_CHECKPOINT_FLUSH;
        aRetJob->mReqFlushCount = smuProperty::getCheckpointFlushCount();
        aRetJob->mFlushJobParam.mChkptFlush.mRedoPageCount
                                = smuProperty::getFastStartIoTarget();
        aRetJob->mFlushJobParam.mChkptFlush.mRedoLogFileCount
                                = smuProperty::getFastStartLogFileTarget();

        /* CheckpointFlusher 한계에 도달했는지 검사 */
        limitCPFlusherCount( aStatistics,
                             aRetJob,
                             ID_FALSE ); /* already locked */
    }
    else
    {
        aRetJob->mType = SDB_FLUSH_JOB_NOTHING;
    }
}

/******************************************************************************
 * Description :
 *  사용자가 요청한 flush작업 요청이 끝났는지 확인하는 수단인
 *  sdbFlushJobDoneNotifyParam 을 초기화 한다.
 *
 *  aParam  - [IN]  초기화할 변수
 ******************************************************************************/
IDE_RC sdsFlushMgr::initJobDoneNotifyParam( sdbFlushJobDoneNotifyParam *aParam )
{
    IDE_TEST( aParam->mMutex.initialize( (SChar*)"SECONDARY_BUFFER_FLUSH_JOB_COND_WAIT_MUTEX",
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_LATCH_FREE_OTHERS )
             != IDE_SUCCESS );

    IDE_TEST_RAISE( aParam->mCondVar.initialize((SChar*)"SECONDARY_BUFFER_FLUSH_JOB_COND_WAIT_COND") != IDE_SUCCESS, 
                    ERR_COND_INIT );

    aParam->mJobDone = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_INIT );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondInit ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * Description :
 *  초기화된 sdbFlushJobDoneNotifyParam을 바탕으로
 *  요청된 작업이 종료하기를 대기한다.
 *
 *  aParam  - [IN]  대기를 위해 필요한 변수
 *************************************************************************/
IDE_RC sdsFlushMgr::waitForJobDone( sdbFlushJobDoneNotifyParam *aParam )
{
    IDE_ASSERT(aParam->mMutex.lock(NULL) == IDE_SUCCESS);

    /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                 wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
    while ( aParam->mJobDone == ID_FALSE )
    {
        // 등록한 job이 처리 완료될때까지 대기한다.
        // cond_wait을 수행함과 동시에 mMutex를 푼다.
        IDE_TEST_RAISE( aParam->mCondVar.wait(&aParam->mMutex)
                       != IDE_SUCCESS, ERR_COND_WAIT );
    }

    IDE_ASSERT( aParam->mMutex.unlock() == IDE_SUCCESS );

    IDE_TEST_RAISE( aParam->mCondVar.destroy() != IDE_SUCCESS,
                    ERR_COND_DESTROY );

    IDE_TEST( aParam->mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_WAIT );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondWait) );
    }
    IDE_EXCEPTION( ERR_COND_DESTROY );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondDestroy) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 *    job을 처리했음을 알린다. 만약 jobDone을 세팅하지 않았으면
 *    이 함수안에서 아무런 작업도 하지 않는다.
 *    jobDone은 job을 등록할 때 옵션으로 줄 수 있다.
 *
 * aParam   - [IN]  이 변수에 대기하고 있는 쓰레드들에게 job이 종료 되었음을
 *                  알린다.
 ***************************************************************************/
void sdsFlushMgr::notifyJobDone(sdbFlushJobDoneNotifyParam *aParam)
{
    if (aParam != NULL)
    {
        IDE_ASSERT(aParam->mMutex.lock(NULL) == IDE_SUCCESS);

        IDE_ASSERT(aParam->mCondVar.signal() == IDE_SUCCESS);

        aParam->mJobDone = ID_TRUE;

        IDE_ASSERT(aParam->mMutex.unlock() == IDE_SUCCESS);
    }
}

/******************************************************************************
 * Description : Replacement Flush 의 조건
 * 해당 블록이 사용중이 아니며 블록의 상태가 dirty 면 flush 대상으로 삼는다.
 ******************************************************************************/
idBool sdsFlushMgr::isCond4ReplaceFlush()
{   
    idBool sRet = ID_FALSE;

    if ( sdsBufferMgr::hasExtentMovedownDone() == ID_TRUE )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

/******************************************************************************
 * Description :
 *    flusher가 job을 처리하고 다음 job을 처리할 때까지 대기할 시간을
 *    반환한다. 시스템의 상황에 따라 가변적이다. flush할게 많으면
 *    대기 시간은 짧고 할게 별로 없으면 대기 시간은 길어진다.
 ******************************************************************************/
idBool sdsFlushMgr::isBusyCondition()
{
    idBool sRet = ID_FALSE;

    if( mReqJobQueue[mReqJobGetPos].mType != SDB_FLUSH_JOB_NOTHING )
    {
        sRet = ID_TRUE;
    }

    if( isCond4ReplaceFlush() == ID_TRUE )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

/*************************************************************************
 * Description : CP Flusher갯수가 max 값보다 크지 않도록 조절한다.
 *************************************************************************/
void sdsFlushMgr::limitCPFlusherCount( idvSQL      * aStatistics, 
                                       sdbFlushJob * aRetJob,
                                       idBool        aAlreadyLocked )
{
    if( ( smuProperty::getMaxSBufferCPFlusherCnt() == 0 ) ||
        ( aRetJob->mType != SDB_FLUSH_JOB_CHECKPOINT_FLUSH ) )
    {
        /* Property가 0이거나, CheckpointFlush가 아니면 무시한다. */
    }
    else
    {
        if( aAlreadyLocked == ID_FALSE )
        {
            IDE_ASSERT( mReqJobMutex.lock( aStatistics ) == IDE_SUCCESS );
        }

        /* BUG-32664 */
        if( mActiveCPFlusherCount >= smuProperty::getMaxSBufferCPFlusherCnt() )
        {
            aRetJob->mReqFlushCount = 0;
            aRetJob->mFlushJobParam.mChkptFlush.mRedoPageCount    
                                    = ID_ULONG_MAX;
            aRetJob->mFlushJobParam.mChkptFlush.mRedoLogFileCount 
                                    = ID_UINT_MAX;
        }
        else
        {
            mActiveCPFlusherCount ++;
        }

        if( aAlreadyLocked == ID_FALSE )
        {
            IDE_ASSERT( mReqJobMutex.unlock() == IDE_SUCCESS );
        }
    }
    return;
}

/*************************************************************************
 * Description : flusher들중 가장 작은 recoveryLSN을 리턴한다.
 *  aStatistics - [IN]  통계정보
 *  aRet        - [OUT] flusher들중 가장 작은 recoveryLSN을 리턴한다.
 *************************************************************************/
void sdsFlushMgr::getMinRecoveryLSN( idvSQL *aStatistics,
                                     smLSN  *aRet )
{
    UInt   i;
    smLSN  sFlusherMinLSN;

    SM_LSN_MAX( *aRet );
    for (i = 0; i < mMaxFlusherCount; i++)
    {
        mFlushers[i].getMinRecoveryLSN( aStatistics, &sFlusherMinLSN );

        if ( smLayerCallback::isLSNLT( &sFlusherMinLSN, aRet ) == ID_TRUE )
        {
            SM_GET_LSN( *aRet, sFlusherMinLSN );
        }
    }
}

/******************************************************************************
 * Description : 동작하고 있는 flusher의 수를 반환한다
 ******************************************************************************/
UInt sdsFlushMgr::getActiveFlusherCount()
{
    UInt i;
    UInt sActiveCount = 0;

    for( i = 0 ; i < mMaxFlusherCount ; i++ )
    {
        if( mFlushers[ i ].isRunning() == ID_TRUE )
        {
            sActiveCount++;
        }
    }
    return sActiveCount;
}

