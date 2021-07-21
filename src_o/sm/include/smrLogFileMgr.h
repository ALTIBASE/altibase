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
 * $Id: smrLogFileMgr.h 89697 2021-01-05 10:29:13Z et16 $
 **********************************************************************/

#ifndef _O_SMR_LOG_FILE_MGR_H_
#define _O_SMR_LOG_FILE_MGR_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smrLogFile.h>
#include <smrLogMultiplexThread.h>

class smrLFThread;


/* 여러개의 로그파일을 하나의 연속된 로그로 관리하는 클래스
 *
 * 로그파일 하나를 다 사용해서 다음 로그 파일로 switch할 경우의 비용을
 * 최소화 하기 위해서 앞으로 사용할 로그파일들을 미리 준비해 놓는
 * log file prepare 쓰레드의 역할도 수행한다.
 * 
 */
class smrLogFileMgr : public idtBaseThread
{
//For Operation
public:
    /* log file prepare 쓰레드의 run함수.
     *
     * 로그파일 하나를 다 사용해서 다음 로그 파일로 switch할 경우의 비용을
     * 최소화 하기 위해서 앞으로 사용할 로그파일들을 미리 준비해 놓는다.
     *
     * 로그파일을 준비하는 일은 다음과 같이 두가지 경우에 실시한다.
     *   1. 특정 시간동안 기다려가며 주기적으로 실시
     *   2. preCreateLogFile 함수 호출에 의해서
     */
    virtual void run();

    /* 로그파일을 오픈하여 해당 로그파일이
     * 오픈된 로그파일 리스트에 들어가도록 보장한다.
     */
    
    IDE_RC open(UInt                aFileNo,
                idBool              aWrite,
                smrLogFile**        aLogFile);
    
    /* 로그파일을 참조하는 쓰레드가 더 이상 없는 경우,
     * 해당 로그파일을 close하고 로그파일 list에서 close한 로그파일을 제거한다.
     */ 

    IDE_RC close(smrLogFile*  aLogFile);

    /* 로그파일을 오픈하여 디스크에서 읽어 온다 */ 
    IDE_RC openLstLogFile( UInt          aFileNo,
                           UInt          aOffset,
                           smrLogFile ** aLogFile );
    
    IDE_RC closeAllLogFile();

    /* smuProperty::getLogFilePrepareCount()를 유지하도록 로그파일을
     * 미리 생성한다.
     */
    IDE_RC addEmptyLogFile();

    /* log file prepare 쓰레드를 깨워서 로그파일을 미리 생성해 둔다.
     *
     * aWait => ID_TRUE 이면 해당 로그파일이 생성될 때까지 기다린다.
     *          ID_FALSE 이면 로그파일을 만들라는 요청만 하고 바로 리턴한다.
     */
    IDE_RC preCreateLogFile( idBool aWait );
    
    /* 이전에 prepare 해 두었던 로그파일들을 모두 open한다.
     */
    IDE_RC preOpenLogFile();

    /* 현재 기록중인 로그파일의 다음 로그파일을
     * 새로 기록할 로그파일로 설정한다.
     * switch할 로그파일이 존재하지 않을 경우, 로그파일을 새로 생성한다.
     */
    IDE_RC switchLogFile( smrLogFile**  sCurLogFile );
    
    /* 로그파일 prepare 쓰레드를 중지한다.
     */
    IDE_RC shutdown();

    /* 로그파일 리스트에서 로그파일을 찾는다.
     */
    idBool findLogFile( UInt           aLogFileNo, 
                        smrLogFile**   aLogFile);


    /* 로그파일 객체를 로그파일 리스트에 추가한다.
     */
    IDE_RC addLogFile( smrLogFile *aNewLogFile );

    /* 시스템에서 로그관리자 초기화시에 호출되는 함수로써
     * 사용하던 로그파일 준비하고, 로그파일 관리자 thread를
     * 시작한다.
     */
    IDE_RC startAndWait( smLSN       * aEndLSN, 
                         UInt          aLstCreateLog,
                         idBool        aRecovery,
                         smrLogFile ** aLogFile );

    /* 로그파일을 로그파일 리스트에서 제거한다.
     * 주의! thread-safe하지 않은 함수임.
     */
    inline void removeLogFileFromList( smrLogFile *aLogFile);

    /* CREATE DB 수행시에 호출되며, 0번째 로그파일을 생성한다.
     *
     * aLogPath - [IN] 로그파일들을 생성할 경로
     */
    static IDE_RC create( const SChar * aLogPath );


