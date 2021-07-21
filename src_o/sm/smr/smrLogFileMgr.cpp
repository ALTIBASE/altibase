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
 * $Id: smrLogFileMgr.cpp 89697 2021-01-05 10:29:13Z et16 $
 **********************************************************************/

#include <smErrorCode.h>
#include <smm.h>
#include <smr.h>
#include <smiMain.h>

smrLogMultiplexThread * smrLogFileMgr::mSyncThread;
smrLogMultiplexThread * smrLogFileMgr::mCreateThread;
smrLogMultiplexThread * smrLogFileMgr::mDeleteThread;
smrLogMultiplexThread * smrLogFileMgr::mAppendThread;

smrLogFileMgr::smrLogFileMgr() : idtBaseThread()
{

}

smrLogFileMgr::~smrLogFileMgr()
{

}

/* CREATE DB 수행시에 호출되며, 0번째 로그파일을 생성한다.
 *
 * aLogPath - [IN] 로그파일들을 생성할 경로
 */
IDE_RC smrLogFileMgr::create( const SChar * aLogPath )
{
    smrLogFile     sLogFile;
    SChar          sLogFileName[SM_MAX_FILE_NAME];
    SChar        * sAlignedsLogFileInitBuffer = NULL;
    SChar        * sLogFileInitBuffer = NULL;
    const SChar ** sLogMultiplexPath;
    UInt           sLogMultiplexIdx;
    UInt           sMultiplexCnt;

    // BUG-14625 [WIN-ATAF] natc/TC/Server/sm4/sm4.ts가
    // 동작하지 않습니다.
    //
    // 만약 Direct I/O를 한다면 Log Buffer의 시작 주소 또한
    // Direct I/O Page크기에 맞게 Align을 해주어야 한다.
    // 이에 대비하여 로그 버퍼 할당시 Direct I/O Page 크기만큼
    // 더 할당한다.

    IDE_TEST( iduFile::allocBuff4DirectIO(
                  IDU_MEM_SM_SMR,
                  smuProperty::getFileInitBufferSize(),
                  (void**)&sLogFileInitBuffer,
                  (void**)&sAlignedsLogFileInitBuffer )
              != IDE_SUCCESS );

    //create log file 0
    idlOS::snprintf( sLogFileName, 
                     SM_MAX_FILE_NAME,
                     "%s%c%s0",
                     aLogPath,
                     IDL_FILE_SEPARATOR,
                     SMR_LOG_FILE_NAME );

    IDE_TEST( sLogFile.initialize() != IDE_SUCCESS );

    IDE_TEST_RAISE( idf::access(sLogFileName, F_OK) == 0,
                    error_already_exist_logfile );

    IDE_TEST( sLogFile.create( sLogFileName,
                               sAlignedsLogFileInitBuffer,
                               smuProperty::getLogFileSize() ) != IDE_SUCCESS );

    sLogMultiplexPath = smuProperty::getLogMultiplexDirPath();
    sMultiplexCnt     = smuProperty::getLogMultiplexCount();

    for( sLogMultiplexIdx = 0; 
         sLogMultiplexIdx < sMultiplexCnt;
         sLogMultiplexIdx++ )
    {
        idlOS::snprintf(sLogFileName, SM_MAX_FILE_NAME,
                        "%s%c%s0",
                        sLogMultiplexPath[sLogMultiplexIdx],
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME);

        IDE_TEST( sLogFile.create( sLogFileName,
                                   sAlignedsLogFileInitBuffer,
                                   smuProperty::getLogFileSize() ) 
                  != IDE_SUCCESS );
    }


    IDE_TEST( sLogFile.destroy() != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free(sLogFileInitBuffer) != IDE_SUCCESS );

    sLogFileInitBuffer = NULL;
    sAlignedsLogFileInitBuffer = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_exist_logfile )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistLogFile ) );
    }
    IDE_EXCEPTION_END;

    if ( sLogFileInitBuffer != NULL )
    {
        IDE_ASSERT( iduMemMgr::free(sLogFileInitBuffer) == IDE_SUCCESS );
        sLogFileInitBuffer = NULL;
        sAlignedsLogFileInitBuffer = NULL;
    }

    return IDE_FAILURE;

}
/***********************************************************************
 * Description : 로그파일 관리자를 초기화 한다.
 *
 * aLogPath     - [IN] 로그가 저장될 디렉토
 * aArchLogPath - [IN] 아카이브 로그가 저장될 디렉토
 * aLFThread    - [IN] 이 로그파일 관리자에 속한 로그파일들을
                       Flush해줄 로그파일 Flush 쓰레드
 ***********************************************************************/
IDE_RC smrLogFileMgr::initialize( const SChar     * aLogPath,
                                  const SChar     * aArchivePath,
                                  smrLFThread     * aLFThread )
{

    UInt sPageSize;
    UInt sPageCount;
    UInt sBufferSize;

    IDE_DASSERT( aArchivePath != NULL);
    IDE_DASSERT( aLogPath  != NULL );
    IDE_DASSERT( aLFThread != NULL );
    
    mLFThread    = aLFThread;
    mSrcLogPath  = aLogPath;
    mArchivePath = aArchivePath; 
    
    mLogFileInitBufferPtr = NULL;
    mLogFileInitBuffer    = NULL;

    /* BUG-48409 prepare logfile에 사용할 임시 로그 파일명을
     * ogfile Group 별로 임피 파일명을 미리 만들어둔다.*/
    idlOS::snprintf( mTempLogFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s%",
                     aLogPath,
                     IDL_FILE_SEPARATOR,
                     SMR_TEMP_LOG_FILE_NAME );

    IDE_TEST( smrLogMultiplexThread::initialize( &mSyncThread,
                                                 &mCreateThread,
                                                 &mDeleteThread,
                                                 &mAppendThread,
                                                 mSrcLogPath )
              != IDE_SUCCESS );

    
    IDE_TEST( mMutex.initialize((SChar*)"LOGFILE_MANAGER_MUTEX",
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    IDE_TEST( mMtxList.initialize((SChar*)"OPEN_LOGFILE_LIST_MUTEX",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    IDE_TEST( mMemPool.initialize(IDU_MEM_SM_SMR,
                                 (SChar*)"OPEN_LOGFILE_MEM_LIST",
                                 1,                                 /* aListCount */
                                 ID_SIZEOF(smrLogFile),             /* aElemSize  */
                                 SMR_LOG_FULL_SIZE,                 /* aElemCount */
                                 IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                 ID_TRUE,							/* UseMutex   */
                                 IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte  */
                                 ID_FALSE,							/* ForcePooling */
                                 ID_TRUE,							/* GarbageCollection */
                                 ID_TRUE,                           /* HWCacheLine */
                                 IDU_MEMPOOL_TYPE_LEGACY            /* mempool type */) 
               != IDE_SUCCESS);			
    
    IDE_TEST_RAISE( mCV.initialize((SChar *)"LOGFILE_MANAGER_COND")
                    != IDE_SUCCESS, err_cond_var_init );

    sPageSize   = idlOS::getpagesize();
    sPageCount  = (smuProperty::getFileInitBufferSize() + sPageSize - 1) / sPageSize;
    sBufferSize = sPageCount * sPageSize;

    // BUG-14625 [WIN-ATAF] natc/TC/Server/sm4/sm4.ts가
    // 동작하지 않습니다.
    //
    // 만약 Direct I/O를 한다면 Log Buffer의 시작 주소 또한
    // Direct I/O Page크기에 맞게 Align을 해주어야 한다.
    // 이에 대비하여 로그 버퍼 할당시 Direct I/O Page 크기만큼
    // 더 할당한다.
    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMR,
                                           sBufferSize,
                                           (void**)&mLogFileInitBufferPtr,
                                           (void**)&mLogFileInitBuffer )
              != IDE_SUCCESS );

    mFstLogFile.mPrvLogFile = &mFstLogFile;
    mFstLogFile.mNxtLogFile = &mFstLogFile;

    mFinish = ID_FALSE;
    mResume = ID_FALSE;

    // 현재 Open된 LogFile의 갯수
    mLFOpenCnt = 0;

    // log switch 발생시 wait event가 발생한 횟수 
    mLFPrepareWaitCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;
    
    if ( mLogFileInitBufferPtr != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mLogFileInitBufferPtr )
                    == IDE_SUCCESS );
        
        mLogFileInitBuffer = NULL;
        mLogFileInitBufferPtr = NULL;
    }
    
    return IDE_FAILURE;

}

