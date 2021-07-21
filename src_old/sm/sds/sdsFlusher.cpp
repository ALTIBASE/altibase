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
   sdbFlusher.cpp를 기본으로 만든 파일 이므로
   sdbFlusher에 수정이 생기면 이 파일에도 적용 검토!!! 
 **********************************************************************/

#include <smDef.h>
#include <smErrorCode.h>
#include <sdbDef.h>
#include <smrRecoveryMgr.h>
#include <sds.h>
#include <sdsReq.h>


extern "C" SInt
sdsCompareFileInfo( const void* aElem1, const void* aElem2 )
{
    sdbSyncFileInfo sFileInfo1;
    sdbSyncFileInfo sFileInfo2;

    sFileInfo1 = *(sdbSyncFileInfo*)aElem1;
    sFileInfo2 = *(sdbSyncFileInfo*)aElem2;

    if ( sFileInfo1.mSpaceID > sFileInfo2.mSpaceID )
    {
        return 1;
    }
    else
    {
        if ( sFileInfo1.mSpaceID < sFileInfo2.mSpaceID )
        {
            return -1;
        }
    }

    if ( sFileInfo1.mFileID > sFileInfo2.mFileID )
    {
        return 1;
    }
    else
    {
        if ( sFileInfo1.mFileID < sFileInfo2.mFileID )
        {
            return -1;
        }
    }
    return 0;
}

sdsFlusher::sdsFlusher() : idtBaseThread()
{

}

sdsFlusher::~sdsFlusher()
{

}

/************************************************************************
 * Description : flusher 객체를 초기화한다. 
 *  aFlusherID      - [IN]  이 객체의 고유 ID. 
 *  aPageSize       - [IN]  페이지 크기. 
 *  aIOBPageCount   - [IN]  flusher의 IOB 크기로서 단위는 page 개수이다.
 *  aCPListSet      - [IN]  플러시 해야 하는 FlushList가 속해있는 buffer pool
 *                          에 속한 checkpoint set
 ************************************************************************/