    /*   aPrvLogFile의 바로 뒤에 aNewLogFile을 끼워넣는다.
     */
    IDE_RC AddLogFileToList( smrLogFile *aPrvLogFile,
                             smrLogFile *aNewLogFile );

    /* 로그파일들을 DISK에서 제거한다.
     * checkpoint도중 로그파일 삭제가 실패하면 에러상황으로 처리하고,
     * restart recovery시에 로그파일 삭제가 실패하면 정상상황으로 처리한다.
     */
    void removeLogFile( UInt   aFstFileNo, 
                        UInt   aLstFileNo, 
                        idBool aIsCheckpoint );

    inline void setFileNo( UInt  aFileNo, 
                           UInt  aTargetFileNo,
                           UInt  aLstFileNo)
    {
        mCurFileNo    = aFileNo;
        mTargetFileNo = aTargetFileNo;
        mLstFileNo    = aLstFileNo;
    }

    inline void getLstLogFileNo( UInt *aLstFileNo )
    {
        *aLstFileNo = mLstFileNo;
    }

    inline void getCurLogFileNo( UInt *aCurFileNo )
    {
        *aCurFileNo = mCurFileNo;
    }

    /* 로그파일 번호에 해당하는 로그 데이터가 있는지 체크한다.
     */
    IDE_RC checkLogFile( UInt aFileNo );

    // 로그파일 관리자를 초기화 한다.
    IDE_RC initialize( const SChar     * aLogPath, 
                       const SChar     * aArchivePath,
                       smrLFThread     * aLFThread );
#if 0 // not used
    // aFileNo에 해당하는 Logfile이 존재하는지 Check한다.
    IDE_RC isLogFileExist(UInt aFileNo, idBool & aIsExist);
#endif
    // 로그파일 관리자를 해제한다.
    IDE_RC destroy();

    /*
      aFileNo에 해당하는 Logfile이 Open되었는지 check하고
      Open되어있다면 LogFile의 Reference Count를 증가시킨다.
    */
    IDE_RC checkLogFileOpenAndIncRefCnt(UInt         aFileNo,
                                        idBool     * aAlreadyOpen,
                                        smrLogFile** aLogFile);

    static void removeLogFiles( UInt          aFstFileNo, 
                                UInt          aLstFileNo, 
                                idBool        aIsCheckpoint,
                                idBool        aIsMultiplexLogFile,
                                const SChar * aLogPath );

    smrLogFileMgr();
    virtual ~smrLogFileMgr();

    inline UInt getLFOpenCnt() {return mLFOpenCnt;}
    
    inline UInt getLFPrepareWaitCnt() {return mLFPrepareWaitCnt;}

    inline UInt getLFPrepareCnt() {return smuProperty::getLogFilePrepareCount();}
    
    inline UInt getLstPrepareLFNo() {return mLstFileNo;}
    
    inline UInt getCurWriteLFNo() {return mCurFileNo;}

    inline const SChar * getSrcLogPath() { return mSrcLogPath;}

private:
    inline IDE_RC lockListMtx() { return mMtxList.lock( NULL ); }
    inline IDE_RC unlockListMtx() { return mMtxList.unlock(); }

    inline IDE_RC lock() { return mMutex.lock( NULL ); }
    inline IDE_RC unlock() { return mMutex.unlock(); }

//For Log Mgr
private:
    
    // 이 로그파일 관리자에 속한 로그파일들을 Flush해줄 로그파일 Flush 쓰레드
    smrLFThread     * mLFThread;
    
    // log file prepare thread를 종료해야 하는 상황인지 여부
    // mFinish == ID_TRUE 인 조건에만 thread가 실행되어야 한다.
    idBool       mFinish;
    
    // log file prepare thread가 log file 생성작업중인지 여부
    // mResume == ID_FALSE 이면 thread가 sleep상태에 있는 것이다.
    idBool       mResume;

    // log file prepare thread와 다른 thread간의 동시성제어를 위한
    // Condition value.
    // mMutex와 함께 사용된다.
    iduCond      mCV;
    
    // 현재 로그를 기록하고 있는 로그파일번호
    UInt         mCurFileNo;
    
    // 마지막으로 prepare된 Log file번호
    UInt         mLstFileNo;
    
    // 첫번째 prepared된 Log file번호
    // 항상 mCurFileNo + 1의 값을 지닌다.
    UInt         mTargetFileNo;

    // log file prepare thread와 다른 thread간의 동시성제어를 위한 Mutex
    iduMutex     mMutex;

