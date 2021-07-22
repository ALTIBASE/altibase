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
 * $$Id$
 * 로그 다중화 thread 구현 파일
 * 로그 다중화에는 append thread(log buffer에 로그 기록 다중화)
 *                 sync   thread(log file에 로그 기록 다중화)
 *                 create thread(log file 생성 다중화 )
 *                 delete thread(log file 삭제 다중화 )
 *  가 필요 하고 위 네가지 thread가 구현되어있다.
 **********************************************************************/

#include <smErrorCode.h>
#include <smr.h>
#include <smrReq.h>


/* appen log parameter */
volatile UInt        smrLogMultiplexThread::mOriginalCurFileNo;

/* sync log parameter */
idBool               smrLogMultiplexThread::mSyncLstPageInLstLF;
smrSyncByWho         smrLogMultiplexThread::mWhoSync;
UInt                 smrLogMultiplexThread::mOffsetToSync;
UInt                 smrLogMultiplexThread::mFileNoToSync;

/* create logfile parameter */
SChar              * smrLogMultiplexThread::mLogFileInitBuffer;
UInt               * smrLogMultiplexThread::mTargetFileNo;
UInt                 smrLogMultiplexThread::mLstFileNo;
SInt                 smrLogMultiplexThread::mLogFileSize;

/* delete logfile parameter */
idBool               smrLogMultiplexThread::mIsCheckPoint;
UInt                 smrLogMultiplexThread::mDeleteFstFileNo;
UInt                 smrLogMultiplexThread::mDeleteLstFileNo;

/* class member variable */
idBool               smrLogMultiplexThread::mFinish;
UInt                 smrLogMultiplexThread::mMultiplexCnt;
smrLogFileOpenList * smrLogMultiplexThread::mLogFileOpenList;
const SChar        * smrLogMultiplexThread::mOriginalLogPath;
UInt                 smrLogMultiplexThread::mOriginalLstLogFileNo;
idBool               smrLogMultiplexThread::mIsOriginalLogFileCreateDone;

smrLogMultiplexThread::smrLogMultiplexThread() : idtBaseThread()
{
}

smrLogMultiplexThread::~smrLogMultiplexThread()
{
}

