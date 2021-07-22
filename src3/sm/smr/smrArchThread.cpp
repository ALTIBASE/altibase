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
 * $$Id: smrArchThread.cpp 83870 2018-09-03 04:32:39Z kclee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smu.h>
#include <smrDef.h>
#include <smrArchThread.h>
#include <smrArchMultiplexThread.h>
#include <smrReq.h>

smrArchThread::smrArchThread() : idtBaseThread()
{
    
}

smrArchThread::~smrArchThread()
{
    
}

// 아카이브 쓰레드 객체를 초기화 한다.
// aArchivePath   - [IN] 아카이브 로그가 저장될 디렉토리
// aLogFileMgr    - [IN] 이 아카이브 쓰레드가 아카이브할 로그파일들을 관리하는
//                     로그파일 관리자.
// aLstArchFileNo - [IN] 마지막으로 Archive한 File No
IDE_RC smrArchThread::initialize( const SChar   * aArchivePath,
                                  smrLogFileMgr * aLogFileMgr,
                                  UInt            aLstArchFileNo)
{
    const SChar ** sArchiveMultiplexPath;
    UInt           sArchPathIdx;

    IDE_DASSERT( aArchivePath != NULL );
    IDE_DASSERT( aLogFileMgr != NULL );

    /*
     * PROJ-2232 Multiplex archivelog
     * 프로퍼티로부터 archive될 디렉토리 path와 갯수를 가져온다. 
     */ 
    sArchiveMultiplexPath = smuProperty::getArchiveMultiplexDirPath();
    mArchivePathCnt       = smuProperty::getArchiveMultiplexCount() + 1;

    /* 
     * ARCHIVE_DIR? ARCHIVE_MULTIPLEX_DIR에서 가져온 디렉토리path를
     * mArchivePath변수에 저장한다. 
     */ 
    for( sArchPathIdx = 0; 
         sArchPathIdx < mArchivePathCnt; 
         sArchPathIdx++ )
    {
        if( sArchPathIdx == SMR_ORIGINAL_ARCH_DIR_IDX )
        {
            mArchivePath[sArchPathIdx] = aArchivePath;
        }
        else
        {
            mArchivePath[sArchPathIdx] = 
                            sArchiveMultiplexPath[ sArchPathIdx-1 ];
        }

        IDE_TEST_RAISE(idf::access( mArchivePath[sArchPathIdx], F_OK) != 0,
                       path_exist_error);
        IDE_TEST_RAISE(idf::access( mArchivePath[sArchPathIdx], W_OK) != 0,
                       err_no_write_perm_path);
        IDE_TEST_RAISE(idf::access( mArchivePath[sArchPathIdx], X_OK) != 0,
                       err_no_execute_perm_path);
    }

    IDE_DASSERT( mArchivePathCnt != 0 );
    
    /*PROJ-2232 Multiplex archive log*/
    IDE_TEST( smrArchMultiplexThread::initialize( &mArchMultiplexThreadArea,
                                                  mArchivePath,
                                                  mArchivePathCnt )
              != IDE_SUCCESS );

    mLogFileMgr = aLogFileMgr;
    
    IDE_TEST(mMtxArchList.initialize((SChar*)"Archive Log List Mutex",
                                     IDU_MUTEX_KIND_POSIX,
                                     IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);
    
    IDE_TEST(mMtxArchThread.initialize((SChar*)"Archive Log Thread Mutex",
                                       IDU_MUTEX_KIND_POSIX,
                                       IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    IDE_TEST(mMemPool.initialize(IDU_MEM_SM_SMR,
                                 (SChar *)"Archive Log Thread Mem Pool",
                                 1,
                                 ID_SIZEOF(smrArchLogFile),
                                 SMR_ARCH_THREAD_POOL_SIZE,
                                 IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                 ID_TRUE,							/* UseMutex */
                                 IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                 ID_FALSE,							/* ForcePooling */
                                 ID_TRUE,							/* GarbageCollection */
                                 ID_TRUE,                           /* HWCacheLine */
                                 IDU_MEMPOOL_TYPE_LEGACY            /* mempool type */) 
             != IDE_SUCCESS);			

    IDE_TEST_RAISE(mCv.initialize((SChar *)"Archive Log Thread Cond")
                   != IDE_SUCCESS, err_cond_var_init);
    
    mResume      = ID_TRUE;

    mArchFileList.mArchPrvLogFile = &mArchFileList;
    mArchFileList.mArchNxtLogFile = &mArchFileList;
    
    mLstArchFileNo = aLstArchFileNo;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(path_exist_error);
    {               
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath,
                                mArchivePath[sArchPathIdx] ));
    }
    IDE_EXCEPTION(err_no_write_perm_path);
    {               
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoWritePermFile,
                                mArchivePath[sArchPathIdx] ));
    }
    IDE_EXCEPTION(err_no_execute_perm_path);
    {               
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExecutePermFile,
                                mArchivePath[sArchPathIdx] ));
    }
    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                SM_TRC_MRECOVERY_ARCH_WARNING);
    
    return IDE_FAILURE;
    
}

