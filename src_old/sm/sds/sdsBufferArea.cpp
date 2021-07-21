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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer 
 **********************************************************************/

/******************************************************************************
 * Description :
 *    sdsBufferArea 는  BCB를 만들어 관리한다.
 ******************************************************************************/
#include <smu.h>
#include <smErrorCode.h>
#include <sds.h>

/******************************************************************************
 * Description :
 *  aExtentCnt [IN] : Extent 수 (Secondary Buffer는 익스텐트단위로 동작한다)
 *  aBCBCnt    [IN] : page cnt 수: BCB의 수
 ******************************************************************************/
IDE_RC sdsBufferArea::initializeStatic( UInt aExtentCnt, 
                                        UInt aBCBCnt ) 
{
    sdsBCB    * sBCB;
    UInt        sBCBID  = 0;
    SInt        sState  = 0;
    UInt        i;
    SChar       sTmpName[128];

    mBCBCount       = 0;
    mBCBExtentCount = aExtentCnt;
    mMovedownPos    = 0;
    mFlushPos       = 0;
    mWaitingClientCount = 0;

    IDU_FIT_POINT_RAISE( "sdsBufferArea::initialize::malloc1", 
                          ERR_INSUFFICIENT_MEMORY );

    /* BCB Arrary */
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (UInt)ID_SIZEOF(sdsBCB*) * aBCBCnt,
                                       (void**)&mBCBArray ) 
                    != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sState = 1;

    /* BCB초기화 및 할당 */
    IDE_TEST( mBCBMemPool.initialize( IDU_MEM_SM_SDS,
                                      (SChar*)"SDS_BCB_MEMORY_POOL",
                                      1,                 // Multi Pool Cnt
                                      ID_SIZEOF(sdsBCB), // Element size
                                      aBCBCnt,           // Element count In Chun 
                                      ID_UINT_MAX,       // chunk limit 
                                      ID_FALSE,          // use mutex
                                      8,                 // align byte
                                      ID_FALSE,			 // ForcePooling
                                      ID_TRUE,			 // GarbageCollection
                                      ID_TRUE,			 // HWCacheLine
                                      IDU_MEMPOOL_TYPE_LEGACY  /* mempool type */) 
                != IDE_SUCCESS);			
    sState = 2;

    /* BCB초기화 및 할당 */
    for( i = 0; i < aBCBCnt ; i++ )
    {
        IDE_TEST( mBCBMemPool.alloc((void**)&sBCB) != IDE_SUCCESS );

        IDE_TEST( sBCB->initialize( sBCBID ) != IDE_SUCCESS );
        
        mBCBArray[sBCBID] = sBCB;
        sBCBID++;
    }
    mBCBCount = sBCBID;
    
    // wait mutex 초기화
    idlOS::snprintf( sTmpName,
                     ID_SIZEOF( sTmpName ),
                     "SECONDARY_EXTENT_WAIT_MUTEX" );
    
    IDE_TEST( mMutexForWait.initialize( 
                sTmpName,
                IDU_MUTEX_KIND_POSIX,
                IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_EXTENT_WAIT )
              != IDE_SUCCESS );
    sState = 3;
 
    // condition variable 초기화
    idlOS::snprintf( sTmpName,
                     ID_SIZEOF(sTmpName),
                     "SECONDARY_BUFFER_COND" );

    IDE_TEST_RAISE( mCondVar.initialize(sTmpName) != IDE_SUCCESS,
                    ERR_COND_VAR_INIT );
    sState = 4;

    /* 익스텐트 관리 */
    idlOS::snprintf( sTmpName,
                     ID_SIZEOF( sTmpName ),
                     "SECONDARY_FLUSH_EXTENT_MUTEX" );
    
    IDE_TEST( mExtentMutex.initialize( 
                sTmpName,
                IDU_MUTEX_KIND_NATIVE,
                IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_EXTENT_MUTEX )
              != IDE_SUCCESS );
    sState = 5;

    IDU_FIT_POINT_RAISE( "sdsBufferArea::initialize::malloc2", 
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDS,
                                       (ULong)ID_SIZEOF(sdsExtentStateType) *
                                       mBCBExtentCount,
                                       (void**)&mBCBExtState ) 
                   != IDE_SUCCESS,
                   ERR_INSUFFICIENT_MEMORY );
    sState = 6;

    /* 초기화 */ 
    for( i = 0 ; i< mBCBExtentCount ; i++ )
    {
        mBCBExtState[i] = SDS_EXTENT_STATE_FREE; 
    } 
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION( ERR_COND_VAR_INIT );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondInit ) );
    }
    IDE_EXCEPTION_END;

    for( ; sBCBID > 0 ; sBCBID-- )
    {
        sBCB = mBCBArray[sBCBID-1];

        IDE_ASSERT( sBCB->destroy() == IDE_SUCCESS );
        mBCBMemPool.memfree( sBCB );
    }
    
    switch( sState )
    {
        case 6:
            IDE_ASSERT( iduMemMgr::free( mBCBExtState ) == IDE_SUCCESS );
        case 5:
            IDE_ASSERT( mExtentMutex.destroy() == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( mCondVar.destroy() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( mMutexForWait.destroy() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( mBCBMemPool.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( mBCBArray ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 ******************************************************************************/
IDE_RC sdsBufferArea::destroyStatic()
{
    UInt     i;
    sdsBCB  *sBCB;

    for( i = 0; i < mBCBCount ; i++ )
    {
        sBCB = mBCBArray[i];
        IDE_ASSERT( sBCB->destroy() == IDE_SUCCESS );
        mBCBMemPool.memfree( sBCB );
    }

    IDE_ASSERT( iduMemMgr::free( mBCBExtState ) == IDE_SUCCESS );
    IDE_ASSERT( mExtentMutex.destroy() == IDE_SUCCESS );
    IDE_ASSERT( mCondVar.destroy() == 0 );
    IDE_ASSERT( mMutexForWait.destroy() == IDE_SUCCESS );
    IDE_ASSERT( mBCBMemPool.destroy() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::free( mBCBArray ) == IDE_SUCCESS );
    mBCBArray = NULL;

    return IDE_SUCCESS;
}

/******************************************************************************
 * Description : Secondary Flusher 가 flush 해야할 Extent가 있는지 확인 
 *  aExtIndex [OUT] - flush 대상 Extent
 ******************************************************************************/
idBool sdsBufferArea::getTargetFlushExtentIndex( UInt *aExtIndex )
{
    sdsExtentStateType sState;

    idBool sIsSuccess = ID_FALSE;   

    IDE_ASSERT( mExtentMutex.lock( NULL /*aStatistics*/ ) == IDE_SUCCESS );

    sState = mBCBExtState[mFlushPos];

    switch( sState )
    {
        /* sdbFlusher가 movedown을 완료 했지만 아직 datafile에 반영 전인
           Ext를 가져온다 */
        case SDS_EXTENT_STATE_MOVEDOWN_DONE :
            *aExtIndex = mFlushPos;
            sIsSuccess  = ID_TRUE;
            mBCBExtState[mFlushPos] = SDS_EXTENT_STATE_FLUSH_ING;
            mFlushPos = (mFlushPos+1) % mBCBExtentCount;
            break;

        /* 할일이 없거나 
           secondary Buffer의 Flusher 엄청 빠르거나 많아서 한바퀴를 돌아
           온 상황. */
        case SDS_EXTENT_STATE_FREE:
        case SDS_EXTENT_STATE_FLUSH_ING:
        case SDS_EXTENT_STATE_FLUSH_DONE:
            /* nothing to do */
            break; 

        /* sdbFlusher가 먼가를 내렸쓰고 있으면 다음턴에 flush시킬수 있도록
           mFlushPos 증가 안함 */
        case SDS_EXTENT_STATE_MOVEDOWN_ING:
            /* nothing to do */
            break; 

        default:
            ideLog::log( IDE_SERVER_0, 
                    "Unexpected BCBExtent [%d] : %d", 
                    mFlushPos, sState );
            IDE_DASSERT( 0 );
            break;
    }

    IDE_ASSERT( mExtentMutex.unlock() == IDE_SUCCESS );

     /* 인덱스를 받아 오지 못한 상황이면 False 반환 */
    return sIsSuccess;
}

/******************************************************************************
 * Description : sdbFlusher가 secondary Buffer에 movedown 할 idx 확인
 *  aExtIndex -[OUT]  
 ******************************************************************************/
IDE_RC sdsBufferArea::getTargetMoveDownIndex( idvSQL  * aStatistics, 
                                              UInt    * aExtIndex )
{
    sdsExtentStateType sState;
    idBool             sIsSuccess;   
    PDL_Time_Value     sTV;

retry:
    IDE_ASSERT( mExtentMutex.lock( aStatistics ) == IDE_SUCCESS );

    sIsSuccess = ID_FALSE;

    sState = mBCBExtState[mMovedownPos];

    switch ( sState )
    {
            /* 초기화된 직후거나
               이미 datafile에 반영이 되었으면      
               해당 Ext를 가져온다   */
        case SDS_EXTENT_STATE_FREE:
        case SDS_EXTENT_STATE_FLUSH_DONE:
            *aExtIndex = mMovedownPos;
            sIsSuccess = ID_TRUE;
            mBCBExtState[mMovedownPos] = SDS_EXTENT_STATE_MOVEDOWN_ING;
            mMovedownPos = (mMovedownPos+1) % mBCBExtentCount;
            break;

            /* secondary Buffer의 Flusher 가 엄청 밀려서 한바퀴를 돌아
               따라온 상황이다.
               더 뒤쪽으로 찾지 말고 secondary Flusher를 깨워서 기다린다. */  
        case SDS_EXTENT_STATE_MOVEDOWN_ING:
        case SDS_EXTENT_STATE_MOVEDOWN_DONE:
            /* nothing to do */
            break;

            /* sdsFlusher가 datafile에 flush 중이면 
               다음 Ext도 Flush 중일수 있으니
               더 뒤쪽으로 찾지 말고 다음 턴에 해당 Ext에 movedown 할수있도록
               mMovedownPos 증가 시키지 않고 대기 */
        case SDS_EXTENT_STATE_FLUSH_ING:
            /* nothing to do */
            break;

        default:
            ideLog::log( IDE_SERVER_0, 
                         "Unexpected BCBExtent [%d] : %d", 
                         mMovedownPos, sState );
            IDE_DASSERT( 0 );
            break;
    }

    IDE_ASSERT( mExtentMutex.unlock() == IDE_SUCCESS );
    
    if( sIsSuccess != ID_TRUE )
    {
        IDE_TEST( sdsFlushMgr::flushPagesInExtent( aStatistics ) 
                  != IDE_SUCCESS );

        sdsBufferMgr::applyVictimWaits(); 
        sTV.set( 0, 100 );
        idlOS::sleep(sTV);

        goto retry;
    }
    else 
    {
        /* 정상적으로 인덱스를 받아서 flusher를 동작시킨다 */
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : flush 다한 Extent를 sdbFlusher가 movedown할수있도록 상태변경 
 *  aExtIndex [IN]  
 ******************************************************************************/
IDE_RC sdsBufferArea::changeStateFlushDone( UInt aExtIndex )
{
    IDE_ASSERT( mExtentMutex.lock( NULL /*aStatistics*/ ) == IDE_SUCCESS );

    mBCBExtState[aExtIndex] = SDS_EXTENT_STATE_FLUSH_DONE;

    IDE_ASSERT( mExtentMutex.unlock() == IDE_SUCCESS );
    // 빈 EXTENT를 기다리고 있는 쓰레드가 있으면 깨운다.
    if( mWaitingClientCount > 0 )
    {
        IDE_ASSERT( mMutexForWait.lock( NULL /*aStatistics*/ ) == IDE_SUCCESS );

        IDE_TEST_RAISE( mCondVar.broadcast() != IDE_SUCCESS,
                        ERR_COND_SIGNAL );

        IDE_ASSERT( mMutexForWait.unlock() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_SIGNAL );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondSignal) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : movedown한 Extent를 sdsFlusher가 flush할수있도록 상태를 변경
 *  aExtIndex [IN]
 ******************************************************************************/
IDE_RC sdsBufferArea::changeStateMovedownDone( UInt aExtIndex )
{
    IDE_ASSERT( mExtentMutex.lock( NULL /*aStatistics*/ ) == IDE_SUCCESS );

    mBCBExtState[aExtIndex] = SDS_EXTENT_STATE_MOVEDOWN_DONE;

    IDE_ASSERT( mExtentMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/******************************************************************************
 * Description :  
 * aFunc     [IN]  각 BCB에 적용할 함수
 * aObj      [IN]  aFunc수행할때 필요한 변수
 ******************************************************************************/
IDE_RC sdsBufferArea::applyFuncToEachBCBs( sdsBufferAreaActFunc   aFunc,
                                           void                 * aObj )
{
    sdsBCB  * sSBCB;
    UInt      i;

    for( i = 0 ; i < mBCBCount ; i++ )
    {
        sSBCB = getBCB( i );
        IDE_TEST( aFunc( sSBCB, aObj) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