    // mFstLogFile및 mOpenLogFileCount 에 대한 동시성 제어를 위한 Mutex
    iduMutex     mMtxList;

    // 현재 열려있는 로그파일 갯수
    UInt         mOpenLogFileCount;
    
    // 현재 열려있는 로그파일의 linked list
    // 이 링크드 리스트의 동작방식은
    // smrLFThread의  mSyncLogFileList 에 대한 주석에 자세히 기술되어 있다.
    smrLogFile   mFstLogFile;

    // smrLogFile 객체가 사용할 메모리 영역을 지니는 memory pool
    // iduMemPool자체가 thread safe하므로, 별도의 동시성 제어를 할 필요가 없다.
    iduMemPool   mMemPool;

    // 로그파일을 초기화할 데이터를 지니는 버퍼
    // OS PAGE 크기만큼 align되어 있다.
    SChar*       mLogFileInitBuffer;
    
    // mLogFileInitBuffer 가 실제로 할당된 align되지 않은 주소.
    SChar*       mLogFileInitBufferPtr;

    // 현재 Open된 LogFile의 갯수
    UInt         mLFOpenCnt;

    // log switch 발생시 wait event가 발생한 횟수
    UInt         mLFPrepareWaitCnt;

    const SChar*  mSrcLogPath;
    const SChar*  mArchivePath;

    SChar        mTempLogFileName[SM_MAX_FILE_NAME]; /* BUG-49409 */

public:
    static smrLogMultiplexThread * mSyncThread;
    static smrLogMultiplexThread * mCreateThread;
    static smrLogMultiplexThread * mDeleteThread;
    static smrLogMultiplexThread * mAppendThread;
};


/* 로그파일을 로그파일 리스트에서 제거한다.
 * 주의! thread-safe하지 않은 함수임.
 */
inline void smrLogFileMgr :: removeLogFileFromList( smrLogFile*  aLogFile )
{
    aLogFile->mPrvLogFile->mNxtLogFile = aLogFile->mNxtLogFile;
    aLogFile->mNxtLogFile->mPrvLogFile = aLogFile->mPrvLogFile;

    mLFOpenCnt--;
}

inline void smrLogFileMgr::removeLogFiles( UInt          aFstFileNo, 
                                           UInt          aLstFileNo, 
                                           idBool        aIsCheckPoint,
                                           idBool        aIsMultiplexLogFile,
                                           const SChar * aLogPath )
{
    UInt    i               = 0;
    UInt    sStartFileNum   = 0;
    SChar   sLogFileName[ SM_MAX_FILE_NAME ];

    IDE_ASSERT( aLogPath != NULL );

    sStartFileNum   = aFstFileNo;

    /* BUG-42589: 로그 파일 삭제 할 때 체크포인트 상황이 아니어도(restart recovery)
     * 로그 파일 삭제 범위를 트레이스 로그로 남긴다. */
    if ( aIsCheckPoint == ID_FALSE )
    {
        ideLog::log( IDE_SM_0, "Remove Log File : File[<%"ID_UINT32_FMT"> ~ <%"ID_UINT32_FMT">]\n", aFstFileNo, aLstFileNo-1 );
    }
    else
    {
        // do nothing
    }

#if defined(VC_WINCE)
    //fix ERROR_SHARING_VIOLATION error

    if( (aFstFileNo - smuProperty::getChkptIntervalInLog()) > 0 )
    {
        sStartFileNum   = aFstFileNo - smuProperty::getChkptIntervalInLog();
    }
    else
    {
        sStartFileNum   = 0;
    }

    for( i = sStartFileNum ; i < aLstFileNo ; i++ )
#else
    for( i = sStartFileNum ; i < aLstFileNo ; i++ )
#endif
    {
        idlOS::snprintf(sLogFileName,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT, 
                        aLogPath, 
                        IDL_FILE_SEPARATOR, 
                        SMR_LOG_FILE_NAME, 
                        i);

        if( (aIsMultiplexLogFile == ID_TRUE) && 
            (idf::access( sLogFileName, F_OK) != 0) )
        {
            continue;
        }

        if(smrLogFile::remove(sLogFileName, aIsCheckPoint) != IDE_SUCCESS)
        {
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_MRECOVERY_LOGFILEMGR_CANNOT_REMOVE_LOGFILE,
                        sLogFileName,
                        (UInt)errno);
        }
    }
}

#endif /* _O_SMR_LOG_FILE_MGR_H_ */