IDE_RC smrLogFileMgr::destroy()
{
    
    smrLogFile *sCurLogFile;
    smrLogFile *sNxtLogFile;

    IDE_TEST( smrLogMultiplexThread::destroy( mSyncThread, 
                                              mCreateThread, 
                                              mDeleteThread, 
                                              mAppendThread )
              != IDE_SUCCESS );
    
    sCurLogFile = mFstLogFile.mNxtLogFile;
    
    while ( sCurLogFile != &mFstLogFile )
    {
        sNxtLogFile = sCurLogFile->mNxtLogFile;
       
        IDE_TEST( sCurLogFile->close() != IDE_SUCCESS );
        IDE_TEST( sCurLogFile->destroy() != IDE_SUCCESS );
        
        IDE_TEST( mMemPool.memfree(sCurLogFile) != IDE_SUCCESS );
        
        sCurLogFile = sNxtLogFile;
    }

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    IDE_TEST( mMtxList.destroy() != IDE_SUCCESS );
    IDE_TEST( mMemPool.destroy() != IDE_SUCCESS );

    IDE_ASSERT(mLogFileInitBuffer != NULL);

    IDE_TEST( iduMemMgr::free( mLogFileInitBufferPtr )
              != IDE_SUCCESS );
    
    mLogFileInitBuffer = NULL;
    mLogFileInitBufferPtr = NULL;

    IDE_TEST_RAISE( mCV.destroy() != IDE_SUCCESS, err_cond_var_destroy );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* ------------------------------------------------
 * Description :
 *
 * 시스템에서 로그관리자 초기화시에 호출되는 함수로써
 * 사용하던 로그파일 준비하고, 로그파일 관리자 thread를
 * 시작한다.
 *
 * - 로그파일이 존재하지 않을 경우, 미리 특정 개수
 * (PREPARE_LOG_FILE_COUNT)만큼의 빈 로그파일들을 생성하여
 * 유지한다.
 * - 마지막 사용한 로그파일을 오픈한다.
 * - 지금 생성하고 오픈한 로그파일이 아니고, 그냥 오픈한 로그파일이면
 * sync 로그파일 list에 등록한다.
 * - 미리 오픈할 로그파일을 오픈하여 로그파일 list와 sync 
 * 로그파일 list 등록한다.
 * - 로그파일관리자 thread를 시작한다.
 *
 * aEndLSN          [IN] - Redo가 완료된 시점의 LSN (비정상 종료) 혹은
 *                         Loganchor에 저장된 서버종료 시점의 LSN (정상 종료)
 * aLSTCreatedLF    [IN] - Redo가 완료된 시점의 로그파일 번호(비정상종료) 혹은
 *                         Loganchor에 저장된 가장 최신에 생성된 
 *                         로그파일번호(정상종료)
 * aRecovery        [IN] - Recover수행 여부
 *
 * ----------------------------------------------*/
IDE_RC smrLogFileMgr::startAndWait( smLSN       * aEndLSN, 
                                    UInt          aLstCreatedLF,
                                    idBool        aRecovery,
                                    smrLogFile ** aLogFile )
{

    smrLogFile    * sLogFile          = NULL;
    smrLogFile    * sMultiplexLogFile = NULL;
    UInt            sLogMultiplexIdx;
    SChar           sLogFileName[ SM_MAX_FILE_NAME ];
    SInt            sState  = 0;

    setFileNo( aEndLSN->mFileNo, 
               aEndLSN->mFileNo + 1, 
               aLstCreatedLF );

    idlOS::snprintf( sLogFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s%"ID_UINT32_FMT"", 
                     mSrcLogPath, 
                     IDL_FILE_SEPARATOR,
                     SMR_LOG_FILE_NAME, 
                     aEndLSN->mFileNo );

    /* -------------------------------------------------------
     * [1] If the last log file doesn't exist, 
     *     the log file must be created before any processing
     *
     * aEndLSN이 속하는 로그파일이 존재하지 않을 경우,
     * 해당 log 파일을 새로 생성해준다.
     *
     * aEndLSN에 해당하는 로그파일이 없는 경우는
     * 해당 로그파일이 flush되기 전에 비정상 종료한 경우 
     * 혹은 서버 정상종료후 로그파일을 삭제한 경우이다.
     ------------------------------------------------------- */
    if ( idf::access(sLogFileName, F_OK) != 0 )
    {
        mLstFileNo = mCurFileNo -1;

        /* 로그파일을 생성해주면서 생성된 로그파일을 open한다. */
        IDE_TEST( addEmptyLogFile() != IDE_SUCCESS );
        sState = 1; 
    }
    else
    {
        /*  file exists */
    }

    /* -----------------------------------------------------
     * [2] open the last used log file 
     *
     * 로그파일 리스트를 검사하여 이미 로그파일이 open되어있을경우 
     * open하지않고 로그파일 리스트에 존재하는 로그파일을 가져온다.
     ----------------------------------------------------- */
    IDE_TEST( openLstLogFile( aEndLSN->mFileNo, 
                              aEndLSN->mOffset, 
                              &sLogFile) 
              != IDE_SUCCESS );

    *aLogFile = sLogFile;

    if ( smrLogMultiplexThread::mMultiplexCnt != 0 )
    {
        smrLogMultiplexThread::setOriginalCurLogFileNo( aEndLSN->mFileNo );

        for( sLogMultiplexIdx = 0; 
             sLogMultiplexIdx < smrLogMultiplexThread::mMultiplexCnt;
             sLogMultiplexIdx++ )
        { 
            /* 다중화 로그파일중 마지막 log file 이전 log file중 invalid한 
             * log file이 존재하면 복원한다.*/
            IDE_TEST( mCreateThread[sLogMultiplexIdx].recoverMultiplexLogFile( 
                                                                aEndLSN->mFileNo)
                      != IDE_SUCCESS );

            /* 마지막 다중화 log file을 open한다. */
            IDE_TEST( mCreateThread[sLogMultiplexIdx].openLstLogFile( 
                                                  aEndLSN->mFileNo,
                                                  aEndLSN->mOffset,
                                                  sLogFile,
                                                  &sMultiplexLogFile )
                      != IDE_SUCCESS );

            mAppendThread[sLogMultiplexIdx].setCurLogFile( 
                                                    sMultiplexLogFile,
                                                    sLogFile );

            IDE_TEST( mAppendThread[sLogMultiplexIdx].startThread() 
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* nothing to do */
    }
    
    if ( sState == 0 )
    {
        // log flush 쓰레드에게 flush(sync)대상 로그파일로 등록한다.
        IDE_TEST( mLFThread->addSyncLogFile(sLogFile) != IDE_SUCCESS );

        /* -----------------------------------------------------
         * [3] 이전에 prepare 해 두었던 로그파일들을 모두 open한다.
         *
         * BUG-35043 LOG_DIR에 로그파일 없다면, 서버 시작시에 생성된 로그파일이 
         * 두 번 open됩니다. 
         * preOpenLogFile함수에서 open할 로그파일이 로그파일 리스트에 존재하는지
         * 확인하지 않습니다. 이로인해 정상종료후 로그파일이 없는 상태에서
         * 서버를 시작하면 [1]에서 open된 prepare로그파일이 다시한번 open되어
         * 로그파일리스트에 동일한 로그파일이 2개 존재하게됩니다.
         ----------------------------------------------------- */
        if ( ( mTargetFileNo <= mLstFileNo ) && 
             ( aRecovery == ID_FALSE ) ) 
        {
            IDE_TEST( preOpenLogFile() != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
    }

    /* ------------------------------------------------------------------------
     * [4] Log File Create Thread would be created 
     ---------------------------------------------------------------------- */
    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    while(1)
    {
        // smuProperty::getLogFilePrepareCount() 갯수만큼
        // 로그파일을 미리 생성해 둔다.
        if ( mTargetFileNo + smuProperty::getLogFilePrepareCount() <=
            mLstFileNo + 1)
        {
            break;
        }
        // log file prepare thread에게 로그파일을 하나 만들도록 요청하고,
        // 로그파일이 하나 다 만들어질 때까지 기다린다.
        IDE_TEST( preCreateLogFile(ID_TRUE) != IDE_SUCCESS );
        idlOS::thr_yield();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aFileNo가 가리키는 LogFile(보통 마지막 파일)을 Open한다.
 *               aLogFilePtr에 Open된 Logfile Pointer를 Setting해준다.
 *
 * aFileNo     - [IN] open할 LogFile No
 * aOffset     - [IN] 로그가 기록될 위치 
 * aLogFilePtr - [OUT] open된 logfile를 가리킨다.
 **********************************************************************/
IDE_RC smrLogFileMgr::openLstLogFile( UInt          aFileNo,
                                      UInt          aOffset,
                                      smrLogFile ** aLogFile )
{
    /* ---------------------------------------------
     * In server booting, mRestart set to ID_TRUE
     * apart from restart recovery.
     * In durability-1, only when server booting,
     * log file is read from disk.
     * --------------------------------------------- */
    IDE_TEST( open( aFileNo,
                    ID_TRUE /*For Write*/,
                    aLogFile ) != IDE_SUCCESS);

    (*aLogFile)->setPos(aFileNo, aOffset);

    /* PROJ-2162 RestartRiskReduction
     * DB가 Consistent하지 않으면, LogFile을 갱신하지 않는다. */
    if( ( smrRecoveryMgr::getConsistency() == ID_TRUE ) ||
        ( smuProperty::getCrashTolerance() == 2 ) )
    {
        /* 마지막 Redo한 위치 이후를 싹 지워버림.
         * 하지만 Inconsistency하기 때문에 지우면 안됨 */
        (*aLogFile)->clear(aOffset);
    }
    else
    {
        /* nothing to do ... */
    }

    
    if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MMAP )
    {
        (void)(*aLogFile)->touchMMapArea();
    }
    else
    {
        IDE_TEST( (*aLogFile)->readFromDisk( 0,
                                            (SChar*)((*aLogFile)->mBase),
                                            /* BUG-15532: 윈도우 플랫폼에서 DirectIO를 사용할경우
                                             * 서버 시작실패 IO크기가 Sector Size로 Align되어야됨 */
                                            idlOS::align( aOffset, 
                                                          iduProperty::getDirectIOPageSize() ) )
                   != IDE_SUCCESS);
    }
    /*
     * To Fix BUG-11450  LOG_DIR, ARCHIVE_DIR 의 프로퍼티 내용이 변경되면
     *                   DB가 깨짐
     *
     * 로그파일의 File Begin Log가 정상인지 체크한다.
     *
     * 사용자가 정상 Shutdown한 후에 Log Directory위치들을 서로 바꿔칠 경우에
     * 대비하여 여기에서 Log File이 특정 FileNo에 해당하는
     * 로그파일인지 체크한다.
     */
    IDE_TEST( (*aLogFile)->checkFileBeginLog( aFileNo )
              != IDE_SUCCESS );

    // If the last log file is created in redoALL(),
    // the current m_cRef is 2 !!
    (*aLogFile)->mRef = 1;

    smrLogMgr::setLstLSN( aFileNo, aOffset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 이전에 prepare 해 두었던 로그파일들을 모두 open한다.
 *
 * 이 함수에서는 log file list에 대해 동시성을 제어할 필요가 없다.
 * 왜냐하면, log file list에 접근하고 있는 다른 thread가 없는 상황,
 * 즉, smrLogFileMgr::startAndWait 에서만 호출되기 때문이다.
 */
IDE_RC smrLogFileMgr::preOpenLogFile()
{
    SChar           sLogFileName[SM_MAX_FILE_NAME];
    smrLogFile    * sNewLogFile;
    UInt            sFstFileNo;
    UInt            sLstFileNo;
    UInt            sFileNo;
    smrLogFile    * sDummyLogFile;
    UInt            sLogMultiplexIdx;
    UInt            sState = 0;
    
    sFstFileNo = mCurFileNo + 1;  // prepare된 첫번째 로그파일
    sLstFileNo = mLstFileNo;      // 맨 마지막 로그파일

    for ( sFileNo=sFstFileNo ; sFileNo<=sLstFileNo ; sFileNo++ )
    {
        sState = 0;
        idlOS::snprintf(sLogFileName,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        mSrcLogPath, 
                        IDL_FILE_SEPARATOR, 
                        SMR_LOG_FILE_NAME, 
                        sFileNo);

        /* ----------------------------------------------------------------- 
         * In case server boot 
         * if some prepared log file does not exist this is not fatal error, 
         * so the logfiles
         * which are between the next log file of not existing log file
         * and the last created log file will be newly created!!
         * ---------------------------------------------------------------- */
        if ( idf::access(sLogFileName, F_OK) != 0 )
        {
            setFileNo(mCurFileNo, 
                      sFileNo,
                      sFileNo - 1);
            break;
        }

        /* 이미 등록된 경우 */
        if ( findLogFile( sFileNo, &sDummyLogFile ) == ID_TRUE )
        {
            /* nothing to do */
        }
        else
        {
            /* smrLogFileMgr_preOpenLogFile_alloc_NewLogFile.tc */
            IDU_FIT_POINT("smrLogFileMgr::preOpenLogFile::alloc::NewLogFile");
            IDE_TEST( mMemPool.alloc((void**)&sNewLogFile) != IDE_SUCCESS );
            sState = 1;
            IDE_TEST( sNewLogFile->initialize() != IDE_SUCCESS );
            sState = 2;
            
            IDE_TEST( sNewLogFile->open( sFileNo,
                                         sLogFileName,
                                         ID_FALSE, /* aIsMultiplexLogFile */
                                         (UInt)smuProperty::getLogFileSize(),
                                         ID_TRUE) != IDE_SUCCESS );
            sState = 3;
 
            //init log file info
            sNewLogFile->mFileNo   = sFileNo;
            sNewLogFile->mOffset   = 0;
            sNewLogFile->mFreeSize = smuProperty::getLogFileSize();
 
            (void)sNewLogFile->touchMMapArea();
            
            IDE_TEST( addLogFile(sNewLogFile) != IDE_SUCCESS );
            sState = 4;
        }
        
        for( sLogMultiplexIdx = 0; 
             sLogMultiplexIdx < smrLogMultiplexThread::mMultiplexCnt;
             sLogMultiplexIdx++ )
        {
            IDE_TEST( mCreateThread[sLogMultiplexIdx].preOpenLogFile( 
                                                sFileNo,
                                                mLogFileInitBuffer,
                                                smuProperty::getLogFileSize() )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
    case 4:
        (void)lockListMtx();
        (void)removeLogFileFromList(sNewLogFile);
        (void)unlockListMtx();

    case 3:
        (void)sNewLogFile->close();
        
    case 2:
        (void)sNewLogFile->destroy();
        
    case 1:
        (void)mMemPool.memfree(sNewLogFile); 
        break;
    }
    
    return IDE_FAILURE;
    
}

/* 로그파일을 오픈하여 해당 로그파일이
 * 오픈된 로그파일 리스트에 들어가도록 보장한다.
 *
 * 로그파일 하나를 open하는 과정은 DISK I/O를 수반하는 작업으로,
 * 로그파일을 open 전 과정동안 로그파일 list Mutex인,
 * mMtxList 를 잡고 있게 되면 동시성이 현저히 떨어지게 된다.
 *
 * 그래서 로그파일이 list에 없는지 확인한 후 새로운 로그파일객체를 하나 만들고
 * 해당 로그파일에 Mutex를 잡은 상태로 로그파일 리스트에 추가한 후,
 * 바로 로그파일 list Mutex를 풀어준다.
 * 그리고 로그파일이 open되어 모두 메모리로 올라온 후에,
 * 로그파일의 Mutex를 풀어주도록 한다.
 *
 * 이렇게 되면, 아직 open되지 않은 로그파일의 객체가 로그파일 list에 들어가
 * 있을 수 있다.
 * 로그파일 list안에 있는 하나의 로그파일에 대해
 * 로그파일 Mutex를 한번 잡아보도록 한다.
 *
 * 이는, 로그파일을 open하는 쓰레드가 해당 로그파일을 open중인경우,
 * 로그파일의 open이 완료될 때까지 기다리도록 한다.
 *
 * aFileNo  - [IN] open할 로그파일 번호
 * aWrite   - [IN] 쓰기모드로 open할지 여부
 * aLogFile - [OUT] open된 로그파일의 객체
 *
 */
IDE_RC smrLogFileMgr::open( UInt           aFileNo,
                            idBool         aWrite,
                            smrLogFile  ** aLogFile )
{

    smrLogFile* sNewLogFile;
    smrLogFile* sPrvLogFile;
    SChar       sLogFileName[SM_MAX_FILE_NAME];

    *aLogFile = NULL;

    IDE_ASSERT( lockListMtx() == IDE_SUCCESS );

    // aFileNo번의 로그파일이 open된 로그파일 list에 존재하는 경우
    if ( findLogFile(aFileNo, &sPrvLogFile) == ID_TRUE)
    {
        sPrvLogFile->mRef++;
        *aLogFile = sPrvLogFile;

        IDE_ASSERT( unlockListMtx() == IDE_SUCCESS );
        
        /* 아직 open되지 않은 로그파일의 객체가 로그파일 list에 들어가
         * 있을 수 있다.
         * 로그파일 list안에 있는 하나의 로그파일에 대해
         * 로그파일 Mutex를 한번 잡아보도록 한다.
         *
         * 이를 통해 로그파일을 open하는 쓰레드가 해당 로그파일을 open중인경우,
         * 로그파일의 open이 완료될 때까지 기다리도록 한다.
         */
        IDE_ASSERT( (*aLogFile)->lock() == IDE_SUCCESS );
        IDE_TEST_RAISE( (*aLogFile)->isOpened() == ID_FALSE,
                        err_wait_open_file);
        IDE_ASSERT( (*aLogFile)->unlock() == IDE_SUCCESS );
    }
    // aFileNo번의 로그파일이 open된 로그파일 list에 존재하지 않는 경우
    // sPrvLogFile 은 aFileNo로그 파일의 바로 앞 로그파일이다.
    else
    {
        /* BUG-36744
         * 새로운 smrLogFile 객체를 만들어야 한다.
         * 일단 unlockListMtx()를 하고, 객체를 생성한다. */
        IDE_ASSERT( unlockListMtx() == IDE_SUCCESS );

        if ( ( smrRecoveryMgr::getArchiveMode() == SMI_LOG_ARCHIVE ) &&
             ( smrRecoveryMgr::getLstDeleteLogFileNo() > aFileNo ) )
        {

            // 다음 버그의 재현을 위해 4.3.7과 동일하게
            // ASSERT코드를 넣습니다.
            //
            // BUG-17541 [4.3.7] 운영중 로그파일 Open하다가
            // 이미 삭제된 로그파일이어서 ASSERT맞고 사망
            IDE_ASSERT( smiGetStartupPhase() == SMI_STARTUP_CONTROL );
            
            idlOS::snprintf( sLogFileName,
                             SM_MAX_FILE_NAME,
                             "%s%c%s%"ID_UINT32_FMT,
                             mArchivePath,
                             IDL_FILE_SEPARATOR, 
                             SMR_LOG_FILE_NAME, 
                             aFileNo );
        }
        else
        {
            idlOS::snprintf(sLogFileName,
                            SM_MAX_FILE_NAME,
                            "%s%c%s%"ID_UINT32_FMT, 
                            mSrcLogPath, 
                            IDL_FILE_SEPARATOR, 
                            SMR_LOG_FILE_NAME, 
                            aFileNo);
        }

        IDE_ASSERT(mMemPool.alloc((void**)&sNewLogFile)
                   == IDE_SUCCESS );
        
        new (sNewLogFile) smrLogFile();
 
        IDE_ASSERT( sNewLogFile->initialize() == IDE_SUCCESS );
        
        sNewLogFile->mFileNo             = aFileNo;

        IDE_ASSERT( lockListMtx() == IDE_SUCCESS );

        /* List에 존재하는지 다시 한번 확인한다. */
        if ( findLogFile(aFileNo, &sPrvLogFile) == ID_TRUE)
        {
            sPrvLogFile->mRef++;
            *aLogFile = sPrvLogFile;

            IDE_ASSERT( unlockListMtx() == IDE_SUCCESS );

            IDE_ASSERT( sNewLogFile->destroy() == IDE_SUCCESS );
            IDE_ASSERT( mMemPool.memfree(sNewLogFile)
                        == IDE_SUCCESS );

            IDE_ASSERT( (*aLogFile)->lock() == IDE_SUCCESS );
            IDE_TEST_RAISE( (*aLogFile)->isOpened() == ID_FALSE,
                            err_wait_open_file);
            IDE_ASSERT( (*aLogFile)->unlock() == IDE_SUCCESS );
        }
        else
        {
            *aLogFile = sNewLogFile;

            // Add log file to log file list
            (void)AddLogFileToList(sPrvLogFile, sNewLogFile); 

            // 로그파일 list의 Mutex를 풀고 로그파일을 open하는 작업을
            // 계속 수행하기 위해 해당 로그파일에 Mutex를 잡는다.
            IDE_ASSERT( sNewLogFile->lock() == IDE_SUCCESS );

            sNewLogFile->mRef++;

            // 로그파일을 open하기 전에 로그파일 list의 Mutex를 풀어준다.
            IDE_ASSERT( unlockListMtx() == IDE_SUCCESS );

            IDE_TEST( sNewLogFile->open( aFileNo,
                                         sLogFileName,
                                         ID_FALSE, /* aIsMultiplexLogFile */
                                         smuProperty::getLogFileSize(),
                                         aWrite ) != IDE_SUCCESS );

            IDE_ASSERT( sNewLogFile->unlock() == IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_wait_open_file);
    {
        errno = 0;
        IDE_SET( ideSetErrorCode( smERR_ABORT_WaitLogFileOpen,
                                 (*aLogFile)->getFileName() ) );
    }
    IDE_EXCEPTION_END;

    IDE_ASSERT( lockListMtx() == IDE_SUCCESS );

    (*aLogFile)->mRef--;

    if ( (*aLogFile)->mRef == 0 )
    {
        removeLogFileFromList(*aLogFile);
        /* open을 수행하던 Thread가 open이 실패하자 OpenFileList에서
           객체만 제거하고 나감. 때문에 Resource에 대한 정리가 필요함.*/
        IDE_ASSERT( (*aLogFile)->unlock() == IDE_SUCCESS );
        IDE_ASSERT( (*aLogFile)->destroy() == IDE_SUCCESS );
        IDE_ASSERT( mMemPool.memfree((*aLogFile)) == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( (*aLogFile)->unlock() == IDE_SUCCESS );
    }

    IDE_ASSERT( unlockListMtx() == IDE_SUCCESS );
    
    *aLogFile = NULL;
    
    return IDE_FAILURE;

}

/* 로그파일을 참조하는 쓰레드가 더 이상 없는 경우,
 * 해당 로그파일을 close하고 로그파일 list에서 close한 로그파일을 제거한다.
 *
 * aLogFile - [IN] close할 로그파일
 */ 
IDE_RC smrLogFileMgr::close(smrLogFile *aLogFile)
{

    UInt sState = 0;
    
    if ( aLogFile != NULL )
    {
        IDE_TEST( lockListMtx() != IDE_SUCCESS );
        sState = 1;

        IDE_ASSERT( aLogFile->mRef != 0 );
        
        aLogFile->mRef--;

        // 더 이상 로그파일을 참조하는 쓰레드가 없는 경우
        if ( aLogFile->mRef == 0 )
        {
            //remove log file from log file list
            removeLogFileFromList(aLogFile);

            sState = 0;

            /* BUG-17254 DEC에서는 log file에 대해 두 가지 open을 동시지원하지 못한다.
               따라서 log file을 list에서 빼고 mutex를 풀기 전에
               close를 함으로써 다른 쓰레드가 mutex를 잡고 list를 봤을 때
               리스트에 이 파일이 없으면 close()되었음을 보장할 수 있다. */
#if defined(DEC_TRU64)
            IDE_TEST( aLogFile->close() != IDE_SUCCESS );

            IDE_TEST( unlockListMtx() != IDE_SUCCESS );

            /* FIT/ART/rp/Bugs/BUG-17254/BUG-17254.tc */
            IDU_FIT_POINT( "1.BUG-17254@smrLogFileMgr::close" );
#else
            IDE_TEST( unlockListMtx() != IDE_SUCCESS );

            /* FIT/ART/rp/Bugs/BUG-17254/BUG-17254.tc */
            IDU_FIT_POINT( "1.BUG-17254@smrLogFileMgr::close" );

            IDE_TEST( aLogFile->close() != IDE_SUCCESS );
#endif
            IDE_TEST( aLogFile->destroy() != IDE_SUCCESS );

            // mMemPool은 thread safe한 객체로, 동시성 제어가 불필요하다.
            IDE_TEST( mMemPool.memfree(aLogFile) != IDE_SUCCESS );
        }
        else // 아직 로그파일을 참조하는 쓰레드가 하나라도 있는 경우
        {
            sState = 0;
            IDE_TEST( unlockListMtx() != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        (void)unlockListMtx();
    }

    return IDE_FAILURE;

}

/* log file prepare 쓰레드의 run함수.
 *
 * 로그파일 하나를 다 사용해서 다음 로그 파일로 switch할 경우의 비용을
 * 최소화 하기 위해서 앞으로 사용할 로그파일들을 미리 준비해 놓는다.
 *
 * 로그파일을 준비하는 일은 다음과 같이 두가지 경우에 실시한다.
 *   1. 특정 시간동안 기다려가며 주기적으로 실시
 *   2. preCreateLogFile 함수 호출에 의해 
 */
void smrLogFileMgr::run()
{
    UInt              sState = 0;
    PDL_Time_Value    sTV;

  startPos:
    sState=0;

    // 쓰레드를 종료해야 하는 경우가 아닌 동안...
    while(mFinish == ID_FALSE)
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sState = 1;

        IDE_TEST_RAISE(mCV.broadcast() != IDE_SUCCESS, err_cond_signal);
        mResume = ID_FALSE;

        while(mResume != ID_TRUE)
        {
            sTV.set(idlOS::time(NULL) + smuProperty::getLogFilePreCreateInterval());

            sState = 0;
            IDE_TEST_RAISE(mCV.timedwait(&mMutex, &sTV, IDU_IGNORE_TIMEDOUT)
                           != IDE_SUCCESS, err_cond_wait);
            mResume = ID_TRUE;
        }

        if ( mFinish == ID_TRUE )
        {
            sState = 0;
            IDE_TEST( unlock() != IDE_SUCCESS );

            break;
        }

        if ( smuProperty::isRunLogPrepareThread() == SMU_THREAD_OFF )
        {
            // To Fix PR-14783
            // System Thread의 작업을 수행하지 않도록 한다.
            continue;
        }
        else
        {
            // Go Go
        }

        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );

        IDE_TEST( addEmptyLogFile() != IDE_SUCCESS );
    }

    return ;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        (void)unlock();
    }

    ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                SM_TRC_MRECOVERY_LOGFILEMGR_INSUFFICIENT_MEMORY);

    goto startPos;

}

/* ------------------------------------------------
 * Description :
 *
 * smuProperty::getLogFilePrepareCount()를 유지하도록 로그파일을
 * 미리 생성한다.
 * ----------------------------------------------*/
IDE_RC smrLogFileMgr::addEmptyLogFile()
{

    SChar           sLogFileName[SM_MAX_FILE_NAME];
    smrLogFile*     sNewLogFile;
    UInt            sLstFileNo  = 0;
    UInt            sTargetFileNo;
    UInt            sState      = 0;
    
    /* BUG-39764 : Log file add 상태 */
    idBool          sLogFileAdd = ID_FALSE;    

    sTargetFileNo = mTargetFileNo;

    smrLogMultiplexThread::mIsOriginalLogFileCreateDone = ID_FALSE;
    
    IDE_TEST( smrLogMultiplexThread::wakeUpCreateThread( 
                                            mCreateThread,
                                            &sTargetFileNo,
                                            mLstFileNo,
                                            mLogFileInitBuffer,
                                            smuProperty::getLogFileSize() )
              != IDE_SUCCESS );


    // 아직 로그파일이 smuProperty::getLogFilePrepareCount() 만큼
    // 미리 준비되지 못한 경우에 한해서
    while ( sTargetFileNo + smuProperty::getLogFilePrepareCount() >
            ( mLstFileNo + 1 ) )
    {
        sLstFileNo = mLstFileNo + 1;

        if ( sLstFileNo == ID_UINT_MAX )
        {
            sLstFileNo = 0;
        }

        idlOS::snprintf(sLogFileName,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        mSrcLogPath,
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME,
                        sLstFileNo);

        /* smrLogFileMgr_addEmptyLogFile_alloc_NewLogFile.tc */
        IDU_FIT_POINT("smrLogFileMgr::addEmptyLogFile::alloc::NewLogFile");
        IDE_TEST( mMemPool.alloc((void**)&sNewLogFile) != IDE_SUCCESS );
        sState = 1;
        IDE_TEST( sNewLogFile->initialize() != IDE_SUCCESS );
        sState = 2;

        /* BUG-48409 temp 파일을 이용한 logfile 생성 방법 제공 */
        if ( smuProperty::getUseTempForPrepareLogFile() == ID_TRUE )
        {
            IDE_TEST( sNewLogFile->prepare( sLogFileName,
                                            mTempLogFileName,
                                            mLogFileInitBuffer,
                                            smuProperty::getLogFileSize() )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sNewLogFile->create( sLogFileName,
                                           mLogFileInitBuffer,
                                           smuProperty::getLogFileSize() )
                      != IDE_SUCCESS );
        }

        IDE_TEST( sNewLogFile->open( sLstFileNo,
                                     sLogFileName,
                                     ID_FALSE, /* aIsMultiplexLogFile */
                                     smuProperty::getLogFileSize(),
                                     ID_TRUE) != IDE_SUCCESS );

        //init log file info
        sNewLogFile->mFileNo   = sLstFileNo;
        sNewLogFile->mOffset   = 0;
        sNewLogFile->mFreeSize = smuProperty::getLogFileSize();

        (void)sNewLogFile->touchMMapArea();

        // 로그파일 리스트에 대한 동시성 제어는 이 함수 안에서 한다.
        IDE_TEST( addLogFile( sNewLogFile ) != IDE_SUCCESS );
        sState = 3;

        IDE_TEST( lock() != IDE_SUCCESS );
        sState = 4;

        mLstFileNo = sLstFileNo;

        // 로그파일 생성이 완료되었음을 다른 쓰레드들에게 알린다.
        IDE_TEST_RAISE(mCV.broadcast() != IDE_SUCCESS, err_cond_signal);

        sState = 3;
        IDE_TEST( unlock() != IDE_SUCCESS );
        
        sTargetFileNo = mTargetFileNo;

        sLogFileAdd = ID_TRUE;
    }

    smrLogMultiplexThread::mOriginalLstLogFileNo        = mLstFileNo;
    smrLogMultiplexThread::mIsOriginalLogFileCreateDone = ID_TRUE;

    IDE_TEST( smrLogMultiplexThread::wait( mCreateThread ) != IDE_SUCCESS );

    /* BUG-39764 : 새로운 로그 파일 생성이 완료되면 로그앵커의 
     * "마지막으로 생성된 로그 파일 번호 (Last Created Logfile Num)" 항목을 
     * 업데이트하고 로그앵커를 파일에 플러쉬한다.  */
    if ( sLogFileAdd == ID_TRUE )
    {
        IDE_TEST( smrRecoveryMgr::getLogAnchorMgr()->updateLastCreatedLogFileNumAndFlush( sLstFileNo ) 
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
     

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    switch (sState)
    {
    case 4:
        (void)unlock();

    case 3:
        (void)lockListMtx();
        (void)removeLogFileFromList(sNewLogFile);
        (void)unlockListMtx();

    case 2:
        (void)sNewLogFile->destroy();

    case 1:
        (void)mMemPool.memfree(sNewLogFile);
        break;
    }

    return IDE_FAILURE;
}

/* log file prepare 쓰레드를 깨워서 로그파일을 미리 생성해 둔다.
 *
 * aWait => ID_TRUE 이면 해당 로그파일이 생성될 때까지 기다린다.
 *          ID_FALSE 이면 로그파일을 만들라는 요청만 하고 바로 리턴한다.
 */
IDE_RC smrLogFileMgr::preCreateLogFile(idBool aWait)
{

    UInt sState = 0;

    if ( aWait == ID_TRUE )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sState = 1;
    
        if ( mFinish == ID_FALSE )
        {
            if ( mResume == ID_TRUE ) 
            {
                sState = 0;
                IDE_TEST_RAISE(mCV.wait(&mMutex) != IDE_SUCCESS, err_cond_wait);
                sState = 1;
            }

            mResume = ID_TRUE;
            
            IDE_TEST_RAISE(mCV.broadcast() != IDE_SUCCESS, err_cond_signal);
        }

        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }
    else
    {
        if ( mResume != ID_TRUE )
        {
            mResume = ID_TRUE;
            IDE_TEST_RAISE(mCV.broadcast() != IDE_SUCCESS, err_cond_signal);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        (void)unlock();
    }
    
    return IDE_FAILURE;

}

/* 로그파일 prepare 쓰레드를 중지한다.
 */
IDE_RC smrLogFileMgr::shutdown()
{

    UInt         sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    // 로그파일 prepare쓰레드가 현재 로그파일을 생성하고 있는 중이면

    /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                 wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
    while ( mResume == ID_TRUE ) 
    {
        // 해당 로그파일 생성이 완료될때까지 대기한다.
        sState = 0;
        IDE_TEST_RAISE(mCV.wait(&mMutex) != IDE_SUCCESS, err_cond_wait);
        sState = 1;
    }

    // 쓰레드가 종료되도록 쓰레드 제어변수 설정
    mFinish = ID_TRUE;
    mResume = ID_TRUE;

    // sleep상태의 쓰레드를 깨워서 mFinish == ID_TRUE임을 체크하고
    // 쓰레드가 종료되도록 한다.
    IDE_TEST_RAISE(mCV.broadcast() != IDE_SUCCESS, err_cond_signal);
    
    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    // 쓰레드가 완전히 종료될 때까지 기다린다.
    IDE_TEST_RAISE(join() != IDE_SUCCESS, err_thr_join);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));    
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        (void)unlock();
    }

    return IDE_FAILURE;
}

/* 현재 기록중인 로그파일의 다음 로그파일을 새로 기록할 로그파일로 설정한다.
 * switch할 로그파일이 존재하지 않을 경우, 로그파일을 새로 생성한다.
 */
IDE_RC smrLogFileMgr::switchLogFile( smrLogFile **aCurLogFile )
{
    smrLogFile*  sPrvLogFile;
    
    mCurFileNo++;

    sPrvLogFile = (*aCurLogFile);

    mTargetFileNo = mCurFileNo + 1;
        
    while(1)
    {
        // Switch할 로그파일이 존재하는 경우
        if ( mCurFileNo <= mLstFileNo )
        {
            // prepare된 로그파일 하나를 사용할 것이므로,
            // 로그파일 prepare 쓰레드를 깨워서
            // 로그파일 하나를 새로 생성하도록 지시한다.
            // 로그파일이 생성될때까지 기다리지 않는다.
            IDE_TEST( preCreateLogFile(ID_FALSE) != IDE_SUCCESS );

            *aCurLogFile = (*aCurLogFile)->mNxtLogFile;
            
            IDE_ASSERT((*aCurLogFile)->mFileNo == mCurFileNo);
            
            (*aCurLogFile)->mRef++;

            sPrvLogFile->mSwitch = ID_TRUE;

            break;
        }
        else
        {
            // 로그파일을 생성한다.
            // 로그파일이 생성될 때까지 기다린다.
            mLFPrepareWaitCnt++;
            IDE_TEST( preCreateLogFile(ID_TRUE) != IDE_SUCCESS );
        }
        
        idlOS::thr_yield();
    }
    
    /* PROJ-2232 log multiplex
     * 다중화 쓰레드에 현재 switch되어 로그가 쓰일 logfile번호를 전달한다.*/
    smrLogMultiplexThread::setOriginalCurLogFileNo( mCurFileNo );

    IDU_FIT_POINT( "1.BUG-38801@smrLogFileMgr::switchLogFile" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* 로그파일 리스트에서 로그파일을 찾는다.
 * aLogFileNo - [IN] 찾을 로그파일
 *
 * Return값 - ID_TRUE : aLogFileNo와 일치하는 로그파일을 찾았다.
 *                      *aLogFile 에 찾은 로그파일 객체를 설정
 *            ID_FALSE : aLogFileNo와 일치하는 로그파일을 찾지 못했다.
 *                       *aLogFile에 설정되는 로그파일 객체 바로 다음에
 *                       (만약 필요하다면)aLogFileNo에 해당하는
 *                       로그파일을 생성하면 된다.
 */
idBool smrLogFileMgr::findLogFile(UInt           aLogFileNo, 
                                  smrLogFile**   aLogFile)
{

    smrLogFile*   sCurLogFile;

    IDE_ASSERT(aLogFile != NULL);

    *aLogFile = NULL;
    
    sCurLogFile = mFstLogFile.mNxtLogFile;
    
    while(sCurLogFile != &mFstLogFile)
    {
        // 정확히 일치하는 로그파일 발견
        if ( sCurLogFile->mFileNo == aLogFileNo )
        {
            *aLogFile = sCurLogFile;
            
            return ID_TRUE;
        }
        // 로그파일 리스트는 로그파일 번호순으로 정렬되어있다.
        // 로그파일 번호가 찾고자 하는것보다 크다면 로그파일을 찾지 못한 것임
        else if ( sCurLogFile->mFileNo > aLogFileNo )
        {
            // aLogFileNo보다 작은 로그파일 번호를 가지는
            // 로그파일 객체중 가장 마지막것을 리턴.
            *aLogFile = sCurLogFile->mPrvLogFile;
            break;
        } 

        sCurLogFile = sCurLogFile->mNxtLogFile;
    }

    // aLogFileNo까지 로그파일이 생성조차 되지 않은 경우
    if ( *aLogFile == NULL )
    {
        // 로그파일 리스트의 맨 마지막 로그파일을 리턴.
        *aLogFile = sCurLogFile->mPrvLogFile; 
    }
    
    return ID_FALSE;
    
}

/*
 * 로그파일 객체를 로그파일 리스트에 추가한다.
 */
IDE_RC smrLogFileMgr::addLogFile( smrLogFile *aNewLogFile )
{

    smrLogFile*  sPrvLogFile;
    UInt         sState = 0;

    IDE_TEST( lockListMtx() != IDE_SUCCESS );
    sState = 1;

    // 로그파일을 삽입할 로그파일을 찾아낸다.
#ifdef DEBUG
    IDE_ASSERT( findLogFile( aNewLogFile->mFileNo, &sPrvLogFile )
                == ID_FALSE );
#else
    (void)findLogFile( aNewLogFile->mFileNo, &sPrvLogFile );
#endif

    // sPrevLogFile 의 next로 aNewLogFile을 매단다.
    AddLogFileToList( sPrvLogFile, aNewLogFile ); //add log file to list

    sState = 0;
    IDE_TEST( unlockListMtx() != IDE_SUCCESS );

    // Sync될 로그파일로 등록한다.
    IDE_TEST( mLFThread->addSyncLogFile( aNewLogFile )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( unlockListMtx() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/* 로그파일들을 DISK에서 제거한다.
 *
 * smrLogFile::remove 는 
 * checkpoint도중 로그파일 삭제가 실패하면 에러상황으로 처리하고,
 * restart recovery시에 로그파일 삭제가 실패하면 정상상황으로 처리한다.
 */
void smrLogFileMgr::removeLogFile( UInt     aFstFileNo, 
                                   UInt     aLstFileNo, 
                                   idBool   aIsCheckPoint)
{
    smrLogMultiplexThread::wakeUpDeleteThread( mDeleteThread,
                                               aFstFileNo,
                                               aLstFileNo,
                                               aIsCheckPoint );

    removeLogFiles( aFstFileNo, 
                    aLstFileNo, 
                    aIsCheckPoint, 
                    ID_FALSE, 
                    mSrcLogPath );

    (void)smrLogMultiplexThread::wait( mDeleteThread );
}

/* 로그파일 번호에 해당하는 로그 데이터가 있는지 체크한다.
 */
IDE_RC smrLogFileMgr::checkLogFile(UInt aFileNo)
{

    iduFile         sFile;
    SChar           sLogFileName[SM_MAX_FILE_NAME];

    // 로그를 디스크의 파일에 기록하는 경우
    // 로그파일을 한번 열어본다.
    idlOS::snprintf(sLogFileName,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT, 
                    mSrcLogPath,
                    IDL_FILE_SEPARATOR, 
                    SMR_LOG_FILE_NAME, 
                    aFileNo);
    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    
    IDE_TEST( sFile.setFileName(sLogFileName) != IDE_SUCCESS );
    IDE_TEST( sFile.open() != IDE_SUCCESS );
    IDE_TEST( sFile.close() != IDE_SUCCESS );
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/* 현재 로그파일 리스트에 있는 모든 로그파일을 close하고
 * 로그파일 리스트에서 제거한다.
 */
IDE_RC smrLogFileMgr::closeAllLogFile()
{

    smrLogFile*     sCurLogFile;
    smrLogFile*     sNxtLogFile;

    sCurLogFile = mFstLogFile.mNxtLogFile;
    while(sCurLogFile != &mFstLogFile)
    {
        sNxtLogFile = sCurLogFile->mNxtLogFile;

        sCurLogFile->mRef = 1;
        IDE_TEST( close(sCurLogFile) != IDE_SUCCESS );

        sCurLogFile = sNxtLogFile;
    }

    IDE_ASSERT(&mFstLogFile == mFstLogFile.mNxtLogFile);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smrLogFileMgr::AddLogFileToList(smrLogFile*  aPrvLogFile,
                                       smrLogFile*  aNewLogFile)
{

    smrLogFile *sNxtLogFile;

    mLFOpenCnt++;
    
    sNxtLogFile = aPrvLogFile->mNxtLogFile;

    aPrvLogFile->mNxtLogFile = aNewLogFile;
    sNxtLogFile->mPrvLogFile = aNewLogFile;

    aNewLogFile->mNxtLogFile = sNxtLogFile;
    aNewLogFile->mPrvLogFile = aPrvLogFile;

    return IDE_SUCCESS;
}

#if 0 // not used
/***********************************************************************
 * Description : aFileNo에 해당하는 Logfile이 존재하는지 Check한다.
 *
 * aFileNo   - [IN]  Log File Number
 * aIsExist  - [OUT] aFileNo에 해당하는 File이 있는 ID_TRUE, 아니면
 *                   ID_FALSE
*/ 
IDE_RC smrLogFileMgr::isLogFileExist(UInt aFileNo, idBool & aIsExist)
{
    SChar    sLogFilename[SM_MAX_FILE_NAME];
    iduFile  sFile;
    ULong    sLogFileSize = 0;
    
    aIsExist = ID_TRUE;
    
    idlOS::snprintf(sLogFilename,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT,
                    mSrcLogPath,
                    IDL_FILE_SEPARATOR,
                    SMR_LOG_FILE_NAME,
                    aFileNo);
    
    if ( idf::access(sLogFilename, F_OK) != 0 )
    {
        aIsExist = ID_FALSE;
    }
    else
    {
        IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                    1, /* Max Open FD Count */
                                    IDU_FIO_STAT_OFF,
                                    IDV_WAIT_INDEX_NULL ) 
                 != IDE_SUCCESS );
        
        IDE_TEST( sFile.setFileName(sLogFilename) != IDE_SUCCESS );

        IDE_TEST( sFile.open() != IDE_SUCCESS );
        IDE_TEST( sFile.getFileSize(&sLogFileSize) != IDE_SUCCESS );
        IDE_TEST( sFile.close() != IDE_SUCCESS );
        IDE_TEST( sFile.destroy() != IDE_SUCCESS );
        
        if ( sLogFileSize != smuProperty::getLogFileSize() )
        {
            aIsExist = ID_FALSE;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* 모든 Error는 Fatal임 */
    return IDE_FAILURE;
}
#endif

/***********************************************************************
 * Description : aFileNo에 해당하는 Logfile이 Open되었는지 check하고 
 *               Open되어있다면 LogFile의 Reference Count를 증가시킨다.
 *
 * aFileNo      - [IN]  Check할 LogFile Number
 * aAlreadyOpen - [OUT] aFileNo에 해당하는 LogFile이 존재한다면 ID_TRUE, 아니면
 *                      ID_FALSE.
 * aLogFile     - [OUT] aFileNo에 해당하는 LogFile이 존재한다면 aLogFile에
 *                      smrLogFile Ponter를 넘겨주고, 아니면 NULL
*/
IDE_RC smrLogFileMgr::checkLogFileOpenAndIncRefCnt( UInt         aFileNo,
                                                    idBool     * aAlreadyOpen,
                                                    smrLogFile** aLogFile )
{
    SInt sState = 0;
    smrLogFile* sPrvLogFile;

    IDE_DASSERT( aAlreadyOpen != NULL );
    IDE_DASSERT( aLogFile != NULL );

    // Output Parameter 초기화
    *aAlreadyOpen = ID_FALSE;
    *aLogFile     = NULL;
    
    IDE_TEST( lockListMtx() != IDE_SUCCESS );
    sState = 1;
        
    // aFileNo번의 로그파일이 open된 로그파일 list에 존재하는 경우
    if ( findLogFile( aFileNo, &sPrvLogFile ) == ID_TRUE )
    {
        sPrvLogFile->mRef++;
        *aLogFile = sPrvLogFile;

        *aAlreadyOpen = ID_TRUE;
        
        sState = 0;
        IDE_TEST( unlockListMtx() != IDE_SUCCESS );
        
        /* 아직 open되지 않은 로그파일의 객체가 로그파일 list에 들어가
         * 있을 수 있다.
         * 로그파일 list안에 있는 하나의 로그파일에 대해
         * 로그파일 Mutex를 한번 잡아보도록 한다.
         *
         * 이를 통해 로그파일을 open하는 쓰레드가 해당 로그파일을 open중인경우,
         * 로그파일의 open이 완료될 때까지 기다리도록 한다.
         */
        IDE_TEST( (*aLogFile)->lock() != IDE_SUCCESS );
        IDE_TEST( (*aLogFile)->unlock() != IDE_SUCCESS );
    }
    else
    {
        // aFileNo에 해당하는 LogFile이 존재하지 않는다.
        sState = 0;
        IDE_TEST( unlockListMtx() != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        (void)unlockListMtx();
        IDE_POP();
    }
    
    return IDE_FAILURE;
}
