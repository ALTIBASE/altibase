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
 * $Id: smrArchThread.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMR_ARCH_THREAD_H_
#define _O_SMR_ARCH_THREAD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <smrArchMultiplexThread.h>

#define SMR_ARCH_THREAD_POOL_SIZE  (50)
#define SMR_ORIGINAL_ARCH_DIR_IDX  (0)

class smrArchThread : public idtBaseThread
{
//For Operation    
public:
    // 아카이브할 로그파일 리스트에 로그파일을 하나 새로 추가한다.
    IDE_RC addArchLogFile(UInt aLogFileNo);
    
    // 아카이브할 로그파일 리스트에서 로그파일 노드를 하나 제거한다.
    IDE_RC removeArchLogFile(smrArchLogFile *aLogFile);

    // 아카이브할 로그파일 리스트에 있는 로그파일들을 아카이빙한다.
    // 아카이브 쓰레드가 주기적으로,
    // 혹은 요청에 의해 깨어나서 수행하는 함수이다.
    IDE_RC archLogFile();

    // 마지막으로 아카이브된 파일번호를 가져온다. 
    IDE_RC setLstArchLogFileNo(UInt    aArchLogFileNo);
    // 마지막으로 아카이브된 파일번호를 설정한다.
    IDE_RC getLstArchLogFileNo(UInt*   aArchLogFileNo);

    // 다음으로 아카이브할 로그파일번호를 가져온다.
    IDE_RC getArchLFLstInfo(UInt   * aArchFstLFileNo,
                            idBool * aIsEmptyArchLFLst );
    
    // 아카이브할 로그파일 리스트를 모두 초기화 한다.
    IDE_RC clearArchList();

    virtual void run();
    // 아카이브 쓰레드를 시작시키고, 쓰레드가 정상적으로
    // 시작될 때까지 기다린다.
    IDE_RC startThread();
    
    // 아카이브 쓰레드를 중지하고, 쓰레드가 정상적으로
    // 중지되었을 때까지 기다린다.
    IDE_RC shutdown();

    // 아카이브 쓰레드 객체를 초기화 한다.
    IDE_RC initialize( const SChar   * aArchivePath,
                       smrLogFileMgr * aLogFileMgr,
                       UInt            aLstArchFileNo);

    // 서버 스타트업 시에 아카이브할 로그파일 리스트를 재구축한다.
    IDE_RC recoverArchiveLogList( UInt aStartNo,
                                  UInt aEndNo );
    
    // 아카이브 쓰레드 객체를 해제 한다.
    IDE_RC destroy();
    
    IDE_RC lockListMtx() { return mMtxArchList.lock( NULL ); }
    IDE_RC unlockListMtx() { return mMtxArchList.unlock(); }

    IDE_RC lockThreadMtx() { return mMtxArchThread.lock( NULL ); }
    IDE_RC unlockThreadMtx() { return mMtxArchThread.unlock(); }
    
    // 아카이브 쓰레드를 깨워서
    // 현재 아카이브 대상인 로그파일들을 아카이브 시킨다.
    IDE_RC wait4EndArchLF( UInt aToFileNo );

    smrArchThread();
    virtual ~smrArchThread();

//For Member
private:
    // 아카이브 로그가 저장될 디렉토리
    // Log File Group당 하나의 unique한 아카이브 디렉토리가 필요하다.
    const SChar            * mArchivePath[SM_ARCH_MULTIPLEX_PATH_CNT + 1];
    // 이 아카이브 쓰레드가 아카이브할 로그파일들을 관리하는 로그파일 관리자
    smrLogFileMgr          * mLogFileMgr;
    
    // 잠들어 있는 아카이브 쓰레드를 깨우기위한 condition value
    iduCond                  mCv;

    // smrArchLogFile의 할당/해제를 담당할 mempool
    iduMemPool               mMemPool;

    // 아카이브 쓰레드를 중지해야 할 지의 여부
    idBool                   mFinish;
    // 아카이브 쓰레드가 동작중인지 여부
    idBool                   mResume;

    // mLstArchFileNo 와 mArchFileList 에 대한 동시성 제어를 위한 Mutex
    iduMutex                 mMtxArchList;
    // 아카이브 쓰레드의 동시성 제어를 위한 Mutex
    iduMutex                 mMtxArchThread;

    // 마지막 아카이브된 로그파일번호
    UInt                     mLstArchFileNo;
    // 아카이브할 로그파일 번호들을 지닌 리스트
    smrArchLogFile           mArchFileList;

    UInt                     mArchivePathCnt;

    smrArchMultiplexThread * mArchMultiplexThreadArea;
};

#endif