/* 서버 스타트업 시에
 * 아카이브할 로그파일 리스트를 재구축한다.
 *
 * aStartNo - [IN] 아카이브 대상 로그파일인지 체크할 첫번째 로그파일의 번호
 * aEndNo   - [IN] 아카이브 대상 로그파일인지 체크할 마지막 로그파일의 번호
 *
 * 아카이브할 로그파일인지 체크할 대상이 되는 로그파일들은 다음과 같다.
 *
 * [ 이전에 정상 Shutdown된 경우 ]
 *     aStartNo - 마지막 삭제된 (아카이브된) 로그파일 번호
 *     aEndNo   - 마지막 로그파일 번호
 *
 * [ 이전에 비정상 Shutdown된 경우 ]
 *     aStartNo - 마지막 삭제된 (아카이브된) 로그파일 번호
 *     aEndNo   - Restart Recovery의 RedoLSN
 */
IDE_RC smrArchThread::recoverArchiveLogList( UInt aStartNo,
                                             UInt aEndNo )
    
{
    iduFile     sFile;
    UInt        sState      = 0;
    UInt        sFileCount  = 0;
    UInt        sCurFileNo;
    UInt        i;
    UInt        sAlreadyArchivedCnt = 0;    /* BUG-39746 */
    SChar       sLogFileName[SM_MAX_FILE_NAME];
    ULong       sFileSize = 0;
    idBool      sIsArchived;
    idBool      sCanUpdateLstArchLogFileNum = ID_TRUE;  /* BUG-39746 */
    
    /* ------------------------------------------------
     * BUG-12246 서버 구동시 ARCHIVE할 로그파일을 구해야함
     * 1) 서버가 비정상종료한 경우
     * lst delete logfile no 부터 recovery lsn까지 archive dir을
     * 검사하여 존재하지 않는 로그파일만 다시 archive list에 등록한다.
     * 그 이후는 redoAll 단계에서 등록하도록 한다.
     * 2) 서버가 정상종료한 경우
     * lst delete logfile no부터 end lsn까지 archive dir을 검사하여
     * 존재하지 않는 로그파일만 다시 archive list에 등록한다. 
     * ----------------------------------------------*/

    IDE_ASSERT(aStartNo <= aEndNo);

    for (sCurFileNo = aStartNo; sCurFileNo < aEndNo; sCurFileNo++)
    {
        sIsArchived         = ID_TRUE;
        sAlreadyArchivedCnt = 0;

        for( i = 0; i < mArchivePathCnt; i++ )
        {
            idlOS::memset(sLogFileName, 0x00, SM_MAX_FILE_NAME);

            idlOS::snprintf(sLogFileName,
                            SM_MAX_FILE_NAME,
                            "%s%c%s%"ID_UINT32_FMT,
                            mArchivePath[i],
                            IDL_FILE_SEPARATOR,
                            SMR_LOG_FILE_NAME,
                            sCurFileNo);

            if (idf::access(sLogFileName, F_OK) == 0)
            {
                IDE_TEST(sFile.initialize(IDU_MEM_SM_SMR,
                                          1, /* Max Open FD Count */
                                          IDU_FIO_STAT_OFF,
                                          IDV_WAIT_INDEX_NULL)
                         != IDE_SUCCESS);
                sState = 1;

                IDE_TEST(sFile.setFileName(sLogFileName) != IDE_SUCCESS);
                IDE_TEST(sFile.open() != IDE_SUCCESS);
                sState = 2;

                IDE_TEST( sFile.getFileSize( &sFileSize ) != IDE_SUCCESS );

                sState = 1;
                IDE_TEST(sFile.close() != IDE_SUCCESS);

                sState = 0;
                IDE_TEST(sFile.destroy() != IDE_SUCCESS);

                // 아카이브 로그 디렉토리에 로그 파일이 존재할 경우
                if ( sFileSize == (ULong)smuProperty::getLogFileSize() )
                {
                    IDE_DASSERT( mLstArchFileNo <= sCurFileNo );

                    /* BUG-39746
                     * 각 archive Path 마다 실제 아카이브 됐는지 검사하여
                     * sAlreadyArchivedCnt를 증가시킨다. */
                    sAlreadyArchivedCnt++;

                    /* BUG-39746
                     * 모든 ArchivePath마다 아카이브 되어 있고,
                     * 현 로그파일 이전에 모든 파일이 
                     * 아카이브하는데 문제가 없다면,
                     * 즉, LstArchLogFileNo를 갱신해도 괜찮다면 설정한다. */
                    if( (sAlreadyArchivedCnt == mArchivePathCnt) &&
                        (sCanUpdateLstArchLogFileNum == ID_TRUE ) )
                    {
                        // 마지막 아카이브된 로그파일번호 설정
                        setLstArchLogFileNo( sCurFileNo );
                    }
                    else
                    {
                        // do nothing
                    }
                }
                else
                {
                    // 사이즈가 다르면 아카이브 해야 함
                    sIsArchived = ID_FALSE;
                    break;
                }
            }
            else
            {
                /* 한번이라도 아카이브 해야 할 파일을 찾았다면,
                 * 더이상 LstArchLogFileNo를 갱신하지 못한다. */
                sCanUpdateLstArchLogFileNum = ID_FALSE;

                sIsArchived = ID_FALSE;
                break;
            }
        }

        if( sIsArchived == ID_FALSE )
        {
            // 아카이브 로그 디렉토리에 로그파일이 없으면
            // 아카이브 대상 로그파일 리스트에 추가
            IDE_TEST(addArchLogFile(sCurFileNo) != IDE_SUCCESS);

            sFileCount++;
        }
        else
        {
            // 마지막 아카이브된 로그파일번호 설정
            setLstArchLogFileNo( sCurFileNo );
        }
    }

    if (aStartNo < aEndNo)
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_ARCH_REBUILD_LOGFILE,
                    sFileCount);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch(sState)
    {
    case 2:
        IDE_ASSERT(sFile.close() == IDE_SUCCESS); 
        
    case 1:
        IDE_ASSERT(sFile.destroy() == IDE_SUCCESS);
    }
    IDE_POP();
    
    return IDE_FAILURE;
}