IDE_RC sdsFlusher::initialize ( UInt  aFlusherID,
                                UInt  aPageSize,
                                UInt  aIOBPageCount )
{
    SChar   sTmpName[128];
    UInt    i;
    SInt    sState = 0;

    mFlusherID    = aFlusherID;
    mPageSize     = aPageSize;   /* SD_PAGE_SIZE */
    mIOBPageCount = aIOBPageCount;
    mIOBPos       = 0;
    mFinish       = ID_FALSE;
    mStarted      = ID_FALSE;
    mWaitTime     = smuProperty::getDefaultFlusherWaitSec();
    mCPListSet    = sdsBufferMgr::getCPListSet();
    mBufferArea   = sdsBufferMgr::getSBufferArea(); 
    mSBufferFile  = sdsBufferMgr::getSBufferFile();
    mMeta         = sdsBufferMgr::getSBufferMeta();
 
    SM_LSN_MAX( mMinRecoveryLSN );
    SM_LSN_INIT( mMaxPageLSN );

    // 통계 정보를 위한 session 정보 초기화
    // BUG-21155 : current session을 초기화
    idvManager::initSession(&mCurrSess, 0 /* unused */, NULL /* unused */);

    idvManager::initSQL( &mStatistics,
                         &mCurrSess,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         IDV_OWNER_FLUSHER);

    idlOS::memset( sTmpName, 0, 128);
    idlOS::snprintf( sTmpName,
                     128,
                     "SECONDARY_BUFFER_FLUSHER_MIN_RECOVERY_LSN_MUTEX_%"ID_UINT32_FMT,
                     aFlusherID );

    IDE_TEST( mMinRecoveryLSNMutex.initialize( 
           sTmpName,
           IDU_MUTEX_KIND_NATIVE,
           IDB_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FLUSHER_MIN_RECOVERY_LSN)
           != IDE_SUCCESS);
    sState = 1;

    idlOS::memset( sTmpName, 0, 128 );
    idlOS::snprintf( sTmpName,
                     128,
                     "SECONDARY_BUFFER_FLUSHER_COND_WAIT_MUTEX_%"ID_UINT32_FMT,
                     aFlusherID);

    IDE_TEST( mRunningMutex.initialize( sTmpName,
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_LATCH_FREE_OTHERS )
             != IDE_SUCCESS);
    sState = 2;

    /* TC/FIT/Limit/sm/sdsFlusher_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsFlusher::initialize::malloc1", 
                          ERR_INSUFFICIENT_MEMORY );

    // IOB 초기화
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       mPageSize * (mIOBPageCount + 1),
                                       (void**)&mIOBSpace ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 3;

    idlOS::memset((void*)mIOBSpace, 0, mPageSize * (mIOBPageCount + 1));

    mIOB = (UChar*)idlOS::align( (void*)mIOBSpace, mPageSize );

    /* TC/FIT/Limit/sm/sdsFlusher_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsFlusher::initialize::malloc2", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF(UChar*) * mIOBPageCount,
                                       (void**)&mIOBPtr ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 4;

    /* TC/FIT/Limit/sm/sdsFlusher_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsFlusher::initialize::malloc3", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF(sdsBCB*) * mIOBPageCount,
                                       (void**)&mIOBBCBArray ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 5;


    /* TC/FIT/Limit/sm/sdsFlusher_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdsFlusher::initialize::malloc4", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF( sdbSyncFileInfo ) * mIOBPageCount,
                                       (void**)&mFileInfoArray ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 6;
    // condition variable 초기화
    // flusher가 쉬는 동안 여기에 대기를 한다.
    idlOS::snprintf( sTmpName,
                     ID_SIZEOF( sTmpName ),
                     "SECONDARY_BUFFER_FLUSHER_COND_%"ID_UINT32_FMT,
                     aFlusherID);

    IDE_TEST_RAISE( mCondVar.initialize(sTmpName) != IDE_SUCCESS,
                    ERR_COND_VAR_INIT);
    sState = 7;

    for (i = 0; i < mIOBPageCount; i++)
    {
        mIOBPtr[i] = mIOB + (mPageSize * i);
    }

    IDE_TEST( mDWFile.create( mFlusherID, 
                              mPageSize, 
                              mIOBPageCount, 
                              SD_LAYER_SECONDARY_BUFFER )
              != IDE_SUCCESS);
    sState = 8; 

    mFlusherStat.initialize( mFlusherID );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_VAR_INIT );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondInit) );
    }
    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY );
    {
        IDE_SET( ideSetErrorCode(idERR_ABORT_InsufficientMemory) );
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 8:
            IDE_ASSERT( mDWFile.destroy() == IDE_SUCCESS );
        case 7:
            IDE_ASSERT( mCondVar.destroy() == IDE_SUCCESS );
        case 6:
            IDE_ASSERT( iduMemMgr::free( mFileInfoArray ) == IDE_SUCCESS );
        case 5:
            IDE_ASSERT( iduMemMgr::free( mIOBBCBArray ) == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( iduMemMgr::free( mIOBPtr ) == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( iduMemMgr::free( mIOBSpace ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( mRunningMutex.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mMinRecoveryLSNMutex.destroy() == IDE_SUCCESS );
        default:
           break;
    }

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : flusher 객체
 ****************************************************************************/
IDE_RC sdsFlusher::destroy()
{
    IDE_ASSERT( mDWFile.destroy() == IDE_SUCCESS );

    IDE_TEST_RAISE( mCondVar.destroy() != IDE_SUCCESS, ERR_COND_DESTROY );

    IDE_ASSERT( iduMemMgr::free( mFileInfoArray ) == IDE_SUCCESS );
    mFileInfoArray = NULL;

    IDE_ASSERT( iduMemMgr::free( mIOBBCBArray ) == IDE_SUCCESS );
    mIOBBCBArray = NULL;

    IDE_ASSERT( iduMemMgr::free( mIOBPtr ) == IDE_SUCCESS );
    mIOBPtr = NULL;

    IDE_ASSERT( iduMemMgr::free( mIOBSpace ) == IDE_SUCCESS );
    mIOBSpace = NULL;

    IDE_ASSERT( mRunningMutex.destroy() == IDE_SUCCESS );

    IDE_ASSERT( mMinRecoveryLSNMutex.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_DESTROY );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondDestroy ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :
 ****************************************************************************/
IDE_RC sdsFlusher::start()
{
    mFinish = ID_FALSE;

    IDE_TEST(idtBaseThread::start() != IDE_SUCCESS);

    IDE_TEST(idtBaseThread::waitToStart() != IDE_SUCCESS);

    mFlusherStat.applyStart();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :
 *    flusher 쓰레드의 실행을 동기적으로 종료시킨다.
 *  aStatistics - [IN]  통계정보
 *  aNotStarted - [OUT] flusher가 아직 시작되지 않았다면 ID_TRUE를 리턴. 그 외엔
 *                      ID_FALSE를 리턴
 ****************************************************************************/
IDE_RC sdsFlusher::finish( idvSQL *aStatistics, idBool *aNotStarted )
{
    IDE_ASSERT( mRunningMutex.lock(aStatistics) == IDE_SUCCESS );

    if( mStarted == ID_FALSE )
    {
        IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );
        *aNotStarted = ID_TRUE;
    }
    else
    {
        mFinish      = ID_TRUE;
        mStarted     = ID_FALSE;
        *aNotStarted = ID_FALSE;

        IDE_TEST_RAISE( mCondVar.signal() != 0, ERR_COND_SIGNAL );

        IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );

        IDE_TEST_RAISE( join() != IDE_SUCCESS,  ERR_THR_JOIN );

        mFlusherStat.applyFinish();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THR_JOIN );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_Systhrjoin ) );
    }
    IDE_EXCEPTION( ERR_COND_SIGNAL);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondSignal ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : flusher 쓰레드가 start되면 불리는 함수이다.
 ****************************************************************************/
