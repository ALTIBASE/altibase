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
 * $Id: smrLFThread.h 82426 2018-03-09 05:12:27Z emlee $
 **********************************************************************/

#ifndef _O_SMR_LF_THREAD_H_
#define _O_SMR_LF_THREAD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <smrDef.h>
#include <smrArchThread.h>
#include <smrLogFile.h>
#include <idvTime.h>

/* Log Fulsh Thread
 *
 * 주기적으로 깨어나서 현재 Flush할 로그파일들을 Flush한다.
 *
 * 혹은, 외부 모듈들의 요청에 의해 로그파일들을 Flush한다.
 *
 */

class smrLFThread : public idtBaseThread
{
//For Operation    
public:
    /* Sync대상 로그파일 리스트에 로그파일을 추가
     */
    IDE_RC addSyncLogFile( smrLogFile    * aLogFile );

    /* Sync대상 로그파일 리스트에서 로그파일을 제거
     */
    IDE_RC removeSyncLogFile( smrLogFile    * aLogFile );

    /* aFileNo, aOffset까지 로그가 sync되었음을 보장한다.
     * 
     * 1. Commit Transaction의 Durability를 보장하기 위해 호출
     * 2. Log sync를 보장하기 위해 LogSwitch 과정에서 호출
     * 3. 버퍼 관리자에 의해 호출되며, 기본적인 동작은
     *    noWaitForLogSync 와 같다.
     */
    IDE_RC syncOrWait4SyncLogToLSN( smrSyncByWho  aSyncWho,
                                    UInt          aFileNo,
                                    UInt          aOffset,
                                    UInt        * aSyncedLFCnt );

    /* aFileNo, aOffset까지 로그가 sync되었음을 보장한다.
     *
     * 이미 sync되었는지 한번만 check 후 바로 로그를 sync한다.
     */
    IDE_RC syncLogToDiskByGroup( smrSyncByWho aWhoSync,
                                 UInt         aFileNo,
                                 UInt         aOffset,
                                 idBool     * aIsSyncLogToLSN,
                                 UInt       * aSyncedLFCnt );

    // 특정 LSN까지 로그가 sync되었는지 확인한다.
    IDE_RC isSynced( UInt    aFileNo,
                     UInt    aOffset,
                     idBool* aIsSynced) ;
    
    // 현재까지 sync된 LSN을 리턴한다.
    // mSyncLSN 에 대한 thread-safe한 getter
    IDE_RC getSyncedLSN( smLSN *aLSN );

    // 현재까지 sync된 위치를 mSyncLSN 에 갱신한다.
    // mSyncLSN 에 대한 thread-safe한 setter
    IDE_RC setSyncedLSN( UInt    aFileNo,
                         UInt    aOffset );

    // 테이블 백업 파일을 삭제할지의 여부를 설정한다.
    IDE_RC setRemoveFlag( idBool aRemove );
    
    virtual void run();
    IDE_RC shutdown();

    /* 인스턴스를 초기화하고 로그 Flush 쓰레드를 시작함
     */
    IDE_RC initialize( smrLogFileMgr   * aLogFileMgr,
                       smrArchThread   * aArchThread );
    
    IDE_RC destroy();
   
    void dump();

    // V$LFG 에 보여줄 Group Commit 통계치
    // Commit하려는 트랜잭션들이 Log가 Flush되기를 기다린 횟수
    inline UInt getGroupCommitWaitCount();
    // Commit하려는 트랜잭션들이 Flush하려는 Log의 위치가
    // 이미 Log가 Flush된 것으로 판명되어 빠져나간 횟수
    inline UInt getGroupCommitAlreadySyncCount();
    // Commit하려는 트랜잭션들이 실제로 Log 를 Flush한 횟수
    inline UInt getGroupCommitRealSyncCount();

    smrLFThread();
    virtual ~smrLFThread();
public:

    inline IDE_RC lockListMtx()
    { return mMtxList.lock( NULL ); };
    inline IDE_RC unlockListMtx()
    { return mMtxList.unlock(); };

    inline IDE_RC lockThreadMtx()
    { return mMtxThread.lock( NULL ); };
    inline IDE_RC unlockThreadMtx()
    { return mMtxThread.unlock(); };

    inline IDE_RC lockSyncLSNMtx()
    { return mMtxSyncLSN.lock( NULL ); };
    inline IDE_RC unlockSyncLSNMtx()
    { return mMtxSyncLSN.unlock(); };

