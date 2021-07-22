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
 * $Id: sdrMiniTrans.cpp 89495 2020-12-14 05:19:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 mini-transaction에 대한 구현 파일이다.
 *
 *
 * # Mtx의 logging 모드와 flush list 등록
 *
 * MTX 사용상황  로깅 모드          flush list 등록 조건
 *-----------------------------------------------------------
 * temp table      no                  X latch page
 * ----------------------------------------------------------
 * redo 시         no                  X latch page
 * ----------------------------------------------------------
 * normal          yes                 X latch page 와
 *                                     begin, end LSN 값 있을때
 * ----------------------------------------------------------
 * normal          no   -> 이런 경우의 mtx는 사용되지 않는다.
 *
 **********************************************************************/


#include <ide.h>

#include <smxTrans.h>
#include <smrLogMgr.h>
#include <sdb.h>

#include <sct.h>
#include <sdrReq.h>
#include <sdrMiniTrans.h>
#include <sdrMtxStack.h>
#include <sdptbDef.h>
#include <sdnIndexCTL.h>


/* --------------------------------------------------------------------
 * sdrMtxLatchMode에 따른 release함수
 * ----------------------------------------------------------------- */
sdrMtxLatchItemApplyInfo sdrMiniTrans::mItemVector[SDR_MTX_RELEASE_FUNC_NUM] =
{
    // page x latch
    {
        &sdbBufferMgr::releasePage,
        &sdbBufferMgr::releasePageForRollback,
        &sdbBCB::isSamePageID,
        &sdbBCB::dump,
        SDR_MTX_BCB,
        "SDR_MTX_PAGE_X"
    },
    // page s latch
    {
        &sdbBufferMgr::releasePage,
        &sdbBufferMgr::releasePageForRollback,
        &sdbBCB::isSamePageID,
        &sdbBCB::dump,
        SDR_MTX_BCB,
        "SDR_MTX_PAGE_S"
    },
    // page no latch
    {
        &sdbBufferMgr::releasePage,
        &sdbBufferMgr::releasePageForRollback,
        &sdbBCB::isSamePageID,
        &sdbBCB::dump,
        SDR_MTX_BCB,
        "SDR_MTX_PAGE_NO"
    },
    // latch x
    {
		// wrapper
        &sdrMiniTrans::unlock,
        &sdrMiniTrans::unlock,
        &iduLatch::isLatchSame,
        &sdrMiniTrans::dump,
        SDR_MTX_LATCH,
        "SDR_MTX_LATCH_X"
    },
    // latch s
    {
		// wrapper
        &sdrMiniTrans::unlock,
        &sdrMiniTrans::unlock,
        &iduLatch::isLatchSame,
        &sdrMiniTrans::dump,
        SDR_MTX_LATCH,
        "SDR_MTX_LATCH_S"
    },
    // mutex
    {
        &iduMutex::unlock,
        &iduMutex::unlock,
        &iduMutex::isMutexSame,
        &iduMutex::dump,
        SDR_MTX_MUTEX,
        "SDR_MTX_MUTEX_X"
    }
};

/*PROJ-2162 RestartRiskReduction */
#if defined(DEBUG)
UInt sdrMiniTrans::mMtxRollbackTestSeq = 0;
#endif

/***********************************************************************
 * Description : sdrMiniTrans 초기화
 *
 * 또한dynamic 버퍼는 시스템 구동시에 전역 초기화가 되어야 하며,
 * 아래와 같이 latch 종류별 latch release 벡터 초기화가 필요하다.
 * + page latch이면  -> sdbBufferMgr::releasePage
 * + iduLatch이면    -> iduLatch::unlock
 * + iduMutex이면    -> object.unlock()
 *
 * sdrMtx 자료구조를 초기화한다.
 * mtx 스택을 초기화하고, 트랜잭션의 로그버퍼를 초기화한다.
 * mtx 인터페이스는 항상 start, commit 순으로 호출되어야 한다.
 * dml에 대한 로그 작성을 위해 replication 정보를 위한 변수들을
 * 초기화 한다.
 *
 * - 1st. code design
 *   + stack을 초기화 한다.
 *   + log mode를 set한다. -> setLoggingMode
 *   + trans를 set한다.
 *   + if (logging mode 모드이면)
 *        log를 write하기 위해 array를 initialize한다.
 *
 * - 2nd. code design
 *   + stack을 초기화 한다.      -> sdrMtxStack::initiailze
 *   + mtx 로깅 모드를 설정한다. -> sdrMiniTrans::setMtxMode
 *   + 트랜잭션을 설정한다.
 *   + undoable 여부를 설정한다.
 *   + modified를 초기화한다.
 *   + begin LSN, end LSN 초기화한다.
 **********************************************************************/
IDE_RC sdrMiniTrans::initialize( idvSQL*       aStatistics,
                                 sdrMtx*       aMtx,
                                 void*         aTrans,
                                 sdrMtxLogMode aLogMode,
                                 idBool        aUndoable,
                                 UInt          aDLogAttr ) // disk log의 attr, smrDef 참조
{
    UInt sState = 0;

    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMtxStack::initialize( &aMtx->mLatchStack )
              != IDE_SUCCESS );
    sState=1;

    IDE_TEST( sdrMtxStack::initialize( &aMtx->mXLatchPageStack )
              != IDE_SUCCESS );
    sState=2;

    aMtx->mStatSQL  = aStatistics;

    if( aLogMode == SDR_MTX_LOGGING )
    {

        // dynamic 버퍼를 초기화한다. 이 플래그가 설정되면 mtx에서
        // fix 하고 있는 page들에 대해서 unfix 시에 BCB에
        // LSN이 설정되고, 이 후에 page flush 과정에서
        // physical header에 mUpdateLSN을 설정한다.
        // 만약, mtx가 SDR_MTX_NOLOGGING 모드로 초기화가
        // 되었다면, 이 함수를 호출하지 않는다.
        IDE_TEST( smuDynArray::initialize( &aMtx->mLogBuffer )
                  != IDE_SUCCESS );
        sState=3;
    }

    SMU_LIST_INIT_BASE(&(aMtx->mPendingJob));

    aMtx->mTrans     = aTrans;

    SM_LSN_INIT( aMtx->mBeginLSN );
    SM_LSN_INIT( aMtx->mEndLSN );

    aMtx->mOpType    = 0;
    aMtx->mDataCount = 0;

    aMtx->mDLogAttr  = 0;

    aMtx->mDLogAttr  = aDLogAttr;

    SM_LSN_INIT(aMtx->mPPrevLSN);

    aMtx->mTableOID  = SM_NULL_OID;
    aMtx->mRefOffset = 0;

    // mtx가 접근하는 tablespace이다.
    aMtx->mAccSpaceID = 0;
    aMtx->mLogCnt     = 0;

    /* Flag를 초기화 한다. */
    aMtx->mFlag  = SDR_MTX_DEFAULT_INIT ;

    // TASK-2398 Log Compress
    //
    // 로그 압축여부 (기본값 : 압축 실시 )
    //   만약 로그를 압축하지 않도록 설정된 Tablespace에 대한
    //   로그를 기록하게되면 이 Flag를 ID_FALSE로 바꾸게 된다.

    /* Logging 여부 설정함 */
    aMtx->mFlag &= ~SDR_MTX_LOGGING_MASK;
    if( aLogMode == SDR_MTX_LOGGING )
    {
        aMtx->mFlag |= SDR_MTX_LOGGING_ON;
    }
    else
    {
        aMtx->mFlag |= SDR_MTX_LOGGING_OFF;
    }

    /* Mtx 동작 중 Property가 바뀔 수 있기 때문에, 미리 저장해둠. */
    aMtx->mFlag &= ~SDR_MTX_STARTUP_BUG_DETECTOR_MASK;
    if( smuProperty::getSmEnableStartupBugDetector() == 1 )
    {
        aMtx->mFlag |= SDR_MTX_STARTUP_BUG_DETECTOR_ON;
    }
    else
    {
        aMtx->mFlag |= SDR_MTX_STARTUP_BUG_DETECTOR_OFF;
    }

    /* Undo가능여부를 설정함 */
    aMtx->mFlag &= ~SDR_MTX_UNDOABLE_MASK;
    if( aUndoable == ID_TRUE )
    {
        aMtx->mFlag |= SDR_MTX_UNDOABLE_ON;
    }
    else
    {
        aMtx->mFlag |= SDR_MTX_UNDOABLE_OFF;
    }


    /* BUG-32579 The MiniTransaction commit should not be used in
     * exception handling area. */
    aMtx->mRemainLogRecSize = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 3:
            IDE_ASSERT( smuDynArray::destroy( &aMtx->mLogBuffer )
                        == IDE_SUCCESS );

        case 2:
            IDE_ASSERT( sdrMtxStack::destroy( &aMtx->mXLatchPageStack )
                        == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( sdrMtxStack::destroy( &aMtx->mLatchStack )
                        == IDE_SUCCESS );

        default:
            break;
    }

    return IDE_FAILURE;

}

/* Mtx StartInfo를 생성한다. */
void sdrMiniTrans::makeStartInfo( sdrMtx* aMtx,
                                  sdrMtxStartInfo * aStartInfo )
{
    IDE_ASSERT( aMtx != NULL );
    IDE_ASSERT( aStartInfo != NULL );

    aStartInfo->mTrans   = getTrans( aMtx );
    aStartInfo->mLogMode = getLogMode( aMtx );
    return;
}


/***********************************************************************
 * Description : sdrMiniTrans 해제
 **********************************************************************/
IDE_RC sdrMiniTrans::destroy( sdrMtx * aMtx )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    IDE_TEST( destroyPendingJob( aMtx ) 
              != IDE_SUCCESS );

    IDE_TEST( sdrMtxStack::destroy( &aMtx->mLatchStack )
              != IDE_SUCCESS );

    IDE_TEST( sdrMtxStack::destroy( &aMtx->mXLatchPageStack )
              != IDE_SUCCESS );

    if( getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        IDE_TEST( smuDynArray::destroy( &aMtx->mLogBuffer )
                  != IDE_SUCCESS );
    }

    aMtx->mFlag        = SDR_MTX_DEFAULT_DESTROY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}



/***********************************************************************
 * Description : mtx의 초기화 및 시작
 *
 * initialize redirect
 **********************************************************************/