/***********************************************************************
 * Description :
 * log파일 다중화를 수행하는 thread를 초기화한다.
 *
 * aSyncThread      - [IN/OUT] smrFileLogMgr의 syncThread pointer
 * aCreateThread    - [IN/OUT] smrFileLogMgr의 createThread pointer
 * aDeleteThread    - [IN/OUT] smrFileLogMgr의 deleteThread pointer
 * aAppendThread    - [IN/OUT] smrFileLogMgr의 appendThread pointer
 * aOriginalLogPath - [IN] 원복파일의 저장 위치
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::initialize( 
                                smrLogMultiplexThread  ** aSyncThread,
                                smrLogMultiplexThread  ** aCreateThread,
                                smrLogMultiplexThread  ** aDeleteThread,
                                smrLogMultiplexThread  ** aAppendThread,
                                const SChar             * aOriginalLogPath )
{
    UInt           sMultiplexIdx;
    const SChar ** sLogMultiplexPath;
    UInt           sState                = 0;
    UInt           sSyncState            = 0;
    UInt           sCreateState          = 0;
    UInt           sDeleteState          = 0;
    UInt           sAppendState          = 0;
    UInt           sLogFileOpenListState = 0;

    IDE_ASSERT( aSyncThread    != NULL );
    IDE_ASSERT( aCreateThread  != NULL );
    IDE_ASSERT( aDeleteThread  != NULL );
    IDE_ASSERT( aAppendThread  != NULL );

    sLogMultiplexPath  = smuProperty::getLogMultiplexDirPath();
    mMultiplexCnt      = smuProperty::getLogMultiplexCount();
    mFinish            = ID_FALSE;
    mOriginalLogPath   = aOriginalLogPath;

    IDE_TEST_CONT( mMultiplexCnt == 0, skip_multiplex_log_file );

    /* smrLogMultiplexThread_initialize_calloc_SyncThread.tc */
    IDU_FIT_POINT("smrLogMultiplexThread::initialize::calloc::SyncThread");
    IDE_TEST( iduMemMgr::calloc( 
                            IDU_MEM_SM_SMR,
                            1,
                            (ULong)ID_SIZEOF( smrLogMultiplexThread ) * mMultiplexCnt,
                            (void**)aSyncThread) 
              != IDE_SUCCESS );
    sState = 1;

    /* smrLogMultiplexThread_initialize_calloc_CreateThread.tc */
    IDU_FIT_POINT("smrLogMultiplexThread::initialize::calloc::CreateThread");
    IDE_TEST( iduMemMgr::calloc( 
                            IDU_MEM_SM_SMR,
                            1,
                            (ULong)ID_SIZEOF( smrLogMultiplexThread ) * mMultiplexCnt,
                            (void**)aCreateThread) 
              != IDE_SUCCESS );
    sState = 2;

    /* smrLogMultiplexThread_initialize_calloc_DeleteThread.tc */
    IDU_FIT_POINT("smrLogMultiplexThread::initialize::calloc::DeleteThread");
    IDE_TEST( iduMemMgr::calloc( 
                            IDU_MEM_SM_SMR,
                            1,
                            (ULong)ID_SIZEOF( smrLogMultiplexThread ) * mMultiplexCnt,
                            (void**)aDeleteThread) 
              != IDE_SUCCESS );
    sState = 3;

    /* smrLogMultiplexThread_initialize_calloc_AppendThread.tc */
    IDU_FIT_POINT("smrLogMultiplexThread::initialize::calloc::AppendThread");
    IDE_TEST( iduMemMgr::calloc( 
                            IDU_MEM_SM_SMR,
                            1,
                            (ULong)ID_SIZEOF( smrLogMultiplexThread ) * mMultiplexCnt,
                            (void**)aAppendThread) 
              != IDE_SUCCESS );
    sState = 4;

    /* 다중화된 log파일들의 open log file list를 위한 메모리 할당 */
    /* smrLogMultiplexThread_initialize_calloc_LogFileOpenList.tc */
    IDU_FIT_POINT("smrLogMultiplexThread::initialize::calloc::LogFileOpenList");
    IDE_TEST( iduMemMgr::calloc( 
                            IDU_MEM_SM_SMR,
                            1,
                            (ULong)ID_SIZEOF( smrLogFileOpenList ) * mMultiplexCnt,
                            (void**)&mLogFileOpenList) 
              != IDE_SUCCESS );
    sState = 5;

    /* thread type별 다중화 개수만큼 쓰레드 초기화 */
    for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        new(&((*aSyncThread)[sMultiplexIdx])) smrLogMultiplexThread();
        IDE_TEST( (*aSyncThread)[sMultiplexIdx].startAndInitializeThread( 
                                            sLogMultiplexPath[sMultiplexIdx],
                                            sMultiplexIdx,
                                            SMR_LOG_MPLEX_THREAD_TYPE_SYNC )
                  != IDE_SUCCESS );
        sSyncState++;

        new(&((*aCreateThread)[sMultiplexIdx])) smrLogMultiplexThread();
        IDE_TEST( (*aCreateThread)[sMultiplexIdx].startAndInitializeThread( 
                                            sLogMultiplexPath[sMultiplexIdx],
                                            sMultiplexIdx,
                                            SMR_LOG_MPLEX_THREAD_TYPE_CREATE )
                  != IDE_SUCCESS );
        sCreateState++;

        new(&((*aDeleteThread)[sMultiplexIdx])) smrLogMultiplexThread();
        IDE_TEST( (*aDeleteThread)[sMultiplexIdx].startAndInitializeThread( 
                                            sLogMultiplexPath[sMultiplexIdx],
                                            sMultiplexIdx,
                                            SMR_LOG_MPLEX_THREAD_TYPE_DELETE )
                  != IDE_SUCCESS );
        sDeleteState++;

        new(&((*aAppendThread)[sMultiplexIdx])) smrLogMultiplexThread();
        IDE_TEST( (*aAppendThread)[sMultiplexIdx].startAndInitializeThread( 
                                            sLogMultiplexPath[sMultiplexIdx],
                                            sMultiplexIdx,
                                            SMR_LOG_MPLEX_THREAD_TYPE_APPEND )
                  != IDE_SUCCESS );
        sAppendState++;

        /* 다중화 된 open log file list를 초기화 */
        IDE_TEST( initLogFileOpenList( &mLogFileOpenList[sMultiplexIdx],
                                       sMultiplexIdx ) 
                  != IDE_SUCCESS );
        sLogFileOpenListState++;
    }
    
    IDE_EXCEPTION_CONT( skip_multiplex_log_file ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        for ( sMultiplexIdx = 0; 
              sMultiplexIdx <= sLogFileOpenListState; 
              sMultiplexIdx++ )
        {
            IDE_ASSERT( destroyLogFileOpenList( &mLogFileOpenList[sMultiplexIdx],
                                                &(*aSyncThread)[sMultiplexIdx] )
                        == IDE_SUCCESS );
        }
 
        for ( sMultiplexIdx = 0; 
              sMultiplexIdx <= sSyncState; 
              sMultiplexIdx++ )
        {
            IDE_ASSERT( (*aSyncThread)[sMultiplexIdx].shutdownAndDestroyThread() 
                        == IDE_SUCCESS );
        }
 
        for ( sMultiplexIdx = 0; 
              sMultiplexIdx <= sCreateState; 
              sMultiplexIdx++ )
        {
            IDE_ASSERT( (*aCreateThread)[sMultiplexIdx].shutdownAndDestroyThread() 
                        == IDE_SUCCESS );
        }
 
        for ( sMultiplexIdx = 0; 
              sMultiplexIdx <= sDeleteState; 
              sMultiplexIdx++ )
        {
            IDE_ASSERT( (*aDeleteThread)[sMultiplexIdx].shutdownAndDestroyThread() 
                        == IDE_SUCCESS );
        }
 
        for ( sMultiplexIdx = 0; 
              sMultiplexIdx <= sAppendState; 
              sMultiplexIdx++ )
        {
            IDE_ASSERT( (*aAppendThread)[sMultiplexIdx].shutdownAndDestroyThread() 
                        == IDE_SUCCESS );
        }
    }
    switch( sState )
    {
        case 5:
            IDE_ASSERT( iduMemMgr::free( mLogFileOpenList ) == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( iduMemMgr::free( aAppendThread ) == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( iduMemMgr::free( aDeleteThread ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( iduMemMgr::free( aCreateThread ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( aSyncThread )   == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * log파일 다중화를 수행하는 thread를 destroy한다.
 *
 * aSyncThread    - [IN] smrLogMgr의 syncThread pointer
 * aCreateThread  - [IN] smrLogMgr의 createThread pointer
 * aDeleteThread  - [IN] smrLogMgr의 deleteThread pointer
 * aAppendThread  - [IN] smrLogMgr의 appendThread pointer
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::destroy( 
                            smrLogMultiplexThread * aSyncThread,
                            smrLogMultiplexThread * aCreateThread,
                            smrLogMultiplexThread * aDeleteThread,
                            smrLogMultiplexThread * aAppendThread )
{
    UInt sMultiplexIdx;
    UInt sState                = 5;
    UInt sSyncState            = mMultiplexCnt;
    UInt sCreateState          = mMultiplexCnt;
    UInt sDeleteState          = mMultiplexCnt;
    UInt sAppendState          = mMultiplexCnt;
    UInt sLogFileOpenListState = mMultiplexCnt;

    IDE_TEST_CONT( mMultiplexCnt == 0, skip_destroy );

    mFinish = ID_TRUE;

    for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        sAppendState--;
        IDE_TEST( aAppendThread[sMultiplexIdx].shutdownAndDestroyThread()
                  != IDE_SUCCESS );

        sSyncState--;
        IDE_TEST( aSyncThread[sMultiplexIdx].shutdownAndDestroyThread()
                  != IDE_SUCCESS );

        sCreateState--;
        IDE_TEST( aCreateThread[sMultiplexIdx].shutdownAndDestroyThread()
                  != IDE_SUCCESS );

        sDeleteState--;
        IDE_TEST( aDeleteThread[sMultiplexIdx].shutdownAndDestroyThread()
                  != IDE_SUCCESS );

        sLogFileOpenListState--;
        IDE_TEST( destroyLogFileOpenList( &mLogFileOpenList[sMultiplexIdx],
                                          &aSyncThread[sMultiplexIdx] )
                  != IDE_SUCCESS );
    }
    sState = 4;
    IDE_TEST( iduMemMgr::free( aSyncThread ) != IDE_SUCCESS );

    sState = 3;
    IDE_TEST( iduMemMgr::free( aCreateThread ) != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( iduMemMgr::free( aDeleteThread ) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( iduMemMgr::free( aAppendThread ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( mLogFileOpenList ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_destroy ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for ( sMultiplexIdx = ( mMultiplexCnt - sAppendState ); 
          sMultiplexIdx < mMultiplexCnt; 
          sMultiplexIdx++ )
    {
        IDE_ASSERT( aAppendThread[sMultiplexIdx].shutdownAndDestroyThread()
                    == IDE_SUCCESS );
    }

    for ( sMultiplexIdx = ( mMultiplexCnt - sSyncState ); 
          sMultiplexIdx < mMultiplexCnt; 
          sMultiplexIdx++ )
    {
        IDE_ASSERT( aSyncThread[sMultiplexIdx].shutdownAndDestroyThread()
                    == IDE_SUCCESS );
    }

    for ( sMultiplexIdx = ( mMultiplexCnt - sCreateState ); 
          sMultiplexIdx < mMultiplexCnt; 
          sMultiplexIdx++ )
    {
        IDE_ASSERT( aCreateThread[sMultiplexIdx].shutdownAndDestroyThread()
                    == IDE_SUCCESS );
    }

    for ( sMultiplexIdx = ( mMultiplexCnt - sDeleteState ); 
          sMultiplexIdx < mMultiplexCnt; 
          sMultiplexIdx++ )
    {
        IDE_ASSERT( aDeleteThread[sMultiplexIdx].shutdownAndDestroyThread()
                    == IDE_SUCCESS );
    }

    for ( sMultiplexIdx = ( mMultiplexCnt - sLogFileOpenListState ); 
          sMultiplexIdx < mMultiplexCnt; 
          sMultiplexIdx++ )
    {
        IDE_ASSERT( destroyLogFileOpenList( &mLogFileOpenList[sMultiplexIdx],
                                            &aSyncThread[sMultiplexIdx] )
                    == IDE_SUCCESS );
    }

    switch( sState )
    {
        case 5:
            IDE_ASSERT( iduMemMgr::free( aSyncThread ) == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( iduMemMgr::free( aCreateThread ) == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( iduMemMgr::free( aDeleteThread ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( iduMemMgr::free( aAppendThread ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( mLogFileOpenList ) == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 다중화 thread를 초기화하고 start 한다.
 *
 * aMultiplexPath  - [IN] 다중화 디렉토리 
 * aMultiplexIdx   - [IN] 쓰레드의 다중화 index 번호
 * aThreadType     - [IN] 초기화할 thread 타입(sync,append,create,delete)
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::startAndInitializeThread( 
                                    const SChar              * aMultiplexPath, 
                                    UInt                       aMultiplexIdx,
                                    smrLogMultiplexThreadType  aThreadType )
{
    UInt    sState = 0;
    SChar   sMutexName[128] = {'\0',};
    SChar   sCVName[128] = {'\0',};

    mMultiplexIdx  = aMultiplexIdx;
    mThreadType    = aThreadType;
    mThreadState   = SMR_LOG_MPLEX_THREAD_STATE_WAIT;
    mMultiplexPath = aMultiplexPath;

    /* 다중화 디렉토리 검사 */
    IDE_TEST( checkMultiplexPath() != IDE_SUCCESS );

    idlOS::snprintf( sCVName, 
                     128, 
                     "MULTIPLEX_LOG_TYPE_%"ID_UINT32_FMT"_COND_%"ID_UINT32_FMT, 
                     aThreadType,
                     aMultiplexIdx ); 

    IDE_TEST( mCv.initialize( sCVName ) != IDE_SUCCESS );
    sState = 1;

    idlOS::snprintf( sMutexName, 
                     128, 
                     "MULTIPLEX_LOG_TYPE_%"ID_UINT32_FMT"_MTX_%"ID_UINT32_FMT, 
                     aThreadType,
                     aMultiplexIdx ); 

    IDE_TEST( mMutex.initialize( sMutexName,
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 2;

    /* 
     * append thread는 서버 시작시 log file open list에 마지막
     * log file이 추가된 이후에 시작한다. 
     */
    if ( mThreadType != SMR_LOG_MPLEX_THREAD_TYPE_APPEND )
    {
        IDE_TEST( startThread() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

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
 * Description :
 * 다중화 log파일을 생성할 디렉토리를 검사한다.
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::checkMultiplexPath()
{
    IDE_TEST_RAISE(idf::access( mMultiplexPath, F_OK) != 0,
                   path_exist_error);

    IDE_TEST_RAISE(idf::access( mMultiplexPath, W_OK) != 0,
                   err_no_write_perm_path);

    IDE_TEST_RAISE(idf::access( mMultiplexPath, X_OK) != 0, 
                   err_no_execute_perm_path);

    return IDE_SUCCESS;

    IDE_EXCEPTION(path_exist_error)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath,
                                mMultiplexPath));
    }
    IDE_EXCEPTION(err_no_write_perm_path);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoWritePermFile,
                                mMultiplexPath));
    }
    IDE_EXCEPTION(err_no_execute_perm_path);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExecutePermFile,
                                mMultiplexPath));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 다중화 thread를 종료하고 destroy 한다.
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::shutdownAndDestroyThread()
{
    UInt sState = 2;

    IDE_ASSERT( mFinish == ID_TRUE );

    if ( mThreadType != SMR_LOG_MPLEX_THREAD_TYPE_APPEND )
    {
        /* condwait하고있는 thread를 깨운다 */
        IDE_TEST( wakeUp() != IDE_SUCCESS );
    }

    IDE_TEST( join() != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( mCv.destroy() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    
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
 * Description :
 * smrLFThread::syncToLSN에서 호출된다.
 * 다중화된 log의 내용을 다중화된 log file로 sync한다.
 *
 * aSyncThread             - [IN] log sync thread 들
 * aWhoSync,               - [IN] smrLFThread에 의한 sync인지 tx에의한 sync인지
 * aSyncLstPageInLstLF     - [IN] log file의 마지막 페이를 쓸것인지
 * aFileNoToSync           - [IN] sync할 fileno 마지막
 * aOffsetToSync           - [IN] sync할 마지막 파일의 offset
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::wakeUpSyncThread( 
                                smrLogMultiplexThread  * aSyncThread,
                                smrSyncByWho             aWhoSync,
                                idBool                   aSyncLstPageInLstLF,
                                UInt                     aFileNoToSync,
                                UInt                     aOffsetToSync )
{
    UInt sMultiplexIdx;

    IDE_TEST_CONT( mMultiplexCnt == 0, skip_sync );

    IDE_ASSERT( aSyncThread != NULL );

    IDE_ASSERT( aSyncThread[0].mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_SYNC );

    /* log file sync를 위한 인자 전달 */
    mSyncLstPageInLstLF = aSyncLstPageInLstLF;
    mWhoSync            = aWhoSync;
    mFileNoToSync       = aFileNoToSync;
    mOffsetToSync       = aOffsetToSync;

    /* 다중화된 수 만큼 sync thread를 wakeup 시킨다. */
    for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aSyncThread[sMultiplexIdx].wakeUp() != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( skip_sync ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * smrLogFileMgr::addEmptyLogFile에서 호출된다.
 * 다중화 로그파일을 미리 생성한다.
 *
 * aCreateThread       - [IN] log file create thread 들
 * aTargetFileNo       - [IN] log file 생성을 시작할 file 번호
 * aLstFileNo          - [IN] 마지막에 생성된 file 번호
 * aLogFileInitBuffer  - [IN] log file의 내용을 초기화할 buffer
 * aLogFileSize        - [IN] 생성할 log file 크기
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::wakeUpCreateThread( 
                                    smrLogMultiplexThread * aCreateThread,
                                    UInt                  * aTargetFileNo,
                                    UInt                    aLstFileNo,
                                    SChar                 * aLogFileInitBuffer,
                                    SInt                    aLogFileSize )
{
    UInt sMultiplexIdx;

    IDE_TEST_CONT( mMultiplexCnt == 0, skip_create );

    IDE_ASSERT( aCreateThread      != NULL );
    IDE_ASSERT( aLogFileInitBuffer != NULL );

    IDE_ASSERT( aCreateThread[0].mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );
    
    /* log file create를 위한 인자 전달 */
    mLogFileInitBuffer = aLogFileInitBuffer;
    mTargetFileNo      = aTargetFileNo;      
    mLstFileNo         = aLstFileNo;      
    mLogFileSize       = aLogFileSize; 
    
    /* 다중화된 수 만큼 log file create thread를 wakeup 시킨다. */
    for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aCreateThread[sMultiplexIdx].wakeUp() != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( skip_create ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * smrLogFileMgr::removeLogFile에서 호출된다.
 * 다중화 로그파일 삭제한다.
 *
 * aDeleteThread        - [IN] log file delete thread 들
 * aDeleteFstFileNo     - [IN] 삭제할 log파일의 첫번째 file 번호
 * aDeleteLstFileNo     - [IN] 삭제할 log파일의 마지막 file 번호
 * aIsCheckPoint        - [IN] checkpoint에의해 호출되는지 여부
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::wakeUpDeleteThread( 
                                        smrLogMultiplexThread * aDeleteThread,
                                        UInt                    aDeleteFstFileNo,
                                        UInt                    aDeleteLstFileNo,
                                        idBool                  aIsCheckPoint )
{
    UInt sMultiplexIdx;

    IDE_TEST_CONT( mMultiplexCnt == 0, skip_delete );

    IDE_ASSERT( aDeleteThread != NULL );

    IDE_ASSERT( aDeleteThread[0].mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_DELETE );
    
    /* log file delete를 위한 인자 전달 */
    mDeleteFstFileNo = aDeleteFstFileNo;
    mDeleteLstFileNo = aDeleteLstFileNo;
    mIsCheckPoint    = aIsCheckPoint;     

    /* 다중화된 수 만큼 log file delete thread를 wakeup 시킨다. */
    for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aDeleteThread[sMultiplexIdx].wakeUp() != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( skip_delete ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * append, sync, create, delete thread 모두 run함수를 이용한다. 
 * thread type에 맞는 함수를 수행한다.
 * 항상 condwait상태이면 각 thread의 wakeup함수에해서만 깨어난다.(append
 * Thread는 제외 )
 * wakeup되었을때 thread의 상태는 SMR_LOG_MPLEX_THREAD_STATE_WAKEUP이여만 하고
 * sleep(condwait)일때는 thread의 상태가 SMR_LOG_MPLEX_THREAD_STATE_WAIT이여만 한다.
 ***********************************************************************/
void smrLogMultiplexThread::run()
{     
    PDL_Time_Value  sTV;
    UInt            sSkipCnt = 0;
    UInt            sSpinCount;
    SInt            sRC;
    UInt            sState = 0;

    sSpinCount = smuProperty::getLogMultiplexThreadSpinCount();
    sTV.set( 0, 1 );

    IDE_TEST( lock() != IDE_SUCCESS ); 
    sState = 1;

    while( mFinish != ID_TRUE )
    {
        IDE_ASSERT( mThreadState == SMR_LOG_MPLEX_THREAD_STATE_WAIT );
    
        if ( mThreadType != SMR_LOG_MPLEX_THREAD_TYPE_APPEND )
        { 
            IDE_TEST_RAISE( mCv.wait(&mMutex) != IDE_SUCCESS, error_cond_wait );

            /* BUG-44779 AIX에서 Spurious wakeups 현상으로
             * 깨우지 않은 Thread가 Wakeup되는 경우가 있습니다.*/
            while ( mThreadState != SMR_LOG_MPLEX_THREAD_STATE_WAKEUP )
            {
                ideLog::log( IDE_SM_0,
                             "MultiplexThread spurious wakeups (Thread State : %"ID_UINT32_FMT")\n",
                             mThreadState );

                IDE_TEST_RAISE( mCv.wait(&mMutex) != IDE_SUCCESS, error_cond_wait );
            }
        }

        if ( mFinish == ID_TRUE )
        {
            break;
        }

        switch( mThreadType )
        {
            case SMR_LOG_MPLEX_THREAD_TYPE_CREATE:
            {
                sRC = createLogFile();
                break;
            }
            case SMR_LOG_MPLEX_THREAD_TYPE_DELETE:
            {
                sRC = deleteLogFile();
                break;
            }
            case SMR_LOG_MPLEX_THREAD_TYPE_SYNC:
            {
                sRC = syncLog();
                break;
            }
            case SMR_LOG_MPLEX_THREAD_TYPE_APPEND:
            {
                sRC = appendLog( &sSkipCnt );

                if ( (sSkipCnt > sSpinCount) && (sSpinCount != ID_UINT_MAX) )
                {
                    idlOS::sleep( sTV );
                    sSkipCnt = 0;
                }
                else
                {
                    /* sleep하지 않음 */
                }

                break;
            }
            default:
            {
                IDE_ASSERT(0);
            }
        }

        if ( ( sRC != IDE_SUCCESS ) && ( errno !=0  ) && ( errno != ENOSPC ) )
        {
            mThreadState = SMR_LOG_MPLEX_THREAD_STATE_ERROR;
            IDE_RAISE(error_log_multiplex);  
        }
        else
        {
            mThreadState = SMR_LOG_MPLEX_THREAD_STATE_WAIT;
        }
    }

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS ); 

    return;

    IDE_EXCEPTION(error_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION(error_log_multiplex);
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS ); 
    }

    ideLog::log( IDE_SERVER_0, 
                 "Multiplex Thread Type: %u\n"
                 "Multiplex index      : %u\n"
                 "Multiplex LogFileNo  : %u\n"
                 "MultiplexPath        : %s\n",
                 mThreadType,
                 mMultiplexIdx,
                 mMultiplexLogFile->mFileNo,
                 mMultiplexPath ); 

    IDE_ASSERT(0);

    return;
}

/***********************************************************************
 * Description :
 * condwait하고있는 thread를 깨운다.
 * wakeup되기전에 thread의 상태를 SMR_LOG_MPLEX_THREAD_STATE_WAKEUP상태로
 * 설정한다.
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::wakeUp()
{
    UInt sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS ); 
    sState = 1;

    /* thread 상태를 wakeup상태로 설정 */
    mThreadState = SMR_LOG_MPLEX_THREAD_STATE_WAKEUP;

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
 * Description :
 * thread 수행이 완료될때까지 대기한다.
 * aThread      - [IN] 수행이 완료되기를 기다릴 thread들
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::wait( smrLogMultiplexThread * aThread )
{
    UInt            sMultiplexIdx;
    PDL_Time_Value  sTV;
    idBool          sIsDone  = ID_FALSE;
    idBool          sIsError = ID_FALSE;
     
    IDE_TEST_CONT( mMultiplexCnt == 0, skip_wait );

    IDE_ASSERT( aThread != NULL );
    IDE_ASSERT( aThread[0].mThreadType != SMR_LOG_MPLEX_THREAD_TYPE_APPEND );

    sTV.set( 0, 10 );

    while( (sIsDone == ID_FALSE) && (mFinish != ID_TRUE) )
    {
        sIsDone = ID_TRUE;
        for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
        {
            /* 예외가 발생한 thread가있는지 확인 */
            if ( aThread[sMultiplexIdx].mThreadState == 
                                        SMR_LOG_MPLEX_THREAD_STATE_ERROR )
            {
                sIsError = ID_TRUE;
            }

            /* SMR_LOG_MPLEX_THREAD_STATE_WAKEUP상태인 thread가 
             * 존재하면 계속 대기
             */
            if ( aThread[sMultiplexIdx].mThreadState == 
                                        SMR_LOG_MPLEX_THREAD_STATE_WAKEUP )
            {
                sIsDone = ID_FALSE;
                break;
            }
        }

        idlOS::sleep( sTV );
    }

    IDE_TEST_RAISE( sIsError == ID_TRUE, error_log_multiplex );

    IDE_EXCEPTION_CONT( skip_wait ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_log_multiplex);
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 다중화된 log file buffer에 원본 log file buffer의 로그를 복사한다.
 *
 * aSkipCnt - [OUT] 함수가 호출되었지만 로그가 복사되지 않은 횟수
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::appendLog( UInt * aSkipCnt )
{
    smrLogFileOpenList  * sOpenLogFileList;
    smrLogFile          * sPrevOriginalLogFile;
    volatile UInt         sOriginalCurFileNo;

    SInt        sCopySize;
    UInt        sLogFileSize;
    UInt        sBarrier        = 0;
    SChar     * sCopyData;
    idBool      sIsSwitchOriginalLogFile;
    idBool      sIsNeedRestart  = ID_FALSE; /* BUG-38801 */
    smLSN       sLstLSN;                    /* BUG-38801 */
    smLSN       sSyncLSN;                   /* BUG-35392 */

    IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_APPEND );
    SM_LSN_MAX ( sLstLSN );
    SM_LSN_MAX ( sSyncLSN );

    sOpenLogFileList = getLogFileOpenList();

    IDE_ASSERT( sOpenLogFileList->mLogFileOpenListLen != 0 );
    IDE_ASSERT( (mMultiplexLogFile  != NULL) && 
                (mOriginalLogFile   != NULL) );
    IDE_ASSERT( mOriginalLogFile->mFileNo == mMultiplexLogFile->mFileNo );

    sLogFileSize        = smuProperty::getLogFileSize();
    sOriginalCurFileNo  = mOriginalCurFileNo;

    while( mMultiplexLogFile->mFileNo <= sOriginalCurFileNo )
    {
        IDE_ASSERT( mMultiplexLogFile->mFileNo == mOriginalLogFile->mFileNo );

        /* code reodering을 방지 */
        IDE_ASSERT( sBarrier == 0 );
        if ( sBarrier++ == 0 )
        {
            sIsSwitchOriginalLogFile = mOriginalLogFile->mSwitch;

            /* BUG-35392 
             * 원본 log file이 switch되더라도 dummy log가 존재할 수 있다.
             * 다중화 log file은 dummy log를 완전한 log로 덮어쓰는 연산이 없다.
             * 따라서 switch된 원본 log file에 dummy log가 존재하지 않음을
             * 보장해야한다. */
            if ( sIsSwitchOriginalLogFile == ID_TRUE )
            {
                SM_SET_LSN( sSyncLSN,
                            mOriginalLogFile->mFileNo,
                            mOriginalLogFile->mOffset );

                // Sync 가능한 최대의 LSN 까지 대기
                smrLogMgr::waitLogSyncToLSN( &sSyncLSN,
                                             smuProperty::getLFThrSyncWaitMin(),
                                             smuProperty::getLFThrSyncWaitMax() );
            }
        }

        /* BUG-35392
         * logfile에 저장된 Log의 마지막 Offset을 받아온다. 
         * FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 더미를 포함하지 않는 로그 레코드의 Offset */
        smrLogMgr::getLstLogOffset( &sLstLSN );

        if ( sBarrier++ == 1 )
        {
            /* 다중화 로그버퍼로 복사할 크기 계산 */
            if ( sLstLSN.mFileNo == mMultiplexLogFile->mFileNo )
            {
                /* LstLSN 이 현재 복사할 mMultiplex와 같은 로그파일 이라면
                 * LstLSN 까지는 복사해도 된다. */
                sCopySize = sLstLSN.mOffset - mMultiplexLogFile->mOffset;
            }
            else
            {
                /* LstLSN이 mMultiplex 와 같지 않다면, ( LstLSN > mMuiltiplex )
                 * multiplex 할 로그파일이 1개 이상 쌓였다는 의미.
                 * 따라서, LstLSN이 아닌, mOriginal 에서 복사할 범위를 받아온다. */
                sCopySize = mOriginalLogFile->mOffset - mMultiplexLogFile->mOffset;

                IDE_ASSERT( mOriginalLogFile->mOffset >= mMultiplexLogFile->mOffset );
            }
        }

        /* 다중화 로그 버퍼로 복사할 로그 포인터 위치 계산 */
        sCopyData = ((SChar*)mOriginalLogFile->mBase)  + mMultiplexLogFile->mOffset;

        /* BUG-45711 UncompletedLstLSN을 동시에 갱신하면서 
         * sLstLSN이 mMultiplexLogFile.LSN보다 작게 들어올수 있다. 
           이미 해당 로그 이후까지 multiplexing 된거니까 넘어가면 됨. */
        if ( sCopySize > 0 )
        {
            IDE_ASSERT( (mMultiplexLogFile->mOffset + sCopySize) <= sLogFileSize );
            mMultiplexLogFile->append( sCopyData, sCopySize );
            (*aSkipCnt) = 0;
        }
        else
        {
            if ( sIsSwitchOriginalLogFile == ID_TRUE )
            {
                checkOriginalLogSwitch( &sIsNeedRestart );

                if ( sIsNeedRestart == ID_TRUE )
                {
                    sOriginalCurFileNo = mOriginalCurFileNo;
                    sBarrier = 0;
                    continue;
                }

                /* 다음 로그파일이 log file open list에 존재하지 않으면
                 * 다중화 create Thread가 다음 로그파일을 만들고 list에
                 * 매달때까지 대기한다. */
                if ( (mMultiplexLogFile->mFileNo + 1) 
                    != mMultiplexLogFile->mNxtLogFile->mFileNo )
                {
                    wait4NextLogFile( mMultiplexLogFile );
                }

                /* 다중화 로그파일을 switch하고 다음 로그파일을 가져온다. */
                mMultiplexLogFile->mSwitch = ID_TRUE;
                mMultiplexLogFile          = mMultiplexLogFile->mNxtLogFile;

                sPrevOriginalLogFile = mOriginalLogFile;
                mOriginalLogFile     = mOriginalLogFile->mNxtLogFile;

                /* 
                 * 다중화 로그버퍼에 모든 로그가 반영이 되었음을
                 * 알린다.(SMR_LT_FILE_END까지)
                 * 로그가 다중화 되지 않았다면 다중화가 완료될때까지 
                 * smrLFThread에서 로그 파일을 close하지 않는다.
                 */
                sPrevOriginalLogFile->mMultiplexSwitch[mMultiplexIdx] = ID_TRUE;
            }
            else
            {
                if ( mMultiplexLogFile->mFileNo == sOriginalCurFileNo )
                {
                    /* 모든 원본 로그를 다중화 함 */
                    (*aSkipCnt)++;
                    break;
                }
                else
                {
                    IDE_ASSERT( mMultiplexLogFile->mFileNo <
                                sOriginalCurFileNo );
                    /* 
                     * 다중화 해야할 로그가 남아있음 
                     * continue 
                     */
                }
            }
        }

        sOriginalCurFileNo = mOriginalCurFileNo;
        sBarrier = 0;
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 * 다중화된 log file buffer의내용을 log file로 sync한다.
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::syncLog()
{
    smrLogFile         * sNxtLogFile;
    smrLogFile         * sCurLogFile;
    smrLogFile         * sOpenLogFileListHdr;
    UInt                 sSyncOffset;
    UInt                 sCurFileNo;
    idBool               sSyncLstPage;
    idBool               sIsSwitchLF;

    IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_SYNC );

    sOpenLogFileListHdr = getOpenFileListHdr();

    sCurLogFile = sOpenLogFileListHdr->mNxtLogFile;

    /* 
     * BUG-36025 Server can be abnormally shutdown when multiplex log sync thread
     * works slowly than LFThread 
     * 다중화 로그 sync thread가 더디게 동작하게 되면 원본 로그파일보다 더많은
     * 로그파일을 sync할 수 있다. 따라서 assert code를 제거하고 if문으로 대체하여
     * altibase_sm.log에 메세지를 찍어주고 해당 다중호 로그파일 sync를 skip한다.
     * 메세지는 page가 버퍼에서 쫓겨나 로그가 sync될때에만 기록된다.(WAL)
     * LFThread가 스스로 동작하여 로그를 sync할때에는 mFileNoToSync가 UINT MAX로
     * 되기때문에 해당 메세지가 찍히지 않는다.
     */
    if ( sCurLogFile->mFileNo > mFileNoToSync )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     "Mutiplex %s%clogfile%u is already synced",
                     mMultiplexPath, 
                     IDL_FILE_SEPARATOR, 
                     mFileNoToSync );

        IDE_CONT( skip_sync );
    }
    else
    {
        /* nothing to do */
    }

    /* FIT/ART/sm/Bugs/BUG-36025/BUG-36025.sql */
    IDU_FIT_POINT( "1.BUG-36025@smrLogMultiplexThread::syncLog" );

    while( sCurLogFile != sOpenLogFileListHdr )
    {
        sNxtLogFile = sCurLogFile->mNxtLogFile;
        sCurFileNo  = sCurLogFile->mFileNo;

        if ( sCurLogFile->getEndLogFlush() == ID_FALSE )
        {
            sIsSwitchLF = sCurLogFile->mSwitch; 

            if ( ( sCurFileNo == mFileNoToSync ) &&
                 ( sIsSwitchLF == ID_FALSE ) )
            {
                /* BUG-37018 There is some mistake on logfile Offset calculation 
                 * log file을 sync 할때 다중화 log file buffer에 기록된 log의
                 * offset이 요청된 mOffsetToSync보다 클때까지 대기한다. */
                while( sCurLogFile->mOffset < mOffsetToSync )
                {
                    idlOS::thr_yield();
                }

                sSyncOffset  = mOffsetToSync;
                sSyncLstPage = mSyncLstPageInLstLF;
            }
            else
            {
                sSyncOffset  = sCurLogFile->mOffset;
                sSyncLstPage = ID_TRUE;
            }

            /* log file을 sync한다. */
            IDE_TEST( sCurLogFile->syncLog( sSyncLstPage, sSyncOffset ) 
                      != IDE_SUCCESS );

            if ( sIsSwitchLF == ID_TRUE )
            {
                /* 더 이상 로그파일 중 디스크에 기록할 로그가 없이
                 * 모두 디스크에 반영이 되었다. */
                sCurLogFile->setEndLogFlush( ID_TRUE );
            }
            else
            {
                break;
            }
        }

        /* 
         * sync한 log file이 switch되어있다면 
         * log file을 list에 유지할 필요가 없다
         */
        if ( ( sCurLogFile->getEndLogFlush() == ID_TRUE ) && 
             ( mWhoSync == SMR_LOG_SYNC_BY_LFT ) )
        {
            IDE_TEST( closeLogFile( sCurLogFile, 
                                    ID_TRUE ) /*aRemoveFromList*/
                      != IDE_SUCCESS );
        }

        if ( sCurFileNo == mFileNoToSync )
        {
            break;
        }

        sCurLogFile = sNxtLogFile;
    }

    IDE_EXCEPTION_CONT( skip_sync );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 다중화 log file을 생성하고 log file open list에 추가한다.
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::createLogFile()
{
    smrLogFile     * sNewLogFile;
    SChar            sLogFileName[SM_MAX_FILE_NAME] = {'\0',};
    PDL_Time_Value   sTV;
    UInt             sLstFileNo;
    UInt             sState = 0;

    IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );

    sLstFileNo = mLstFileNo;

    idlOS::snprintf(sLogFileName,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT"",
                    mMultiplexPath,
                    IDL_FILE_SEPARATOR,
                    SMR_LOG_FILE_NAME,
                    sLstFileNo);

    /* 원본로그파일의 마지막 로그파일 번호를 가진 로그파일이 존재하는지 확인 */
    IDE_ASSERT( idf::access(sLogFileName, F_OK) == 0 );

    sTV.set( 0, 1 ); 

    while(1)
    {
        while( (*mTargetFileNo) + smuProperty::getLogFilePrepareCount() >
               sLstFileNo + 1 )
        {
            sLstFileNo++;
 
            if ( sLstFileNo == ID_UINT_MAX )
            {
                sLstFileNo = 0;
            }
 
            idlOS::snprintf(sLogFileName,
                            SM_MAX_FILE_NAME,
                            "%s%c%s%"ID_UINT32_FMT"",
                            mMultiplexPath,
                            IDL_FILE_SEPARATOR,
                            SMR_LOG_FILE_NAME,
                            sLstFileNo);
  
            /* smrLogMultiplexThread_createLogFile_calloc_NewLogFile.tc */
            IDU_FIT_POINT("smrLogMultiplexThread::createLogFile::calloc::NewLogFile");
            IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                         1,
                                         ID_SIZEOF( smrLogFile ),
                                         (void**)&sNewLogFile)
                              != IDE_SUCCESS );
            sState = 1;
  
            IDE_TEST( sNewLogFile->initialize() != IDE_SUCCESS ); 
            sState = 2;
  
            IDE_TEST( sNewLogFile->create( sLogFileName,
                                           mLogFileInitBuffer,
                                           mLogFileSize )
                      != IDE_SUCCESS );
            sState = 3;
  
            IDE_TEST( sNewLogFile->open( sLstFileNo,
                                         sLogFileName,
                                         ID_TRUE, /* aIsMultiplexLogFile */
                                         smuProperty::getLogFileSize(),
                                         ID_TRUE) 
                      != IDE_SUCCESS );
            sState = 4;
  
            sNewLogFile->mFileNo   = sLstFileNo; 
            sNewLogFile->mOffset   = 0;
            sNewLogFile->mFreeSize = smuProperty::getLogFileSize();
            sNewLogFile->mRef++;
            (void)sNewLogFile->touchMMapArea();
  
            IDE_TEST( add2LogFileOpenList( sNewLogFile, ID_TRUE ) 
                      != IDE_SUCCESS );
        }
 
        /* 
         * 원본 로그파일의 mTargetFileNo가 로그파일 생성 도중 바뀔수 있음으로
         * 만들어진 마지막 원본 로그파일의 번호를 확인한다.
         */
        if ( ( mIsOriginalLogFileCreateDone != ID_TRUE ) || 
             ( mOriginalLstLogFileNo > sLstFileNo ) )
        {
            idlOS::sleep( sTV );
        }
        else
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 4:
            IDE_ASSERT( sNewLogFile->close() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( sNewLogFile->remove( sLogFileName, ID_FALSE ) 
                        == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sNewLogFile->destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( sNewLogFile ) == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * 다중화 log file을 삭제한다.
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::deleteLogFile()
{
    smrLogFileMgr::removeLogFiles( mDeleteFstFileNo, 
                                   mDeleteLstFileNo, 
                                   mIsCheckPoint,
                                   ID_TRUE,
                                   mMultiplexPath );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 * log file을 open하고 log file open list에 추가한다.
 * server startup시에만 호출된다.
 *
 * aLogFileNo - [IN]  open할 log file 번호
 * aAddToList - [IN]  open할 log file을 list에 추가할것인지 결정
 * aLogFile   - [OUT] open한 log file
 * aISExist   - [OUT] open할 log file이 존재하는지 반환
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::openLogFile( UInt          aLogFileNo, 
                                           idBool        aAddToList,
                                           smrLogFile ** aLogFile,
                                           idBool      * aIsExist )
{
    smrLogFileOpenList * sLogFileOpenList;
    smrLogFile         * sNewLogFile;
    SChar                sLogFileName[SM_MAX_FILE_NAME] = {'\0',};
    UInt                 sState = 0;

    IDE_ASSERT( (mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_CREATE) ||
                (mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_SYNC) );

    IDE_ASSERT( aLogFile != NULL );

    idlOS::snprintf(sLogFileName,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT,
                    mMultiplexPath,
                    IDL_FILE_SEPARATOR,
                    SMR_LOG_FILE_NAME,
                    aLogFileNo);

    sLogFileOpenList = getLogFileOpenList();

    IDE_TEST( sLogFileOpenList->mLogFileOpenListMutex.lock( NULL ) 
              != IDE_SUCCESS );
    sState = 1;
    
    if ( idf::access(sLogFileName, F_OK) == 0 )
    {
        sNewLogFile = findLogFileInList( aLogFileNo, sLogFileOpenList );

        if ( sNewLogFile == NULL )
        {
            /* smrLogMultiplexThread_openLogFile_calloc_NewLogFile.tc */
            IDU_FIT_POINT("smrLogMultiplexThread::openLogFile::calloc::NewLogFile");
            IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                         1,
                                         ID_SIZEOF( smrLogFile ),
                                         (void**)&sNewLogFile)
                      != IDE_SUCCESS );
            sState = 2;
  
            IDE_TEST( sNewLogFile->initialize() != IDE_SUCCESS ); 
            sState = 3;
  
            IDE_TEST( sNewLogFile->open( aLogFileNo,
                                         sLogFileName,
                                         ID_TRUE, /* aIsMultiplexLogFile */
                                         smuProperty::getLogFileSize(),
                                         ID_TRUE ) 
                      != IDE_SUCCESS );
            sState = 4;
  
            sNewLogFile->mFileNo   = aLogFileNo; 
            sNewLogFile->mOffset   = 0;
            sNewLogFile->mFreeSize = smuProperty::getLogFileSize();
            sNewLogFile->mRef++;
            (void)sNewLogFile->touchMMapArea();
 
            if ( aAddToList == ID_TRUE )
            {
                IDE_TEST( add2LogFileOpenList( sNewLogFile, ID_FALSE ) 
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }

        *aIsExist = ID_TRUE;
        *aLogFile = sNewLogFile;
    }
    else
    {
        *aLogFile = NULL;
        *aIsExist = ID_FALSE;
    }

    sState = 0;
    IDE_TEST( sLogFileOpenList->mLogFileOpenListMutex.unlock() 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 4:
            IDE_ASSERT( sNewLogFile->close() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( sNewLogFile->destroy() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( iduMemMgr::free( sNewLogFile ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sLogFileOpenList->mLogFileOpenListMutex.unlock() 
                        == IDE_SUCCESS );
        case 0:
        default:
            break;
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * BUG-35051 서버 생성이후 바로 로그 다중화 프로퍼티를 설정하면 비정상
 * 종료합니다. 
 *
 * 서버 정상 종료 이후 다시 시작시 smrLogFileMgr::preOpenLogFile()에 의해
 * 호출됨. 
 * 서버 시작시 prepare된 다중화 로그파일을 open한다. prepare된 로그파일이 
 * 존재하지 않는다면 생성해주고 open한다.
 * 본 함수는 로그파일안에 로그가 기록되지않은 Prepare된 로그파일을 
 * open함으로 파일을 생성하더라도 문제가 없다.
 *
 * aLogFileNo           - [IN] open할 log file 번호
 * aLogFileInitBuffer   - [IN] 로그파일 생성시 사용될 초기화 버퍼 
 * aLogFileSize         - [IN] 로그파일 크기
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::preOpenLogFile( UInt    aLogFileNo,
                                              SChar * aLogFileInitBuffer,
                                              UInt    aLogFileSize )
{
    smrLogFile * sMultiplexLogFile;
    smrLogFile   sNewLogFile;
    idBool       sLogFileExist                  = ID_FALSE;
    SChar        sLogFileName[SM_MAX_FILE_NAME] = {'\0',};
    UInt         sState = 0;
    idBool       sLogFileState = ID_FALSE;

    IDE_ASSERT( aLogFileInitBuffer != NULL );
    IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );

    idlOS::snprintf(sLogFileName,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT,
                    mMultiplexPath,
                    IDL_FILE_SEPARATOR,
                    SMR_LOG_FILE_NAME,
                    aLogFileNo);

    /* prepare된 aLogFileNo를 가진 로그파일이 다중화 디렉토리에 
     * 존재하는지 확인한다. 없다면 해당 로그파일을 만들어 준다 */ 
    if ( idf::access(sLogFileName, F_OK) != 0 )
    {
        IDE_TEST( sNewLogFile.initialize() != IDE_SUCCESS ); 
        sState = 1;

        IDE_TEST( sNewLogFile.create( sLogFileName,
                                      aLogFileInitBuffer,
                                      aLogFileSize )
                  != IDE_SUCCESS );
        sLogFileState = ID_TRUE;

        sState = 0;
        IDE_TEST( sNewLogFile.destroy() != IDE_SUCCESS ); 
    }
    else
    {
        /* nothing to do */
    }

    /* 다중화 로그파일을 open한다 */
    IDE_TEST( openLogFile( aLogFileNo,
                           ID_TRUE, /*addToList*/
                           &sMultiplexLogFile,
                           &sLogFileExist )
              != IDE_SUCCESS );

    IDE_ASSERT( (sLogFileExist == ID_TRUE) && (sMultiplexLogFile != NULL) );

    IDE_ASSERT( sMultiplexLogFile->mFileNo == aLogFileNo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( sNewLogFile.destroy() == IDE_SUCCESS ); 
        case 0:
        default:
            break;
    }

    if ( sLogFileState == ID_TRUE )
    {
        IDE_ASSERT( smrLogFile::remove( sLogFileName,
                                        ID_FALSE ) /*aIsCheckpoint*/
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * log file을 close하고 log file open list에서 삭제한다.
 *
 * aLogFile        - [IN] close할 log file
 * aRemoceFromList - [IN] close할 log file을 list에서 제거 할지 결정
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::closeLogFile( smrLogFile * aLogFile, 
                                            idBool aRemoveFromList )
{
    UInt sState = 3;

    IDE_ASSERT( aLogFile != NULL );
    
    if ( aRemoveFromList == ID_TRUE )
    {
        IDE_TEST( removeFromLogFileOpenList( aLogFile ) != IDE_SUCCESS );
    }

    IDE_ASSERT( aLogFile->mNxtLogFile == NULL );
    IDE_ASSERT( aLogFile->mPrvLogFile == NULL );

    aLogFile->mRef--;

    IDE_ASSERT( aLogFile->mRef  == 0 );

    sState = 2;
    IDE_TEST( aLogFile->close() != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( aLogFile->destroy() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( aLogFile ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            IDE_ASSERT( aLogFile->close() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( aLogFile->destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( aLogFile ) == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 서버구동시(smrLogFileMgr::startAndWait) 마지막에 사용한 log file을 open한다.
 * smrLogMgr::openLstLogFile 함수 참고
 *
 *
 * aLogFileNo       - [IN]  마지막에 사용한 log file 번호
 * aOffset          - [IN] 다음 log를 기록할 offset
 * aOriginalLogFile - [IN] 원본 로그파일
 * aLogFile         - [OUT] open한 last log file
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::openLstLogFile( UInt          aLogFileNo,
                                              UInt          aOffset,
                                              smrLogFile  * aOriginalLogFile,
                                              smrLogFile ** aLogFile )
{
    smrLogFile * sLogFile;
    idBool       sLogFileExist;
    SChar        sLogFileName[SM_MAX_FILE_NAME] = {'\0',};
    UInt         sState = 0;

    IDE_ASSERT( aLogFile         != NULL );
    IDE_ASSERT( aOriginalLogFile != NULL );
    IDE_ASSERT( mThreadType      == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );

    IDE_TEST( openLogFile( aLogFileNo, 
                           ID_TRUE, /*addToList*/
                           &sLogFile, 
                           &sLogFileExist ) 
              != IDE_SUCCESS );

    if ( sLogFileExist == ID_TRUE )
    {
        sState = 1;
        IDE_ASSERT( sLogFile != NULL );

        IDE_TEST( recoverLogFile( sLogFile ) != IDE_SUCCESS );

        sLogFile->setPos( aLogFileNo, aOffset );

        sLogFile->clear( aOffset );
    
        if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY )
        {
            /* log buffer type이 memory일경우 마지막 다중화 로그파일의 내용이 
             * 원본 로그파일의 내용과 다를수 있다.(smrLFThread동작여부에 따라)
             * 따라서 원본로그파일의 내용을 다중화 로그파일에 copy한다. */

            idlOS::memcpy( sLogFile->mBase, 
                           aOriginalLogFile->mBase, 
                           smuProperty::getLogFileSize() ); 
        }

        IDE_TEST( sLogFile->checkFileBeginLog( aLogFileNo ) 
                  != IDE_SUCCESS );

        (void)sLogFile->touchMMapArea();

    }
    else
    {
        /* 
         * 서버 시작시 마지막 다중화 로그파일이 존재하지 않는 경우이다. 
         * 다중화 디렉토리를 변경하거나 추가했을때의 상황 임으로
         * 원본 로그파일의 마지막 로그파일을 복사하여 다중화 한다. 
         */
        idlOS::snprintf(sLogFileName,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        mMultiplexPath,
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME,
                        aLogFileNo);
        
        IDE_TEST( aOriginalLogFile->mFile.copy( NULL, sLogFileName ) 
                  != IDE_SUCCESS );

        IDE_TEST( openLogFile( aLogFileNo, 
                               ID_TRUE, /*addToList*/
                               &sLogFile, 
                               &sLogFileExist ) 
                  != IDE_SUCCESS );

        IDE_ASSERT( sLogFileExist == ID_TRUE );
    }

    *aLogFile = sLogFile;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( closeLogFile( sLogFile, 
                                  ID_TRUE ) /*aRemoveFromList*/
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * logFileOpenList를 initialize한다.
 * 
 * aLogFileOpenList - [IN] initialize할 log file open list
 * aLostIdx         - [IN] 다중화 Idx번호 
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::initLogFileOpenList( 
                            smrLogFileOpenList * aLogFileOpenList,
                            UInt                 aListIdx )
{
    smrLogFile * sLogFileOpenListHdr;
    SChar        sMutexName[128] = {'\0',};
    UInt         sState = 0;

    IDE_ASSERT( aLogFileOpenList != NULL );
    IDE_ASSERT( aListIdx < mMultiplexCnt );

    sLogFileOpenListHdr = &aLogFileOpenList->mLogFileOpenListHdr;
    
    /* list 초기화 */
    sLogFileOpenListHdr->mPrvLogFile = sLogFileOpenListHdr;
    sLogFileOpenListHdr->mNxtLogFile = sLogFileOpenListHdr;
    
    idlOS::snprintf( sMutexName, 
                     128, 
                     "LOG_FILE_OPEN_LIST_IDX%"ID_UINT32_FMT, 
                     aListIdx );

    IDE_TEST( aLogFileOpenList->mLogFileOpenListMutex.initialize(
                                                        sMutexName,
                                                        IDU_MUTEX_KIND_NATIVE,
                                                        IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    aLogFileOpenList->mLogFileOpenListLen = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( aLogFileOpenList->mLogFileOpenListMutex.destroy()
                        == IDE_SUCCESS );
        case 0:
        default:
            break;
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * logFileOpenList를 destroy한다.
 * 
 * aLogFileOpenList - [IN] destroy할 log file open list
 * aSyncThread      - [IN] smrLogMultiplexThread 객체 
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::destroyLogFileOpenList( 
                            smrLogFileOpenList    * aLogFileOpenList,
                            smrLogMultiplexThread * aSyncThread )
{
    smrLogFile         * sCurLogFile;
    smrLogFile         * sNxtLogFile;
    smrLogFileOpenList * sLogFileOpenList;
    UInt                 sLogFileOpenListLen;
    UInt                 sLogFileOpenListCnt;
    UInt                 sState = 1;

    IDE_ASSERT( aLogFileOpenList           != NULL );
    IDE_ASSERT( aSyncThread                != NULL );
    IDE_ASSERT( aSyncThread[0].mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_SYNC );

    sLogFileOpenList    = aSyncThread->getLogFileOpenList();
    sCurLogFile         = sLogFileOpenList->mLogFileOpenListHdr.mNxtLogFile;
    sLogFileOpenListLen = aLogFileOpenList->mLogFileOpenListLen;

    for ( sLogFileOpenListCnt = 0; 
         sLogFileOpenListCnt < sLogFileOpenListLen; 
         sLogFileOpenListCnt++ )
    {
        sNxtLogFile = sCurLogFile->mNxtLogFile;
        IDE_TEST( aSyncThread->closeLogFile( sCurLogFile, 
                                             ID_TRUE ) /*aRemoveFromList*/
                  != IDE_SUCCESS );
        sCurLogFile = sNxtLogFile;
    }

    IDE_ASSERT( sLogFileOpenList->mLogFileOpenListLen == 0 );
    IDE_ASSERT( (sLogFileOpenList->mLogFileOpenListHdr.mNxtLogFile == 
                &sLogFileOpenList->mLogFileOpenListHdr) && 
                (sLogFileOpenList->mLogFileOpenListHdr.mPrvLogFile ==
                &sLogFileOpenList->mLogFileOpenListHdr) );

    sState = 0;
    IDE_TEST( aLogFileOpenList->mLogFileOpenListMutex.destroy()
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( aLogFileOpenList->mLogFileOpenListMutex.destroy()
                        == IDE_SUCCESS );
        case 0:
        default:
            break;
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * log file을 log file open list 마지막에 추가한다.
 *
 * aNewLogFile - [IN] list에 추가할 log file
 * aWithLock   - [IN] list에 log file을 추가할때 lock 을 값을것인지 결정
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::add2LogFileOpenList( smrLogFile * aNewLogFile,
                                                   idBool       aWithLock )
{
    smrLogFile         * sPrvLogFile;
    smrLogFile         * sLogFileOpenListHdr;
    smrLogFileOpenList * sLogFileOpenList;
    UInt                 sState = 0;
#ifdef DEBUG 
    smrLogFile         * sNxtLogFile;
#endif

    IDE_ASSERT( aNewLogFile            != NULL );
    IDE_ASSERT( aNewLogFile->mIsOpened == ID_TRUE );
    IDE_ASSERT( mThreadType            == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );

    sLogFileOpenList    = getLogFileOpenList();
    sLogFileOpenListHdr = &sLogFileOpenList->mLogFileOpenListHdr;

    if ( aWithLock == ID_TRUE )
    {
        IDE_TEST( sLogFileOpenList->mLogFileOpenListMutex.lock( NULL ) 
                  != IDE_SUCCESS );
        sState = 1;
    }

#ifdef DEBUG 
    sNxtLogFile = sLogFileOpenListHdr->mNxtLogFile;
    while( sLogFileOpenListHdr != sNxtLogFile )
    {
        IDE_ASSERT( sNxtLogFile->mFileNo != aNewLogFile->mFileNo );
        sNxtLogFile = sNxtLogFile->mNxtLogFile;
    } 
#endif

    sPrvLogFile = sLogFileOpenListHdr->mPrvLogFile;

    sPrvLogFile->mNxtLogFile = aNewLogFile;    
    sLogFileOpenListHdr->mPrvLogFile = aNewLogFile;

    aNewLogFile->mNxtLogFile = sLogFileOpenListHdr;
    aNewLogFile->mPrvLogFile = sPrvLogFile;

    sLogFileOpenList->mLogFileOpenListLen++;

#ifdef DEBUG 
    checkLogFileOpenList( sLogFileOpenList );
#endif

    if ( aWithLock == ID_TRUE )
    {
        sState = 0;
        IDE_TEST( sLogFileOpenList->mLogFileOpenListMutex.unlock() 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sLogFileOpenList->mLogFileOpenListMutex.unlock() 
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
}

/*
 * prepare logfile의 수가 0일경우 list 동시성 제어 고려 필요
 */
/***********************************************************************
 * Description :
 * log file을 log file open list로부터 제거한다.
 *
 * aRemoveLogFile - [IN] list에서 삭제할 log file
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::removeFromLogFileOpenList( 
                                    smrLogFile * aRemoveLogFile )
{
    smrLogFile         * sPrvLogFile;
    smrLogFile         * sNxtLogFile;
    smrLogFileOpenList * sLogFileOpenList;
    UInt                 sState = 0;
    
    IDE_ASSERT( aRemoveLogFile != NULL );
    IDE_ASSERT( mThreadType    == SMR_LOG_MPLEX_THREAD_TYPE_SYNC );

    sLogFileOpenList = getLogFileOpenList();

    IDE_ASSERT( &sLogFileOpenList->mLogFileOpenListHdr != aRemoveLogFile );

    IDE_TEST( sLogFileOpenList->mLogFileOpenListMutex.lock( NULL ) 
              != IDE_SUCCESS );
    sState = 1;

    sPrvLogFile = aRemoveLogFile->mPrvLogFile;
    sNxtLogFile = aRemoveLogFile->mNxtLogFile;

    sPrvLogFile->mNxtLogFile = aRemoveLogFile->mNxtLogFile;    
    sNxtLogFile->mPrvLogFile = aRemoveLogFile->mPrvLogFile;

    sLogFileOpenList->mLogFileOpenListLen--;

#ifdef DEBUG 
    checkLogFileOpenList( sLogFileOpenList );
#endif

    sState = 0;
    IDE_TEST( sLogFileOpenList->mLogFileOpenListMutex.unlock() 
              != IDE_SUCCESS );

    aRemoveLogFile->mNxtLogFile = NULL;
    aRemoveLogFile->mPrvLogFile = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sLogFileOpenList->mLogFileOpenListMutex.unlock() 
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 다중화 log file을 복구한다.
 * 다중화 log file 복구는 마지막 log file 이전의 log파일이 invalid할 경우에만
 * 수행하며 valid한 이전 log file이 나오면 중지한다.
 *
 * aLstLogFileNo - [IN] 마지막 log가 기록된 다중화 log file 번호
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::recoverMultiplexLogFile( UInt aLstLogFileNo )
{
    idBool       sIsCompleteLogFile;
    idBool       sLogFileExist;
    smrLogFile * sLogFile;
    UInt         sLogFileNo;
    UInt         sState = 0;

    IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );

    sLogFileNo = aLstLogFileNo;

    while( sLogFileNo != 0 )
    {
        sIsCompleteLogFile = ID_FALSE;

        /* 마지막 log file의 복구는 openLstLogFile함수에서 수행한다. */
        sLogFileNo--;

        IDE_TEST( openLogFile( sLogFileNo, 
                               ID_FALSE, /*addToList*/
                               &sLogFile, 
                               &sLogFileExist ) 
                  != IDE_SUCCESS )

        /* 
         * log 다중화 시점에따라 마지막 log file이전의 
         * log file이 존재하지 않을수 있다.
         */
        if ( sLogFileExist != ID_TRUE )
        {
            /* 파일이 존재하지 않으면 open되지않기 때문에 close하지 않는다 */
            IDE_CONT( skip_recover );
        }   
        sState = 1;

        IDE_TEST( isCompleteLogFile( sLogFile, &sIsCompleteLogFile ) 
                  != IDE_SUCCESS );

        if ( sIsCompleteLogFile == ID_FALSE )
        {
            /* log file이 제대로 sync되지 못했다. log file 복구 수행 */
            IDE_TEST( recoverLogFile( sLogFile ) != IDE_SUCCESS );
        }
        else
        {
            /* 
             * log file이 valid하다. valid한 log file의 이전 log file들은 
             * logfile thread에의해 정상정으로 sync되어있는 상태이므로 
             * 복구 수행을 더이상 할 필요가 없다.
             */
        }
        
        sState = 0;
        IDE_TEST( closeLogFile( sLogFile, 
                                ID_FALSE ) /*aRemoveFromList*/
                  != IDE_SUCCESS );

        if ( (sLogFileNo == 0) || (sIsCompleteLogFile == ID_TRUE) )
        {
            break;
        }
    }

    IDE_EXCEPTION_CONT( skip_recover );
     
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( closeLogFile( sLogFile, 
                                      ID_FALSE ) /*aRemoveFromList*/
                        == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * 복구가 필요한 다중화 log file인지 검사한다. log file에
 * SMR_LT_FILE_END log가 기록 되어있지 않으면 false를 반환한다.
 *
 * aLogFile             - [IN]  valid한지 검사할 log file
 * aIsCompleteLogFile   - [OUT] log file이 완전히 sync되어있는지 결과 전달
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::isCompleteLogFile( 
                                    smrLogFile * aLogFile,
                                    idBool     * aIsCompleteLogFile )
{
    smrLogHead   sLogHead;
    SChar      * sLogPtr;
    smLSN        sLSN;
    idBool       sFindFileEndLog;
    UInt         sLogOffset;
    UInt         sLogSize;
    UInt         sLogHeadSize;

    IDE_ASSERT( mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_CREATE );
    
    sLogOffset      = 0;
    sFindFileEndLog = ID_FALSE;
    sLogHeadSize    = ID_SIZEOF( smrLogHead );

    while(1)
    {
        aLogFile->read( sLogOffset, &sLogPtr );

        if ( smrLogComp::isCompressedLog( sLogPtr ) == ID_TRUE )
        {
            sLogSize    = smrLogComp::getCompressedLogSize( sLogPtr );
            sLogOffset += sLogSize;
        }
        else
        {
            //BUG-34791 SIGBUS occurs when recovering multiplex logfiles 
            aLogFile->read( sLogOffset, (SChar*)&sLogHead, sLogHeadSize );
            sLogSize = smrLogHeadI::getSize( &sLogHead );

            /* BUG-35392 */
            if ( smrLogHeadI::isDummyLog( &sLogHead ) == ID_FALSE )
            {
                SM_SET_LSN( sLSN, aLogFile->mFileNo, sLogOffset );

                if ( aLogFile->isValidLog( &sLSN,
                                           &sLogHead,
                                           sLogPtr,
                                           sLogSize ) == ID_TRUE )
                {
                    /* SMR_LT_FILE_END가 정상적으로 기록되어있다면 
                     * 완전히 sync된log file이다. */
                    if ( smrLogHeadI::getType( &sLogHead ) == SMR_LT_FILE_END )
                    {
                        sFindFileEndLog = ID_TRUE;
                        break;
                    }

                }
                else
                {
                    /* nothing to do */
                }
            }

            sLogOffset += sLogSize;
        }

        if ( (sLogOffset > smuProperty::getLogFileSize()) || (sLogSize == 0) )
        {
            break;
        }
    }

    *aIsCompleteLogFile = sFindFileEndLog;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 * 서버 재구동시 sync되지 못한 다중화 log file을 복구한다.
 * 원본 log file의 전체 내용을 다중화 log file로 copy한다.
 *
 * aLogFile - [IN]  복구할 다중화 log file
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::recoverLogFile( smrLogFile * aLogFile )
{
    SChar       sOriginalLogFileName[SM_MAX_FILE_NAME] = {'\0',};
    smrLogFile  sOriginalLogFile; 
    UInt        sState = 0;

    idlOS::snprintf( sOriginalLogFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s%"ID_UINT32_FMT,
                     mOriginalLogPath,
                     IDL_FILE_SEPARATOR,
                     SMR_LOG_FILE_NAME,
                     aLogFile->mFileNo );

    IDE_TEST( sOriginalLogFile.initialize() != IDE_SUCCESS ); 
    sState = 1;

    IDE_TEST( sOriginalLogFile.open( aLogFile->mFileNo,
                                     sOriginalLogFileName,
                                     ID_TRUE, /* aIsMultiplexLogFile */
                                     smuProperty::getLogFileSize(),
                                     ID_FALSE ) 
              != IDE_SUCCESS ); 
    sState = 2;
    
    /* 다중화 log buffer에 원본 log buffer의 내용을 copy한다. */
    idlOS::memcpy( aLogFile->mBase, 
                   sOriginalLogFile.mBase, 
                   smuProperty::getLogFileSize() );

    /* 다중화 log buffer의 내용을 logfile로 기록한다. */
    IDE_TEST( aLogFile->syncToDisk( 0, smuProperty::getLogFileSize() ) 
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sOriginalLogFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
        case 1:
            IDE_ASSERT( sOriginalLogFile.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * resetlog수행시 reset된 로그버퍼의 내용을 파일로 기록한다.
 *
 * aSyncThread      - [IN] 
 * aOffset          - [IN] sync할 시작 offset
 * aLogFileSize     - [IN] 로그파일 크기
 * aOriginalLogFile - [IN] reset된 원본 로그파일
 ***********************************************************************/
IDE_RC smrLogMultiplexThread::syncToDisk( 
                                    smrLogMultiplexThread * aSyncThread,
                                    UInt                    aOffset,
                                    UInt                    aLogFileSize,
                                    smrLogFile            * aOriginalLogFile )
{
    smrLogFile * sLogFile;
    UInt         sMultiplexIdx;
    UInt         sState = 0;
    idBool       sLogFileExist;

    IDE_TEST_CONT( mMultiplexCnt == 0, skip_sync );

    IDE_ASSERT( aSyncThread      != NULL );
    IDE_ASSERT( aOriginalLogFile != NULL );
    IDE_ASSERT( aSyncThread[0].mThreadType == SMR_LOG_MPLEX_THREAD_TYPE_SYNC );

    for ( sMultiplexIdx = 0; sMultiplexIdx < mMultiplexCnt; sMultiplexIdx++ )
    {
        IDE_TEST( aSyncThread[sMultiplexIdx].openLogFile( 
                                                     aOriginalLogFile->mFileNo, 
                                                     ID_FALSE,  /*addToList*/
                                                     &sLogFile,
                                                     &sLogFileExist ) 
                  != IDE_SUCCESS );
        if ( sLogFileExist != ID_TRUE )
        {
            continue;
        }
        sState = 1;

        sLogFile->clear( aOffset ); 
 
        IDE_TEST( sLogFile->syncToDisk( aOffset, aLogFileSize ) != IDE_SUCCESS );
 
        sState = 0;
        IDE_TEST( aSyncThread[sMultiplexIdx].closeLogFile( 
                                                sLogFile, 
                                                ID_FALSE ) /*aRemoveFromList*/
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( skip_sync ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( aSyncThread[sMultiplexIdx].closeLogFile( 
                                                        sLogFile, 
                                                        ID_FALSE ) /*aRemoveFromList*/
                        == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * log file open list에 aLogFileNo를 가진 로그파일이 이미 존재하는지 검사한다.
 *
 * aLogFileNo       - [IN] 검색할 logfile번호 
 * aLogFileOpenList - [IN] 검색할 list
 ***********************************************************************/
smrLogFile * smrLogMultiplexThread::findLogFileInList( 
                                UInt                 aLogFileNo,
                                smrLogFileOpenList * aLogFileOpenList)
{
    smrLogFile * sCurLogFile;
    smrLogFile * sLogFileOpenListHdr;
    smrLogFile * sFindLogFile = NULL;

    sLogFileOpenListHdr =  &aLogFileOpenList->mLogFileOpenListHdr;

    sCurLogFile = sLogFileOpenListHdr->mNxtLogFile;

    while( sLogFileOpenListHdr != sCurLogFile )
    {
        if ( sCurLogFile->mFileNo == aLogFileNo )
        {
            sFindLogFile = sCurLogFile;
            break;
        }
        sCurLogFile = sCurLogFile->mNxtLogFile;
    }

    return sFindLogFile;
}

#ifdef DEBUG
/***********************************************************************
 * Description :
 * DEBUG에서만 동작
 * LogFileOpenList에 저장된 list 길리와 실제 list 길이를 검사한다.
 *
 * aLogFileOpenList - [IN] 검사할 LogFileOpenList
 ***********************************************************************/
void smrLogMultiplexThread::checkLogFileOpenList( 
                                smrLogFileOpenList * aLogFileOpenList)
{
    smrLogFile * sCurLogFile;
    smrLogFile * sLogFileOpenListHdr;
    UInt         sLogFileOpenListLen;

    sLogFileOpenListHdr =  &aLogFileOpenList->mLogFileOpenListHdr;

    sCurLogFile         =  sLogFileOpenListHdr->mNxtLogFile;
    sLogFileOpenListLen = 0;

    while( sLogFileOpenListHdr != sCurLogFile )
    {
        sCurLogFile = sCurLogFile->mNxtLogFile;

        sLogFileOpenListLen++;
    }

    IDE_DASSERT( sLogFileOpenListLen == aLogFileOpenList->mLogFileOpenListLen );

    sCurLogFile         =  sLogFileOpenListHdr->mPrvLogFile;
    sLogFileOpenListLen = 0;
    while( sLogFileOpenListHdr != sCurLogFile )
    {
        sCurLogFile = sCurLogFile->mPrvLogFile;

        sLogFileOpenListLen++;
    }

    IDE_DASSERT( sLogFileOpenListLen == aLogFileOpenList->mLogFileOpenListLen );
}
#endif

/***********************************************************************
 * Description : BUG-38801
 *      mOriginalLogFile의 Switch가 완료될때까지 대기
 *
 * aIsNeedRestart [IN/OUT] - mOriginalLogFile 의 Switch 완료를 기다려야할지
 *                              + LogMultiplex를 재시도 해야 할지 여부
 *          i ) 처음 aIsNeedRestart가 ID_FALSE로 넘어온다.
 *              이 경우 LstLSN 이 mOriginalLogFile 의 다음 로그로
 *              설정될때까지 대기 하고, aIsNeedRestart를 ID_TRUE로
 *              설정하여 LogMultiplex를 재시도 하도록 한다.
 *          ii) aIsNeedRestart가 ID_TRUE로 넘어온다면,
 *              이미 이 함수를 한번 통과 하여 재시작 한 경우이므로,
 *              다시 재시작 하지 않도록 aIsNeedRestart를 ID_FALSE로 변경
 ***********************************************************************/
void smrLogMultiplexThread::checkOriginalLogSwitch( idBool    * aIsNeedRestart )
{
    smLSN           sNewLstLSN;     /* BUG-38801 */
    PDL_Time_Value  sSleepTimeVal;  /* BUG-38801 */

    sSleepTimeVal.set( 0, 100 );
    SM_LSN_MAX( sNewLstLSN );

    if ( *aIsNeedRestart == ID_FALSE )
    {
        while( 1 )
        {
            /* logfile에 저장된 Log의 마지막 Offset을 받아온다. 
             * FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 더미를 포함하지 않는 로그 레코드의 Offset */
            smrLogMgr::getLstLogOffset( &sNewLstLSN );

            if ( (sNewLstLSN.mFileNo > mOriginalLogFile->mFileNo) &&
                 (sNewLstLSN.mOffset != 0) )
            {
                /* 다음 파일로 넘어가면 break */
                *aIsNeedRestart = ID_TRUE;
                break;
            }
            else
            {
                idlOS::sleep( sSleepTimeVal );
            }
        } // end of while
    }
    else
    {
        /* aIsNeedRestart가 TRUE이면, 재 호출 이므로 넘어간다 */
        *aIsNeedRestart = ID_FALSE;
    }
}