void sdsFlusher::run()
{
    sdbFlushJob                      sJob;
    sdbSBufferReplaceFlushJobParam * sReplaceJobParam;
    sdbObjectFlushJobParam         * sObjJobParam;
    sdbChkptFlushJobParam          * sChkptJobParam;

    PDL_Time_Value sTimeValue;
    time_t         sBeforeWait;
    time_t         sAfterWait;
    UInt           sWaitSec;
    ULong          sFlushedCount = 0;
    idBool         sMutexLocked  = ID_FALSE;
    IDE_RC         sRC;
    
    IDE_ASSERT( mRunningMutex.lock( NULL ) == IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    mStarted = ID_TRUE;
    while ( mFinish == ID_FALSE )
    {
        sFlushedCount = 0;
        sdsFlushMgr::getJob( &mStatistics, &sJob );
        switch( sJob.mType )
        {
           case SDB_FLUSH_JOB_REPLACEMENT_FLUSH:
                mFlusherStat.applyReplaceFlushJob();
                sReplaceJobParam = &sJob.mFlushJobParam.mSBufferReplaceFlush;
                IDE_TEST( flushForReplacement( &mStatistics,
                                               sJob.mReqFlushCount,
                                               sReplaceJobParam->mExtentIndex,
                                               &sFlushedCount )
                          != IDE_SUCCESS );
                mFlusherStat.applyReplaceFlushJobDone();
                break;
            case SDB_FLUSH_JOB_CHECKPOINT_FLUSH:
                mFlusherStat.applyCheckpointFlushJob();
                sChkptJobParam = &sJob.mFlushJobParam.mChkptFlush;
                IDE_TEST( flushForCheckpoint( &mStatistics,
                                              sJob.mReqFlushCount,
                                              sChkptJobParam->mRedoPageCount,
                                              sChkptJobParam->mRedoLogFileCount,
                                              &sFlushedCount )
                          != IDE_SUCCESS );
                mFlusherStat.applyCheckpointFlushJobDone();
                break;
            case SDB_FLUSH_JOB_DBOBJECT_FLUSH:
                mFlusherStat.applyObjectFlushJob();
                sObjJobParam = &sJob.mFlushJobParam.mObjectFlush;
                IDE_TEST( flushForObject( &mStatistics,
                                          sObjJobParam->mFiltFunc,
                                          sObjJobParam->mFiltObj,
                                          sObjJobParam->mBCBQueue,
                                          &sFlushedCount )
                          != IDE_SUCCESS );
                mFlusherStat.applyObjectFlushJobDone();
                break;
            default:
                break;
        }

        if( sJob.mType != SDB_FLUSH_JOB_NOTHING ) 
        {
            sdsFlushMgr::updateLastFlushedTime();
            sdsFlushMgr::notifyJobDone( sJob.mJobDoneParam );
        }

        if( sJob.mType == SDB_FLUSH_JOB_REPLACEMENT_FLUSH )
        {   
            /* flush 다한 Extent를 sdbFlusher가 movedown할수있도록 상태변경 */ 
            mBufferArea->changeStateFlushDone( sReplaceJobParam->mExtentIndex );
        } 

        sWaitSec = getWaitInterval( sFlushedCount );
        if( sWaitSec == 0 )
        {
            IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );
            IDE_ASSERT( mRunningMutex.lock( NULL ) == IDE_SUCCESS );
            continue;
        }

        sBeforeWait = idlOS::time( NULL );
        // sdsFlushMgr로부터 얼마나 쉴지 알아와서 time value에 설정한다.
        sTimeValue.set( sBeforeWait + sWaitSec );
        sRC = mCondVar.timedwait(&mRunningMutex, &sTimeValue);
        sAfterWait = idlOS::time( NULL );

        mFlusherStat.applyTotalSleepSec( sAfterWait - sBeforeWait );

        if( sRC == IDE_SUCCESS )
        {
            // cond_signal을 받고 깨어난 경우
            mFlusherStat.applyWakeUpsBySignal();
        }
        else
        {
            if(mCondVar.isTimedOut() == ID_TRUE)
            {
                // timeout에 의애 깨어난 경우
                mFlusherStat.applyWakeUpsByTimeout();
            }
            else
            {
                // 그외의 경우에는 에러로 처리를 한다.
                ideLog::log( IDE_SERVER_0, 
                             "Secondary Flusher Dead [RC:%d, ERRNO:%d]", 
                             sRC, errno );
                IDE_RAISE( ERR_COND_WAIT );
            }
        }
    }

    sMutexLocked = ID_FALSE;
    IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );

    return;

    IDE_EXCEPTION( ERR_COND_WAIT );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondWait ) );
    }
    IDE_EXCEPTION_END;

    if( sMutexLocked == ID_TRUE )
    {
        IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );
    }

    mStarted = ID_FALSE;
    mFinish = ID_TRUE;

    IDE_ASSERT( 0 );

    return;
}

/****************************************************************************
 * Description :
 *    flush list에 대해 flush작업을 수행한다.
 *  aStatistics     - [IN]  각 list 이동시 mutex 획득이 필요하기 때문에
 *                          통계정보가 필요하다.
 *  aReqFlushCount  - [IN]  사용자가 요청한 flush개수
 *                          X-latch가 잡혀있는 버퍼들을 옮길 LRU list
 *  aRetFlushedCount- [OUT] 실제 flush한 page 갯수
 ***************************************************************************/