IDE_RC sdrMiniTrans::begin( idvSQL *      aStatistics,
                            sdrMtx*       aMtx,
                            void*         aTrans,
                            sdrMtxLogMode aLogMode,
                            idBool        aUndoable,
                            UInt          aDLogAttr )
{

    IDE_TEST( initialize( aStatistics,
                          aMtx,
                          aTrans,
                          aLogMode,
                          aUndoable,
                          aDLogAttr )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : mtx의 초기화 및 시작
 *
 * 다른 mtx로부터 trans, logmode 정보를 얻어
 * 새로운 trans를 시작한다.
 * latch, stack을 복사하는 것이 아님.
 **********************************************************************/
IDE_RC sdrMiniTrans::begin(idvSQL*          aStatistics,
                           sdrMtx*          aMtx,
                           sdrMtxStartInfo* aStartInfo,
                           idBool           aUndoable,
                           UInt             aDLogAttr)
{


    IDE_TEST( initialize( aStatistics,
                          aMtx,
                          aStartInfo->mTrans,
                          aStartInfo->mLogMode,
                          aUndoable,
                          aDLogAttr)
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : mtx를 commit한다.
 *
 * mtx를 통해 트랜잭션 로그버퍼에 작성된 로그들을 로그파일에 write하고,
 * 모든 Item에 대해 latch item를 해제하여 fix했던 페이지등를
 * unfix하는 등의 작업을 한다. item의 메모리 해제는 메모리 생성한
 * 쪽의 책임이다.
 *
 * 1) BEGIN-LSN : mtx의 로그를 실제 로그에 쓸 때 시작 LSN
 * 이 LSN은 recovery 시에 최초의 LSN을 결정하기 위하여 사용된다.
 * releasePage 시에 이 값이 BCB의 firstModifiedLSN에 쓰여진다.
 *
 * 2) END-LSN : mtx의 로그를 실제 로그에 쓸 때 end LSN
 * 로그 tail의 마지막 LSN을 가르킨다. releasePage 시에 이 값이 BCB의
 * lastModifiedLSN에 쓰여진다. flush시 lastModifiedLSN까지의 로그를
 * 내릴것이다.
 *
 * redo를 수행할때는 노로깅으로 진행되지만
 * redo가 어디까지 수행됐는지를 페이지에 표시해야 한다.
 * endLSN인자는 만약 redo 수행중이라면 NULL이 아닌 값이 set되며,
 * 이를 flush시에 페이지에 write할 수 있도록 aMtx에 set해 주어야 한다.
 * 기본값은 NULL
 *
 *
 * - 2st. code design
 *   + if ( logging mode라면 )
 *     로그를 실제로 write 하고 Begin LSN, End LSN을 얻는다.
 *     경우에 따라 다음과 같은 로그관리자의 interface를 호출한다.
 *     : redo 로그와 undo record의 redo-undo 로그일때
 *       - smrLogMgr::writeDiskLogRec
 *     : MRDB와 연관있는 로깅이 필요한 NTA 로그일 경우
 *       - smrLogMgr::writeDiskNTALogRec
 *
 *   + while(모든 스택의 Item에 대해)
 *     한 Item을 pop한다.-> pop
 *     item을 해제한다. -> releaseLatchItem
 *
 *   + if( logging mode일때 )
 *        모든 자원을 해제한다.
 *     -> sdrMtxStack::destroy
 **********************************************************************/
IDE_RC sdrMiniTrans::commit( sdrMtx   * aMtx,
                             UInt       aContType,
                             smLSN    * aEndLSN,   /* in */
                             UInt       aRedoType )
{

    sdrMtxLatchItem  *sItem;
    sdrMtxStackInfo   sMtxDPStack;
    UInt              sWriteLogOption;
    SInt              sState   = 0;
    UInt              sAttrLog = 0;
    smLSN            *sPrvLSNPtr;


    /* BUG-32579 The MiniTransaction commit should not be used in
     * exception handling area.
     * 
     * 부분적으로 로그가 쓰여지면 안된다. */
    IDE_ASSERT( aMtx->mRemainLogRecSize == 0 );

    sState = 1;

    if( ( aMtx->mFlag & SDR_MTX_LOG_SHOULD_COMPRESS_MASK ) ==
        SDR_MTX_LOG_SHOULD_COMPRESS_ON )
    {
        sWriteLogOption = SDR_REQ_DISK_LOG_WRITE_OP_COMPRESS_TRUE;
    }
    else
    {
        sWriteLogOption = SDR_REQ_DISK_LOG_WRITE_OP_COMPRESS_FALSE;
    }

    if ( aEndLSN != NULL )
    {
        aMtx->mEndLSN = *aEndLSN;
    }

    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    /* BUG-19122: Restart Recovery LSN이 잘못 설정될 수 있습니다.
     *
     * XLatch를 잡은 페이지를 로깅하기전에 Dirty로 등록한다. */
    if( sdrMtxStack::isEmpty( &aMtx->mXLatchPageStack ) != ID_TRUE)
    {
        IDE_TEST( sdrMtxStack::initialize( &sMtxDPStack )
                  != IDE_SUCCESS );
        sState = 2;
    }

    while( sdrMtxStack::isEmpty( &aMtx->mXLatchPageStack ) != ID_TRUE )
    {
        sItem = sdrMtxStack::pop( &( aMtx->mXLatchPageStack ) );

        IDE_TEST( sdbBufferMgr::setDirtyPage( (UChar*)( sItem->mObject ),
                                              aMtx )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMtxStack::push( &sMtxDPStack, sItem ) != IDE_SUCCESS );
    }

    if( getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        sAttrLog = SM_DLOG_ATTR_WRITELOG_MASK & aMtx->mDLogAttr;

        switch( sAttrLog )
        {
            case SM_DLOG_ATTR_TRANS_LOGBUFF:

                IDE_ASSERT( aMtx->mTrans != NULL );

                /* BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
                 * Transaction log buffer에 기록된 로그가 있으면 flush 해야한다. */
                if( ((smxTrans*)aMtx->mTrans)->isLogWritten() == ID_TRUE )
                {
                    IDE_TEST( smrLogMgr::writeLog( aMtx->mStatSQL,
                                                   aMtx->mTrans,
                                                   ((smxTrans*)aMtx->mTrans)->getLogBuffer(),
                                                   NULL, // Prev LSN Ptr
                                                   &aMtx->mBeginLSN,
                                                   &aMtx->mEndLSN,
                                                   aMtx->mTableOID ) 
                               != IDE_SUCCESS);
                }
                break;
            case SM_DLOG_ATTR_MTX_LOGBUFF : /* xxx 함수로 빼야함 */

                sAttrLog = SM_DLOG_ATTR_LOGTYPE_MASK & aMtx->mDLogAttr;

                // BUG-25128 로깅 된것이 없어도 CLR 로그는 기록되어야 함
                if( (isLogWritten(aMtx) == ID_TRUE) || (sAttrLog == SM_DLOG_ATTR_CLR) )
                {
                    switch( sAttrLog )
                    {
                        case SM_DLOG_ATTR_NTA:
                            IDE_TEST( smLayerCallback::writeDiskNTALogRec( aMtx->mStatSQL,
                                                                           aMtx->mTrans,
                                                                           &aMtx->mLogBuffer,
                                                                           sWriteLogOption,
                                                                           aMtx->mOpType,
                                                                           &(aMtx->mPPrevLSN),
                                                                           aMtx->mAccSpaceID,
                                                                           aMtx->mData,
                                                                           aMtx->mDataCount,
                                                                           &(aMtx->mBeginLSN),
                                                                           &(aMtx->mEndLSN),
                                                                           aMtx->mTableOID )
                                      != IDE_SUCCESS );
                            break;

                        case SM_DLOG_ATTR_REF_NTA:
                            IDE_TEST( smLayerCallback::writeDiskRefNTALogRec( aMtx->mStatSQL,
                                                                              aMtx->mTrans,
                                                                              &aMtx->mLogBuffer,
                                                                              sWriteLogOption,
                                                                              aMtx->mOpType,
                                                                              aMtx->mRefOffset,
                                                                              &(aMtx->mPPrevLSN),
                                                                              aMtx->mAccSpaceID,
                                                                              &(aMtx->mBeginLSN),
                                                                              &(aMtx->mEndLSN),
                                                                              aMtx->mTableOID )
                                      != IDE_SUCCESS );
                            break;

                        case SM_DLOG_ATTR_CLR:
                            IDE_TEST( smLayerCallback::writeDiskCMPSLogRec( aMtx->mStatSQL,
                                                                            aMtx->mTrans,
                                                                            &aMtx->mLogBuffer,
                                                                            sWriteLogOption,
                                                                            &(aMtx->mPPrevLSN),
                                                                            &(aMtx->mBeginLSN),
                                                                            &(aMtx->mEndLSN),
                                                                            aMtx->mTableOID )
                                      != IDE_SUCCESS );
                            break;

                        case SM_DLOG_ATTR_REDOONLY:
                        case SM_DLOG_ATTR_UNDOABLE:
                            if ( SM_IS_LSN_INIT( aMtx->mPPrevLSN ) )
                            {
                                sPrvLSNPtr = NULL;
                            }
                            else
                            {
                                sPrvLSNPtr = &aMtx->mPPrevLSN;
                            }

                            IDE_TEST( smLayerCallback::writeDiskLogRec( aMtx->mStatSQL,
                                                                        aMtx->mTrans,
                                                                        sPrvLSNPtr,
                                                                        &aMtx->mLogBuffer,
                                                                        sWriteLogOption,
                                                                        aMtx->mDLogAttr,
                                                                        aContType,
                                                                        aMtx->mRefOffset,
                                                                        aMtx->mTableOID,
                                                                        aRedoType,
                                                                        &(aMtx->mBeginLSN),
                                                                        &(aMtx->mEndLSN) )
                                      != IDE_SUCCESS );
                            break;
                        case SM_DLOG_ATTR_DUMMY:
                            IDE_TEST( smLayerCallback::writeDiskDummyLogRec( aMtx->mStatSQL,
                                                                             &aMtx->mLogBuffer,
                                                                             sWriteLogOption,
                                                                             aContType,
                                                                             aRedoType,
                                                                             aMtx->mTableOID,
                                                                             &aMtx->mBeginLSN,
                                                                             &aMtx->mEndLSN )
                                      != IDE_SUCCESS );
                            break;
                        default:
                            IDE_ASSERT( 0 );
                            break;
                    }
                }
                break;

            default:
                break;
        }
    }

    /* PROJ-2162 RestartRiskReduction
     * Minitransaction으로 Log를 하나 쓴 후, 이 Log에 대해
     * Redo와 ServiceThread의 수행 결과가 동일한지 검사한다. */
    if( SDR_MTX_STARTUP_BUG_DETECTOR_IS_ON ( aMtx->mFlag ) )
    {
        if( ( getLogMode(aMtx) == SDR_MTX_LOGGING ) &&
            ( isLogWritten(aMtx) == ID_TRUE ) &&
            ( smuDynArray::getSize( &aMtx->mLogBuffer ) > 0 ) &&
            ( isNologgingPersistent(aMtx) == ID_FALSE ) ) /*IndexBUBuild*/
        {
            if( sState < 2 )
            {
                /* 상태가 2보다 작은 경우는, DirtyPage가 없다고 판단되어
                 * DPStack을 초기화 하지 않은 경우다.
                 * 하지만 내부 검증시, Dirty 안되어 있어도 Logging 있으면
                 * Dirtypage를 뒤지려고 하는 만큼, 여기서 '비었다'고
                 * 초기화를 해줘야 한다. */
                IDE_TEST( sdrMtxStack::initialize( &sMtxDPStack )
                          != IDE_SUCCESS );
                sState = 2;
            }
            else
            {
                /* nothing to do ... */
            }

            IDE_ASSERT( validateDRDBRedo( aMtx,
                                          &sMtxDPStack,
                                          &aMtx->mLogBuffer ) == IDE_SUCCESS );
        }
    }

    /* PROJ-2162 RestartRiskReduction
     * Consistency 하지 않으면, PageLSN을 수정하지 않는다. */
    if( ( smrRecoveryMgr::getConsistency() == ID_TRUE ) ||
        ( smuProperty::getCrashTolerance() == 2 ) )
    {
        /* DirtyPage의 PageLSN설정 */
        if ( sState == 2 )
        {
            while( sdrMtxStack::isEmpty( &sMtxDPStack ) != ID_TRUE )
            {
                sItem = sdrMtxStack::pop( &sMtxDPStack );

                sdbBufferMgr::setPageLSN( sItem->mObject, aMtx );
                if ( SDR_MTX_STARTUP_BUG_DETECTOR_IS_ON ( aMtx->mFlag ) )
                {
                    IDE_TEST( smuUtility::freeAlignedBuf( &(sItem->mPageCopy) )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    /* 잡힌 Latch를 푼다. */
    while( sdrMtxStack::isEmpty( &aMtx->mLatchStack ) != ID_TRUE )
    {
        sItem = sdrMtxStack::pop( &( aMtx->mLatchStack ) );
        IDE_TEST( releaseLatchItem( aMtx, sItem ) != IDE_SUCCESS );
    }

    /* BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다.
     * Mtx가 Commit될때 SpaceCache로 Free된 Extent를 반환합니다. */ 
    IDE_TEST( executePendingJob( aMtx,
                                 ID_TRUE)  //aDoCommitOp
              != IDE_SUCCESS );

    if( aEndLSN != NULL )
    {
        SM_SET_LSN(*aEndLSN,
                   aMtx->mEndLSN.mFileNo,
                   aMtx->mEndLSN.mOffset);
    }

    if ( sState == 2 )
    {
        sState = 1;
        IDE_TEST( sdrMtxStack::destroy( &sMtxDPStack )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( destroy(aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-31006 - Debug information must be recorded when an exception 
     *             occurs from mini transaction commit or rollback.
     * mini transaction commit/rollback 과정 중 실패시 디버깅 정보 출력하고
     * 서버를 죽인다. */

    /* dump sdrMtx */
    if ( sState != 0 )
    {
        dumpHex( aMtx );
        dump( aMtx, SDR_MTX_DUMP_DETAILED );
    }

    /* dump variable */
    ideLog::log( IDE_SERVER_0, "[ Dump variable ]" );
    ideLog::log( IDE_SERVER_0,
                 "ContType: %"ID_UINT32_FMT", "
                 "EndLSN: (%"ID_UINT32_FMT", %"ID_UINT32_FMT")",
                 "RedoType: %"ID_UINT32_FMT", "
                 "WriteLogOption: %"ID_UINT32_FMT", "
                 "State: %"ID_INT32_FMT", "
                 "AttrLog: %"ID_UINT32_FMT"",
                 aContType,
                 ( aEndLSN != NULL ) ? aEndLSN->mFileNo : 0,
                 ( aEndLSN != NULL ) ? aEndLSN->mOffset : 0,
                 aRedoType,
                 sWriteLogOption,
                 sState,
                 sAttrLog );

    if( sState == 2 )
    {
        /* dump sMtxDPStack */
        ideLog::log( IDE_SERVER_0, "[ Hex Dump MtxDPStack ]" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sMtxDPStack,
                        ID_SIZEOF(sdrMtxStackInfo) );

        IDE_ASSERT( sdrMtxStack::destroy( &sMtxDPStack )
                    == IDE_SUCCESS );
    }

    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : NOLOGGING Attribute 설정
 * NOLOGGING mini-transaction에 Nologging Persistent attribute를 설정
 * Logging은 안하지만 Persistent한 페이지임 (TempPage가 아닌 페이지).
 **********************************************************************/
void sdrMiniTrans::setNologgingPersistent( sdrMtx* aMtx )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    aMtx->mFlag &= ~SDR_MTX_NOLOGGING_ATTR_PERSISTENT_MASK;
    aMtx->mFlag |=  SDR_MTX_NOLOGGING_ATTR_PERSISTENT_ON;

    return;

}

/***********************************************************************
 * Description : NTA 로깅
 **********************************************************************/
void sdrMiniTrans::setNTA( sdrMtx   * aMtx,
                           scSpaceID  aSpaceID,
                           UInt       aOpType,
                           smLSN*     aPPrevLSN,
                           ULong    * aArrData,
                           UInt       aDataCount )
{
    UInt   sLoop;

    aMtx->mAccSpaceID = aSpaceID;

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_ASSERT( (aMtx->mDLogAttr & SM_DLOG_ATTR_LOGTYPE_MASK)
                 == SM_DLOG_ATTR_REDOONLY );
    IDE_ASSERT( aDataCount <= SM_DISK_NTALOG_DATA_COUNT );

    aMtx->mDLogAttr  |= SM_DLOG_ATTR_NTA;
    aMtx->mOpType     = aOpType;
    aMtx->mDataCount  = aDataCount;

    for ( sLoop = 0; sLoop < aDataCount; sLoop++ )
    {
        aMtx->mData[ sLoop ] = aArrData [ sLoop ];
    }

    SM_SET_LSN(aMtx->mPPrevLSN,
               aPPrevLSN->mFileNo,
               aPPrevLSN->mOffset);
}

/***********************************************************************
 * Description : Index NTA 로깅
 **********************************************************************/
void sdrMiniTrans::setRefNTA( sdrMtx   * aMtx,
                              scSpaceID  aSpaceID,
                              UInt       aOpType,
                              smLSN*     aPPrevLSN )
{
    IDE_ASSERT( aMtx->mRefOffset != 0 );
    
    aMtx->mAccSpaceID = aSpaceID;

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_ASSERT( (aMtx->mDLogAttr & SM_DLOG_ATTR_LOGTYPE_MASK)
                 == SM_DLOG_ATTR_REDOONLY );

    aMtx->mDLogAttr  |= SM_DLOG_ATTR_REF_NTA;
    aMtx->mOpType     = aOpType;

    SM_SET_LSN(aMtx->mPPrevLSN,
               aPPrevLSN->mFileNo,
               aPPrevLSN->mOffset);
}

void sdrMiniTrans::setNullNTA( sdrMtx   * aMtx,
                               scSpaceID  aSpaceID,
                               smLSN*     aPPrevLSN )
{
    aMtx->mAccSpaceID = aSpaceID;

    SM_SET_LSN(aMtx->mPPrevLSN,
               aPPrevLSN->mFileNo,
               aPPrevLSN->mOffset);
}

/***********************************************************************
 * Description : 특정 연산에 대한 CLR 로그 설정
 **********************************************************************/
void sdrMiniTrans::setCLR( sdrMtx* aMtx,
                           smLSN*  aPPrevLSN )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( (aMtx->mDLogAttr & SM_DLOG_ATTR_LOGTYPE_MASK)
                 == SM_DLOG_ATTR_REDOONLY );

    aMtx->mDLogAttr |= SM_DLOG_ATTR_CLR;

    SM_SET_LSN(aMtx->mPPrevLSN,
               aPPrevLSN->mFileNo,
               aPPrevLSN->mOffset);

    return;
}

/***********************************************************************
 * Description : DML관련 undo/redo 로그 위치 기록
 * ref offset은 하나의 smrDiskLog에 기록된 여러개의 disk 로그들 중
 * replication sender가 참고하여 로그를 판독하거나 transaction undo시에
 * 판독하는 DML관련 redo/undo 로그가 기록된 위치를 의미한다.
 **********************************************************************/
void sdrMiniTrans::setRefOffset( sdrMtx* aMtx,
                                 smOID   aTableOID )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_ASSERT( ( aMtx->mFlag & SDR_MTX_REFOFFSET_MASK ) 
                == SDR_MTX_REFOFFSET_OFF );

    aMtx->mRefOffset      = smuDynArray::getSize(&aMtx->mLogBuffer);
    aMtx->mTableOID       = aTableOID;
    aMtx->mFlag          |= SDR_MTX_REFOFFSET_ON;

    return;
}

/***********************************************************************
 *
 * Description :
 *
 * minitrans의 begin lsn을 return
 *
 * Implementation :
 *
 **********************************************************************/
void* sdrMiniTrans::getTrans( sdrMtx     * aMtx )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    return aMtx->mTrans;
}


/***********************************************************************
 *
 * Description :
 *
 * minitrans의 begin lsn을 return
 *
 * Implementation :
 *
 **********************************************************************/
void* sdrMiniTrans::getMtxTrans( void * aMtx )
{
    if ( aMtx != NULL )
    {
        IDE_DASSERT( validate((sdrMtx*)aMtx) == ID_TRUE );

        return ((sdrMtx*)aMtx)->mTrans;
    }

    return (void*)NULL;
}


/***********************************************************************
 * Description : mtx 로깅 모드를 얻는다.
 **********************************************************************/
sdrMtxLogMode sdrMiniTrans::getLogMode(sdrMtx*        aMtx)
{
    sdrMtxLogMode  sRet = SDR_MTX_LOGGING;

    if( ( aMtx->mFlag & SDR_MTX_LOGGING_MASK ) == SDR_MTX_LOGGING_ON )
    {
        sRet = SDR_MTX_LOGGING;
    }
    else
    {
        sRet = SDR_MTX_NOLOGGING;
    }

    return sRet;
}

/***********************************************************************
 * Description : disk log를 위한 attribute를 얻는다..
 **********************************************************************/
UInt sdrMiniTrans::getDLogAttr( sdrMtx * aMtx )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    return aMtx->mDLogAttr;
}

/***********************************************************************
 * Description : 로그 레코드를 초기화한다.
 *
 * 로그 타입과 rid를 기록하고, 기록후에 로그 버퍼의 offset을
 * 반환한다.
 *
 * Implementation :
 *
 * log hdr를 구성한다.
 * log hdr를 write한다.
 *
 **********************************************************************/
IDE_RC sdrMiniTrans::initLogRec( sdrMtx*        aMtx,
                                 scGRID         aWriteGRID,
                                 UInt           aLength,
                                 sdrLogType     aType )
{
    sdrLogHdr   sLogHdr;
    scSpaceID   sSpaceID;

    /* BUG-32579 The MiniTransaction commit should not be used in
     * exception handling area. 
     *
     * 부분적으로 로그가 쓰여지면 안된다. */

    IDE_ASSERT( aMtx->mRemainLogRecSize == 0 );

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( getLogMode(aMtx) == SDR_MTX_LOGGING );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace(SC_MAKE_SPACE(aWriteGRID))
                 == ID_TRUE );

    sLogHdr.mGRID   = aWriteGRID;
    sLogHdr.mLength = aLength;
    sLogHdr.mType   = aType;

    sSpaceID = SC_MAKE_SPACE(aWriteGRID);

    if ( aMtx->mLogCnt == 0 )
    {
        aMtx->mAccSpaceID = sSpaceID;
    }
    else
    {
        // undo 가 확실하면 굳이 확인하지 말자. 성능 문제도 있고..
        if ( sctTableSpaceMgr::isUndoTableSpace( sSpaceID ) == ID_FALSE )
        {
            IDE_ASSERT( sSpaceID == aMtx->mAccSpaceID );
            IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace(sSpaceID) == ID_TRUE ); 
        }
    }

    // TASK-2398 Log Compression
    // Tablespace에 Log Compression을 하지 않도록 설정된 경우
    // 로그 압축을 하지 않도록 설정
    IDE_TEST( checkTBSLogCompAttr(aMtx, sSpaceID) != IDE_SUCCESS );

    aMtx->mLogCnt++;

    aMtx->mRemainLogRecSize = ID_SIZEOF(sdrLogHdr) + aLength;
        
    IDE_TEST( write( aMtx, &sLogHdr, (UInt)ID_SIZEOF(sdrLogHdr) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Tablespace에 Log Compression을 하지 않도록 설정된 경우
   로그 압축을 하지 않도록 설정

   [IN] aMtx     - Mini Transaction
   [IN] aSpaceID - Mini Transaction이 로깅하려는 데이터가 속한
                   Tablespace의 ID
*/
IDE_RC sdrMiniTrans::checkTBSLogCompAttr( sdrMtx*      aMtx,
                                          scSpaceID    aSpaceID )
{
    idBool     sShouldCompress = ID_TRUE;

    IDE_DASSERT( aMtx != NULL );

    // 압축을 하도록 설정되어 있는 경우에만 체크
    if( ( aMtx->mFlag & SDR_MTX_LOG_SHOULD_COMPRESS_MASK ) ==
        SDR_MTX_LOG_SHOULD_COMPRESS_ON )
    {
        // 첫번째 접근한 Tablespace이거나
        // 첫번째 접근한 Tablespace와 다른 Tablespace라면
        // Tablespace의 로그 압축 여부 체크
        if ( (aMtx->mLogCnt == 0) ||
             (aMtx->mAccSpaceID != aSpaceID ) )
        {
            IDE_TEST( sctTableSpaceMgr::getSpaceLogCompFlag(
                          aSpaceID,
                          & sShouldCompress )
                      != IDE_SUCCESS );

            aMtx->mFlag &= ~SDR_MTX_LOG_SHOULD_COMPRESS_MASK;
            if( sShouldCompress == ID_TRUE )
            {
                aMtx->mFlag |= SDR_MTX_LOG_SHOULD_COMPRESS_ON;
            }
            else
            {
                aMtx->mFlag |= SDR_MTX_LOG_SHOULD_COMPRESS_OFF;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * RID 버전
 * multiple column의 redo log를 write할 때 사용된다.
 **********************************************************************/
IDE_RC sdrMiniTrans::writeLogRec( sdrMtx*      aMtx,
                                  scGRID       aWriteGRID,
                                  void*        aValue,
                                  UInt         aLength,
                                  sdrLogType   aLogType )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aWriteGRID) );

    if( getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        IDE_TEST( initLogRec( aMtx,
                              aWriteGRID,
                              aLength,
                              aLogType )
                  != IDE_SUCCESS );


        if( aValue != NULL )
        {
            IDE_DASSERT( aLength > 0 );

            IDE_TEST( write( aMtx, aValue, aLength )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page 변경부분에 대한 로그만 기록한다.
 *
 * 로그 header 및 body를 동시에 mtx 로그 버퍼에 write
 * write 시에는 해당 버퍼 프레임에 x-latch가 mtx stack에
 * 있어야 한다.
 *
 * - 2nd. code design
 *   + 로그 타입과 RID를 초기화한다.  -> sdrMiniTrans::initLogRec
 *   + value를 log buffer에 write한다 -> sdrMiniTrans::write
 **********************************************************************/
IDE_RC sdrMiniTrans::writeLogRec( sdrMtx*      aMtx,
                                  UChar*       aWritePtr,
                                  void*        aValue,
                                  UInt         aLength,
                                  sdrLogType   aLogType )
{
    idBool    sIsTablePageType = ID_FALSE;
    scGRID    sGRID;
    sdRID     sRID;
    scSpaceID sSpaceID;

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( aWritePtr != NULL );

    if( getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        if ( (aMtx->mDLogAttr & SM_DLOG_ATTR_EXCEPT_MASK)
             == SM_DLOG_ATTR_EXCEPT_INSERT_APPEND_PAGE )
        {
            sIsTablePageType = smLayerCallback::isTablePageType( aWritePtr );

            if( sIsTablePageType == ID_TRUE )
            {
                //----------------------------
                // PROJ-1566
                // insert가 append 방식으로 수행되는 경우,
                // consistent log를 제외하고 page에 대하여 logging 하지 않음
                // page flush 할 때, page 전체에 대하여 logging 하도록 되어있음
                //----------------------------

                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                            "write page log in DPath-Insert "
                            "aLogType %"ID_UINT32_FMT" sIsTablePageType %"ID_UINT32_FMT"",
                            aLogType,  sIsTablePageType );

                IDE_ASSERT( 0 );
            }
        }
        // table page type이 아니면 ( extent or segment ),
        // logging  수행

        sRID     = smLayerCallback::getRIDFromPagePtr( aWritePtr );
        sSpaceID = smLayerCallback::getSpaceID( smLayerCallback::getPageStartPtr( aWritePtr ) );

        SC_MAKE_GRID( sGRID,
                      sSpaceID,
                      SD_MAKE_PID(sRID),
                      SD_MAKE_OFFSET(sRID));

        IDE_TEST( writeLogRec(aMtx,
                              sGRID,
                              aValue,
                              aLength,
                              aLogType )
                  != IDE_SUCCESS );
    }
    else
    {
        // SDR_MTX_NOLOGGING
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : page에 대해 변경과 함께 1, 2, 4, 8 byte 로그도 기록한다.
 *
 * logtype은 각 byte에 맞는 숫자로 넘어온다. 즉, logtype이 value의
 * 크기로도 사용된다. SDR_1BYTE -> 1, SDR_2BYTE->2, SDR_4BYTE->4,
 * SDR_8BYTE->8 이런 식으로 정의된다.
 *
 * 주의 : 이 함수는 page를 같이 변경한다.
 *
 * - 2st. code design
 *   + !!) 특정 page의 변경 dest 포인터에 값을 로그타입에 따라
 *     특정 길이만큼 memcpy한다.
 *   + type과 RID를 설정한다      -> sdrMiniTrans::initLogRec
 *   + value를 write한다.         -> sdrMiniTrans::write
 *   + page에 value를 write한다.  -> idlOS::memcpy
 **********************************************************************/
IDE_RC sdrMiniTrans::writeNBytes( void*           aMtx,
                                  UChar*          aWritePtr,
                                  void*           aValue,
                                  UInt            aLogType )
{
    idBool     sIsTablePageType = ID_FALSE;
    UInt       sLength = aLogType;
    sdrMtx    *sMtx = (sdrMtx *)aMtx;
    scGRID     sGRID;
    sdRID      sRID;
    scSpaceID  sSpaceID;


    IDE_DASSERT( validate(sMtx ) == ID_TRUE );
    IDE_DASSERT( aWritePtr != NULL );
    IDE_DASSERT( aValue != NULL );
    IDE_DASSERT( aLogType == SDR_SDP_1BYTE ||
                 aLogType == SDR_SDP_2BYTE ||
                 aLogType == SDR_SDP_4BYTE ||
                 aLogType == SDR_SDP_8BYTE );

    if( getLogMode(sMtx) == SDR_MTX_LOGGING )
    {
        if ( (sMtx->mDLogAttr & SM_DLOG_ATTR_EXCEPT_MASK)
             == SM_DLOG_ATTR_EXCEPT_INSERT_APPEND_PAGE )
        {
             sIsTablePageType = smLayerCallback::isTablePageType( aWritePtr );
        }
        else
        {
             sIsTablePageType = ID_FALSE;
        }

        if ( sIsTablePageType == ID_FALSE )
        {
            sRID     = smLayerCallback::getRIDFromPagePtr( aWritePtr );
            sSpaceID = smLayerCallback::getSpaceID( smLayerCallback::getPageStartPtr( aWritePtr ) );

            SC_MAKE_GRID( sGRID,
                          sSpaceID,
                          SD_MAKE_PID(sRID),
                          SD_MAKE_OFFSET(sRID));

            IDE_TEST( initLogRec(sMtx,
                                 sGRID,
                                 sLength,
                                 (sdrLogType)aLogType )
                  != IDE_SUCCESS );

            IDE_TEST( write(sMtx,
                            aValue,
                            sLength) != IDE_SUCCESS );
        }
        else
        {
            //----------------------------
            // PROJ-1566
            // insert가 append 방식으로 수행되는 경우,
            // page에 대하여 logging 하지 않음
            // page flush 할 때, page 전체에 대하여 logging 하도록 되어있음
            //----------------------------
        }
    }
    else
    {
        // SDR_MTX_NOLOGGING
    }
    
    //BUG-23471 : overlap in memcpy
    if(aWritePtr != aValue)
    {
        idlOS::memcpy( aWritePtr, aValue, sLength );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : log에 write한다.
 *
 * mtx의 dynamic 로그 버퍼에 특정 길이의 value를 기록한다.
 *
 * - 2nd. code design
 *   + 특정 크기의 value를 mtx 로그 버퍼에 기록한다. -> smuDynBuffer::store
 **********************************************************************/
IDE_RC sdrMiniTrans::write(sdrMtx*        aMtx,
                           void*          aValue,
                           UInt           aLength)
{

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( aValue != NULL );

#if defined(DEBUG)
    /* PROJ-2162 RestartRiskReduction
     * __SM_MTX_ROLLBACK_TEST 값이 설정되면 (0이 아니면) 이 값이
     * MtxLogWrite시마다 증가한다. 그리고 위 Property값에 도달하면
     * 강제로 예외처리한다. */
    if( smuProperty::getSmMtxRollbackTest() > 0 )
    {
        mMtxRollbackTestSeq ++;
        if( mMtxRollbackTestSeq >= smuProperty::getSmMtxRollbackTest() )
        {
            mMtxRollbackTestSeq = 0;
            smuProperty::resetSmMtxRollbackTest();

            ideLog::logCallStack( IDE_SM_0 );
            IDE_RAISE( ABORT_INTERNAL_ERROR );
        }
        else
        {
            /* nothing to do... */
        }
    }
#endif

    if( getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        /* 초기 설정한 크기만큼만 Log를 내려야 함 */
        if( aMtx->mRemainLogRecSize < aLength )
        {
            ideLog::log( IDE_DUMP_0,
                        "ESTIMATED SIZE : %"ID_UINT32_FMT"\n"
                        "VALUE          : %"ID_UINT32_FMT"",
                        aMtx->mRemainLogRecSize,
                        aLength );
            dumpHex( aMtx );
            dump( aMtx, SDR_MTX_DUMP_DETAILED );
            IDE_ASSERT( 0 );
        }
        aMtx->mRemainLogRecSize -= aLength;

        /* 메모리 부족 외에는 마땅한 이유가 없으며,
         * 메모리 부족에 의한 예외도 일어날 가능성이 거의 없다. */
        IDE_ERROR( smuDynArray::store( &aMtx->mLogBuffer,
                                       aValue,
                                       aLength )
                   == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
#if defined(DEBUG)
    IDE_EXCEPTION( ABORT_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );
    }
#endif
    IDE_EXCEPTION_END;

    if( ( aMtx->mFlag & SDR_MTX_UNDOABLE_MASK ) == SDR_MTX_UNDOABLE_OFF )
    {
        ideLog::logMem( IDE_DUMP_0, 
                        (UChar*)aValue, 
                        aLength,
                        "LENGTH : %"ID_UINT32_FMT"",
                        aLength );
        dumpHex( aMtx );
        dump( aMtx, SDR_MTX_DUMP_DETAILED );

        /* PROJ-2162 RestartRiskReduction
         * Undo 불가능한 상태이므로 비정상종료 시킨다. */
        IDE_ASSERT( 0 );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : mtx stack에 latch item(object와 래치 모드)를 넣는다.
 *
 * - 1st. code design
 *   + latch stack에 latch item을 넣는다. -> sdrMtxStack::push
 **********************************************************************/
IDE_RC sdrMiniTrans::push(void*         aMtx,
                          void*         aObject,
                          UInt          aLatchMode)
{
    sdrMtxLatchItem sPushItem;
    sdrMtx *sMtx = (sdrMtx *)aMtx;
    sdrMtxLatchMode sLatchMode = (sdrMtxLatchMode)aLatchMode;

    IDE_DASSERT( validate(sMtx) == ID_TRUE );
    IDE_DASSERT( aObject != NULL );
    IDE_DASSERT( validate(sMtx) == ID_TRUE );

    sPushItem.mObject    = aObject;
    sPushItem.mLatchMode = sLatchMode;

    IDE_TEST( sdrMtxStack::push( &sMtx->mLatchStack,
                                 &sPushItem )
              != IDE_SUCCESS );

    if( aLatchMode == SDR_MTX_PAGE_X )
    {
        IDE_TEST( setDirtyPage( aMtx,
                                ((sdbBCB*)aObject)->mFrame )
                  != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC sdrMiniTrans::pushPage(void*         aMtx,
                              void*         aObject,
                              UInt          aLatchMode)
{
    return push( aMtx,
                 sdbBufferMgr::getBCBFromPagePtr( (UChar*)aObject ),
                 aLatchMode );
}

IDE_RC sdrMiniTrans::setDirtyPage( void*    aMtx,
                                   UChar*   aPagePtr )
{
    sdrMtxLatchItem sPushItem;
    sdrMtx *sMtx = (sdrMtx *)aMtx;

    sPushItem.mObject = sdbBufferMgr::getBCBFromPagePtr( aPagePtr );
    sPushItem.mLatchMode = SDR_MTX_PAGE_X;

    /* 중복제거 */
    if ( existObject( &sMtx->mXLatchPageStack,
                      sPushItem.mObject, 
                      SDR_MTX_BCB ) == NULL )
    {
        /* Property 켜있을때에만, 복사본을 유지함 */
        if ( SDR_MTX_STARTUP_BUG_DETECTOR_IS_ON( sMtx->mFlag ) )
        {
            IDE_TEST( smuUtility::allocAlignedBuf( IDU_MEM_SM_SDR,
                                                   SD_PAGE_SIZE,
                                                   SD_PAGE_SIZE,
                                                   &sPushItem.mPageCopy )
                      != IDE_SUCCESS );

            idlOS::memcpy( sPushItem.mPageCopy.mAlignPtr,
                           aPagePtr,
                           SD_PAGE_SIZE );
        }

        IDE_TEST( sdrMtxStack::push( &sMtx->mXLatchPageStack,
                                     &sPushItem )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/****************************************************************************
 * Description : Mini-transaction의 Commit(또는 Rollback)시에 진행될 일들
 *               을 등록한다.
 *
 * BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다. 
 *
 * aMtx         [in] PendingJob을 달 대상 Mini-transaction 
 * aIsCommitJob [in] Commit시 수행될 일인가?(T), Rollback시 수행될 일인가(F)
 * aFreeData    [in] Destroy시 data를 Free해줄 것인가?
 * aPendingFunc [in] 이후에 실제로 일을 수행할 함수 포인터
 * aData        [in] 일을 수행할 aPendingFunc에게 전달되는 데이터.
 ***************************************************************************/
IDE_RC sdrMiniTrans::addPendingJob( void              * aMtx,
                                    idBool              aIsCommitJob,
                                    sdrMtxPendingFunc   aPendingFunc,
                                    void              * aData )
{
    sdrMtx           * sMtx = (sdrMtx *)aMtx;
    sdrMtxPendingJob * sPendingJob; 
    smuList          * sNewNode;
    UInt               sState = 0;

    IDE_DASSERT( sMtx         != NULL );
    IDE_DASSERT( aPendingFunc != NULL );

    /* TC/Server/LimitEnv/sm/sdr/sdrMiniTrans_addPendingJob_calloc1.sql */
    IDU_FIT_POINT_RAISE( "sdrMiniTrans::addPendingJob::calloc1",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SDR, 
                                       1, 
                                       ID_SIZEOF(smuList),
                                       (void**)&sNewNode ) 
                    != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    SMU_LIST_INIT_BASE(sNewNode);

    /* TC/Server/LimitEnv/sm/sdr/sdrMiniTrans_addPendingJob_calloc2.sql */
    IDU_FIT_POINT_RAISE( "sdrMiniTrans::addPendingJob::calloc2",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SDR,
                                       1,
                                       ID_SIZEOF(sdrMtxPendingJob),
                                       (void**)&sPendingJob) 
                    != IDE_SUCCESS,
                    insufficient_memory );
    sState = 2;

    sPendingJob->mIsCommitJob = aIsCommitJob;
    sPendingJob->mPendingFunc = aPendingFunc;
    sPendingJob->mData        = aData;

    sNewNode->mData = sPendingJob;
    
    SMU_LIST_ADD_LAST(&(sMtx->mPendingJob), sNewNode );
    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 2 :
            IDE_ASSERT( iduMemMgr::free(sPendingJob) == IDE_SUCCESS );
        case 1 :
            IDE_ASSERT( iduMemMgr::free(sNewNode) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : Mini-transaction의 Commit(또는 Rollback)시에 진행될 일들
 *               을, 등록시에 넣은 가상 함수를 호출하여 수행한다.
 *
 * BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다. 
 *
 * aMtx         [in] PendingJob이 달린 Mini-transaction 
 * aDoCommitJob [in] Commit시점인가? (T), Rollback시점인가?(F)
 ***************************************************************************/
IDE_RC sdrMiniTrans::executePendingJob(void   * aMtx,
                                       idBool   aDoCommitJob)
{
    sdrMtx           * sMtx = (sdrMtx *)aMtx;
    smuList          * sOpNode;
    smuList          * sBaseNode;
    sdrMtxPendingJob * sPendingJob; 

    sBaseNode = &(sMtx->mPendingJob);

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = SMU_LIST_GET_NEXT(sOpNode) )
    {
        sPendingJob = (sdrMtxPendingJob*)sOpNode->mData;

        if( sPendingJob->mIsCommitJob == aDoCommitJob )
        {
            IDE_TEST( sPendingJob->mPendingFunc( sPendingJob->mData )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : PendingJob 리스트 및 구조체, Data를 전부 삭제한다.
 *
 * BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다. 
 *
 * aMtx         [in] PendingJob을 지울 대상 Mini-transaction 
 ***************************************************************************/
IDE_RC sdrMiniTrans::destroyPendingJob( void * aMtx )
{
    sdrMtx           * sMtx = (sdrMtx *)aMtx;
    smuList          * sOpNode;
    smuList          * sBaseNode;
    smuList          * sNextNode;
    sdrMtxPendingJob * sPendingJob;
    UInt               sState = 0;

    sBaseNode = &(sMtx->mPendingJob);

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = sNextNode )
    {
        sNextNode = SMU_LIST_GET_NEXT(sOpNode);
        SMU_LIST_DELETE(sOpNode);

        sPendingJob = (sdrMtxPendingJob*)sOpNode->mData;

        sState = 1;
        IDE_TEST(iduMemMgr::free(sPendingJob) != IDE_SUCCESS);

        sState = 2;
        IDE_TEST(iduMemMgr::free(sOpNode) != IDE_SUCCESS);
        sState = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUGBUG
     * 이 경우로 온 경우는 PendingJob List가 깨졌을 경우이다.
     *
     * 제대로 리스트가 구축되어 있는데 이미 누군가가 Free해서 발생한 경우일
     * 수도 있고,
     * 리스트가 깨져있어서, 엉뚱한 객체를 Free하는 것일 수도 있다.
     *
     * 따라서 계속 리스트를 진행하면서 Free를 진행할 수 없는 상황이며, 이에
     * 따라 메모리 Leak이 발생할 수 있지만, 딱히 도리가 없다. */

    switch( sState )
    {
    case 0:
        break;
    case 1:
        ideLog::log( IDE_SERVER_0, "Invalid pendingJob structure -%"ID_POINTER_FMT"\n\
     mIsCommitJob -%"ID_UINT32_FMT" \n\
     mPendingFunc -%"ID_UINT32_FMT" \n\
     mData        -%"ID_UINT32_FMT" \n ",
                     sPendingJob,
                     sPendingJob->mIsCommitJob ,
                     sPendingJob->mPendingFunc ,
                     sPendingJob->mData );
        break;
    case 2:
        ideLog::log( IDE_SERVER_0, "Invalid pendingJob list -%"ID_POINTER_FMT" "
                     "prev-%"ID_POINTER_FMT" next-%"ID_POINTER_FMT"\n",
                     sOpNode,
                     SMU_LIST_GET_PREV(sOpNode),
                     SMU_LIST_GET_NEXT(sOpNode) );
        break;

    }
    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}

idBool sdrMiniTrans::needMtxRollback( sdrMtx * aMtx )
{
    idBool sRet = ID_FALSE;

    if ( ( getLogMode(aMtx) == SDR_MTX_LOGGING )     &&
         ( isNologgingPersistent(aMtx) == ID_FALSE ) &&  /*IndexBUBuild*/
         ( ( aMtx->mFlag & SDR_MTX_IGNORE_UNDO_MASK ) 
           == SDR_MTX_IGNORE_UNDO_OFF ) )
    {
        /* 기록된 Log가 있거나,
         * LogRec을 쓰려고 준비했거나,
         * XLatch를 잡은 페이지가 있으면 Rollback이 필요합니다. */
        if ( ( isLogWritten(aMtx) == ID_TRUE ) ||
             ( aMtx->mRemainLogRecSize > 0 )   || 
             ( sdrMtxStack::isEmpty(&aMtx->mXLatchPageStack) == ID_FALSE ) )
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }
    else
    {
        /* Nologging이고 Ignroe모드면 의미 없습니다. */
        sRet = ID_FALSE;
    }

    return sRet;
}

/***********************************************************************
 * Description :
 *
 * 모든 undo log buffer의 내용을 반영한다.
 * 모든 latch를 해제한다.
 * 이 함수는 begin - commit 사이의 abort로 인하여
 * exception 이 발생했을 때 호출된다.
 *
 * 노로깅일때는 아예 이 함수를 들어올 수 없다.
 * 현재 노로깅일 경우에는 abort가 일어나서는 안된다.
 *
 * Implementation :
 *
 * undo Log Buffer
 *
 * while( 모든 stack item에 대해 )
 *     pop
 *     release
 *
 *
 *
 **********************************************************************/
IDE_RC sdrMiniTrans::rollback( sdrMtx * aMtx )
{
    idvSQL                  sDummyStatistics;
    UInt                    sLogSize        = 0;
    SInt                    sParseOffset    = 0;
    sdrMtxLatchItem        *sItem;
    sdbBCB                 *sTargetBCB;
    idBool                  sSuccess;
    UInt                    sState          = 0;
    ULong                   sSPIDArray[ SDR_MAX_MTX_STACK_SIZE ];
    UInt                    sSPIDCount = 0;
    smuAlignBuf             sPagePtr;
    sdbBCB                * sBCB;
    UInt                    i;

    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    IDE_TEST_CONT( ( aMtx->mFlag & SDR_MTX_INITIALIZED_MASK ) ==
                   SDR_MTX_INITIALIZED_OFF, SKIP );
    sState = 1;

    if ( needMtxRollback( aMtx ) == ID_TRUE )
    {
        if( ( ( aMtx->mFlag & SDR_MTX_UNDOABLE_MASK ) == SDR_MTX_UNDOABLE_OFF ) &&
            ( ( smrRecoveryMgr::isRestart() == ID_FALSE ) ) )
        {
            dumpHex( aMtx );
            dump( aMtx, SDR_MTX_DUMP_DETAILED );
            IDE_ASSERT( 0 );
        }
        else
        {
            /* CanUndo가 False라도, Restart 과정에서는 무시한다.
             * Restart 과정에서의 Rollback이 실패하면
             * 방법이 없으니까. */
        }

        /* OnlineRedo를 통해 Undo함 */
        idlOS::memset( &sDummyStatistics, 0, ID_SIZEOF( idvSQL ) );

        while( sdrMtxStack::isEmpty(&aMtx->mXLatchPageStack) != ID_TRUE )
        {
            sItem = sdrMtxStack::pop( &( aMtx->mXLatchPageStack ) );

            sTargetBCB = (sdbBCB*)sItem->mObject ;

#if defined(DEBUG)
            /* MtxRollback은 거의 발생하지 않는 일이다. 따라서 발생시
             * 어떠한 이상이 있었는지 기록하기 위해 정보를 남긴다. */
            ideLog::logCallStack( IDE_SM_0 );
            ideLog::log( IDE_SM_0,
                         "-----------------------------------------------"
                         "sdrMiniTrans::rollback "
                         "SPACEID:%"ID_UINT32_FMT""
                         ",PAGEID:%"ID_UINT32_FMT""
                         "-----------------------------------------------",
                         sTargetBCB->mSpaceID,
                         sTargetBCB->mPageID );
#endif
            IDE_TEST( smuUtility::allocAlignedBuf( IDU_MEM_SM_SDR,
                                                   SD_PAGE_SIZE,
                                                   SD_PAGE_SIZE,
                                                   &sPagePtr )
                      != IDE_SUCCESS );

            sTargetBCB->lockBCBMutex( &sDummyStatistics );
            IDE_TEST( sdrRedoMgr::generatePageUsingOnlineRedo( 
                                                    &sDummyStatistics,
                                                    sTargetBCB->mSpaceID,
                                                    sTargetBCB->mPageID,
                                                    ID_TRUE, // aReadFromDisk
                                                    sPagePtr.mAlignPtr,
                                                    &sSuccess )
                      != IDE_SUCCESS );
            idlOS::memcpy( sTargetBCB->mFrame,
                           sPagePtr.mAlignPtr,
                           SD_PAGE_SIZE );
            IDE_TEST( smuUtility::freeAlignedBuf( &sPagePtr )
                      != IDE_SUCCESS );

            /* Frame에 BCB 정보를 설정함 */
            sdbBufferMgr::getPool()->setFrameInfoAfterRecoverPage( 
                                                        sTargetBCB,
                                                        ID_TRUE );  // check to online tablespace
            sTargetBCB->unlockBCBMutex();
        }
    }
    else
    {
        /* Page에 대한 Rollback을 하지 않을 경우, 정말 하지 않아도
         * 되는지 검증함.
         * 단 이때 Logging에 Persistent, 즉 Logging이 의미있는 경
         * 우여야 한다.*/
        if ( ( SDR_MTX_STARTUP_BUG_DETECTOR_IS_ON (aMtx->mFlag) ) &&
             ( getLogMode(aMtx) == SDR_MTX_LOGGING )              &&
             ( isNologgingPersistent(aMtx) == ID_FALSE ) )
        {
            idlOS::memset( &sDummyStatistics, 0, ID_SIZEOF( idvSQL ) );
            for( i = 0 ; i < aMtx->mXLatchPageStack.mCurrSize ; i ++ )
            {
                sBCB = (sdbBCB*)(aMtx->mXLatchPageStack.mArray[ i ].mObject);

                IDE_TEST( validateDRDBRedoInternal( &sDummyStatistics,
                                                    aMtx,
                                                    &(aMtx->mXLatchPageStack),
                                                    (ULong*)sSPIDArray,
                                                    &sSPIDCount,
                                                    sBCB->mSpaceID,
                                                    sBCB->mPageID )
                          != IDE_SUCCESS );
            }
        }
        /* XLatchPageStack 비우기 */
        while( sdrMtxStack::isEmpty(&aMtx->mXLatchPageStack) != ID_TRUE )
        {
            sItem = sdrMtxStack::pop( &( aMtx->mXLatchPageStack ) );

        }
    }

    while( sdrMtxStack::isEmpty(&aMtx->mLatchStack) != ID_TRUE )
    {
        sItem = sdrMtxStack::pop( &( aMtx->mLatchStack ) );
        IDE_TEST( releaseLatchItem4Rollback( aMtx,
                                             sItem )
                  != IDE_SUCCESS );
    }

    /* BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다.
     * Mtx가 Commit될때 SpaceCache로 Free된 Extent를 반환합니다. */ 
    IDE_TEST( executePendingJob( aMtx,
                                 ID_FALSE)  //aDoCommitOp
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( destroy(aMtx) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-31006 - Debug information must be recorded when an exception occurs
     *             from mini transaction commit or rollback.
     * mini transaction commit/rollback 과정 중 실패시 디버깅 정보 출력하고
     * 서버를 죽인다. */

    /* dump sdrMtx */
    if ( sState != 0 )
    {
        dumpHex( aMtx );
        dump( aMtx, SDR_MTX_DUMP_DETAILED );
    }

    /* dump variable */
    ideLog::log( IDE_SERVER_0, "[ Dump variable ]" );
    ideLog::log( IDE_SERVER_0,
                 "LogSize: %"ID_UINT32_FMT", "
                 "ParseOffset: %"ID_INT32_FMT", "
                 "State: %"ID_UINT32_FMT"",
                 sLogSize,
                 sParseOffset,
                 sState );

    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *
 * save point까지 latch를 해제한다. page가 수정되었을 경우
 * latch를 해제해서는 안된다. 즉 savepoint는 무효화된다.
 * 이를 log buffer의 크기로 구분한다.
 *
 * Implementation :
 * if( save point의 로그 크기 != 현재 로그 크기 )
 *     return false
 *
 * if( savepoint의 크기 > 현재 stack의 크기 )
 *     return false
 *
 *
 * while( savepoint + 1까지의 stack index에 대해 )
 *     pop
 *     release
 *
 *
 *
 **********************************************************************/
IDE_RC sdrMiniTrans::releaseLatchToSP( sdrMtx       *aMtx,
                                       sdrSavePoint *aSP )
{

    sdrMtxLatchItem *sItem;

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( aSP != NULL );

    IDE_DASSERT( ( (getLogMode(aMtx) == SDR_MTX_LOGGING) &&
                   (aSP->mLogSize == smuDynArray::getSize(&aMtx->mLogBuffer))) ||
                 ( (getLogMode(aMtx) == SDR_MTX_NOLOGGING) &&
                   (aSP->mLogSize == 0) ) );

    while( aSP->mStackIndex < sdrMtxStack::getCurrSize(&aMtx->mLatchStack) )
    {
        sItem = sdrMtxStack::pop( &( aMtx->mLatchStack ) );

        IDE_TEST( releaseLatchItem4Rollback( aMtx,
                                             sItem )
                  != IDE_SUCCESS );
    }

    while( aSP->mXLatchStackIndex < sdrMtxStack::getCurrSize(&aMtx->mXLatchPageStack) )
    {
        sItem = sdrMtxStack::pop( &( aMtx->mXLatchPageStack ) );
        if ( SDR_MTX_STARTUP_BUG_DETECTOR_IS_ON(aMtx->mFlag) )
        {
            IDE_TEST( smuUtility::freeAlignedBuf( &(sItem->mPageCopy) )
                      != IDE_SUCCESS );
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 *
 * savepoint를 설정한다.
 *
 **********************************************************************/
void sdrMiniTrans::setSavePoint( sdrMtx       *aMtx,
                                 sdrSavePoint *aSP )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT(aSP != NULL);

    aSP->mStackIndex = sdrMtxStack::getCurrSize( &aMtx->mLatchStack );

    aSP->mXLatchStackIndex = sdrMtxStack::getCurrSize(
                                            &aMtx->mXLatchPageStack );

    if( getLogMode(aMtx) == SDR_MTX_LOGGING )
    {
        aSP->mLogSize = smuDynArray::getSize(&aMtx->mLogBuffer);
    }
    else
    {
        aSP->mLogSize = 0;
    }

}


/***********************************************************************
 * Description : mtx 스택의 한 item의 release 작업을 수행한다.
 *
 * release란 한 item의 정보를 기초로 commit 시에 해주어야 할 작업을 말한다.
 * 예를 들어, latch를 해제하고, fix count를 감소시키는 등의 작업이 있다.
 *
 * item이 가지고 있는 오브젝트는 sdbBCB, iduLatch, iduMutex 등등 1가지
 * 이상의 latch 종류를 가질 수 있으며, 종류에 따라 release 해야 한다.
 *
 * sdbBCB의 경우는 sdb의 page release 함수를 호출한다.
 * iduLatch와 iduMutex의 경우에는 latch, mutex를 해제해 주면 된다.
 * 이를 위해 각 타입의 release function을 초기화 한 후
 * 호출한다.
 *
 * object            latch mode     release function
 * ----------------------------------------------------------
 * Page               No latch    sdbBufferMgr::releasePage
 * Page               S latch     sdbBufferMgr::releasePage
 * Page               X latch     sdbBufferMgr::releasePage
 * iduLatch           S latch     iduLatch::unlock
 * iduLatch           X latch     iduLatch::unlock
 * iduMutex           X lock      Object.unlock
 *
 * - 1st. code design
 *   + if(object가 null) !!ASSERT
 *   + if( object가 Page에 대한 것이면 )
 *     - BCB를 release한다. ->sdbBufferMgr::releasePage
 *   else if ( object가 iduLatch에 대한 것이면 )
 *     - latch를 해제한다.->iduLatch::unlock
 *   else ( object가 iduMutex에 대한 것이면)
 *     - mutex를 해제한다.->object.unlock()
 *
 * - 2nd. code design
 *   + if(object가 null) assert
 *   + latch item 타입에 따라 release 함수 포인터를 호출한다.
 **********************************************************************/
IDE_RC sdrMiniTrans::releaseLatchItem(sdrMtx*           aMtx,
                                      sdrMtxLatchItem*  aItem)
{

    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( aItem != NULL );

    IDE_TEST( mItemVector[aItem->mLatchMode].mReleaseFunc(
                    aItem->mObject,
                    aItem->mLatchMode,
                    aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Descripition : mtx로 Stack의 특정 page 포인터 반환
 *
 * xLatch 스택에 특정 page ID에 해당하는 BCB가 존재할 경우,
 * 해당 page frame에 대한 포인터를 반환.
 * 없으면 NULL
 *
 * 일반 Stack과는 다름. 왜냐하면 Logging을 위해, FixPage는 다른 MTX로 하고
 * 다른 MTX에서 SetDrity만 한 케이스가 있기 때문. 따라서 XLatchStack을
 * 찾아야 함.
 *
 ***********************************************************************/
UChar* sdrMiniTrans::getPagePtrFromPageID( sdrMtx       * aMtx,
                                           scSpaceID      aSpaceID,
                                           scPageID       aPageID )

{
    sdbBCB            sBCB;
    sdrMtxLatchItem * sFindItem;
    UChar           * sRetPagePtr = NULL;
    sdrMtxStackInfo * aLatchStack = NULL;

    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    aLatchStack = &aMtx->mLatchStack;

    sBCB.mSpaceID = aSpaceID;
    sBCB.mPageID = aPageID;

    sFindItem = existObject( aLatchStack,
                             &sBCB,
                             SDR_MTX_BCB);

    if( sFindItem != NULL )
    {
        sRetPagePtr = ((sdbBCB *)(sFindItem->mObject))->mFrame;
    }
    
    return sRetPagePtr;
}

/***********************************************************************
 * Descripition :
 *
 * releaseToSP 혹은 rollback시에 item을 release한다.
 * 다른 object는 그냥 releae와 같이 unlock을 수행하나,
 * page의 경우 flush list에 달 필요없이 latch만 해제하는 점이 다르다.
 *
 *
 ***********************************************************************/
IDE_RC sdrMiniTrans::releaseLatchItem4Rollback(sdrMtx*           aMtx,
                                               sdrMtxLatchItem*  aItem)
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );
    IDE_DASSERT( aItem != NULL );

    IDE_TEST( mItemVector[aItem->mLatchMode].mReleaseFunc4Rollback(
                    aItem->mObject,
                    aItem->mLatchMode,
                    aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 스택에 오브젝트가 있는지 확인한다.
 * 찾으면 stack의 latch item을, 못찾으면 NULL을 리턴한다.
 *
 * - 1st code design
 *   + 스택에 해당 아이템이 있는지 확인한다. -> sdrMtxStack::find()
 **********************************************************************/
sdrMtxLatchItem* sdrMiniTrans::existObject( sdrMtxStackInfo       * aLatchStack,
                                            void                  * aObject,
                                            sdrMtxItemSourceType    aType )
{
    sdrMtxLatchItem    sSrcItem;
    sdrMtxLatchItem  * sFindItem;

    IDE_DASSERT( aObject != NULL )
  
    sSrcItem.mObject = aObject;

    sFindItem = sdrMtxStack::find( aLatchStack,
                                   &sSrcItem,
                                   aType,
                                   mItemVector );

    return sFindItem;
}

#if 0
/***********************************************************************
 *
 * Description :
 *
 * minitrans의 begin lsn을 return
 *
 * Implementation :
 *
 **********************************************************************/
smLSN sdrMiniTrans::getBeginLSN( void     * aMtx )
{

    IDE_DASSERT( validate((sdrMtx *)aMtx) == ID_TRUE );

    return ((sdrMtx *)aMtx)->mBeginLSN;

}
#endif


/***********************************************************************
 *
 * Description :
 *
 * minitrans의 end lsn을 return
 *
 * Implementation :
 *
 **********************************************************************/
smLSN sdrMiniTrans::getEndLSN( void     * aMtx )
{

    IDE_DASSERT( validate((sdrMtx*)aMtx) == ID_TRUE );

    return ((sdrMtx *)aMtx)->mEndLSN;

}

/***********************************************************************
 *
 * Description :
 *
 * minitrans이 Log를 write한 적이 있는지 검사한다.
 *
 * Implementation :
 *
 **********************************************************************/
idBool sdrMiniTrans::isLogWritten( void     * aMtx )
{
    sdrMtx *sMtx = (sdrMtx*)aMtx;

    IDE_DASSERT( validate(sMtx) == ID_TRUE );

    if( ( smuDynArray::getSize( &sMtx->mLogBuffer ) > 0 ) ||
        ( sMtx->mOpType != SDR_OP_INVALID ) )
    {
        IDE_ASSERT( sMtx->mAccSpaceID != 0 );
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 *
 * Description :
 *
 * minitrans 로그 모드가 logging이냐
 *
 * Implementation :
 *
 **********************************************************************/
idBool sdrMiniTrans::isModeLogging( void     * aMtx )
{

    IDE_DASSERT( validate((sdrMtx*)aMtx) == ID_TRUE );

    return ( getLogMode((sdrMtx *)aMtx) == SDR_MTX_LOGGING ?
             ID_TRUE : ID_FALSE );

}

/***********************************************************************
 *
 * Description :
 *
 * minitrans 로그 모드가 no logging이냐
 *
 * Implementation :
 *
 **********************************************************************/
idBool sdrMiniTrans::isModeNoLogging( void     * aMtx )
{

    IDE_DASSERT( validate((sdrMtx*)aMtx) == ID_TRUE );

    return ( getLogMode((sdrMtx *)aMtx) == SDR_MTX_NOLOGGING ?
             ID_TRUE : ID_FALSE );

}

/***********************************************************************
 *
 * Description :
 *
 * minitrans의 현재 내용을 dump한다.
 *
 * Implementation :
 *
 * mtx struct를 모두 hex dump한다.
 **********************************************************************/
void sdrMiniTrans::dumpHex( sdrMtx  * aMtx )
{
    UChar   * sDumpBuf;
    UInt      sDumpBufSize;

    /* sdrMtx Hexa dump */
    ideLog::log( IDE_SERVER_0, "[ Hex Dump MiniTrans ]" );
    ideLog::logMem( IDE_SERVER_0, (UChar*)aMtx, ID_SIZEOF(sdrMtx) );

    /* dump할 메모리 크기 계산 */
    sDumpBufSize = smuDynArray::getSize( &aMtx->mLogBuffer );

    if( sDumpBufSize > 0 )
    {
        if ( iduMemMgr::malloc( IDU_MEM_SM_SDR,
                                sDumpBufSize,
                                (void**)&sDumpBuf,
                                IDU_MEM_FORCE) == IDE_SUCCESS )
        {
            /* sdrMtx->mLogBuffer Hexa dump */
            ideLog::log( IDE_SERVER_0, "[ Hex Dump LogBuffer ]" );

            smuDynArray::load( &aMtx->mLogBuffer, 
                               (void*)sDumpBuf, 
                               sDumpBufSize );

            ideLog::logMem( IDE_SERVER_0,
                            sDumpBuf,
                            smuDynArray::getSize(&aMtx->mLogBuffer) );

            IDE_ASSERT( iduMemMgr::free( sDumpBuf ) == IDE_SUCCESS );
        }
    }
}

/***********************************************************************
 *
 * Description :
 *
 * minitrans의 현재 내용을 dump한다.
 *
 * Implementation :
 *
 * mtx struct을 모두 표시한다.
 **********************************************************************/
void sdrMiniTrans::dump( sdrMtx          *aMtx,
                         sdrMtxDumpMode   aDumpMode )
{
    IDE_DASSERT( validate(aMtx) == ID_TRUE );

    if( aMtx->mTrans != NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "----------- MiniTrans Begin ----------\n" 
                     "Transaction ID\t: %"ID_UINT32_FMT"\n"
                     "TableSpace ID\t: %"ID_UINT32_FMT"\n"
                     "Flag\t: %"ID_UINT32_FMT"\n"
                     "Used Log Buffer Size\t: %"ID_UINT32_FMT"\n"
                     "Begin LSN\t: %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "End LSN\t: %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "Dis Log Attr\t: %"ID_xINT32_FMT"\n",
                     smLayerCallback::getTransID( aMtx->mTrans ),
                     aMtx->mAccSpaceID,
                     aMtx->mFlag,
                     smuDynArray::getSize(&aMtx->mLogBuffer),
                     aMtx->mBeginLSN.mFileNo,
                     aMtx->mBeginLSN.mOffset,
                     aMtx->mEndLSN.mFileNo,
                     aMtx->mEndLSN.mOffset,
                     aMtx->mDLogAttr );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "----------- MiniTrans Begin ----------\n" 
                     "Transaction ID\t: <NONE>\n"
                     "TableSpace ID\t: %"ID_UINT32_FMT"\n"
                     "Flag\t: %"ID_UINT32_FMT"\n"
                     "Used Log Buffer Size\t: %"ID_UINT32_FMT"\n"
                     "Begin LSN\t: %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "End LSN\t: %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "Dis Log Attr\t: %"ID_xINT32_FMT"\n",
                     aMtx->mAccSpaceID,
                     aMtx->mFlag,
                     smuDynArray::getSize(&aMtx->mLogBuffer),
                     aMtx->mBeginLSN.mFileNo,
                     aMtx->mBeginLSN.mOffset,
                     aMtx->mEndLSN.mFileNo,
                     aMtx->mEndLSN.mOffset,
                     aMtx->mDLogAttr );
    }

    if (aMtx->mOpType != 0)
    {
        ideLog::log( IDE_SERVER_0,
                     "Operational Log Type \t: %"ID_UINT32_FMT"\n"
                     "Prev Prev LSN For NTA\t: %"ID_UINT32_FMT",%"ID_UINT32_FMT"\n"
                     "RID For NTA\t: %"ID_UINT32_FMT",%"ID_UINT32_FMT"\n",
                     aMtx->mOpType,
                     aMtx->mPPrevLSN.mFileNo,
                     aMtx->mPPrevLSN.mOffset ,
                     SD_MAKE_PID(aMtx->mData[0]),
                     SD_MAKE_OFFSET(aMtx->mData[0]) );
    }

    sdrMtxStack::dump( &aMtx->mLatchStack, mItemVector, aDumpMode );

    ideLog::log( IDE_SERVER_0, 
                         "----------- MiniTrans End ----------" );
}

/***********************************************************************
 *
 * Description :
 *
 * validate
 *
 * Implementation :
 *
 **********************************************************************/
#if defined(DEBUG)
idBool sdrMiniTrans::validate( sdrMtx    * aMtx )
{
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( sdrMtxStack::validate( &aMtx->mLatchStack ) == ID_TRUE );

    return ID_TRUE;
}
#endif
/***********************************************************************

 Description :

 Implementation



 **********************************************************************/
idBool sdrMiniTrans::isEmpty( sdrMtx * aMtx )
{
    IDE_DASSERT( aMtx != NULL );

    return sdrMtxStack::isEmpty(&(aMtx->mLatchStack));
}

/***********************************************************************
 * Description : NOLOGGING Persistent 판단
 * NOLOGGING mini-transaction의 persistent 설정 여부를 return
 **********************************************************************/
idBool sdrMiniTrans::isNologgingPersistent( void * aMtx )
{
    sdrMtx     *sMtx;

    IDE_DASSERT( aMtx != NULL );

    sMtx = (sdrMtx*)aMtx;

    return ((sMtx->mFlag & SDR_MTX_NOLOGGING_ATTR_PERSISTENT_MASK) ==
            SDR_MTX_NOLOGGING_ATTR_PERSISTENT_ON)? ID_TRUE : ID_FALSE;
}

/***********************************************************************
 * PROJ-2162 RestartRiskReduction
 *
 * Description : Mtx Commit마다 DRDB Redo기능을 검증함
 *  
 * aMtx          - [IN] 검증대상인 Mtx
 * aMtxStack     - [IN] MiniTransaction의 Dirtypage를 모아둔 Stack
 * aLogBuffer    - [IN] 이번에 기록한 Log들
 **********************************************************************/
IDE_RC sdrMiniTrans::validateDRDBRedo( sdrMtx          * aMtx,
                                       sdrMtxStackInfo * aMtxStack,
                                       smuDynArrayBase * aLogBuffer )
{
    idvSQL             sDummyStatistics;
    SChar            * sRedoLogBuffer = NULL;
    UInt               sRedoLogBufferSize;
    sdrRedoLogData   * sHead;
    sdrRedoLogData   * sData;
    ULong              sSPIDArray[ SDR_MAX_MTX_STACK_SIZE ];
    UInt               sSPIDCount = 0;
    sdbBCB           * sBCB;
    UInt               sState = 0;
    UInt               i;

    idlOS::memset( &sDummyStatistics, 0, ID_SIZEOF( idvSQL ) );

    /*********************************************************************
     * 1) MTX에서 기록한 Log들을 분석하여, 갱신된 페이지들을 찾음
     *********************************************************************/
    
    /* 기록된 Log를 가져옴 */
    sRedoLogBufferSize = smuDynArray::getSize( aLogBuffer);

    /* sdrMiniTrans_validateDRDBRedo_malloc_RedoLogBuffer.tc */
    IDU_FIT_POINT("sdrMiniTrans::validateDRDBRedo::malloc::RedoLogBuffer");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SDR,
                                 sRedoLogBufferSize,
                                 (void**)&sRedoLogBuffer,
                                 IDU_MEM_FORCE) 
              != IDE_SUCCESS );
    smuDynArray::load( aLogBuffer, 
                       sRedoLogBuffer,
                       sRedoLogBufferSize );
    sState = 1;

    IDE_TEST( sdrRedoMgr::generateRedoLogDataList( 0, // dummy TID
                                                   sRedoLogBuffer,
                                                   sRedoLogBufferSize,
                                                   &aMtx->mBeginLSN,
                                                   &aMtx->mEndLSN,
                                                   (void**)&sHead )
              != IDE_SUCCESS );

    /* 검증 시작 */
    sData = sHead;
    do
    {
        IDE_TEST( validateDRDBRedoInternal( &sDummyStatistics,
                                            aMtx,
                                            aMtxStack,
                                            (ULong*)sSPIDArray,
                                            &sSPIDCount,
                                            sData->mSpaceID,
                                            sData->mPageID )
                  != IDE_SUCCESS );

        sData = (sdrRedoLogData*)SMU_LIST_GET_NEXT( &(sData->mNode4RedoLog) )->mData;
    }    
    while( sData != sHead );

    IDE_TEST( sdrRedoMgr::freeRedoLogDataList( (void*)sData ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sRedoLogBuffer ) != IDE_SUCCESS );

    /*********************************************************************
     * 2) Log는 안남았는데, SetDirty된 페이지들을 검증
     *********************************************************************/
    for( i = 0 ; i < aMtxStack->mCurrSize ; i ++ )
    {
        sBCB = (sdbBCB*)(aMtxStack->mArray[ i ].mObject);

        IDE_TEST( validateDRDBRedoInternal( &sDummyStatistics,
                                            aMtx,
                                            aMtxStack,
                                            (ULong*)sSPIDArray,
                                            &sSPIDCount,
                                            sBCB->mSpaceID,
                                            sBCB->mPageID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 1:
        IDE_ASSERT( iduMemMgr::free( sRedoLogBuffer ) == IDE_SUCCESS );
    default:
        break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-2162 RestartRiskReduction
 *
 * Description : 실제로 검증을 수행함.
 * 
 * <<State예외 처리. 안함. 여기서 예외가 발생하면 종료시킬것이기 때문>>
 *
 * aStatistics    - [IN] Dummy 통계정보
 * aMtx           - [IN] 검증대상인 Mtx
 * aMtxStack      - [IN] MiniTransaction의 Dirtypage를 모아둔 Stack
 * aSPIDArray     - [IN] 중복 검사를 막기 위한 Array
 * aSPIDCount     - [IN] 중복 검사를 막기 위한 Array의 객체 개수
 * aSpaceID       - [IN] 검증할 대상 Page의 SpaceID
 * aPageID        - [IN] 검증할 대상 Page의 PageID
 **********************************************************************/

IDE_RC sdrMiniTrans::validateDRDBRedoInternal( 
                                            idvSQL          * aStatistics,
                                            sdrMtx          * aMtx,
                                            sdrMtxStackInfo * aMtxStack,
                                            ULong           * aSPIDArray,
                                            UInt            * aSPIDCount,
                                            scSpaceID         aSpaceID,
                                            scPageID          aPageID )
{
    UChar            * sBufferPage;
    UChar            * sRecovePage;
    sdbBCB             sBCB;
    sdrMtxLatchItem  * sFindItem;
    smuAlignBuf        sPagePtr;
    idBool             sReadFromDisk;
    idBool             sSuccess;
    sdpPageType        sPageType;
    ULong              sSPID;
    UInt               i;

    /****************************************************************
     * 1) 중복 체크 
     ****************************************************************/
    sSPID = ( (ULong)aSpaceID << 32 ) + aPageID;
    for( i = 0 ; i < (*aSPIDCount) ; i ++  )
    {
        if( aSPIDArray[ i ] == sSPID )
        {
            break;
        }
    }
    IDE_TEST_CONT( i != (*aSPIDCount), SKIP );
    aSPIDArray[ (*aSPIDCount) ++ ] = sSPID;

    /****************************************************************
     * 2) 검증할 두 Page 가져오기
     ****************************************************************/
    sBCB.mSpaceID = aSpaceID;
    sBCB.mPageID  = aPageID;
    sFindItem     = existObject( aMtxStack,
                                 &sBCB,
                                 SDR_MTX_BCB );

    /* Log는 기록했는데 setDirty를 안한 경우로 오류임.
       여기서 예외는 어차피 ASSERT 라서 별도의 예외처리안함. */
    IDE_ERROR_MSG( sFindItem != NULL,
                   "[ERROR] - The FindItem is not found." );

    /* Frame 복사해두기. */
    IDE_ASSERT( smuUtility::allocAlignedBuf( IDU_MEM_SM_SDR,
                                             SD_PAGE_SIZE,
                                             SD_PAGE_SIZE,
                                             &sPagePtr )
                == IDE_SUCCESS );
    sBufferPage = sPagePtr.mAlignPtr;
    idlOS::memcpy( sBufferPage, 
                   ((sdbBCB *)(sFindItem->mObject))->mFrame,
                   SD_PAGE_SIZE );

    sRecovePage = sFindItem->mPageCopy.mAlignPtr;
    IDE_ASSERT( sRecovePage != NULL );

    if ( existObject( &aMtx->mLatchStack,
                      &sBCB,
                      SDR_MTX_BCB )
         == NULL )
    {
        /* Latch를 잡지 않은 경우는 두개의 Mtx를 쓰는 CreatePage등으로,
         * Latch는 다른 Mtx에서 잡고, 여기서는 SetDrity하고 Logging만
         * 한 경우이다. 
         * 이 경우에는 페이지를 변경한 후 setDirty를 하는경우가 많아서,
         * Copy본으로 Redo했다가는 다음 경우에 문제가 생기기 때문에, 
         * DiskFile로부터 Page를 읽는다.*/
        sReadFromDisk = ID_TRUE;
    }
    else
    {
        sReadFromDisk = ID_FALSE;
    }

    /****************************************************************
     * 3) 검증할까 말까?
     ****************************************************************/
    sPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)sBufferPage );

    /* BUG-32546 [sm-disk-index] The logging image of DRDB index meta page
     * can be different to page image.
     * 위 버그 해결 후 Skip을 풀것. */
    IDE_TEST_CONT( sPageType == SDP_PAGE_INDEX_META_BTREE, SKIP );
    IDE_TEST_CONT( sPageType == SDP_PAGE_INDEX_META_RTREE, SKIP );

    /* Restart Recovery 시점에는 AgerbleSCN이 설정되어 있지 않기 때문에
     * getCommitSCN을 통한 DelayedStamping을 못한다. 따라서 Stamping
     * 이 필요한 경우는 검증을 하지 않는다. */
    IDE_TEST_CONT( ( smrRecoveryMgr::isRestart() == ID_TRUE ) &&
                    ( ( sPageType == SDP_PAGE_INDEX_BTREE ) ||
                      ( sPageType == SDP_PAGE_INDEX_RTREE ) ||
                      ( sPageType == SDP_PAGE_DATA ) ),
                    SKIP );

    if( SM_IS_LSN_INIT( sdpPhyPage::getPageLSN( sBufferPage ) ) )
    {
        /* CreatePage등을 한 경우 밖에 없음. 따라서 이 경우는
         * RecoverdPage에 BCB의 이전 Frame의 내용이 있기 때문에
         * Page 정보가 정확하지 않으므로 검증하면 안된다.
         * sdbBufferPool::createPage 참고 */
        sSuccess = ID_FALSE;
    }
    else
    {
        IDE_TEST( sdrRedoMgr::generatePageUsingOnlineRedo( 
                                                aStatistics,
                                                aSpaceID,
                                                aPageID,
                                                sReadFromDisk,
                                                sRecovePage,
                                                &sSuccess )
                 != IDE_SUCCESS );
    }

    if( sSuccess == ID_TRUE )
    {
        IDE_TEST( sPageType 
                  != sdpPhyPage::getPageType( (sdpPhyPageHdr*)sRecovePage ) );

        /* Logging없이 수행하는 FastStamping을 위해 똑같이 맞춰준다. */
        if( sPageType == SDP_PAGE_DATA )
        {
            IDE_TEST( sdcTableCTL::stampingAll4RedoValidation(aStatistics,
                                                              sRecovePage,
                                                              sBufferPage )
                      != IDE_SUCCESS );
        }
        if( ( sPageType == SDP_PAGE_INDEX_BTREE ) ||
            ( sPageType == SDP_PAGE_INDEX_RTREE ) )
        {
            IDE_TEST( sdnIndexCTL::stampingAll4RedoValidation(aStatistics,
                                                              sRecovePage,
                                                              sBufferPage )
                      != IDE_SUCCESS );
        }
        IDE_TEST( idlOS::memcmp( sRecovePage  + ID_SIZEOF( sdbFrameHdr ),
                                 sBufferPage  + ID_SIZEOF( sdbFrameHdr ),
                                 SD_PAGE_SIZE - ID_SIZEOF( sdbFrameHdr )
                                              - ID_SIZEOF( sdpPageFooter )
                               ) != 0 );
    }

    IDE_TEST( smuUtility::freeAlignedBuf( &sPagePtr )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "SpaceID: %"ID_UINT32_FMT"\n"
                 "PageID : %"ID_UINT32_FMT"\n",
                 aSpaceID,
                 aPageID );

    dumpHex( aMtx );
    dump( aMtx, SDR_MTX_DUMP_DETAILED );
    /* SetDirty가 안된 경우. Page 정보 찍기 전에 서버 종료시킴 */
    IDE_ASSERT( sFindItem != NULL );

    sdpPhyPage::tracePage( IDE_SERVER_0,
                           sRecovePage,
                           "RedoPage:");
    sdpPhyPage::tracePage( IDE_SERVER_0,
                           sBufferPage,
                           "BuffPage:");
    /* PageType이 달라서 안된 경우. */
    IDE_ASSERT( sdpPhyPage::getPageType( (sdpPhyPageHdr*)sRecovePage ) ==
                sdpPhyPage::getPageType( (sdpPhyPageHdr*)sBufferPage ) );
    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :  BUG-32579 The MiniTransaction commit should not be used in
 * exception handling area.
 ***************************************************************************/
idBool sdrMiniTrans::isRemainLogRec( void * aMtx )
{
    sdrMtx     *sMtx;
    idBool      sRet = ID_FALSE;

    IDE_DASSERT( aMtx != NULL );

    sMtx = (sdrMtx*)aMtx;

    if( sMtx->mRemainLogRecSize == 0 )
    {
        sRet = ID_FALSE;
    }
    else
    {
        sRet = ID_TRUE;
    }

    return sRet;
}