    inline void wait4MultiplexLogFileSwitch( smrLogFile * aLogFile );

private:
    IDE_RC closeLogFile( smrLogFile *aLogFile );
    IDE_RC waitForCV( UInt     aFileNo );
    IDE_RC wakeupWaiterForSync();

    IDE_RC syncToLSN( smrSyncByWho aWhoSync,
                      idBool       aSyncLstPage,
                      UInt         aFileNo,
                      UInt         aOffset,
                      UInt        *aSyncLFCnt );

//For Member
private:
    // 이 로그 Flush 쓰레드가 Flush하는 로그파일들을
    // 관리하는 로그파일 관리자.
    // 하나의 로그를 Flush한 다음 해당 로그 파일을 close해야 하는데,
    // 바로 이 로그파일 관리자가 로그파일의 close를 담당한다.
    smrLogFileMgr    * mLogFileMgr;

    // 로그 Flush thread의 동작을 제어하기 위한 Condition value
    // mMtxThread와 함께 사용된다.
    iduCond            mCV;
    PDL_Time_Value     mTV;

    // 로그파일 Flush 쓰레드를 종료해야 할지의 여부
    // 이 플래그가 세팅되면 run()함수가 룹을 빠져나와서 종료되어야 한다.
    idBool             mFinish;

    // mSyncLogFileList 의 동시성을 제어하기 위해 사용되는 Mutex.
    iduMutex           mMtxList;

    // 로그 Flush thread의 동작을 제어하기 위한 Mutex.
    // mCV, mTV와 함께 사용된다.
    iduMutex           mMtxThread;

    // 32비트에서 64비트 데이터인 mLSN를
    // atomic하게 Read/Write하기 위해 사용하는 Mutex
    iduMutex           mMtxSyncLSN;
    iduCond            mSyncWaitCV;

    // 현재까지 Flush된 LSN.
    smrLSN4Union       mLstLSN;

    // sync 대상인 로그파일의 리스트.
    smrLogFile         mSyncLogFileList;

    // 마지막으로 Log Sync를 한 시각
    idvTime            mLastSyncTime;

    // V$LFG 에 보여줄 Group Commit 통계치
    // Commit하려는 트랜잭션들이 Log가 Flush되기를 기다린 횟수
    UInt               mGCWaitCount;
    // Commit하려는 트랜잭션들이 Flush하려는 Log의 위치가
    // 이미 Log가 Flush된 것으로 판명되어 빠져나간 횟수
    UInt               mGCAlreadySyncCount;
    // Commit하려는 트랜잭션들이 실제로 Log 를 Flush한 횟수
    UInt               mGCRealSyncCount;

    // Archive Thread Ptr
    smrArchThread*     mArchThread;

    // Log가 Sync가 되기를 대기하는 Thread의 갯수
    UInt               mThreadCntWaitForSync;
};


// V$LFG 에 보여줄 Group Commit 통계치
// Commit하려는 트랜잭션들이 Log가 Flush되기를 기다린 횟수
UInt smrLFThread::getGroupCommitWaitCount()
{
    return mGCWaitCount;
}


// Commit하려는 트랜잭션들이 Flush하려는 Log의 위치가
// 이미 Log가 Flush된 것으로 판명되어 빠져나간 횟수
UInt smrLFThread::getGroupCommitAlreadySyncCount()
{
    return mGCAlreadySyncCount;
}


// Commit하려는 트랜잭션들이 실제로 Log 를 Flush한 횟수
UInt smrLFThread::getGroupCommitRealSyncCount()
{
    return mGCRealSyncCount;
}

/* mSyncLogFileList 링크드 리스트의 작동방식
 * ( mTBFileList 도 동일하게 적용됨 )
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

/*
 * 로그파일 다중화가 SMR_LT_FILE_END까지 완료되어 파일 다중화 로그파일 switch가
 * 수행될때 까지 대기한다.
 *
 * aLogFile - [IN] 원본 로그파일
 */ 
inline void smrLFThread::wait4MultiplexLogFileSwitch( smrLogFile * aLogFile )
{
    PDL_Time_Value    sTV;    
    UInt              sMultiplexIdx;

    sTV.set( 0, 1 );

    for( sMultiplexIdx = 0; 
         sMultiplexIdx < smrLogMultiplexThread::mMultiplexCnt;
         sMultiplexIdx++ )
    {
        while( aLogFile->mMultiplexSwitch[ sMultiplexIdx ] == ID_FALSE )
        {
            idlOS::sleep( sTV );
        }
    }
}

#endif /* _O_SMR_LF_THREAD_H_ */