IDE_RC sdsFlusher::flushForReplacement( idvSQL         * aStatistics,
                                        ULong            aReqFlushCount,
                                        ULong            aExtentIndex,
                                        ULong          * aRetFlushedCount)
{
    sdsBCB        * sSBCB;
    UInt            sFlushCount = 0;
    UInt            sReqFlushCount = 0;
    UInt            sSBCBIndex;   

    IDE_ERROR( aReqFlushCount > 0 );

    *aRetFlushedCount = 0;
    /* 한 Extent씩 flush 한다. 한 Extent는 IOBPage 개수 만큼의  페이지로 구성 */
    sReqFlushCount = aReqFlushCount*mIOBPageCount;
    
    sSBCBIndex = aExtentIndex * mIOBPageCount ;

    // sReqFlushCount만큼 flush한다.
    while( sReqFlushCount-- )
    {
        sSBCB = mBufferArea->getBCB( sSBCBIndex );
        sSBCBIndex++;

        IDE_ASSERT( sSBCB != NULL )

       // BCB state 검사를 하기 위해 BCBMutex를 획득한다.
        sSBCB->lockBCBMutex( aStatistics );

        if( (sSBCB->mState == SDS_SBCB_OLD ) ||
            (sSBCB->mState == SDS_SBCB_INIOB ) ||
            (sSBCB->mState == SDS_SBCB_CLEAN ) ||
            (sSBCB->mState == SDS_SBCB_FREE) )
        {
            // REDIRTY, INIOB, CLEAN인 버퍼는  skip
            sSBCB->unlockBCBMutex();

            mFlusherStat.applyReplaceSkipPages();
            continue;
        }
        else
        {
            IDE_ASSERT( sSBCB->mState == SDS_SBCB_DIRTY );
        }

        // flush 조건을 모두 만족했다.
        // 이제 flush를 하자.
        sSBCB->mState = SDS_SBCB_INIOB;
        sSBCB->unlockBCBMutex();

        if( SM_IS_LSN_INIT(sSBCB->mRecoveryLSN) )
        {
            // temp page인 경우 double write를 할 필요가 없기때문에
            // IOB에 복사한 후 바로 그 페이지만 disk에 내린다.
            IDE_TEST( flushTempPage( aStatistics,
                                     sSBCB )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( copyToIOB( aStatistics,
                                 sSBCB )
                      != IDE_SUCCESS);
        }
        mFlusherStat.applyReplaceFlushPages();

        sFlushCount++;
    }

    // IOB에 남은 페이지들을 디스크에 내린다.
    IDE_TEST( writeIOB( aStatistics ) != IDE_SUCCESS );

    *aRetFlushedCount = sFlushCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :
 *    CPListSet의 버퍼들을 recovery LSN이 작은 순으로 flush를 수행한다.
 *
 *  aStatistics         - [IN]  통계정보
 *  aMinFlushCount      - [IN]  flush할 최소 Page 수
 *  aRedoDirtyPageCnt   - [IN]  Restart Recovery시에 Redo할 Page 수,
 *                              반대로 말하면 CP List에 남겨둘 Dirty Page의 수
 *  aRedoLogFileCount   - [IN]  Restart Recovery시에 redo할 LogFile 수
 *  aRetFlushedCount    - [OUT] 실제 flush한 page 갯수
 ****************************************************************************/
IDE_RC sdsFlusher::flushForCheckpoint( idvSQL      * aStatistics,
                                       ULong         aMinFlushCount,
                                       ULong         aRedoPageCount,
                                       UInt          aRedoLogFileCount,
                                       ULong       * aRetFlushedCount )
{
    sdsBCB  * sSBCB;
    ULong     sFlushCount    = 0;
    ULong     sReqFlushCount = 0;
    ULong     sNotDirtyCount = 0;
    smLSN     sReqFlushLSN;
    smLSN     sMaxFlushLSN;
    UShort    sActiveFlusherCount;

    IDE_ASSERT( aRetFlushedCount != NULL );

    *aRetFlushedCount = 0;
    // checkpoint list에서 가장 큰 recoveryLSN -> 최대 flush 할수 있는 LSN
    mCPListSet->getMaxRecoveryLSN( aStatistics, &sMaxFlushLSN );

    //DISK의 마지막 LSN값을 구한다.
    smLayerCallback::getLstLSN( &sReqFlushLSN );

    if ( sReqFlushLSN.mFileNo >= aRedoLogFileCount )
    {
        sReqFlushLSN.mFileNo -= aRedoLogFileCount;
    }
    else
    {
        SM_LSN_INIT( sReqFlushLSN );
    }

    // flush 요청할 page의 수를 계산한다.
    sReqFlushCount = mCPListSet->getTotalBCBCnt();

    if( sReqFlushCount > aRedoPageCount )
    {
        // CP list의 Dirty Page수가 Restart Recovery시에
        // Redo 할 Page 보다 많아졌으므로 그 차이를 flush한다.
        sReqFlushCount -= aRedoPageCount;

        // flush할 page를 작동중인 flusher수(n)로 1/n로 나눈다.
        sActiveFlusherCount = sdsFlushMgr::getActiveFlusherCount();
        IDE_ASSERT( sActiveFlusherCount > 0 );
        sReqFlushCount /= sActiveFlusherCount;

        if( aMinFlushCount > sReqFlushCount )
        {
            sReqFlushCount = aMinFlushCount;
        }
    }
    else
    {
        // CP list의 Dirty Page수가 Restart Recovery시에
        // Redo 할 Page 보다 작다. MinFlushCount 만큼만 flush한다.
        sReqFlushCount = aMinFlushCount;
    }
    // CPListSet에서 recovery LSN이 가장 작은 BCB를 본다.
    sSBCB = (sdsBCB*)mCPListSet->getMin();

    while( sSBCB != NULL )
    {
        if ( smLayerCallback::isLSNLT( &sMaxFlushLSN,
                                       &sSBCB->mRecoveryLSN )
             == ID_TRUE)
        {
            // RecoveryLSN이 최대 flush 하려는 LSN보다 크므로 땡. 
            break;
        }

        if ( ( sFlushCount >= sReqFlushCount ) &&
             ( smLayerCallback::isLSNGT( &sSBCB->mRecoveryLSN, &sReqFlushLSN ) 
               == ID_TRUE) )
        {
            // sReqFlushCount 이상 flush하였고
            // sReqFlushLSN까지 flush하였으면 flush 작업을 중단한다.
            break;
        }

        sSBCB->lockBCBMutex( aStatistics );

        if( sSBCB->mState != SDS_SBCB_DIRTY )
        {
            // DIRTY가 아닌 버퍼는 skip한다.
            // 다음 BCB를 참조한다.
            sSBCB->unlockBCBMutex();

            sNotDirtyCount++;
            if( sNotDirtyCount > mCPListSet->getListCount() )
            {
                IDE_TEST( writeIOB( aStatistics ) != IDE_SUCCESS );
                sNotDirtyCount = 0;
            }
            // nextBCB()를 통해서 얻은 BCB역시 참조하는 동안
            // CPListSet에서 빠질 수 있다.
            // 또한 nextBCB()의 인자로 사용된 sSBCB역시
            // 이 시점에서 CPList에서 빠질 수 있다.
            // 만약 그런 경우라면 nextBCB는 minBCB를 리턴할 것이다.
            sSBCB = (sdsBCB*)mCPListSet->getNextOf( sSBCB );
            mFlusherStat.applyCheckpointSkipPages();
            continue;
        }
        sNotDirtyCount = 0;
        // 다른 flusher들이 flush를 못하게 하기 위해
        // INIOB상태로 변경한다.
        sSBCB->mState = SDS_SBCB_INIOB;
        sSBCB->unlockBCBMutex();

        if( SM_IS_LSN_INIT( sSBCB->mRecoveryLSN ) )
        {
            // TEMP PAGE가 check point 리스트에 달려 있으면 안된다.
            // 그렇지만 문제가 되지는 않기 때문에, release시에 발견되더라도
            // 죽이지 않도록 한다.
            // TEMP PAGE가 아니더라도, mRecoveryLSN이 init값이라는 것은
            // recovery와 상관없는 페이지라는 뜻이다.
            // recovery와 상관없는 페이지는 checkpoint를 하지 않아도 된다.
            IDE_DASSERT( 0 );
            IDE_TEST( flushTempPage( aStatistics, sSBCB ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( copyToIOB( aStatistics, sSBCB ) != IDE_SUCCESS );
        }
        mFlusherStat.applyCheckpointFlushPages();

        sFlushCount++;

        // 다음 flush할 BCB를 구한다.
        sSBCB = (sdsBCB*)mCPListSet->getMin();
    }

    *aRetFlushedCount = sFlushCount;

    // IOB에 남아 있는 버퍼들을 모두 disk에 기록한다.
    IDE_TEST( writeIOB( aStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :
 *  aBCBQueue에 들어있는 BCB를 대상으로 flush를 수행한다.
 *  aStatistics     - [IN]  통계정보.
 *  aFiltFunc       - [IN]  플러시 할지 말지 조건 함수.
 *  aFiltObj        - [IN]  aFiltFunc에 넘겨줄 인자.
 *  aBCBQueue       - [IN]  플러시 해야할 BCB들의 포인터가 들어있는 큐.
 *  aRetFlushedCount- [OUT] 실제 플러시 한 페이지 개수.
 ****************************************************************************/
IDE_RC sdsFlusher::flushForObject( idvSQL       * aStatistics,
                                   sdsFiltFunc    aFiltFunc,
                                   void         * aFiltObj,
                                   smuQueueMgr  * aBCBQueue,
                                   ULong        * aRetFlushedCount )
{
    sdsBCB          * sSBCB  = NULL;
    idBool            sEmpty = ID_FALSE;
    PDL_Time_Value    sTV;
    ULong             sRetFlushedCount = 0;

    while (1)
    {
        IDE_ASSERT( aBCBQueue->dequeue( ID_FALSE, //mutex 잡지 않는다.
                                        (void*)&sSBCB,
                                        &sEmpty )
                    == IDE_SUCCESS );

        if ( sEmpty == ID_TRUE )
        {
            // 더이상 flush할 BCB가 없으므로 루프를 빠져나간다.
            break;
        }
retry:
        if( aFiltFunc( sSBCB, aFiltObj ) != ID_TRUE )
        {
            mFlusherStat.applyObjectSkipPages();
            continue;
        }
        
        // BCB state 검사를 하기 위해 BCBMutex를 획득한다.
        sSBCB->lockBCBMutex(aStatistics);

        if( (sSBCB->mState == SDS_SBCB_CLEAN) ||
            (sSBCB->mState == SDS_SBCB_FREE) )
        {
            sSBCB->unlockBCBMutex();
            mFlusherStat.applyObjectSkipPages();
            continue;
        }

        if( aFiltFunc( sSBCB, aFiltObj ) == ID_FALSE )
        {
            // BCB가 aFiltFunc조건을 만족하지 못하면 플러시 하지 않는다.
            sSBCB->unlockBCBMutex();
            mFlusherStat.applyObjectSkipPages();
            continue;
        }

        // BUG-21135 flusher가 flush작업을 완료하기 위해서는
        // INIOB상태의 BCB상태가 변경되길 기다려야 합니다.
        if( (sSBCB->mState == SDS_SBCB_INIOB) ||
            (sSBCB->mState == SDS_SBCB_OLD ) )
        {
            sSBCB->unlockBCBMutex();
            sTV.set(0, 50000);
            idlOS::sleep(sTV);
            goto retry;
        }
        // flush 조건을 모두 만족했다.
        // 이제 flush를 하자.
        sSBCB->mState = SDS_SBCB_INIOB;
        sSBCB->unlockBCBMutex();

        if( SM_IS_LSN_INIT(sSBCB->mRecoveryLSN) )
        {
            // temp page인 경우 double write를 할 필요가 없기때문에
            // IOB에 복사한 후 바로 그 페이지만 disk에 내린다.
            IDE_TEST( flushTempPage( aStatistics, sSBCB )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( copyToIOB( aStatistics, sSBCB )
                      != IDE_SUCCESS );
        }
        mFlusherStat.applyObjectFlushPages();
        sRetFlushedCount++;
    }

    // IOB를 디스크에 기록한다.
    IDE_TEST( writeIOB( aStatistics )
              != IDE_SUCCESS);

    *aRetFlushedCount = sRetFlushedCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aRetFlushedCount = sRetFlushedCount;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :
 *    temp page인 경우 copyToIOB()대신 이 함수를 사용한다.
 *  aStatistics     - [IN]  통계정보
 *  aBCB            - [IN]  flush할 BCB
 ****************************************************************************/
IDE_RC sdsFlusher::flushTempPage( idvSQL    * aStatistics,
                                  sdsBCB    * aSBCB )
{
    SInt        sWaitEventState = 0;
    idvWeArgs   sWeArgs;
    UChar     * sIOBPtr;
    idvTime     sBeginTime;
    idvTime     sEndTime;
    ULong       sTempWriteTime=0;

    IDE_ASSERT( aSBCB->mCPListNo == SDB_CP_LIST_NONE );
    IDE_ASSERT( smLayerCallback::isTempTableSpace( aSBCB->mSpaceID )
                == ID_TRUE );

    sIOBPtr = mIOBPtr[mIOBPos];

    aSBCB->lockBCBMutex( aStatistics );

    if( aSBCB->mState == SDS_SBCB_INIOB )
    {
        /* PBuffer에 있는 상태를 보고 복사하거나 PBuffer 상태를 바꿔야 하나 
           Mutex 잡고 하는데 무리가 있는듯.  하지 말자. */
        IDE_TEST_RAISE( mSBufferFile->open( aStatistics ) != IDE_SUCCESS,
                        ERR_READ_SECONDARY_BUFFER );

        IDE_TEST_RAISE( mSBufferFile->read( aStatistics,
                               aSBCB->mSBCBID,      /* who */
                               mMeta->getFrameOffset( aSBCB->mSBCBID ),
                               1,                   /* page counter */
                               sIOBPtr )            /* where */
                        != IDE_SUCCESS,
                        ERR_READ_SECONDARY_BUFFER );

        /* BUG-22271 */
        IDV_WEARGS_SET( &sWeArgs, IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_WRITE,
                        0, /* WaitParam1 */
                        0, /* WaitParam2 */
                        0  /* WaitParam3 */ );

        IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );
        sWaitEventState = 1;

        IDV_TIME_GET(&sBeginTime);

        mFlusherStat.applyIOBegin();
        
        IDE_TEST( sddDiskMgr::write( aStatistics,
                                     aSBCB->mSpaceID, 
                                     aSBCB->mPageID, 
                                     mIOBPtr[mIOBPos] )
                  != IDE_SUCCESS );

        aSBCB->mState = SDS_SBCB_CLEAN;
        aSBCB->unlockBCBMutex();

        mFlusherStat.applyIODone();

        IDV_TIME_GET( &sEndTime );
        sTempWriteTime = IDV_TIME_DIFF_MICRO( &sBeginTime, &sEndTime );

        sWaitEventState = 0;
        IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );

        /* BUG-32670    [sm-disk-resource] add IO Stat information
         * for analyzing storage performance.
         * 현재 TempPage는 한번에 한 Page씩 Write함. */
        mFlusherStat.applyTotalFlushTempPages( 1 );
        mFlusherStat.applyTotalTempWriteTimeUSec( sTempWriteTime );
    }
    else 
    {
        sdsBufferMgr::makeFreeBCBForce( aStatistics,
                                        aSBCB ); 
        aSBCB->unlockBCBMutex();
    } 

    /* BUG-32670    [sm-disk-resource] add IO Stat information
     * for analyzing storage performance.
     * 현재 TempPage는 한번에 한 Page씩 Write함. */
    mFlusherStat.applyTotalFlushTempPages( 1 );
    mFlusherStat.applyTotalTempWriteTimeUSec( sTempWriteTime );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_READ_SECONDARY_BUFFER );
    {
        sdsBufferMgr::setUnserviceable();
        IDE_SET( ideSetErrorCode( smERR_FATAL_PageReadStopped ) );
    }
    IDE_EXCEPTION_END;

    if( sWaitEventState == 1 )
    {
        IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
    }

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : BCB의 frame 내용을 IOB에 복사한다.
 *  aStatistics     - [IN]  통계정보
 *  aBCB            - [IN]  BCB
 ****************************************************************************/
IDE_RC sdsFlusher::copyToIOB( idvSQL  * aStatistics,
                              sdsBCB  * aSBCB )
{
    UChar     * sIOBPtr;
    smLSN       sPageLSN;
 
    IDE_ASSERT( mIOBPos < mIOBPageCount );
    IDE_ASSERT( aSBCB->mCPListNo != SDB_CP_LIST_NONE );

    /* PROJ-2162 RestartRiskReduction
     * Consistency 하지 않으면, Flush를 막는다.  */
    if( ( smrRecoveryMgr::getConsistency() == ID_FALSE ) &&
        ( smuProperty::getCrashTolerance() != 2 ) )
    {
        /* Checkpoint Linst에서 BCB를 제거하고 Mutex를
         * 풀어주기만 한다. Flush나 IOB 사용은 안함 */
        mCPListSet->remove( aStatistics, aSBCB );
    }
    else
    {
        // IOB의 minRecoveryLSN을 갱신하기 위해서 mMinRecoveryLSNMutex를 획득한다.
        IDE_ASSERT( mMinRecoveryLSNMutex.lock(aStatistics) == IDE_SUCCESS );

        if ( smLayerCallback::isLSNLT( &aSBCB->mRecoveryLSN, 
                                       &mMinRecoveryLSN )
             == ID_TRUE)
        {
            SM_GET_LSN( mMinRecoveryLSN, aSBCB->mRecoveryLSN );
        }

        IDE_ASSERT( mMinRecoveryLSNMutex.unlock() == IDE_SUCCESS );

        mCPListSet->remove( aStatistics, aSBCB );

        sPageLSN = aSBCB->mPageLSN;

        sIOBPtr = mIOBPtr[ mIOBPos ];

        /* 여기서  PBuffer의 상태를 보고 PBuffer의 상태를 바꾸거나 복사해야하는데 
          mutex 잡는게 병목이 되는듯 하다. 하지 말자. */

        IDE_TEST_RAISE( mSBufferFile->open( aStatistics ) != IDE_SUCCESS,
                        ERR_READ_SECONDARY_BUFFER );

        IDE_TEST_RAISE( mSBufferFile->read( aStatistics,
                                            aSBCB->mSBCBID,     /* who */
                                            mMeta->getFrameOffset( aSBCB->mSBCBID ),
                                            1,                  /* page counter */
                                            sIOBPtr )           /* where */
                        != IDE_SUCCESS,
                        ERR_READ_SECONDARY_BUFFER );

        mIOBBCBArray[mIOBPos] = aSBCB;

        // IOB에 기록된 BCB들의 pageLSN중에 가장 큰 LSN을 설정한다.
        // 이 값은 나중에 WAL을 지키기 위해 사용된다.
        if ( smLayerCallback::isLSNGT( &sPageLSN, &mMaxPageLSN )
             == ID_TRUE )
        {
            SM_GET_LSN( mMaxPageLSN, sPageLSN );
        }

        SM_LSN_INIT( aSBCB->mRecoveryLSN );
        smLayerCallback::calcAndSetCheckSum( sIOBPtr );

        mIOBPos++;
        mFlusherStat.applyINIOBCount( mIOBPos );

        // IOB가 가득찼으면 IOB를 디스크에 내린다.
        if( mIOBPos == mIOBPageCount )
        {
            IDE_TEST( writeIOB( aStatistics ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_READ_SECONDARY_BUFFER );
    {
        sdsBufferMgr::setUnserviceable();
        IDE_SET( ideSetErrorCode( smERR_FATAL_PageReadStopped ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :
 *    IOB를 디스크에 내린다.
 *  aStatistics     - [IN]  통계정보
 ****************************************************************************/
IDE_RC sdsFlusher::writeIOB( idvSQL *aStatistics )
{
    UInt        i;
    UInt        sDummySyncedLFCnt; // 나중에 지우자.
    idvWeArgs   sWeArgs;
    SInt        sWaitEventState = 0;
    sdsBCB    * sSBCB;
   
    idvTime     sBeginTime;
    idvTime     sEndTime;
    ULong       sDWTime;
    ULong       sWriteTime;
    ULong       sSyncTime;
    SInt        sErrState   = 0;
    UInt        sTBSSyncCnt = 0;

    if( mIOBPos > 0 )
    {
        IDE_DASSERT( ( smrRecoveryMgr::getConsistency() == ID_TRUE ) ||
                     ( smuProperty::getCrashTolerance() == 2 ) );
        sErrState = 1;
        // WAL
        // 하지만 있을리 없다.;;;;
        IDE_TEST( smLayerCallback::sync4BufferFlush( &mMaxPageLSN,
                                                     &sDummySyncedLFCnt )
                  != IDE_SUCCESS);
        sErrState = 2;

        if ( smuProperty::getUseDWBuffer() == ID_TRUE )
        {
            IDV_TIME_GET( &sBeginTime );

            mFlusherStat.applyIOBegin();
            IDE_TEST( mDWFile.write( aStatistics,
                                     mIOB,
                                     mIOBPos )
                     != IDE_SUCCESS );
            mFlusherStat.applyIODone();
            sErrState = 3;

            IDV_TIME_GET( &sEndTime );
            sDWTime = IDV_TIME_DIFF_MICRO( &sBeginTime, &sEndTime );
        }
        else
        {
            sDWTime = 0;
        }
        IDV_WEARGS_SET( &sWeArgs, IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_WRITE,
                        0, /* WaitParam1 */
                        0, /* WaitParam2 */
                        0  /* WaitParam3 */ );

        sWriteTime = 0;

        for ( i = 0; i < mIOBPos; i++ )
        {
            IDV_TIME_GET(&sBeginTime);
            IDV_BEGIN_WAIT_EVENT( aStatistics, &sWeArgs );
            sWaitEventState = 1;

            mFlusherStat.applyIOBegin();

            sSBCB = mIOBBCBArray[i];

            sSBCB->lockBCBMutex( aStatistics );

            if( sSBCB->mState == SDS_SBCB_INIOB )
            {
                sSBCB->mState = SDS_SBCB_CLEAN;

                sSBCB->unlockBCBMutex();

                mFileInfoArray[sTBSSyncCnt].mSpaceID = sSBCB->mSpaceID;
                mFileInfoArray[sTBSSyncCnt].mFileID  = SD_MAKE_FID( sSBCB->mPageID );
                sTBSSyncCnt++;

                IDE_TEST( sddDiskMgr::write( aStatistics,
                                             sSBCB->mSpaceID,  
                                             sSBCB->mPageID,
                                             mIOBPtr[i])
                          != IDE_SUCCESS);

                mFlusherStat.applyIODone();
                sErrState = 4;
            }
            else 
            {
                sdsBufferMgr::makeFreeBCBForce( aStatistics,
                                                sSBCB ); 

                sSBCB->unlockBCBMutex();
            }

            sWaitEventState = 0;
            IDV_END_WAIT_EVENT(aStatistics, &sWeArgs);
            IDV_TIME_GET(&sEndTime);
            sWriteTime += IDV_TIME_DIFF_MICRO( &sBeginTime,
                                               &sEndTime);
        }

        /* BUG-23752 */
        IDV_TIME_GET( &sBeginTime );
        IDE_TEST( syncAllFile4Flush( aStatistics,
                                     sTBSSyncCnt ) != IDE_SUCCESS );
        sErrState = 5;

        IDV_TIME_GET(&sEndTime);
        sSyncTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

        mFlusherStat.applyTotalFlushPages( mIOBPos );

        // IOB를 모두 disk에 내렸으므로 IOB를 초기화한다.
        mIOBPos = 0;
        mFlusherStat.applyINIOBCount( mIOBPos );

        SM_LSN_INIT( mMaxPageLSN );

        IDE_ASSERT( mMinRecoveryLSNMutex.lock(aStatistics) == IDE_SUCCESS );
        SM_LSN_MAX( mMinRecoveryLSN );
        IDE_ASSERT( mMinRecoveryLSNMutex.unlock() == IDE_SUCCESS );

        /* BUG-32670    [sm-disk-resource] add IO Stat information
         * for analyzing storage performance.
         * Flush에 걸린 시간들을 저장함. */
        mFlusherStat.applyTotalWriteTimeUSec( sWriteTime );
        mFlusherStat.applyTotalSyncTimeUSec( sSyncTime );
        mFlusherStat.applyTotalDWTimeUSec( sDWTime );
    }
    else
    {
        // IOB가 비어있으므로 할게 없다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( smERR_FATAL_PageFlushStopped, sErrState ) );

    if( sWaitEventState == 1 )
    {
        IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
    }

    return IDE_FAILURE;
}

/****************************************************************************
 * Description :  mFileInfoArray에 있는 File들을 Sort한후에  Sync한다.
 *
 *  aStatistics     - [IN] 통계정보
 *  aSyncArrCnt     - [IN] Sync 할 File Info 의 수
 ****************************************************************************/ 
IDE_RC sdsFlusher::syncAllFile4Flush( idvSQL * aStatistics,
                                      UInt     aSyncArrCnt )
{
    UInt            i;
    sdbSyncFileInfo sLstSyncFileInfo = { SC_NULL_SPACEID, 0 };

    idlOS::qsort( (void*)mFileInfoArray,
                  aSyncArrCnt,
                  ID_SIZEOF( sdbSyncFileInfo ),
                  sdsCompareFileInfo );

    for( i = 1 ; i < aSyncArrCnt ; i++ )
    {
        /* 동일 File에 대해서 두번 Sync하지 않고 한번만 한다. */
        /* drop file의 경우 flusher job을 대기한다.
         * 그러므로 flusher에서는 drop file을 고려하지 않아도 된다.*/
        if (( sLstSyncFileInfo.mSpaceID != mFileInfoArray[i].mSpaceID ) ||
            ( sLstSyncFileInfo.mFileID  != mFileInfoArray[i].mFileID ))
        {
            IDE_ASSERT( mFileInfoArray[i].mSpaceID != SC_NULL_SPACEID );

            sLstSyncFileInfo = mFileInfoArray[i];

            IDE_TEST( sddDiskMgr::syncFile( aStatistics,
                                            sLstSyncFileInfo.mSpaceID,
                                            sLstSyncFileInfo.mFileID )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : 현재 하고 있는 작업이 있다면 작업중인 작업이 종료되는 것을 기다
 *               린다. 만약 없다면 바로 리턴한다.
 ****************************************************************************/
void sdsFlusher::wait4OneJobDone()
{
    IDE_ASSERT( mRunningMutex.lock( NULL ) == IDE_SUCCESS );
    IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );
}

/****************************************************************************
 * Description :
 *    이 flusher가 쉬고 있으면 깨운다. 작업중이면 아무런 영향을 주지 않는다.
 *    깨웠는지의 유무는 aWaken으로 반환된다.
 * Implementation :
 *    flusher가 작업중이었으면 mRunningMutex를 잡는데 실패할 것이다.
 *
 *  aWaken  - [OUT] flusher가 쉬고 있어서 깨웠다면 ID_TRUE를,
 *                  flusher가 작업중이었으면 ID_FALSE를 반환한다.
 ****************************************************************************/
IDE_RC sdsFlusher::wakeUpSleeping( idBool *aWaken )
{
    idBool sLocked;

    IDE_ASSERT( mRunningMutex.trylock(sLocked) == IDE_SUCCESS );

    if( sLocked == ID_TRUE )
    {
        IDE_TEST_RAISE( mCondVar.signal() != IDE_SUCCESS,
                        ERR_COND_SIGNAL );

        IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );

        *aWaken = ID_TRUE;
    }
    else
    {
        *aWaken = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_SIGNAL );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondSignal) );
        IDE_ASSERT( mRunningMutex.unlock() == IDE_SUCCESS );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************************
 * Description :
 *    IOB에 복사된 BCB들 중 recoveryLSN이 가장 작은 것을 얻는다.
 *    이 값은 checkpoint시 사용된다.
 *    restart redo LSN은 이 값과 CPListSet의 minRecoveryLSN중 작은 값으로
 *    결정된다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRet        - [OUT] 최소 recoveryLSN
 ****************************************************************************/
void sdsFlusher::getMinRecoveryLSN( idvSQL *aStatistics,
                                    smLSN  *aRet )
{
    IDE_ASSERT( mMinRecoveryLSNMutex.lock(aStatistics ) == IDE_SUCCESS );

    SM_GET_LSN( *aRet, mMinRecoveryLSN );

    IDE_ASSERT( mMinRecoveryLSNMutex.unlock() == IDE_SUCCESS );
}

/*****************************************************************************
 * Description :
 *  하나의 작업을 끝마친 flusher가 쉬어야 할 시간을 지정해 준다.
 *
 *  aFlushedCount   - [IN]  flusher가 바로전 flush한 페이지 개수
 ****************************************************************************/
UInt sdsFlusher::getWaitInterval( ULong aFlushedCount )
{
    if( sdsFlushMgr::isBusyCondition() == ID_TRUE )
    {
        mWaitTime = 0;
    }
    else
    {
        if( aFlushedCount > 0 )
        {
            mWaitTime = smuProperty::getDefaultFlusherWaitSec();
        }
        else
        {
            // 기존에 한건도 flush하지 않았다면, 푹 쉰다.
            if( mWaitTime < smuProperty::getMaxFlusherWaitSec() )
            {
                mWaitTime++;
            }
        }
    }
    mFlusherStat.applyLastSleepSec(mWaitTime);

    return mWaitTime;
}