// 아카이브 쓰레드를 시작시키고, 쓰레드가 정상적으로
// 시작될 때까지 기다린다.
IDE_RC smrArchThread::startThread()
{
    mFinish      = ID_FALSE;

    IDE_TEST( smrArchMultiplexThread::startThread( mArchMultiplexThreadArea ) 
              != IDE_SUCCESS );

    IDE_TEST(start() != IDE_SUCCESS);
    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// 아카이브 쓰레드를 중지하고, 쓰레드가 정상적으로
// 중지되었을 때까지 기다린다.
IDE_RC smrArchThread::shutdown()
{
    
    UInt          sState = 0;

    IDE_TEST( smrArchMultiplexThread::shutdownThread( mArchMultiplexThreadArea ) 
              != IDE_SUCCESS );

    IDE_TEST(lockThreadMtx() != IDE_SUCCESS);
    sState = 1;
    
    mFinish      = ID_TRUE;

    IDE_TEST_RAISE(mCv.signal() != IDE_SUCCESS, err_cond_signal);

    sState = 0;
    IDE_TEST(unlockThreadMtx() != IDE_SUCCESS);

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

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(unlockThreadMtx() == IDE_SUCCESS);
        IDE_POP();
    }
    
    return IDE_FAILURE;
    
}

// 아카이프 쓰레드 객체를 해제 한다.
IDE_RC smrArchThread::destroy()
{
    /*PROJ-2232 Multiplex archive log*/
    IDE_TEST( smrArchMultiplexThread::destroy( mArchMultiplexThreadArea ) 
              != IDE_SUCCESS ); 

    IDE_TEST(clearArchList() != IDE_SUCCESS);
    
    IDE_TEST(mMtxArchList.destroy() != IDE_SUCCESS);
    IDE_TEST(mMtxArchThread.destroy() != IDE_SUCCESS);
    IDE_TEST(mMemPool.destroy() != IDE_SUCCESS);

    IDE_TEST_RAISE(mCv.destroy() != IDE_SUCCESS, err_cond_destroy);
                   
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_cond_destroy ); 
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

