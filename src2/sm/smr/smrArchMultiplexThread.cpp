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
 * $Id: smrArchMultiplexThread.cpp $
 **********************************************************************/

#include <smErrorCode.h>
#include <smrRecoveryMgr.h>
#include <smrArchMultiplexThread.h>
#include <smrReq.h>

UInt         smrArchMultiplexThread::mMultiplexCnt;
idBool       smrArchMultiplexThread::mFinish;
smrLogFile * smrArchMultiplexThread::mSrcLogFile;

smrArchMultiplexThread::smrArchMultiplexThread() : idtBaseThread()
{
}

smrArchMultiplexThread::~smrArchMultiplexThread()
{
}

/***********************************************************************
 * Description : archMultiplex Thread를 다중화 된 수많큼 초기화한다.
 * 
 * aArchMultiplexThread - [IN] 초기화할 archMultiplexThread객체 pointer
 * aArchPath            - [IN] archive될 디렉토리 path들
 * aMultiplexCnt        - [IN] 초기화할 thread 수
 **********************************************************************/
IDE_RC smrArchMultiplexThread::initialize( 
                            smrArchMultiplexThread ** aArchMultiplexThread,
                            const SChar            ** aArchPath, 
                            UInt                      aMultiplexCnt )
{
    UInt sMultiplexIdx;
    UInt sStartedThreadCnt = 0;
    UInt sAllocState       = 0;

    IDE_DASSERT( aMultiplexCnt        != 0 );
    IDE_DASSERT( aArchMultiplexThread != NULL );

    mFinish       = ID_FALSE;
    mMultiplexCnt = aMultiplexCnt;

    /* smrArchMultiplexThread_initialize_calloc_ArchMultiplexThread.tc */
    IDU_FIT_POINT("smrArchMultiplexThread::initialize::calloc::ArchMultiplexThread");
    IDU_FIT_POINT("BUG-45313@smrArchMultiplexThread::initialize::calloc");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,
                                 (ULong)ID_SIZEOF( smrArchMultiplexThread ) * aMultiplexCnt,
                                 (void**)aArchMultiplexThread )
              != IDE_SUCCESS );
    sAllocState = 1;

    /* 각 thread에 필요한 정보 입력 */
    for( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        new(&((*aArchMultiplexThread)[sMultiplexIdx])) smrArchMultiplexThread();

        IDU_FIT_POINT("BUG-45313@smrArchMultiplexThread::initialize::initializeThread");
        IDE_TEST( (*aArchMultiplexThread)[sMultiplexIdx].initializeThread( 
                                                      aArchPath[sMultiplexIdx],
                                                      sMultiplexIdx ) 
                  != IDE_SUCCESS );
        
        sStartedThreadCnt++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for( sMultiplexIdx = 0; sMultiplexIdx < sStartedThreadCnt; sMultiplexIdx++ )
    {
        IDE_ASSERT( (*aArchMultiplexThread)[sMultiplexIdx].destroyThread() 
                    == IDE_SUCCESS );
    }

    if( sAllocState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( *aArchMultiplexThread ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : archMultiplex Thread를 초기화하고 시작한다.
 * 
 * aArchPath            - [IN] archive될 디렉토리 path
 * aMultiplexIdx        - [IN] 초기화할 thread의 idx 번호
 **********************************************************************/
IDE_RC smrArchMultiplexThread::initializeThread( const SChar * aArchPath,
                                                 UInt          aMultiplexIdx )
{
    SChar sMtxName[128] = {'\0',};
    SChar sCVName[128]  = {'\0',};
    UInt  sState        = 0;

    IDE_DASSERT( aArchPath != NULL );

    mArchivePath = aArchPath;
    IDE_TEST_RAISE( idf::access( aArchPath, F_OK) != 0,
                    path_exist_error);

    mMultiplexIdx = aMultiplexIdx;
    mThreadState  = SMR_LOG_FILE_BACKUP_THREAD_WAIT;

    idlOS::snprintf( sCVName, 
                     128,
                     "Archive Log Thread Cond Idx%"ID_UINT32_FMT,
                     aMultiplexIdx );
                     
    IDE_TEST( mCv.initialize(sCVName) != IDE_SUCCESS );
    sState = 1;

    idlOS::snprintf( sMtxName, 
                     128,
                     "Archive Log Thread Mutex Idx%"ID_UINT32_FMT,
                     aMultiplexIdx );

    IDE_TEST( mMutex.initialize( sMtxName,
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION(path_exist_error)
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVERY_ARCH_WARNING2 );
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, aArchPath));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mCv.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든 archMultiplex Thread 파괴
 **********************************************************************/
IDE_RC smrArchMultiplexThread::destroy( smrArchMultiplexThread * aArchMultiplexThread )
{
    UInt sMultiplexIdx;
    UInt sState = 1;

    for( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aArchMultiplexThread[sMultiplexIdx].destroyThread()
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( aArchMultiplexThread ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for( sMultiplexIdx++; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_ASSERT( aArchMultiplexThread[sMultiplexIdx].destroyThread() 
                    == IDE_SUCCESS );
    }

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( aArchMultiplexThread ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : thread를 종료하고 condition variable과 mutext를 파괴한다.
 **********************************************************************/
IDE_RC smrArchMultiplexThread::destroyThread()
{
    UInt sState = 2;

    IDE_TEST( mCv.destroy() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    sState = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( mCv.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든 thread들을 깨우고 log파일 archive(backup)이 
 *               완료될때까지 대기한다.
 * 
 * aArchMultiplexThread - [IN] 
 * aSrcLogFile          - [IN] archive대상 log파일
 **********************************************************************/
IDE_RC smrArchMultiplexThread::performArchving( 
                        smrArchMultiplexThread * aArchMultiplexThread,
                        smrLogFile             * aSrcLogFile )
{
    UInt sMultiplexIdx;

    IDE_DASSERT( aArchMultiplexThread != NULL );
    IDE_DASSERT( aSrcLogFile          != NULL );

    IDE_ASSERT( aSrcLogFile->mIsOpened == ID_TRUE );
    
    mSrcLogFile  = aSrcLogFile; 
    
    for( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aArchMultiplexThread[sMultiplexIdx].wakeUp() != IDE_SUCCESS );
    }

    IDE_TEST( wait( aArchMultiplexThread ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smrArchMultiplexThread::run()
{     
    SInt sRC;
    UInt sState = 0;

    startPos:

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    while( mFinish != ID_TRUE )
    {
        IDE_TEST_RAISE( mCv.wait(&mMutex) != IDE_SUCCESS, error_cond_wait );
                        

        if( mFinish == ID_TRUE )
        {
            mThreadState = SMR_LOG_FILE_BACKUP_THREAD_WAIT;
            break;
        }

        sRC = backupLogFile();
        if ( (sRC != IDE_SUCCESS) && (errno !=0) && (errno != ENOSPC) )
        {
            IDE_RAISE(error_archive);  
        }

        mThreadState = SMR_LOG_FILE_BACKUP_THREAD_WAIT;
    }

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return;
    IDE_EXCEPTION(error_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }

    IDE_EXCEPTION(error_archive);
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }
    
    mThreadState = SMR_LOG_FILE_BACKUP_THREAD_ERROR;
    
    if( mFinish == ID_FALSE )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_MRECOVERY_ARCH_WARNING3,
                    errno);

        idlOS::sleep(2);
        goto startPos;
    } 

    return;
}

/***********************************************************************
 * Description : condwait하고있는 thread들을 깨운다.
 **********************************************************************/
IDE_RC smrArchMultiplexThread::wakeUp()
{
    UInt sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS ); 
    sState = 1;

    mThreadState = SMR_LOG_FILE_BACKUP_THREAD_WAKEUP ;

    IDE_TEST_RAISE( mCv.signal() != IDE_SUCCESS, error_cond_signal );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS ); 

    return IDE_SUCCESS;
 
    IDE_EXCEPTION(error_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 1:
            IDE_ASSERT( unlock() == IDE_SUCCESS ); 
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : thread들이 log archive를 완료할때 까지 대기한다.
 *
 * aArchMultiplexThread - [IN] wait할 archMultiplexThread
 **********************************************************************/
IDE_RC smrArchMultiplexThread::wait( 
                        smrArchMultiplexThread * aArchMultiplexThread )
{
    UInt           sMultiplexIdx;
    PDL_Time_Value sTV;
    idBool         sIsDone  = ID_FALSE;
    idBool         sIsError = ID_FALSE;

    sTV.set( 0, 10 );

    while( (sIsDone == ID_FALSE) && (mFinish != ID_TRUE) )
    {
        sIsDone = ID_TRUE;
        for( sMultiplexIdx = 0; 
             sMultiplexIdx < mMultiplexCnt; 
             sMultiplexIdx++ )
        {
            if( aArchMultiplexThread[ sMultiplexIdx ].mThreadState == 
                                    SMR_LOG_FILE_BACKUP_THREAD_ERROR )
            {
                sIsError = ID_TRUE;
            }

            if( aArchMultiplexThread[ sMultiplexIdx ].mThreadState == 
                                    SMR_LOG_FILE_BACKUP_THREAD_WAKEUP )
            {
                sIsDone = ID_FALSE;
                break;
            }
        }

        idlOS::sleep( sTV );
    }

    IDE_TEST_RAISE( sIsError == ID_TRUE ,error_archive);  

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_archive);
    IDE_EXCEPTION_END;

    if( mFinish == ID_TRUE )
    {
        for( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
        {
            IDE_ASSERT( aArchMultiplexThread[sMultiplexIdx].wakeUp() == IDE_SUCCESS );
        }
    }


    return IDE_FAILURE;
}

/***********************************************************************
 * Description : log archive를 위해 log파일을 백업한다.
 *               smrArchThread::archLogFile()에 있던 코드들중 log파일을 
 *               backup하기위한 코드를 옴겨왔다.
 *
 * aArchMultiplexThread - [IN] wait할 archMultiplexThread
 **********************************************************************/
IDE_RC smrArchMultiplexThread::backupLogFile()
{
    UInt            sState = 0;
    SChar           sLogFileName[SM_MAX_FILE_NAME];
    SChar           sTempLogFileName[SM_MAX_FILE_NAME];
    UInt            sSystemErrno = 0;
    iduFile         sFile;
    ULong           sFileSize = 0;

    IDE_DASSERT( mArchivePath != NULL );
    IDE_DASSERT( mSrcLogFile  != NULL );

    idlOS::snprintf(sTempLogFileName, SM_MAX_FILE_NAME,
                   "%s%c__%s%"ID_UINT32_FMT"__",
                   mArchivePath,
                   IDL_FILE_SEPARATOR,
                   SMR_LOG_FILE_NAME,
                   mSrcLogFile->mFileNo); 
 
    idlOS::snprintf(sLogFileName, SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT,
                   mArchivePath,
                   IDL_FILE_SEPARATOR,
                   SMR_LOG_FILE_NAME,
                   mSrcLogFile->mFileNo); 
    /* ====================================
     * If the log file is already archived,
     * that log file is skipped.
     * ==================================== */
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
 
        if (sFileSize == (ULong)smuProperty::getLogFileSize())
        {
            sState = 1;
            IDE_TEST(sFile.close() != IDE_SUCCESS);
            sState = 0;
            IDE_TEST(sFile.destroy() != IDE_SUCCESS);
            IDE_CONT( skip_log_file_backup );
        }
        sState = 1;
        IDE_TEST(sFile.close() != IDE_SUCCESS);
        sState = 0;
        IDE_TEST(sFile.destroy() != IDE_SUCCESS);
    }
 
    errno = 0;
 
    if (mSrcLogFile->backup(sTempLogFileName) != IDE_SUCCESS)
    {
        sSystemErrno = ideGetSystemErrno();
        IDE_TEST_RAISE((sSystemErrno != 0) &&
                       (sSystemErrno != ENOSPC),
                 err_archiveBackup);
 
        /* ====================================================
         * In case archive_dir is full during Archive Log File
         * perform the following action according to property.
         * ==================================================== */
 
        /* ====================================================
         * CASE1 : Wait For free disk space
         * ==================================================== */
        if ( smuProperty::getArchiveFullAction() == 1 ) // WAITING
        {
            while ( 1 )
            {
                while (idlVA::getDiskFreeSpace( mArchivePath )
                       < (SInt)smuProperty::getLogFileSize())
                {
                    smLayerCallback::setEmergency( ID_TRUE );
                    if (smrRecoveryMgr::isFinish() == ID_TRUE)
                    {
                        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                                    SM_TRC_MRECOVERY_ARCH_FATAL1);
                        IDE_ASSERT(0);
                    }
                    else
                    {
                        ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                                    SM_TRC_MRECOVERY_ARCH_FATAL2);
                        idlOS::sleep(2);
                    }
                }

                smLayerCallback::setEmergency( ID_FALSE );

                errno = 0;
                if (mSrcLogFile->backup(sTempLogFileName) != IDE_SUCCESS)
                { 
                    sSystemErrno = ideGetSystemErrno();
                    IDE_TEST_RAISE((sSystemErrno != 0) &&
                                   (sSystemErrno != ENOSPC),
                                   err_archiveBackup);
                }
                else
                {
                    break;
                }
            }
        }
        /* ====================================================
         * CASE2 : return success, print out error message and skip archiving a logfile
         * ==================================================== */
        else // ERROR
        {
            /* BUG-42087 ARCHIVE_FULL_ACTION property does not operate as intended in manual
             * sSystemErrno 이 0 이거나 ENOSPC 일 경우에만 해당된다. */

            ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                        SM_TRC_MRECOVERY_ARCH_ABORT1,
                        sLogFileName);

            IDE_CONT( skip_log_file_backup );
        }
    }
 
    IDE_TEST(idf::rename(sTempLogFileName, sLogFileName) != 0 );

    IDE_EXCEPTION_CONT( skip_log_file_backup ); 
 
    return IDE_SUCCESS;
 
    IDE_EXCEPTION(err_archiveBackup);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_ARCH_ABORT2);
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT(sFile.close() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(sFile.destroy() == IDE_SUCCESS);
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smrArchMultiplexThread::startThread(
                    smrArchMultiplexThread * aArchMultiplexThread )
{
    UInt sMultiplexIdx;

    mFinish = ID_FALSE; 

    for( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST_RAISE(idf::access( 
                        aArchMultiplexThread[sMultiplexIdx].mArchivePath, F_OK) 
                        != 0,
                       path_exist_error);

        IDE_TEST( aArchMultiplexThread[sMultiplexIdx].start() != IDE_SUCCESS );
        IDE_TEST( aArchMultiplexThread[sMultiplexIdx].waitToStart(0) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
 
    IDE_EXCEPTION(path_exist_error)
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVERY_ARCH_WARNING2 );
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_NoExistPath, 
                    aArchMultiplexThread[sMultiplexIdx].mArchivePath));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrArchMultiplexThread::shutdownThread(
                    smrArchMultiplexThread * aArchMultiplexThread )
{
    UInt sMultiplexIdx;

    mFinish = ID_TRUE; 

    for( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aArchMultiplexThread[sMultiplexIdx].wakeUp() != IDE_SUCCESS );

        IDE_TEST( aArchMultiplexThread[sMultiplexIdx].join() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
 
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
