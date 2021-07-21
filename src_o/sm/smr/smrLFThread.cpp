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
 * $Id: smrLFThread.cpp 82426 2018-03-09 05:12:27Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smr.h>
#include <smrReq.h>
#include <smxTransMgr.h> /* BUG-35392 */

smrLFThread::smrLFThread() : idtBaseThread()
{
}

smrLFThread::~smrLFThread()
{
}

/* 인스턴스를 초기화하고 로그 Flush 쓰레드를 시작함
 *
 * 이 로그 Flush 쓰레드가 Flush하는 로그파일들을
 * 관리하는 로그파일 관리자.
 */
IDE_RC smrLFThread::initialize( smrLogFileMgr   * aLogFileMgr,
                                smrArchThread   * aArchThread)
{
    IDE_DASSERT( aLogFileMgr != NULL );

    mLogFileMgr = aLogFileMgr;
    mArchThread = aArchThread;
    
    mFinish     = ID_FALSE;
    
    // mSyncLogFileList 의 head,tail := NULL
    mSyncLogFileList.mSyncPrvLogFile = &mSyncLogFileList;
    mSyncLogFileList.mSyncNxtLogFile = &mSyncLogFileList;

    IDE_TEST(mMtxList.initialize((SChar*)"LOG_FILE_SYNC_LIST_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
    
    IDE_TEST(mMtxThread.initialize((SChar*)"LOG_FLUSH_THREAD_MUTEX",
                                   IDU_MUTEX_KIND_POSIX,
                                   IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    /* fix BUG-17602 특정 LSN이 sync되기를 대기하는 경우 단위 sync가 완료될때
     * 까지 sync 여부를 확인할 수 없음
     * 64bit에서는 SyncWait Mutex로 사용됨.
     * 32bit에서는 SyncedLSN 설정 및 판독과 SyncWait Mutex로 사용됨 */
    IDE_TEST(mMtxSyncLSN.initialize((SChar*)"SYNC_LSN_MUTEX",
                                    IDU_MUTEX_KIND_POSIX,
                                    IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    mLstLSN.mLSN.mFileNo = 0;
    mLstLSN.mLSN.mOffset = 0;

    // 마지막으로 Sync한 시각을 현재 시각으로 초기화
    IDV_TIME_GET(&mLastSyncTime);

    mThreadCntWaitForSync = 0;
    
    // V$LFG 에 보여줄 Group Commit 통계치 초기화
    mGCWaitCount = 0;
    mGCAlreadySyncCount = 0;
    mGCRealSyncCount = 0;
    
    IDE_TEST_RAISE(mCV.initialize((SChar *)"LOG_FLUSH_THREAD_COND")
                   != IDE_SUCCESS, err_cond_var_init);

    IDE_TEST_RAISE(mSyncWaitCV.initialize((SChar *)"SYNC_LSN_COND")
                   != IDE_SUCCESS, err_cond_var_init);

    IDE_TEST(start() != IDE_SUCCESS);
    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLFThread::destroy()
{
    IDE_TEST(mMtxList.destroy() != IDE_SUCCESS);
    IDE_TEST(mMtxThread.destroy() != IDE_SUCCESS);

    IDE_TEST(mMtxSyncLSN.destroy() != IDE_SUCCESS);

    IDE_TEST_RAISE(mSyncWaitCV.destroy() != IDE_SUCCESS, err_cond_var_destroy);
 
    IDE_TEST_RAISE(mCV.destroy() != IDE_SUCCESS, err_cond_var_destroy);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Sync대상 로그파일 리스트에 로그파일을 추가
*/

IDE_RC smrLFThread::addSyncLogFile(smrLogFile*   aLogFile)
{
    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    // 추가하려는 로그파일의 이전 := 링크드 리스트의 tail
    aLogFile->mSyncPrvLogFile = mSyncLogFileList.mSyncPrvLogFile;
    // 추가하려는 로그파일의 다음 := NULL
    aLogFile->mSyncNxtLogFile = &mSyncLogFileList;

    // 링크드 리스트의 tail.next := 추가하려는 로그파일
    mSyncLogFileList.mSyncPrvLogFile->mSyncNxtLogFile = aLogFile;
    // 링크드 리스트의 tail := 추가 하려는 로그파일
    mSyncLogFileList.mSyncPrvLogFile = aLogFile;

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Sync대상 로그파일 리스트에서 로그파일을 제거
 */
IDE_RC smrLFThread::removeSyncLogFile(smrLogFile*  aLogFile)
{
    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    /*     LF2제거 이전의 링크드 리스트
     *
     *         (1) ->       (2) ->
     *     LF1         LF2          LF3
     *         <- (3)       <- (4)
     *
     *---------------------------------------
     *     LF2 제거 후의 링크드 리스트
     *
     *         (1) ->                       
     *     LF1         LF3             
     *         <- (4)                  
     *
     *---------------------------------------
     *     제거되어 떨어져 나온 LF2
     *
     *                        (2)->
     *       NULL         LF2         NULL
     *             <- (3)
     */
    
    // (1) := (2)
    aLogFile->mSyncPrvLogFile->mSyncNxtLogFile = aLogFile->mSyncNxtLogFile;
    // (4) := (3)
    aLogFile->mSyncNxtLogFile->mSyncPrvLogFile = aLogFile->mSyncPrvLogFile;
    // (3) := NULL
    aLogFile->mSyncPrvLogFile = NULL;
    // (2) := NULL
    aLogFile->mSyncNxtLogFile = NULL;

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLFThread::getSyncedLSN( smLSN *aLSN )
{
    smrLSN4Union   sTmpLSN;

#ifndef COMPILE_64BIT     
    // BUGBUG lock/unlock사이에 에러 발생하지 않으므로,
    // sState관련 코드 제거해도 무관함.
    SInt    sState = 0;


    IDE_TEST( lockSyncLSNMtx() != IDE_SUCCESS );
    sState = 1;
#endif
    // 64비트의 경우 atomic 하게 mLSN을 한번 읽어온 후
    // aLSN의 멤버들을 설정한다.
    ID_SERIAL_BEGIN( sTmpLSN.mSync = mLstLSN.mSync );

    ID_SERIAL_END( SM_SET_LSN( *aLSN,
                               sTmpLSN.mLSN.mFileNo,
                               sTmpLSN.mLSN.mOffset) );

#ifndef COMPILE_64BIT    
    sState = 0;
    IDE_TEST( unlockSyncLSNMtx() != IDE_SUCCESS );
#endif
    return IDE_SUCCESS;

#ifndef COMPILE_64BIT
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( unlockSyncLSNMtx() == IDE_SUCCESS );
        IDE_POP();
    }
    
    return IDE_FAILURE;
#endif
    
}

/* 특정 LSN까지 로그가 Flush되었는지 확인한다.
 *
 * aIsSynced - [OUT] 해당 LSN까지 sync가 되었는지 여부.
 */
IDE_RC smrLFThread::isSynced( UInt    aFileNo,
                              UInt    aOffset,
                              idBool* aIsSynced )
{
    smrLSN4Union sTmpLSN;
    UInt         sFileNo = ID_UINT_MAX;
    UInt         sOffset = ID_UINT_MAX;
#ifndef COMPILE_64BIT
    SInt         sState = 0;
#endif

    *aIsSynced = ID_FALSE;
        
#ifndef COMPILE_64BIT
    IDE_TEST(lockSyncLSNMtx() != IDE_SUCCESS);
    sState = 1;
#endif

    ID_SERIAL_BEGIN( sTmpLSN.mSync = mLstLSN.mSync );
    ID_SERIAL_EXEC(  sFileNo = sTmpLSN.mLSN.mFileNo, 1 );
    ID_SERIAL_END(   sOffset = sTmpLSN.mLSN.mOffset );
    
    // aFileNo,aFileOffset까지 로그가 이미 Flush되었는지 체크
    if ( aFileNo < sFileNo )
    {
        *aIsSynced = ID_TRUE;
    }
    else
    {
        if ( (aFileNo == sFileNo) && (aOffset <= sOffset) )
        {
            *aIsSynced = ID_TRUE;
        }
    }

    
#ifndef COMPILE_64BIT
    sState = 0;
    IDE_TEST(unlockSyncLSNMtx() != IDE_SUCCESS);
#endif
    
    return IDE_SUCCESS;
    
#ifndef COMPILE_64BIT
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( unlockSyncLSNMtx() == IDE_SUCCESS );
        IDE_POP();
    }
    
    return IDE_FAILURE;
#endif
}

/* 현재까지 sync된 위치를 mLSN 에 갱신한다.
 *
 * 우선, mLSN을 읽어서 이미 sync된 위치를 확인하고
 * 이보다 더 큰 LSN인 경우에만 mLSN에 갱신을 실시한다.
 */
IDE_RC smrLFThread::setSyncedLSN(UInt    aFileNo,
                                 UInt    aOffset)
{
    smrLSN4Union sTmpLSN;

    sTmpLSN.mLSN.mFileNo = aFileNo;
    sTmpLSN.mLSN.mOffset = aOffset;
    
#ifndef COMPILE_64BIT            
    IDE_TEST(lockSyncLSNMtx() != IDE_SUCCESS);
#endif

    if ( smrCompareLSN::isLT( &mLstLSN.mLSN, &sTmpLSN.mLSN ) )
    {
        mLstLSN.mSync = sTmpLSN.mSync;
    }
    else
    {
        /* nothing to do */
    }
    
#ifndef COMPILE_64BIT            
    IDE_TEST(unlockSyncLSNMtx() != IDE_SUCCESS);
#endif
    
    return IDE_SUCCESS;

#ifndef COMPILE_64BIT
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

IDE_RC smrLFThread::waitForCV( UInt aWaitUS )
{
    IDE_RC rc;
    UInt   sState = 0;
    PDL_Time_Value sUntilWaitTV;
    PDL_Time_Value sWaitTV;

    // USec 단위로 Timed Wait 들어간다.
    sWaitTV.set(0, aWaitUS );

    sUntilWaitTV.set( idlOS::time(0) );

    sUntilWaitTV += sWaitTV;

    IDE_TEST( lockSyncLSNMtx() != IDE_SUCCESS );
    sState = 1;

    mThreadCntWaitForSync++;

    rc = mSyncWaitCV.timedwait(&mMtxSyncLSN, &sUntilWaitTV, IDU_IGNORE_TIMEDOUT);

    mThreadCntWaitForSync--;

    IDE_TEST_RAISE( rc != IDE_SUCCESS, err_cond_wait );

    sState = 0;
    IDE_TEST( unlockSyncLSNMtx() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_wait );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState == 1 )
        {
            IDE_ASSERT( unlockSyncLSNMtx() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/* mSyncLogFileList 링크드 리스트의 작동방식
 * ( mTBFileList 도 동일하게 적용됨 )
 * 
 * 
 * 1. 링크드 리스트로 선언된 smrLogFile 객체(mSyncLogFileList)는
 *    실제 로그파일을 저장하는데 사용되지 않는다.
 *
 * 2. mSyncLogFileList 안의 prv, nxt 포인터로 링크드 리스트 안의
 *    로그파일들을 가리키는 용도로만 사용될 뿐이다..
 *
 * 3. nxt포인터는 링크드 리스트의 head 를 가리킨다.
 *
 * 4. prv포인터는 링크드 리스트의 tail 을 가리킨다.
 *
 * 5. 링크드 리스트로 선언된 mSyncLogFileList의 주소(&mSyncLogFileList)는
 *    nxt, prv포인터에 할당될 경우, 링크드 리스트에서 NULL포인터의 역할을 한다.
 *
 */
IDE_RC smrLFThread::wakeupWaiterForSync()
{
    SInt sState = 0;

    // Sync를 기다리는 Thread가 있을 경우에만 깨운다.
    if ( mThreadCntWaitForSync != 0 )
    {
        IDE_TEST( lockSyncLSNMtx() != IDE_SUCCESS );
        sState = 1;

        if ( mThreadCntWaitForSync != 0 )
        {
            IDE_TEST_RAISE(mSyncWaitCV.broadcast()
                           != IDE_SUCCESS, err_cond_signal );
        }

        sState = 0;
        IDE_TEST( unlockSyncLSNMtx() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( unlockSyncLSNMtx() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/* aFileNo, aOffset까지 로그가 sync되었음을 보장한다.
 *
 * 1. Commit Transaction의 Durability를 보장하기 위해 호출
 * 2. Log sync를 보장하기 위해 LogSwitch 과정에서 호출
 * 3. 버퍼 관리자에 의해 호출되며, 기본적인 동작은
 *    noWaitForLogSync 와 같다.
 */
IDE_RC smrLFThread::syncOrWait4SyncLogToLSN( smrSyncByWho  aSyncWho,
                                             UInt          aFileNo,
                                             UInt          aOffset, 
                                             UInt        * aSyncedLFCnt )
{
    idBool  sSynced;
    idBool  sLocked;
    UInt    sState      = 0;
    smLSN   sSyncLSN;

    IDE_ASSERT( aFileNo != ID_UINT_MAX );
    IDE_ASSERT( aOffset != ID_UINT_MAX );

    SM_SET_LSN( sSyncLSN,
                aFileNo,
                aOffset );

    /* BUG-35392
     * 지정된 LSN 까지 sync가 완료되기를 대기한다. */
    smrLogMgr::waitLogSyncToLSN( &sSyncLSN,
                                 smuProperty::getLFGMgrSyncWaitMin(),
                                 smuProperty::getLFGMgrSyncWaitMax() );

    // LSN이 sync가 안된 경우
    while ( 1 )
    {
        IDE_TEST( isSynced( aFileNo, aOffset, &sSynced ) != IDE_SUCCESS );

        if ( sSynced == ID_TRUE )
        {
            break;
        }

        IDE_TEST( mMtxThread.trylock( sLocked ) != IDE_SUCCESS );

        if ( sLocked == ID_TRUE )
        {
            // 자신이 직접 LSN을 Sync해야하는 경우
            sState = 1;

            IDE_TEST( syncLogToDiskByGroup( aSyncWho,
                                            aFileNo,
                                            aOffset,
                                            &sSynced,
                                            aSyncedLFCnt )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( unlockThreadMtx() != IDE_SUCCESS );

            // sync되었다면 완료한다.
            if ( sSynced == ID_TRUE )
            {
                break;
            }
        }

        // 다른 Thread가 Sync를 수행하고 있는 경우
        // Cond_timewait하다가 Signal을 받거나,
        // 특정 시간이 흘러서 다시하번 Sync되었는지
        // 확인한다.
        IDE_TEST( waitForCV( smuProperty::getLFGGroupCommitRetryUSec() )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState == 1 )
        {
            IDE_ASSERT( unlockThreadMtx() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrLFThread::syncToLSN( smrSyncByWho aWhoSync,
                               idBool       aSyncLstPageInLstLF,
                               UInt         aFileNo,
                               UInt         aOffset,
                               UInt       * aSyncLFCnt )
{
    UInt          sState        = 0;
    smrLogFile  * sCurLogFile;
    smrLogFile  * sNxtLogFile;
    UInt          sSyncOffset;
    UInt          sCurFileNo    = 0;
    UInt          sFstSyncLFNo  = 0;
    UInt          sSyncedLFCnt  = 0;
    idBool        sIsSwitchLF;
    idBool        sSyncLstPage;
    smLSN         sSyncLSN;

    /* BUG-35392 */
    if ( ( aFileNo == ID_UINT_MAX ) || ( aOffset == ID_UINT_MAX ))
    {
        smrLogMgr::getLstLSN( &sSyncLSN );
    }
    else
    {
        SM_SET_LSN( sSyncLSN,
                    aFileNo,
                    aOffset );
    }

    /* BUG-35392
     * 지정된 LSN 까지 sync가 완료되기를 대기한다. */
    smrLogMgr::waitLogSyncToLSN( &sSyncLSN,
                                 smuProperty::getLFThrSyncWaitMin(),
                                 smuProperty::getLFThrSyncWaitMax() );

    // 아직 sync되지 않은 경우이다.
    // 로그파일 하나씩 sync 실시.
    IDE_TEST( lockListMtx() != IDE_SUCCESS );
    sState = 1;

    sCurLogFile = mSyncLogFileList.mSyncNxtLogFile;

    sState = 0;
    IDE_TEST( unlockListMtx() != IDE_SUCCESS);

    /* 
     * PROJ-2232 log multiplex
     * 다중화 로그에 대한 WAL이 깨지는것을 방지하기 위해 
     * 로그 다중화 쓰레드를 깨운다.
     */ 
    IDE_TEST( smrLogMultiplexThread::wakeUpSyncThread(
                                            smrLogFileMgr::mSyncThread,
                                            aWhoSync,
                                            aSyncLstPageInLstLF,
                                            sSyncLSN.mFileNo,
                                            sSyncLSN.mOffset )
              != IDE_SUCCESS );

    /* BUG-39953 [PROJ-2506] Insure++ Warning
     * - 현재 파일이 리스트 헤더인지 검사한 후 접근합니다.
     * - 또는, mSyncLogFileList 초기화 시, mFileNo를 초기화하는 방안이 있습니다.
     */
    if ( sCurLogFile != &mSyncLogFileList )
    {
        sFstSyncLFNo = sCurLogFile->mFileNo;
    }
    else
    {
        /* Nothing to do */
    }

    while (sCurLogFile != &mSyncLogFileList)
    {
        sNxtLogFile = sCurLogFile->mSyncNxtLogFile;
        sCurFileNo  = sCurLogFile->mFileNo;

        if ( sCurLogFile->getEndLogFlush() == ID_FALSE )
        {
            sIsSwitchLF = sCurLogFile->mSwitch;

            // sSyncLSN.mFileNo는 아직 sync되지 않은 로그파일번호로
            // mSyncLogFileList에 들어있을 수 밖에 없다.
            // sSyncLSN.mFileNo와 일치하는 로그파일을 만나기 전까지는
            // 해당 로그파일들을 모두 sync한다.
            if ( sSyncLSN.mFileNo == sCurFileNo )
            {
                // 특정 위치까지 sync를 요청 받은 경우에서
                // 마지막 LogFile인 경우이다.
                sSyncOffset  = sSyncLSN.mOffset;
                sSyncLstPage = aSyncLstPageInLstLF;
            }
            else
            {
                // 1. aFileNo가 UInt Max인 경우
                // 2. 특정 위치까지 sync를 요청 받았는데
                //    아직 도달하지 못한 경우
                // BUG-35392
                // 기존에 마지막 파일이라 하더라도 switch 되었으면
                // 여기로 왔는데 이유는 Log File이 switch 되었으면
                // 그냥 mOffset까지 sync 하면 되기 때문이다.
                // 하지만 BUG-28856  마지막 Log File이 아니더라도
                // 아직 log Copy가 완료 되지 않았을 수 있다.
                sSyncOffset  = sCurLogFile->mOffset;
                sSyncLstPage = ID_TRUE;
            }

            // 이제 위에서 isSync를 호출하여 깨운 로그 Flush 쓰레드가
            // 해당 로그파일들을 한번 더 sync하고 mSyncLogFileList 에서 지워준다.
            // 로그 Flush 쓰레드가 로그파일들을 한번씩 더 sync하도록
            // smrLogFile.syncLog를 여러번 호출하여도
            // smrLogFile.syncLog에서는 한번만 싱크하도록 구현되어 있다.
            IDE_TEST( sCurLogFile->syncLog( sSyncLstPage,
                                            sSyncOffset )
                      != IDE_SUCCESS );

            // mLSN 에 현재까지 sync한 LSN 갱신.
            IDE_TEST( setSyncedLSN( sCurLogFile->mFileNo,
                                    sCurLogFile->mSyncOffset )
                      != IDE_SUCCESS );

            // Log Sync를 기다리는 Thread들을 깨운다.
            IDE_TEST( wakeupWaiterForSync() != IDE_SUCCESS );

            if ( ( sSyncedLFCnt % 100 == 0 ) && ( sSyncedLFCnt != 0 )  )
            {
                // 한번 깨어난 다음 Sync를 수행한 로그파일 개수가
                // 100개이상인 경우에 메시지를 남긴다.
                ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                             SM_TRC_MRECOVERY_LFTHREAD_INFO_FOR_SYNC_LOGFILE,
                             sFstSyncLFNo,
                             sCurFileNo,
                             sCurFileNo - sFstSyncLFNo + 1 );

                sFstSyncLFNo += 100;
            }

            if ( sIsSwitchLF == ID_TRUE )
            {
                /* BUG-35392 */
                if ( sCurLogFile->isAllLogSynced() == ID_TRUE )
                {
                    sSyncedLFCnt++;

                    /* 더 이상 로그파일에 디스크에 기록할 로그가 없이
                     * 모두 디스크에 반영이 되었다. */
                    sCurLogFile->setEndLogFlush( ID_TRUE );
                }
                else
                {
                    /* 마지막 파일은 아니지만 Log Copy가 완료 되지 않은 경우 */
                    break;
                }
            }
            else
            {
                /* 아직 기록할 것이 남은 마지막 파일인 경우 */
                break;
            }
        }

        if ( ( sCurLogFile->getEndLogFlush() == ID_TRUE ) &&
             ( aWhoSync == SMR_LOG_SYNC_BY_LFT ) )
        {
            /* PROJ-2232 log 로그 다중화가 완료 될때까지 대기한다. */ 
            wait4MultiplexLogFileSwitch( sCurLogFile );
            /* Sync Thread만이 logfile을 close 한다. */
            IDE_TEST( closeLogFile( sCurLogFile ) != IDE_SUCCESS );
        }

        if ( sSyncLSN.mFileNo == sCurFileNo )
        {
            break;
        }

        sCurLogFile = sNxtLogFile;
    }

    if ( ( sSyncedLFCnt % 100 != 0 ) && ( sSyncedLFCnt > 100 ) )
    {
        // 한번 깨어난 다음 Sync를 수행한 로그파일 개수가
        // 100개이상인 경우에 메시지를 남긴다. 
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     SM_TRC_MRECOVERY_LFTHREAD_INFO_FOR_SYNC_LOGFILE,
                     sFstSyncLFNo,
                     sCurFileNo,
                     sCurFileNo - sFstSyncLFNo + 1 );
    }

    if ( aSyncLFCnt != NULL )
    {
        *aSyncLFCnt = sSyncedLFCnt;
    }

    /* PROJ-2232 다중화로그가 파일로 반영될때까지 대기한다. */
    IDE_TEST( smrLogMultiplexThread::wait( smrLogFileMgr::mSyncThread ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState )
    {
        case 1:
            IDE_ASSERT( unlockListMtx() == IDE_SUCCESS);
        default:
            break;
    }

    return IDE_FAILURE;
}

/* aFileNo, aOffset까지 로그가 sync되었음을 보장한다.
 *
 * [ 주의사항 ]
 * mMtxThread 가 획득된 상태에서 호출되어야 한다.
 */
IDE_RC smrLFThread::syncLogToDiskByGroup( smrSyncByWho aWhoSync,
                                          UInt         aFileNo,
                                          UInt         aOffset,
                                          idBool     * aIsSyncLogToLSN,
                                          UInt       * aSyncedLFCnt )
{
    idBool         sSynced = ID_FALSE;
    idvTime        sCurTime;
    ULong          sTimeDiff;
    
    // 이 속에서 로그 Flush 쓰레드를 깨우게 되는데,
    // 위에서 로그 Flush 쓰레드 래치를 잡았기 때문에,
    // 이 래치를 풀기까지 로그 Flush 쓰레드는 기다려야 한다.
    IDE_TEST( isSynced( aFileNo,
                        aOffset,
                        &sSynced ) != IDE_SUCCESS);

    if ( aWhoSync == SMR_LOG_SYNC_BY_TRX )
    {
        if ( sSynced == ID_TRUE )
        {
            mGCAlreadySyncCount ++;
        }
        else
        {
            // LFG_GROUP_COMMIT_UPDATE_TX_COUNT == 0 이면
            // Group Commit을 Disable시킨다.
            if ( (smuProperty::getLFGGroupCommitUpdateTxCount() != 0 ) &&
                // 이 LFG의 Update Transaction 수가
                // LFG_GROUP_COMMIT_UPDATE_TX_COUNT보다 클 때
                // Group Commit을 동작시킨다.
                ( smrLogMgr::getUpdateTxCount() >=
                  smuProperty::getLFGGroupCommitUpdateTxCount() ) )
            {
                IDV_TIME_GET(&sCurTime);
                sTimeDiff = IDV_TIME_DIFF_MICRO(&mLastSyncTime,
                                                &sCurTime);

                // 만약 마지막 Sync한 이후로 현재 시각이
                // LFG_GROUP_COMMIT_INTERVAL_USEC 만큼 지나지 않았다면
                // 로그파일 Sync를 지연시킨다.
                if ( sTimeDiff < smuProperty::getLFGGroupCommitIntervalUSec() ) 
                {
                    mGCWaitCount++;
                    *aIsSyncLogToLSN = ID_FALSE;

                    return IDE_SUCCESS;
                }
            }

            mGCRealSyncCount ++;
        }
    }

    if ( sSynced == ID_FALSE )
    {
        IDV_TIME_GET( &mLastSyncTime );

        IDE_TEST( syncToLSN( SMR_LOG_SYNC_BY_TRX,
                             ID_TRUE,
                             aFileNo,
                             aOffset,
                             aSyncedLFCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // 이미 sync되었다면 더 이상 할일이 없다.
    }

    *aIsSyncLogToLSN  = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 로그 Flush 쓰레드의 run함수
 * 주기적으로, 혹은 명시적인 요청에 의해 깨어나서 
 * 로그파일을 Flush하고 완전히 Flush된 로그파일을
 * Sync대상 로그파일 리스트에서 제거한다.
 */
void smrLFThread::run()
{
    IDE_RC         rc;
    UInt           sState = 0;
    PDL_Time_Value sCurTimeValue;
    PDL_Time_Value sFixTimeValue;
    UInt           sSyncedLFCnt    = 0;
    
  startPos:
    sState=0;
    
    IDE_TEST( lockThreadMtx() != IDE_SUCCESS );
    sState = 1;

    sFixTimeValue.set(smuProperty::getSyncIntervalSec(),
                      smuProperty::getSyncIntervalMSec() * 1000);
    
    while ( mFinish == ID_FALSE ) 
    {
        sSyncedLFCnt = 0;

        sCurTimeValue = idlOS::gettimeofday();

        sCurTimeValue += sFixTimeValue;

        sState = 0;
        rc = mCV.timedwait(&mMtxThread, &sCurTimeValue, IDU_IGNORE_TIMEDOUT);
        sState = 1;

        if ( mFinish == ID_TRUE )
        {
            break;
        }

        if ( smuProperty::isRunLogFlushThread() == SMU_THREAD_OFF )
        {
            // To Fix PR-14783
            // System Thread의 작업을 수행하지 않도록 한다.
            continue;
        }
        else
        {
            // Go Go 
        }
        
        IDE_TEST_RAISE( rc != IDE_SUCCESS, err_cond_wait );

        // 로그파일을 Flush하고 완전히 Flush된 로그파일을
        // Sync대상 로그파일 리스트에서 제거한다.
        IDE_TEST( syncToLSN( SMR_LOG_SYNC_BY_LFT,
                             ID_FALSE,
                             ID_UINT_MAX,
                             ID_UINT_MAX,
                             &sSyncedLFCnt ) != IDE_SUCCESS );

        if ( sSyncedLFCnt > 100 )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                         SM_TRC_MRECOVERY_LFTHREAD_TOTAL_SYNCED_LOGFILE_COUNT,
                         sSyncedLFCnt );
        }
    }

    // 쓰레드 종료하기 전에 로그파일을 다시한번 Flush한다.
    //
    // BUGBUG 이 코드와 함께 위에서 mFinish == ID_TRUE 체크하여 break하는 코드
    // 빼도 현재 코드와 동작은 같다.

    IDE_TEST( syncToLSN( SMR_LOG_SYNC_BY_LFT,
                         ID_TRUE,
                         ID_UINT_MAX,
                         ID_UINT_MAX,
                         &sSyncedLFCnt )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlockThreadMtx() != IDE_SUCCESS );
    
    return;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        (void)unlockThreadMtx();
        IDE_POP();
    }

    ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                SM_TRC_MRECOVERY_LFTHREAD_INSUFFICIENT_MEMORY_WARNNING);

    goto startPos;
}

/* 로그 Flush 쓰레드를 종료시킨다.
 */
IDE_RC smrLFThread::shutdown()
{
    UInt           sState = 0;

    // 쓰레드가 잠자고 있다는 것을 보장하기 위해 쓰레드 래치를 잡음
    IDE_TEST(lockThreadMtx() != IDE_SUCCESS);
    sState = 1;

    // 쓰레드 종료하도록 플래그 설정
    mFinish = ID_TRUE;

    // 쓰레드를 깨운다.
    IDE_TEST_RAISE(mCV.signal() != IDE_SUCCESS, err_cond_signal);

    // 쓰레드가 깨어날 수 있도록 쓰레드 래치 해제
    sState = 0;
    IDE_TEST( unlockThreadMtx() != IDE_SUCCESS );

    IDE_TEST_RAISE(join() != IDE_SUCCESS, err_thr_join);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));   
    }
    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0)
    {
        IDE_PUSH();
        (void)unlockThreadMtx();
        IDE_POP();
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그 파일을 close한다.
 *
 * 1. sync logfile list에서 제거
 * 2. Archive Mode이면 Archive List에 추가
 * 3. logfile close요청.
 *
 * aLogFile - [IN] logfile pointer
 *
 **********************************************************************/
IDE_RC smrLFThread::closeLogFile( smrLogFile *aLogFile )
{
    // 파일을 통채로 Flush했으므로 sync할 로그파일 리스트에서 제거.
    IDE_TEST( removeSyncLogFile( aLogFile )
              != IDE_SUCCESS);

    // 아카이브 모드일 때, 아카이브 로그로 추가.
    if (smrRecoveryMgr::getArchiveMode()
        == SMI_LOG_ARCHIVE)
    {
        IDE_TEST(mArchThread->addArchLogFile( aLogFile->mFileNo )
                 != IDE_SUCCESS);
    }

    IDE_DASSERT( mLogFileMgr != NULL );
            
    IDE_TEST( mLogFileMgr->close(aLogFile)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void smrLFThread::dump()
{
    smrLogFile *sCurLogFile;
    smrLogFile *sNxtLogFile;

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_LFTHREAD_SYNC_LOGFILE_LIST);
    sCurLogFile = mSyncLogFileList.mSyncNxtLogFile;
    while(sCurLogFile != &mSyncLogFileList)
    {
        sNxtLogFile = sCurLogFile->mSyncNxtLogFile;
        
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_LFTHREAD_SYNC_LOGFILE_NO,
                    sCurLogFile->mFileNo); 
        sCurLogFile = sNxtLogFile;
    }
}