void smrArchThread::run()
{

    IDE_RC         rc;
    UInt           sState = 0;
    PDL_Time_Value sCurTimeValue;
    PDL_Time_Value sFixTimeValue;
  
  startPos:
    sState=0;
    
    IDE_TEST( lockThreadMtx() != IDE_SUCCESS);
    sState = 1;

    sFixTimeValue.set(smuProperty::getSyncIntervalSec(),
                      smuProperty::getSyncIntervalMSec() * 1000);
    
    while(mFinish == ID_FALSE)
    {
        mResume = ID_FALSE;
        sCurTimeValue = idlOS::gettimeofday();
        sCurTimeValue += sFixTimeValue;
      
        sState = 0;
        rc = mCv.timedwait(&mMtxArchThread, &sCurTimeValue);
        sState = 1;

        if(mFinish == ID_TRUE)
        {
            break;
        }

        if ( smuProperty::isRunArchiveThread() == SMU_THREAD_OFF )
        {
            // To Fix PR-14783
            // System Thread의 작업을 수행하지 않도록 한다.
            continue;
        }
        else
        {
            // Go Go 
        }

        if(rc != IDE_SUCCESS) // cond_wait에 지정한 시간이 지나서 깨어난 경우
        {
            IDE_TEST_RAISE(mCv.isTimedOut() != ID_TRUE, err_cond_wait);
            mResume = ID_TRUE;
        }
        else // cond_signal에 의해 깨어난 경우
        {
            if (mResume == ID_FALSE)
            {
                // mResume이 ID_FALSE이면
                // cond_wait하는 interval을 재설정하라고 signal을 날린것이다.

                // Todo : cond_wait하는 interval을 재설정 하도록 구현
                continue;
            }
            // mResume이 ID_TRUE이면 아카이빙을 실시한다.
        }

        // 아카이브 쓰레드가 깨어났다.
        // 아카이브할 로그파일리스트를 보고 아카이빙 실시.
        rc = archLogFile();
        
        if ( rc != IDE_SUCCESS && errno !=0 && errno != ENOSPC )
        {
            IDE_RAISE(error_archive);
        }
    }

    /* ------------------------------------------------
     * recovery manager destroy시에 archive thread가
     * 살아있으면, archive를 수행하고, 살아있지 않으면,
     * archive thread destroy시에 archive log list를
     * clear 해버린다.
     * ----------------------------------------------*/   
    sState = 0;
    IDE_TEST(unlockThreadMtx() != IDE_SUCCESS);
    
    return;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION(error_archive);
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if(sState != 0)
    {
        IDE_ASSERT(unlockThreadMtx() == IDE_SUCCESS);
    }
    IDE_POP();
    
    if (mFinish == ID_FALSE)
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_MRECOVERY_ARCH_WARNING3,
                    errno);

        idlOS::sleep(2);
        goto startPos;
    }
    
    return;
    
}

// 아카이브할 로그파일 리스트에 있는 로그파일들을 아카이빙한다.
// 아카이브 쓰레드가 주기적으로, 혹은 요청에 의해 깨어나서 수행하는 함수이다.
IDE_RC smrArchThread::archLogFile()
{

    UInt            sState = 0;
    UInt            sCurFileNo;
    SChar           sSrcLogFileName[SM_MAX_FILE_NAME];
    smrArchLogFile *sCurLogFilePtr;
    smrArchLogFile *sNxtLogFilePtr;
    smrLogFile     *sSrcLogFile = NULL;

    IDE_TEST(lockListMtx() != IDE_SUCCESS);
    sState = 1;

    sCurLogFilePtr = mArchFileList.mArchNxtLogFile;

    sState = 0;
    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    /* ================================================== 
     * For Each Log File in archive log file list,       
     * backup already open/synced log file to archive_dir
     * ================================================== */
    while(sCurLogFilePtr != &mArchFileList)
    {
        sNxtLogFilePtr = sCurLogFilePtr->mArchNxtLogFile;
        sCurFileNo = sCurLogFilePtr->mFileNo;

        IDE_TEST_RAISE( mLogFileMgr->open( sCurFileNo,
                                           ID_FALSE,/* For Read */
                                           &sSrcLogFile )
                        != IDE_SUCCESS, open_err );

        /*PROJ-2232 Multiplex archive log*/
        IDE_TEST( smrArchMultiplexThread::performArchving( 
                                                mArchMultiplexThreadArea,
                                                sSrcLogFile )
                  != IDE_SUCCESS );

        /* ================================================================
         * In checkpointing to determine the log file number to be removed,
         * set the last archived log file number.
         * ================================================================ */
        IDE_TEST( setLstArchLogFileNo(sCurFileNo) != IDE_SUCCESS);

        IDE_TEST( removeArchLogFile(sCurLogFilePtr) != IDE_SUCCESS);

        IDE_TEST( mLogFileMgr->close(sSrcLogFile) != IDE_SUCCESS);
        sSrcLogFile = NULL;
        sState = 0;

        sCurLogFilePtr = sNxtLogFilePtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( open_err )
    {
       ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                    "fail to open logfile\n"
                    "Log File Path : %s\n"
                    "Log File No   : %"ID_UINT32_FMT"\n",
                    mLogFileMgr->getSrcLogPath(),
                    sCurFileNo );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT(unlockListMtx() == IDE_SUCCESS);
    }

    if (sSrcLogFile != NULL)
    {
        IDE_ASSERT(mLogFileMgr->close(sSrcLogFile) == IDE_SUCCESS);
        sSrcLogFile = NULL;
    }

    if( mArchMultiplexThreadArea[0].mFinish == ID_TRUE )
    {
        mFinish = mArchMultiplexThreadArea[0].mFinish;
    }


    /* BUG-36662 Add property for archive thread to kill server when doesn't
     * exist source logfile */
    if( smuProperty::getCheckSrcLogFileWhenArchThrAbort() == ID_TRUE )
    {
        idlOS::snprintf(sSrcLogFileName, SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        mLogFileMgr->getSrcLogPath(),
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME,
                        sCurFileNo); 

        IDE_ASSERT( idf::access(sSrcLogFileName, F_OK) == 0 );
    }

    IDE_POP();

    return IDE_FAILURE;
    
}

/* 아카이브할 로그파일 리스트에 로그파일을 하나 새로 추가한다.
 *
 * aLogFileNo - [IN] 새로 아카이브 대상에 추가될 로그파일의 번호
 */
IDE_RC smrArchThread::addArchLogFile(UInt  aLogFileNo)
{

    smrArchLogFile *sArchLogFile;

    /* smrArchThread_addArchLogFile_alloc_ArchLogFile.tc */
    IDU_FIT_POINT("smrArchThread::addArchLogFile::alloc::ArchLogFile");
    IDE_TEST(mMemPool.alloc((void**)&sArchLogFile) != IDE_SUCCESS);
    sArchLogFile->mFileNo         = aLogFileNo;
    sArchLogFile->mArchNxtLogFile = NULL;
    sArchLogFile->mArchPrvLogFile = NULL;
    
    IDE_TEST(lockListMtx() != IDE_SUCCESS);
    
    sArchLogFile->mArchPrvLogFile = mArchFileList.mArchPrvLogFile;
    sArchLogFile->mArchNxtLogFile = &mArchFileList;
    
    mArchFileList.mArchPrvLogFile->mArchNxtLogFile = sArchLogFile;
    mArchFileList.mArchPrvLogFile = sArchLogFile;
    
    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

// 아카이브할 로그파일 리스트에서 로그파일 노드를 하나 제거한다.
IDE_RC smrArchThread::removeArchLogFile(smrArchLogFile   *aLogFile)
{

    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    aLogFile->mArchPrvLogFile->mArchNxtLogFile = aLogFile->mArchNxtLogFile;
    aLogFile->mArchNxtLogFile->mArchPrvLogFile = aLogFile->mArchPrvLogFile;
    aLogFile->mArchPrvLogFile = NULL;
    aLogFile->mArchNxtLogFile = NULL;

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);
    
    IDE_TEST(mMemPool.memfree(aLogFile) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// 마지막으로 아카이브된 파일번호를 설정한다.
IDE_RC smrArchThread::setLstArchLogFileNo(UInt  aArchLogFileNo)
{

    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    mLstArchFileNo = aArchLogFileNo;

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// 마지막으로 아카이브된 파일번호를 가져온다.
IDE_RC smrArchThread::getLstArchLogFileNo(UInt *aArchLogFileNo)
{
    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    *aArchLogFileNo = mLstArchFileNo;

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* 다음으로 아카이브할 로그파일번호를 가져온다.
 *
 * aArchFstLFileNo   - [OUT] 아카이브할 첫번째 로그파일 번호
 * aIsEmptyArchLFLst - [OUT] 아카이브 LogFile List가 비었으면 ID_TRUE를 return한다.
 */
IDE_RC smrArchThread::getArchLFLstInfo(UInt   * aArchFstLFileNo,
                                       idBool * aIsEmptyArchLFLst )
{

    smrArchLogFile *sCurLogFilePtr;

    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    sCurLogFilePtr = mArchFileList.mArchNxtLogFile;

    if (sCurLogFilePtr != &mArchFileList)
    {
        *aArchFstLFileNo   = sCurLogFilePtr->mFileNo;
        *aIsEmptyArchLFLst = ID_FALSE;
    }
    else
    {
        /* Archive LogFile List가 비어 있다. */
        *aArchFstLFileNo   = ID_UINT_MAX;
        *aIsEmptyArchLFLst = ID_TRUE;
    }

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// 아카이브할 로그파일 리스트를 모두 초기화 한다.
IDE_RC smrArchThread::clearArchList()
{

    smrArchLogFile *sCurLogFilePtr;
    smrArchLogFile *sNxtLogFilePtr;

    IDE_TEST(lockListMtx() != IDE_SUCCESS);

    sCurLogFilePtr = mArchFileList.mArchNxtLogFile;

    IDE_TEST(unlockListMtx() != IDE_SUCCESS);

    while(sCurLogFilePtr != &mArchFileList)
    {
        sNxtLogFilePtr = sCurLogFilePtr->mArchNxtLogFile;

        (void)removeArchLogFile(sCurLogFilePtr);

        sCurLogFilePtr = sNxtLogFilePtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* 아카이브 쓰레드를 깨워서 Archive LogFile List에서 aToFileNo까지
 * 로그파일들을 아카이브 시킨다.
 *
 * caution !!:
 * 이 함수를 호출하기 전에 반드시 aToFileNo까지 Archive LogFile List
 * 에 추가하고 이 함수를 호출하여야 한다.
 *
 * aToFileNo - [IN] aToFileNo까지 Archiving될때까지 대기한다.
 */
IDE_RC smrArchThread::wait4EndArchLF( UInt aToFileNo )
{
    SInt    sState = 0;
    UInt    sFstLFOfArchLFLst;
    idBool  sIsEmptyArchLFLst;

    // 아카이브 쓰레드가 작업을 완료할 때까지 기다린다.
    while(1)
    {
        IDE_TEST( lockThreadMtx() != IDE_SUCCESS );
        sState = 1;

        if( mResume != ID_TRUE )
        {
            // 아카이브 Thread가 깨어났을 때, signal에 의해 깨어난 경우
            // mResume이 ID_FALSE이면 cond_wait하는 interval을 재설정하고
            // mResume이 ID_TRUE이면 아카이빙을 실시한다.
            mResume = ID_TRUE;
            IDE_TEST_RAISE( mCv.signal() != IDE_SUCCESS, err_cond_signal );
        }

        sState = 0;
        IDE_TEST( unlockThreadMtx() != IDE_SUCCESS );

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_ARCH_WAITING,
                    aToFileNo);
        idlOS::sleep(5);

        IDE_TEST( getArchLFLstInfo( &sFstLFOfArchLFLst,
                                    &sIsEmptyArchLFLst )
                  != IDE_SUCCESS );

        /* BUG-23693: [SD] Online Backup시 LogFile을 강제 Switch시 현재 LogFile
         * 까지만 Archiving을 해야 합니다. */
        if( ( sIsEmptyArchLFLst == ID_TRUE ) ||
            ( sFstLFOfArchLFLst > aToFileNo ) )
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(unlockThreadMtx() == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;

}
