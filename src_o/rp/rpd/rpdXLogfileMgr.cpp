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

/*******************************************************************************
 * $Id: rpdXLogfileMgr.cpp
 ******************************************************************************/

#include <smi.h>
#include <mtc.h>
#include <rpdXLogfileMgr.h>
#include <rpuProperty.h>

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::initialize                 *
 * ------------------------------------------------------------------*
 * 각종 인자를 초기화 시킨 후 replication meta에서 read 정보를 가져온다.
 * 이후 write 정보를 read 정보에 맞춘다.
 *
 * aReplName        - [IN] 해당 replication의 이름
 * aXLogLSN         - [IN] replication meta에 저장된 마지막으로 읽은 xlog의 위치
 * aExceedTimeout   - [IN] read 대기 시간 기준(해당 시간보다 길게 대기할 경우 timeout 처리)
 * aExitFlag        - [IN] read 무한 대기를 막기 위한 종료 flag의 포인터
 * aReceiverPtr     - [IN] 해당 xlogfile manager를 사용하는 receiver의 포인터
 *********************************************************************/
IDE_RC rpdXLogfileMgr::initialize( SChar *      aReplName,
                                   rpXLogLSN    aInitXLogLSN,
                                   UInt         aExceedTimeout,
                                   idBool *     aExitFlag,
                                   void *       aReceiverPtr,
                                   rpXLogLSN    aLastXLogLSN)
{
    UInt    sReadXLogfileNo = 0;
    SChar   sBuffer[IDU_MUTEX_NAME_LEN];
    UInt    sState = 0;
    SInt    sLengthCheck = 0;
    UInt    sLastFileNo;
    /* 클래스 내부 변수 초기화 */

    /* write 관련 변수 */
    mWriteXLogLSN  = RP_XLOGLSN_INIT;
    mWriteFileNo   = 0;
    mWriteBase     = NULL;
    mCurWriteXLogfile = NULL;

    /* read 관련 변수 */
    mReadXLogLSN   = RP_XLOGLSN_INIT;
    mReadFileNo    = 0;
    mReadBase      = NULL;
    mCurReadXLogfile = NULL;

    /* file list 관련 변수 */
    mXLogfileListHeader = NULL;
    mXLogfileListTail = NULL;

    /* thread 제어 관련 변수 */
    mExitFlag = aExitFlag;
    mEndFlag = ID_FALSE;
    mIsReadSleep = ID_FALSE;
    mIsWriteSleep = ID_FALSE;
    mWaitWrittenXLog = ID_TRUE;

    /* 기타 변수 */
    mFileMaxSize = RPU_REPLICATION_XLOGFILE_SIZE;
    mExceedTimeout = aExceedTimeout;
    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&mXLogDirPath ) != IDE_SUCCESS );
    idlOS::strncpy( mRepName, aReplName, QCI_MAX_NAME_LEN + 1 );

    /* write info의 동시성 제어를 위한 mutex */
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_WRITEINFO_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mWriteInfoMutex.initialize( sBuffer,
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );
    sState = 1;

    /* read info의 동시성 제어를 위한 mutex */
    idlOS::memset( sBuffer, 0, IDU_MUTEX_NAME_LEN );
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_READINFO_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mReadInfoMutex.initialize( sBuffer,
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );
    sState = 2;

    /* xlogfile list의 동시성 제어를 위한 mutex */
    idlOS::memset( sBuffer, 0, IDU_MUTEX_NAME_LEN );
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_FILELIST_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mXLogfileListMutex.initialize( sBuffer,
                                             IDU_MUTEX_KIND_POSIX,
                                             IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );
    sState = 3;

    /* read의 conditional variable 및 관련 mutex 초기화 */
    IDE_TEST( mReadWaitCV.initialize() != IDE_SUCCESS );

    idlOS::memset( sBuffer, 0, IDU_MUTEX_NAME_LEN );
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_READ_CV_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mReadWaitMutex.initialize( sBuffer,
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );
    sState = 4;

    /* write의 conditional variable 및 관련 mutex 초기화 */
    IDE_TEST( mWriteWaitCV.initialize() != IDE_SUCCESS );

    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "XLOGFILE_WRITE_CV_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );
    IDE_TEST( mWriteWaitMutex.initialize( sBuffer,
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );
    sState = 5;

    if( RP_IS_INIT_XLOGLSN( aInitXLogLSN ) == ID_FALSE )
    {
        /* 기존에 사용하던 xlogfile이 있다면 해당 파일부터 사용해야 한다.           *
         * 이 경우 read/write정보를 먼저 세팅한 후 creater를 생성해야               *
         * creater가 새 파일을 이전에 사용하던 파일 다음부터 정상적으로 생성한다.   */

        /* 기존 meta data가 있다면 해당 파일부터 시작한다. */
        RP_GET_FILENO_FROM_XLOGLSN( sReadXLogfileNo, aInitXLogLSN );

        /* 기존 사용하던 파일이 있을 경우 해당 파일을 open 한다. */
        IDE_TEST( openXLogfileForRead( sReadXLogfileNo ) != IDE_SUCCESS );

        setReadInfoWithLock( aInitXLogLSN, sReadXLogfileNo );

        /* read 정보를 세팅 한 후에 write 정보도 세팅한다. */
        IDE_TEST( findLastWrittenFileAndSetWriteInfo() != IDE_SUCCESS );
        if ( aReceiverPtr != NULL )
        {
            /* file create thread 생성 및 초기화 */
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                               ID_SIZEOF( rpdXLogfileCreater ),
                                               (void**)&(mXLFCreater),
                                               IDU_MEM_IMMEDIATE ) != IDE_SUCCESS, err_AllocFail );
            sState = 6;

            new (mXLFCreater) rpdXLogfileCreater;

            IDE_TEST( mXLFCreater->initialize( aReplName,
                                               aInitXLogLSN,
                                               this,
                                               aReceiverPtr ) != IDE_SUCCESS );

            /* creater의 마지막으로 생성된 파일 번호를 변경한다. */
            mXLFCreater->mLastCreatedFileNo = mWriteFileNo;

            IDE_TEST( mXLFCreater->start() != IDE_SUCCESS );
            sState = 7;

            ideLog::log( IDE_RP_0, "[Receiver] Replication %s start XLogfile Creater thread", mRepName );
        }
    }
    else
    {
        /* 기존에 사용하던 xlogfile이 없다면 creater가 새 파일을 먼저 생성해야 하므로           *
         * creater를 먼저 생성하고 creater가 만든 새 파일을 이용해 read/write 정보를 세팅한다.  */

        /* file create thread 생성 및 초기화 */
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                           ID_SIZEOF( rpdXLogfileCreater ),
                                           (void**)&(mXLFCreater),
                                           IDU_MEM_IMMEDIATE ) != IDE_SUCCESS, err_AllocFail );
        sState = 6;

        new (mXLFCreater) rpdXLogfileCreater;

        IDE_TEST( mXLFCreater->initialize( aReplName,
                                           aInitXLogLSN,
                                           this,
                                           aReceiverPtr ) != IDE_SUCCESS );

        IDE_TEST( mXLFCreater->start() != IDE_SUCCESS );   
        sState = 7;

        ideLog::log( IDE_RP_0, "[Receiver] Replication %s start XLogfile Creater thread", mRepName );

        /* 기존 파일이 없다면 creater가 생성해준 파일을 사용해야 한다. */
        IDE_TEST( openFirstXLogfileForRead() != IDE_SUCCESS );

        /* write info를 read info에 맞춰주도록 한다. */
        setWriteInfoWithLock( mReadXLogLSN, mReadFileNo );

        mWriteBase        = mReadBase;
        mCurWriteXLogfile = mCurReadXLogfile;

        incFileRefCnt( mCurWriteXLogfile );
    }

    if ( RP_IS_INIT_XLOGLSN( aLastXLogLSN ) == ID_FALSE )
    {
        RP_GET_FILENO_FROM_XLOGLSN( sLastFileNo, aLastXLogLSN );
        ACP_UNUSED(sLastFileNo);
        //setWriteInfoWithLock( aLastXLogLSN, sLastFileNo );
    }

    if ( aReceiverPtr != NULL )
    {
        /* flusher thread 생성 및 초기화 */
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                           ID_SIZEOF( rpdXLogfileFlusher ),
                                           (void**)&(mXLFFlusher),
                                           IDU_MEM_IMMEDIATE ) != IDE_SUCCESS, err_AllocFail );
        sState = 8;

        new (mXLFFlusher) rpdXLogfileFlusher;

        IDE_TEST( mXLFFlusher->initialize( aReplName,
                                           this ) != IDE_SUCCESS );;
        IDE_TEST( mXLFFlusher->start() != IDE_SUCCESS );
        sState = 9;

        ideLog::log( IDE_RP_0, "[Receiver] Replication %s start XLogfile Flusher thread", mRepName );
    }
    IDE_DASSERT( mReadBase != NULL );
    IDE_DASSERT( mWriteBase != NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_AllocFail )
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdXLogfileMgr::initialize",
                                "thread & mutex"));
    }
    IDE_EXCEPTION( too_long_mutex_name )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_MUTEX_NAME, sBuffer ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 9:
            mXLFFlusher->mExitFlag = ID_TRUE;
            (void)mXLFFlusher->join();
            (void)mXLFFlusher->finalize();
        case 8:
            (void)iduMemMgr::free( mXLFFlusher );
        case 7:
            mXLFCreater->mExitFlag = ID_TRUE;
            (void)mXLFCreater->join();
            mXLFCreater->finalize();
        case 6:
            (void)iduMemMgr::free( mXLFCreater );
        case 5:
            (void)mWriteWaitMutex.destroy();
        case 4:
            (void)mReadWaitMutex.destroy();
        case 3:
            (void)mXLogfileListMutex.destroy();
        case 2:
            (void)mReadInfoMutex.destroy();
        case 1:
            (void)mWriteInfoMutex.destroy();
        default:
            break;
    }

    while( mXLogfileListHeader != NULL )
    {
        if( idlOS::munmap( mXLogfileListHeader->mBase, RPU_REPLICATION_XLOGFILE_SIZE ) == IDE_SUCCESS )
        {
            (void)mXLogfileListHeader->mRefCntMutex.destroy();
            (void)mXLogfileListHeader->mFile.close();
            (void)mXLogfileListHeader->mFile.destroy();

            mXLogfileListHeader = mXLogfileListHeader->mNextXLogfile;
            (void)iduMemMgr::free( mXLogfileListHeader->mPrevXLogfile );
        }
        else
        {
            IDE_ERRLOG(IDE_RP_0);
            (void)mXLogfileListHeader->mRefCntMutex.destroy();

            mXLogfileListHeader = mXLogfileListHeader->mNextXLogfile;
        }

        mXLogfileListHeader->mPrevXLogfile = NULL;
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::finalize                   *
 * ------------------------------------------------------------------*
 * rpdXLogfileMgr 클래스를 정리한다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::finalize()
{
    /* read와 write를 막기 위해 endflag를 세팅한다. */
    mEndFlag = ID_TRUE;

    /* create thread와 flush thread를 종료한다. */
    IDE_TEST( stopSubThread() != IDE_SUCCESS );

    /* create thread와 flush thread를 정리한다. */
    destroySubThread();

    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    /* write/flush된 파일은 flush thread가 닫고 open만 하고 사용하지 않은 파일은 create thread에서 닫았으니
     * 여기서는 read중인 파일만 닫으면 된다.*/

    /* read 중인 파일이 있을 경우 해당 파일을 닫는다. */
    if( mXLogfileListHeader != NULL )
    {
        IDE_DASSERT( mXLogfileListHeader == mXLogfileListTail );
        IDE_DASSERT( mXLogfileListHeader == mCurReadXLogfile );
        IDE_DASSERT( getFileRefCnt( mXLogfileListHeader ) == 1 );

        if( idlOS::munmap( mXLogfileListHeader->mBase, RPU_REPLICATION_XLOGFILE_SIZE ) == IDE_SUCCESS )
        {
            if( mXLogfileListHeader->mRefCntMutex.destroy() != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_RP_0);
            }
            if( mXLogfileListHeader->mFile.close() != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_RP_0);
            }
            if( mXLogfileListHeader->mFile.destroy() != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_RP_0);
            }
            (void)iduMemMgr::free( mXLogfileListHeader );
        }
        else
        {
            IDE_ERRLOG(IDE_RP_0);

            if( mXLogfileListHeader->mRefCntMutex.destroy() != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_RP_0);
            }
        }
    }
    else
    {
        /* read 중인 파일이 없을 경우는 할 필요가 없다.*/
    }

    IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

    (void)iduMemMgr::free( mXLFCreater );
    (void)iduMemMgr::free( mXLFFlusher );

    if( mWriteWaitMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }
    if( mReadWaitMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }
    if( mXLogfileListMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }
    if( mReadInfoMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }
    if( mWriteInfoMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::removeAllXLogfiles         *
 * ------------------------------------------------------------------*
 * 해당 replication의 모든 XLogfile을 제거한다.
 * 주의 : 해당 함수는 static 함수로 xlogfileMgr이 내려간 상태에서 사용 가능하다.
 *        사용중인 파일을 제거하고 싶다면 먼저 해당 replication을 내린 후에 사용해야 한다.
 * aReplName - [IN] 제거할 xlogfile이 연결된 replication의 이름
 * aFileNo   - [IN] 제거할 xlogfile의 기준점(replication meta에 보관된 파일 번호)
 *********************************************************************/
IDE_RC rpdXLogfileMgr::removeAllXLogfiles( SChar *  aReplName,
                                           UInt     aFileNo )
{
    SChar   sFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    SChar * sXLogDirPath = NULL;
    idBool  sIsFileExist = ID_TRUE;
    UInt    sFileNo;
    SInt    sLengthCheck = 0; 
    SInt    rc = 0;

    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&sXLogDirPath ) != IDE_SUCCESS );

    /* 주어진 fileno보다 큰 파일을 제거한다. */
    sFileNo = aFileNo;

    while( sIsFileExist == ID_TRUE )
    {
        sLengthCheck = idlOS::snprintf( sFilePath,
                                        XLOGFILE_PATH_MAX_LENGTH,
                                        "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                        sXLogDirPath,
                                        IDL_FILE_SEPARATOR,
                                        aReplName,
                                        sFileNo );
        IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

        if( rpdXLogfileCreater::isFileExist( sFilePath ) == ID_TRUE )
        {
            rc = idf::unlink( sFilePath );
            IDE_TEST_RAISE( rc != 0, unlink_fail );

            sFileNo++;
        }
        else
        {
            sIsFileExist = ID_FALSE;
        }
    }

    sIsFileExist = ID_TRUE;

    /* 주어진 fileno보다 작은 파일을 제거한다. */
    if( aFileNo > 0 )
    {
        sFileNo = aFileNo - 1;

        while( sIsFileExist == ID_TRUE )
        {
            sLengthCheck = idlOS::snprintf( sFilePath,
                                            XLOGFILE_PATH_MAX_LENGTH,
                                            "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                            sXLogDirPath,
                                            IDL_FILE_SEPARATOR,
                                            aReplName,
                                            sFileNo );
            IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

            if( rpdXLogfileCreater::isFileExist( sFilePath ) == ID_TRUE )
            {
                rc = idf::unlink( sFilePath );
                IDE_TEST_RAISE( rc != 0, unlink_fail );

                if( sFileNo > 0 )
                {
                    sFileNo--;
                }
                else
                {
                    break;
                }
            }
            else
            {
                sIsFileExist = ID_FALSE;
            }
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_file_path )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_FILE_PATH, sFilePath ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION( unlink_fail )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_FAILURE_UNLINK_FILE, sFilePath, rc ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::checkRemainXLog            *
 * ------------------------------------------------------------------*
 * 읽을 xlog가 있는지 확인한다.
 * 만약 읽을 xlog가 없을 경우 대기한다.
 * 해당 함수는 read(receiver)에서 사용한다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::checkRemainXLog()
{
    UInt    sLoopCnt = 0;
    idBool  sIsSuccess = ID_FALSE;
    rpXLogLSN sWriteXLogLSN;

    while( sIsSuccess != ID_TRUE )
    {
        IDU_FIT_POINT( "rpdXLogfileMgr::checkRemainXLog" );

        sWriteXLogLSN = getWriteXLogLSNWithLock();
        if( mReadXLogLSN < sWriteXLogLSN )
        {
            sIsSuccess = ID_TRUE;
        }
        else
        {
            IDE_TEST_RAISE( mWaitWrittenXLog == ID_FALSE, end_of_xlogfiles );
            /* 읽을 xlog가 없으므로 대기한다. */
            sleepReader();

            IDE_TEST_RAISE( ++sLoopCnt > mExceedTimeout, err_Timeout );
            IDE_TEST_RAISE( *mExitFlag == ID_TRUE, exitFlag_on );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_Timeout )
    {
        /* 대기 시간이 기준을 넘겨 예외 발생 */
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TIMEOUT_EXCEED ) );
    }
    IDE_EXCEPTION( exitFlag_on )
    {
        /* 종료 flag가 세팅되어 있음 */
        IDE_SET( ideSetErrorCode( rpERR_ABORT_END_THREAD, mRepName ) );
    }
    IDE_EXCEPTION( end_of_xlogfiles )
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_RPX_END_OF_XLOGFILES ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getRemainSize            *
 * ------------------------------------------------------------------*
 * 현재 read중인 파일의 끝까지 남은 데이터의 크기를 반환한다.
 * 해당 함수는 read(receiver)에서 호출된다.
 *
 * aRemainSize  - [OUT] 해당 파일의 남은 데이터 크기
 *********************************************************************/
IDE_RC rpdXLogfileMgr::getRemainSize( UInt *    aRemainSize )
{
    UInt                sReadOffset = 0;
    rpdXLogfileHeader * sFileHeader = (rpdXLogfileHeader*)mReadBase;
    UInt                sFileWriteOffset = sFileHeader->mFileWriteOffset;

    RP_GET_OFFSET_FROM_XLOGLSN( sReadOffset, mReadXLogLSN );

    /* read가 write보다 앞설수는 없다. */ 
    IDE_DASSERT( sReadOffset <= sFileWriteOffset );

    if( ( sFileWriteOffset == sReadOffset ) && ( mCurReadXLogfile->mIsWriteEnd == ID_TRUE ) )
    {
        /* 해당 파일을 다 읽고 다음 파일로 넘어가야 할 상황 */
        IDE_TEST( getNextFileRemainSize( aRemainSize ) != IDE_SUCCESS );
    }
    else
    {
        /* 해당 파일에 읽을게 남았거나 쓸 내용이 남아있는 상황 */
        *aRemainSize = sFileWriteOffset - sReadOffset;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getNextFileRemainSize    *
 * ------------------------------------------------------------------*
 * 현재 read 중인 파일의 다음 번호 파일의 남은 데이터의 크기를 반환한다.
 * 만약 해당 파일이 메모리에 없다면 헤더 부분만 읽어 크기를 체크한다.
 * aSize    - [OUT] 다음 파일의 남은 데이터 크기
 *********************************************************************/
IDE_RC rpdXLogfileMgr::getNextFileRemainSize( UInt *    aRemainSize )
{
    UInt                sResult = 0;
    UInt                sNextFileNo = mCurReadXLogfile->mFileNo + 1;
    iduFile             sNextFile;
    SChar               sNextFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    rpdXLogfileHeader * sFileHeaderPtr = NULL;
    rpdXLogfileHeader   sFileHeader;


    /* xlogfile list를 따라가야 하므로 list lock을 잡는다. */
    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    if( mCurReadXLogfile->mNextXLogfile == NULL )
    {
        /* read가 write를 따라잡았으나 아직 새 파일이 생성되어있지 않아 write가 대기중인 상태. *
         * 이 경우 0을 리턴하면 상위 모듈에서 busy wait로 재수행한다.                          */
        sResult = 0;
        IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        if( mCurReadXLogfile->mNextXLogfile->mFileNo == sNextFileNo )
        {
            /* 다음 파일이 메모리에 있는 상태라면 그 정보를 가져와 쓰면 된다. */
            sFileHeaderPtr = (rpdXLogfileHeader*)mCurReadXLogfile->mNextXLogfile->mBase;
            sResult = sFileHeaderPtr->mFileWriteOffset;

            IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );
        }
        else
        {
            IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

            idlOS::snprintf( sNextFilePath,
                             XLOGFILE_PATH_MAX_LENGTH,
                             "%s%cxlogfile%s_%"ID_UINT32_FMT,
                             mXLogDirPath,
                             IDL_FILE_SEPARATOR,
                             mRepName,
                             sNextFileNo );

            IDE_DASSERT( rpdXLogfileCreater::isFileExist( sNextFilePath ) == ID_TRUE );

            IDE_TEST( sNextFile.initialize( IDU_MEM_OTHER, //차후 xlogfile용 추가할 것
                                            1, /* Max Open FD Count*/
                                            IDU_FIO_STAT_OFF,
                                            IDV_WAIT_INDEX_NULL )
                      != IDE_SUCCESS );

            IDE_TEST( sNextFile.setFileName( sNextFilePath ) != IDE_SUCCESS );

            /* 파일을 open한다. */
            IDE_TEST( sNextFile.openUntilSuccess( ID_FALSE, O_RDWR ) != IDE_SUCCESS );

            /* header(file write offset) 만 mapping을 수행한다. */
            IDE_TEST( sNextFile.read( NULL,
                                      0,
                                      &sFileHeader,
                                      XLOGFILE_HEADER_SIZE_INIT ) != IDE_SUCCESS );

            IDE_DASSERT( sFileHeader.mFileWriteOffset > XLOGFILE_HEADER_SIZE_INIT );

            sResult = sFileHeader.mFileWriteOffset;

            /* 파일을 정리한다. */
            IDE_TEST( sNextFile.close() != IDE_SUCCESS );
            IDE_TEST( sNextFile.destroy() != IDE_SUCCESS );
        }
    }

    *aRemainSize = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::readXLog                   *
 * ------------------------------------------------------------------*
 * read XlogLSN에 해당하는 데이터를 읽어 버퍼에 복사한다.
 * 해당 함수는 read에서만 수행되기 때문에 readInfo에 대한 쓰기와
 * writeInfo에 대한 읽기에 대해서는 lock을 잡고 수행해야 한다.
 * 주의 : 일반 작업시에는 checkRemainXLog 함수에서, recovery시에는 isLastXlog 함수에서 
 *        읽을 xlog가 있는지 확인하고 들어오기 때문에 
 *        read offset과 write offset이 같은 경우 뭔가 문제가 있는 상황이다.
 *
 * aDest      - [OUT] 읽은 데이터를 복사할 버퍼
 * aLength    - [IN]  읽을 데이터의 길이
 * aIsConvert - [IN]  little endian일 경우 변환이 필요한지 여부
 *********************************************************************/
IDE_RC rpdXLogfileMgr::readXLog( void * aDest,
                                 UInt   aLength,
                                 idBool aIsConvert )
{
    rpdXLogfileHeader * sFileHeader = NULL;
    rpXLogLSN           sReadXLogLSN  = RP_XLOGLSN_INIT;
    UInt                sReadFileNo   = 0;
    UInt                sReadOffset   = 0;
    rpXLogLSN           sWriteXLogLSN = RP_XLOGLSN_INIT;
    UInt                sWriteFileNo  = 0;
    UInt                sFileWriteOffset;
    idBool              sIsWait = ID_FALSE;
    UChar               sTemp[500] = {0,};
    UShort              sTempSize = 0;
    ULong               sLoopCnt = 0;

    sFileHeader = (rpdXLogfileHeader*)mReadBase;
    sFileWriteOffset = sFileHeader->mFileWriteOffset;   

    IDE_DASSERT( mEndFlag == ID_FALSE );

    IDU_FIT_POINT( "rpdXLogfileMgr::readXLog" );

    sReadXLogLSN = mReadXLogLSN;
    RP_GET_XLOGLSN( sReadFileNo, sReadOffset, sReadXLogLSN );

    sWriteXLogLSN = getWriteXLogLSNWithLock();
    RP_GET_FILENO_FROM_XLOGLSN( sWriteFileNo, sWriteXLogLSN );

    IDE_DASSERT( sReadFileNo <= sWriteFileNo );

    while( ( sReadOffset == sFileWriteOffset ) && ( sReadFileNo == sWriteFileNo ) )
    {
        /* write 하는 도중 read가 write를 따라 잡을 경우                           * 
         * file write offset이 바뀌거나 write가 다음 파일로 넘어갈때까지 대기한다. */
        sFileHeader = (rpdXLogfileHeader*)mReadBase;
        sFileWriteOffset = sFileHeader->mFileWriteOffset;

        sWriteXLogLSN = getWriteXLogLSNWithLock();
        RP_GET_FILENO_FROM_XLOGLSN( sWriteFileNo, sWriteXLogLSN );

        IDE_TEST_RAISE( *mExitFlag == ID_TRUE, exitFlag_on );
        IDE_TEST_RAISE( mWaitWrittenXLog == ID_FALSE, err_Timeout );
        IDE_TEST_RAISE( sLoopCnt++ > 5000000, err_Timeout );
    }

    if( ( sReadOffset == sFileWriteOffset ) && (mCurReadXLogfile->mIsWriteEnd == ID_TRUE ) )
    {
        /* 해당 파일을 모두 읽었으므로 다음 파일에 접근한다. */

        IDE_DASSERT( sReadOffset != RP_XLOGLSN_INIT );
        IDE_TEST( openXLogfileForRead( ++sReadFileNo ) != IDE_SUCCESS );

        sReadXLogLSN = mReadXLogLSN;
        RP_GET_XLOGLSN( sReadFileNo, sReadOffset, sReadXLogLSN );

        sFileHeader = (rpdXLogfileHeader*)mReadBase;
        sFileWriteOffset = sFileHeader->mFileWriteOffset;
    }

    do
    {
        if( sReadFileNo == sWriteFileNo )
        {
            /* read가 write를 앞설수는 없다. */
            IDE_DASSERT( sReadOffset <= sFileWriteOffset );

            if( ( sFileWriteOffset - sReadOffset ) >= aLength )
            {
                /* 남은 데이터가 읽을 데이터의 길이보다 길 경우 정상적으로 read를 수행한다. */
                sIsWait = ID_FALSE;

                /* read(memcpy)를 수행한다. */
#ifdef ENDIAN_IS_BIG_ENDIAN
                 /* big endian일 경우 그대로 복사한다. */
                idlOS::memcpy( aDest, (SChar*)mReadBase + sReadOffset, aLength );
#else
                /* little endian일 경우 변환하여 복사해야 한다. */
                if( aIsConvert == ID_TRUE )
                {
                    switch( aLength )
                    {
                        case 2:
                            RP_ENDIAN_ASSIGN2( (SChar*)aDest, (SChar*)mReadBase + sReadOffset );
                            break;
                        case 4:
                            RP_ENDIAN_ASSIGN4( (SChar*)aDest, (SChar*)mReadBase + sReadOffset );
                            break;
                        case 8:
                            RP_ENDIAN_ASSIGN8( (SChar*)aDest, (SChar*)mReadBase + sReadOffset );
                            break;
                        default:
                            idlOS::memcpy( aDest, (SChar*)mReadBase + sReadOffset, aLength );
                            break;
                    }
                }
                else
                {
                    idlOS::memcpy( aDest, (SChar*)mReadBase + sReadOffset, aLength );
                }
#endif

#if defined(DEBUG)
                if( aLength <= 200 )
                {
                    byteToString( (UChar*)aDest, (UShort)aLength,
                                  sTemp, &sTempSize );
                    ideLog::log(IDE_RP_6,"[r] - %d|%d|%d : %s",sReadFileNo, sReadOffset, aLength, sTemp );
                }
                else
                {
                    ideLog::log(IDE_RP_6,"[r] - %d|%d|%d",sReadFileNo, sReadOffset, aLength );
                }
#else
     ACP_UNUSED(sTempSize);
     ACP_UNUSED(sTemp);
#endif

                /* read Offset을 갱신하고 readXLogLSN을 저장한다. */
                sReadOffset += aLength;
                sReadXLogLSN = RP_SET_XLOGLSN( sReadFileNo , sReadOffset );
                setReadInfoWithLock( sReadXLogLSN, sReadFileNo );
            }
            else
            {
                /* 남은 데이터보다 읽을 데이터의 길이가 더 길경우 아직          *
                 * 해당 xlog가 완전히 쓰여진게 아니므로 busy wait로 대기한다.   */
                sIsWait = ID_TRUE;

                IDE_TEST_RAISE( *mExitFlag == ID_TRUE, exitFlag_on );
                IDE_TEST_RAISE( mWaitWrittenXLog == ID_FALSE, err_Timeout );
                IDE_TEST_RAISE( sLoopCnt++ > 5000000, err_Timeout );

                /* write가 다음 파일로 넘어갔을 가능성이 있으므로 write 정보를 다시 받아온다. */
                sWriteXLogLSN = getWriteXLogLSNWithLock();
                RP_GET_FILENO_FROM_XLOGLSN( sWriteFileNo, sWriteXLogLSN );

                sFileHeader = (rpdXLogfileHeader*)mReadBase;
                sFileWriteOffset = sFileHeader->mFileWriteOffset;

                IDU_FIT_POINT( "rpdXLogfileMgr::readXLog::busyWait" );
            }
        }
        else
        {
            IDE_DASSERT( sReadOffset + aLength <= sFileWriteOffset );

            /* 파일이 다르면 신경쓸 필요가 없다. */
            sIsWait = ID_FALSE;

            /* read(memcpy)를 수행한다. */
#ifdef ENDIAN_IS_BIG_ENDIAN
            /* big endian일 경우 그대로 복사한다. */
            idlOS::memcpy( aDest, (SChar*)mReadBase + sReadOffset, aLength );
#else
            /* little endian일 경우 변환하여 복사해야 한다. */
            if( aIsConvert == ID_TRUE )
            {
                switch( aLength )
                {
                    case 2:
                        RP_ENDIAN_ASSIGN2( (SChar*)aDest, (SChar*)mReadBase + sReadOffset );
                        break;
                    case 4:
                        RP_ENDIAN_ASSIGN4( (SChar*)aDest, (SChar*)mReadBase + sReadOffset );
                        break;
                    case 8:
                        RP_ENDIAN_ASSIGN8( (SChar*)aDest, (SChar*)mReadBase + sReadOffset );
                        break;
                    default:
                        idlOS::memcpy( aDest, (SChar*)mReadBase + sReadOffset, aLength );
                        break;
                }
            }
            else
            {
                idlOS::memcpy( aDest, (SChar*)mReadBase + sReadOffset, aLength );
            }
#endif

#if defined(DEBUG)
            if( aLength <= 200 )
            {
                byteToString( (UChar*)aDest, (UShort)aLength,
                              sTemp, &sTempSize );
                ideLog::log(IDE_RP_6,"[r] - %d|%d|%d : %s",sReadFileNo, sReadOffset,aLength, sTemp );
            }
            else
            {
                ideLog::log(IDE_RP_6,"[r] - %d|%d|%d",sReadFileNo, sReadOffset,aLength );
            }
#endif

            /* read Offset을 갱신하고 readXLogLSN을 저장한다. */
            sReadOffset += aLength;
            sReadXLogLSN = RP_SET_XLOGLSN( sReadFileNo, sReadOffset );
            setReadInfoWithLock( sReadXLogLSN, sReadFileNo );
        }

    }while( sIsWait == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_Timeout )
    {
        /* 대기 시간이 기준을 넘겨 예외 발생 */
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TIMEOUT_EXCEED ) );
    }
    IDE_EXCEPTION( exitFlag_on )
    {
        /* 종료 flag가 세팅되어 있음 */
        IDE_SET( ideSetErrorCode( rpERR_ABORT_END_THREAD, mRepName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::readXLogDirect             *
 * ------------------------------------------------------------------*
 * read XlogLSN에 해당하는 데이터의 포인터를 반환한다.
 * 해당 함수는 read에서만 수행되기 때문에 readInfo에 대한 쓰기와
 * writeInfo에 대한 읽기에 대해서는 lock을 잡고 수행해야 한다.
 * diread direct로 직접 포인터를 넘길 경우 endian에 대한 처리가 되어있지 않으므로
 * 포인터를 받은 쪽에서 직접 해줘야 한다.
 * 주의 : 일반 작업시에는 checkRemainXLog 함수에서, recovery시에는 isLastXlog 함수에서 
 *        읽을 xlog가 있는지 확인하고 들어오기 때문에 
 *        read offset과 write offset이 같은 경우 뭔가 문제가 있는 상황이다.
 *
 * aMMapPtr - [OUT] 해당 하는 데이터의 포인터(반환용)
 * aLength  - [IN]  읽을 데이터의 길이(offset 증가용)
 *********************************************************************/
IDE_RC rpdXLogfileMgr::readXLogDirect( void **  aMMapPtr,
                                       UInt     aLength )
{
    rpXLogLSN sReadXLogLSN  = RP_XLOGLSN_INIT;
    UInt      sReadFileNo   = 0;
    UInt      sReadOffset   = 0;
    rpXLogLSN sWriteXLogLSN = RP_XLOGLSN_INIT;
    UInt      sWriteFileNo  = 0;
    UInt      sFileWriteOffset = *(UInt*)mReadBase;
    idBool    sIsWait = ID_FALSE;

    IDE_DASSERT( mEndFlag == ID_FALSE );

    sReadXLogLSN = mReadXLogLSN;
    RP_GET_XLOGLSN( sReadFileNo, sReadOffset, sReadXLogLSN );

    sWriteXLogLSN = getWriteXLogLSNWithLock();
    RP_GET_FILENO_FROM_XLOGLSN( sWriteFileNo, sWriteXLogLSN );

    IDE_DASSERT( sReadFileNo <= sWriteFileNo );

    if( sReadOffset == sFileWriteOffset )
    {
        /* read offset과 file write offset이 같다면                          *
         * read가 write를 따라 잡았거나 해당 파일을 모두 읽은 상태이나       *
         * checkRemainXLog에서 읽을 내용이 있는지 확인하고 읽으러 들어오므로 *
         * read가 write를 따라 잡을 수는 없다.                               */
        //IDE_TEST_RAISE( sReadFileNo == sWriteFileNo, err_Offset );

        /* 해당 파일을 모두 읽었으므로 다음 파일에 접근한다. */
        IDE_TEST( openXLogfileForRead( ++sReadFileNo ) != IDE_SUCCESS );
    }

    do{
        if( sReadFileNo == sWriteFileNo )
        {
            /* read가 write를 앞설수는 없다. */
            IDE_DASSERT( sReadOffset <= sFileWriteOffset );

            if( ( sFileWriteOffset - sReadOffset ) >= aLength )
            {
                /* 남은 데이터가 읽을 데이터의 길이보다 길 경우 정상적으로 read를 수행한다. */
                sIsWait = ID_FALSE;

                /* 직접 읽도록 mmap상 포인터를 넘긴다. */
                *aMMapPtr = (SChar*)mReadBase + sReadOffset;

                /* read Offset을 갱신하고 readXLogLSN을 저장한다. */
                sReadOffset += aLength;
                sReadXLogLSN = RP_SET_XLOGLSN( sReadFileNo, sReadOffset );
                setReadInfoWithLock( sReadXLogLSN, sReadFileNo );
            }
            else
            {
                /* 남은 데이터보다 읽을 데이터의 길이가 더 길경우 아직          *
                 * 해당 xlog가 완전히 쓰여진게 아니므로 busy wait로 대기한다.   */
                sIsWait = ID_TRUE;

                /* write가 다음 파일로 넘어갔을 가능성이 있으므로 write 정보를 다시 받아온다. */
                sWriteXLogLSN = getWriteXLogLSNWithLock();
                RP_GET_FILENO_FROM_XLOGLSN( sWriteFileNo, sWriteXLogLSN );
            }
        }
        else
        {
            /* 파일이 다르면 신경쓸 필요가 없다. */
            sIsWait = ID_FALSE;

            /* 직접 읽도록 mmap상 포인터를 넘긴다. */
            *aMMapPtr = (SChar*)mReadBase + sReadOffset;

            /* read Offset을 갱신하고 readXLogLSN을 저장한다. */
            sReadOffset += aLength;
            sReadXLogLSN = RP_SET_XLOGLSN( sReadFileNo, sReadOffset );
            setReadInfoWithLock( sReadXLogLSN, sReadFileNo );
        }

    }while( sIsWait == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::writeXLog                  *
 * ------------------------------------------------------------------*
 * 입력받은 xlog를 mmap에 쓰고 offset을 증가시킨다.
 * 만약, 현재 파일에 공간이 없어 다 쓰지 못할 정도일 경우에는 새 파일을 열고 해당 파일에 쓴다.
 *
 * aSrc     - [IN] mmap에 복사할 데이터의 포인터
 * aLength  - [IN] mmap에 복사할 데이터의 길이
 * aXSN     - [IN] 해당 xlog의 XSN
 *            (단, write 대상 piece가 xlog의 첫 piece가 아니라면 SM_SN_NULL값만 가진다.)
 *********************************************************************/
IDE_RC rpdXLogfileMgr::writeXLog( void *    aSrc,
                                  UInt      aLength,
                                  smSN      aXSN )
{
    rpXLogLSN           sWriteXLogLSN = RP_XLOGLSN_INIT;
    UInt                sWriteFileNo  = 0;
    UInt                sWriteOffset  = 0;
    rpdXLogfileHeader * sFileHeader = NULL;
#if defined(DEBUG)
    UChar               sTemp[500] ={0,};
    UShort              sTempSize = 0;
#endif

    IDU_FIT_POINT( "rpdXLogfileMgr::writeXLog" );

    IDE_DASSERT( mEndFlag == ID_FALSE );

    IDE_DASSERT( aLength <= ( mFileMaxSize - XLOGFILE_HEADER_SIZE_INIT ) );

    sWriteXLogLSN = mWriteXLogLSN;
    RP_GET_XLOGLSN( sWriteFileNo, sWriteOffset, sWriteXLogLSN );

    sFileHeader = (rpdXLogfileHeader*)mWriteBase;

    IDE_DASSERT( sWriteOffset == sFileHeader->mFileWriteOffset );
    IDE_DASSERT( sWriteOffset <= mFileMaxSize );

    /* 새로 저장할 xlog가 들어갈 공간이 있는지 체크한다. */
    if( sWriteOffset + aLength > mFileMaxSize )
    {
        /* 기존 파일에 write가 끝났다는 것을 표시한다. */
        mCurWriteXLogfile->mIsWriteEnd = ID_TRUE;

        /* 공간이 부족할 경우 새 파일에 접근해야 한다. */
        IDE_TEST( openXLogfileForWrite() != IDE_SUCCESS );

        IDE_TEST_RAISE( *mExitFlag == ID_TRUE, exitFlag_on );

        /* 새 파일에 맞춰 file no와 offset을 갱신한다. */
        sWriteFileNo++;
        sWriteOffset = XLOGFILE_HEADER_SIZE_INIT;

        sFileHeader = (rpdXLogfileHeader*)mWriteBase;
    }

    /* 공간이 있을 경우 현재 파일에 write(memcpy)를 수행한다. */
    idlOS::memcpy( (SChar*)mWriteBase + sWriteOffset, aSrc, aLength );

#if defined(DEBUG)
    if( aLength <= 200 )
    {
        byteToString( (UChar*)aSrc, (UShort)aLength,
                      sTemp, &sTempSize );

        ideLog::log(IDE_RP_6,"[w] - %d|%d|%d : %s", sWriteFileNo, sWriteOffset, aLength, sTemp );
    }
    else
    {
        ideLog::log(IDE_RP_6,"[w] - %d|%d|%d", sWriteFileNo, sWriteOffset,aLength );
    }
#endif

    /* file write offset을 갱신한다. */
    sWriteOffset += aLength;
    sFileHeader->mFileWriteOffset = sWriteOffset;

    /* 새로 쓴 xlog가 해당 파일에 쓰인 첫 xlog의 맨 앞 piece라면 header에 추가 정보를 삽입한다.  */
    if( ( aXSN != SM_SN_NULL ) && ( sFileHeader->mFileFirstXSN == SM_SN_NULL ) )
    {

        sFileHeader->mFileFirstXLogOffset = sWriteOffset - aLength;
        sFileHeader->mFileFirstXSN = aXSN;
    }

    /* write offset에 맞춰 write XLogLSN을 갱신한다. */
    sWriteXLogLSN = RP_SET_XLOGLSN( sWriteFileNo, sWriteOffset );
    setWriteInfoWithLock( sWriteXLogLSN, sWriteFileNo );

    wakeupReader();

    return IDE_SUCCESS;

    IDE_EXCEPTION( exitFlag_on )
    {
        /* 종료 flag가 세팅되어 있음 */
        IDE_SET( ideSetErrorCode( rpERR_ABORT_END_THREAD, mRepName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::openXLogfileForWrite       *
 * ------------------------------------------------------------------*
 * write(transfer)에서 새 xlogfile을 연다.
 * write 경우 create된 xlogfile이 열려있으므로 실제로는 열지 않고 포인터만 이동한다.
 * 기존에 사용하던 파일의 sync 및 close는 여기서 수행하지 않는다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::openXLogfileForWrite()
{
    rpdXLogfile *   sOldFile    = NULL;
    rpXLogLSN       sNewXLogLSN = RP_XLOGLSN_INIT;
    UInt            sNewFileNo  = 0;
    UInt            sNewOffset  = 0;
    UInt            sPrepareFileCnt = 0;
    idBool          sIsSuccess  = ID_FALSE;
    PDL_Time_Value  sCheckTime;
    PDL_Time_Value  sSleepSec;

    IDE_DASSERT( mCurWriteXLogfile != NULL );

    sCheckTime.initialize();
    sSleepSec.initialize();

    sSleepSec.set( 1 );

    while( ( sIsSuccess != ID_TRUE ) && ( *mExitFlag == ID_FALSE ) )
    {
        IDE_TEST_RAISE( mXLFCreater->isCreaterAlive() == ID_FALSE, creater_die );

        mXLFCreater->getPrepareFileCnt( &sPrepareFileCnt );

        /* 미리 준비된 파일이 있을 경우 해당 파일을 사용한다. */
        if( sPrepareFileCnt > 0 )
        {
            IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

            IDE_ASSERT( mCurWriteXLogfile->mNextXLogfile != NULL );

            sOldFile = mCurWriteXLogfile;
            mCurWriteXLogfile = mCurWriteXLogfile->mNextXLogfile;

            IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

            /* 기존 파일을 flusher에 등록한다. */
            IDE_TEST_RAISE( mXLFFlusher->isFlusherAlive() == ID_FALSE, flusher_die );
            mXLFFlusher->addFileToFlushList( sOldFile );

            /* flush thread에게 알람을 보낸다. */
            mXLFFlusher->wakeupFlusher();

            /* write offset 정보를 세팅한다. */
            sNewFileNo = mCurWriteXLogfile->mFileNo;

            sNewOffset = XLOGFILE_HEADER_SIZE_INIT;
            sNewXLogLSN = RP_SET_XLOGLSN( sNewFileNo, sNewOffset );

            setWriteInfoWithLock( sNewXLogLSN, sNewFileNo );
            mWriteBase = mCurWriteXLogfile->mBase;

            sIsSuccess = ID_TRUE;

            IDE_TEST_RAISE( mXLFCreater->isCreaterAlive() == ID_FALSE, creater_die );

            /* prepare file count를 1 감소시킨다. */
            mXLFCreater->decPrepareFileCnt();
        }
        else
        {
            IDU_FIT_POINT( "rpdXLogfileMgr::openXLogfileForWrite::shortageFile" );

            /* 새 파일을 생성하는 것 보다 write가 빨라 다음 파일이 없는 상황 */
            /* create thread를 깨우고 대기한다. */
            IDE_TEST_RAISE( mXLFCreater->isCreaterAlive() == ID_FALSE, creater_die );

            mXLFCreater->wakeupCreater();

            sleepWriter();
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( creater_die )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_CREATER_THREAD_NOT_ALIVE, mRepName ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION( flusher_die )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_FLUSHER_THREAD_NOT_ALIVE, mRepName ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::openXLogfileForRead        *
 * ------------------------------------------------------------------*
 * read(receiver)에서 새 xlogfile을 연다.
 * 새로 열 파일이 write에서 사용중이라면 따로 열지 않고 해당 파일을 그대로 사용한다.
 * 기존에 사용하던 파일은 close처리 한다.
 *
 * aXLogfileNo - [IN] 새로 열 xlogfile의 번호
 *********************************************************************/
IDE_RC rpdXLogfileMgr::openXLogfileForRead( UInt aXLogfileNo )
{
    rpdXLogfile *   sNewFile = NULL;
    rpdXLogfile *   sOldFile = NULL;
    rpXLogLSN       sNewXLogLSN = RP_XLOGLSN_INIT;
    UInt            sNewFileNo = 0;
    UInt            sNewOffset = 0;

    sOldFile = mCurReadXLogfile;

    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    if( ( mCurReadXLogfile != NULL ) &&
        ( mCurReadXLogfile->mNextXLogfile != NULL ) &&
        ( mCurReadXLogfile->mNextXLogfile->mFileNo == aXLogfileNo ) )
    {
        /* 다음 파일이 열려고 한 파일이라면 그대로 사용하면 된다. */
        IDU_FIT_POINT( "rpdXLogfileMgr::openXLogfileForRead::incFileRefCnt" );
        incFileRefCnt( mCurReadXLogfile->mNextXLogfile );

        IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

        mCurReadXLogfile = mCurReadXLogfile->mNextXLogfile;
        sNewOffset = XLOGFILE_HEADER_SIZE_INIT;
        sNewFileNo = aXLogfileNo;
        sNewXLogLSN = RP_SET_XLOGLSN( sNewFileNo, sNewOffset );
        setReadInfoWithLock( sNewXLogLSN, sNewFileNo ); 
        mReadBase  = mCurReadXLogfile->mBase;
    }
    else
    {
        IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

        if( ( mCurReadXLogfile != NULL ) &&
            ( mCurReadXLogfile->mNextXLogfile != NULL ) )
        {
            IDE_DASSERT( mCurReadXLogfile->mNextXLogfile->mFileNo > aXLogfileNo );
        }

        /* 파일을 열고 mapping한다. */
        IDE_TEST( openAndMappingFile( aXLogfileNo, &sNewFile ) != IDE_SUCCESS );

        /* 새로 open한 파일을 file list에 삽입한다. */
        addNewFileToFileList( sOldFile, sNewFile );

        /* 메모리에 없어서 직접 open할 경우 해당 파일은 write가 이미 끝난 파일이다. */
        sNewFile->mIsWriteEnd = ID_TRUE; 

        /* read 관련 정보를 갱신한다. */
        sNewOffset = XLOGFILE_HEADER_SIZE_INIT;
        sNewFileNo = aXLogfileNo;
        sNewXLogLSN = RP_SET_XLOGLSN( sNewFileNo, sNewOffset );
        setReadInfoWithLock( sNewXLogLSN, sNewFileNo );

        mReadBase = sNewFile->mBase;
        mCurReadXLogfile = sNewFile;
    }

    if( sOldFile != NULL )
    {
        decFileRefCnt( sOldFile );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::openFirstXLogfileForRead   *
 * ------------------------------------------------------------------*
 * read(receiver)에서 첫 xlogfile을 연다.
 * 첫 파일의 경우 read에서 직접 열지 않고 create thread에서 생성한 파일을 사용한다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::openFirstXLogfileForRead()
{
    UInt        sPrepareFileCnt;
    rpXLogLSN   sNewXLogLSN = 0;
    UInt        sNewFileNo = 0;
    UInt        sNewOffset = 0;

    IDE_TEST_RAISE( mXLFCreater->isCreaterAlive() == ID_FALSE, creater_die );

    mXLFCreater->getPrepareFileCnt( &sPrepareFileCnt );

    /* 아직 생성된 파일이 없다면 생성될때까지 busy wait로 대기한다. */
    while( sPrepareFileCnt == 0 )
    {
        mXLFCreater->wakeupCreater();

        mXLFCreater->getPrepareFileCnt( &sPrepareFileCnt );
    }

    IDE_DASSERT( mCurReadXLogfile != NULL );

    /* read 관련 정보를 맞춰준다. */
    sNewOffset = XLOGFILE_HEADER_SIZE_INIT;
    sNewFileNo = mCurReadXLogfile->mFileNo;
    sNewXLogLSN = RP_SET_XLOGLSN( sNewFileNo, sNewOffset );
    setReadInfoWithLock( sNewXLogLSN, sNewFileNo );

    mReadBase = mCurReadXLogfile->mBase;

    return IDE_SUCCESS;

    IDE_EXCEPTION( creater_die )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_CREATER_THREAD_NOT_ALIVE, mRepName ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::findLastWrittenFileAndSetWriteInfo  *
 * ------------------------------------------------------------------*
 * write info를 세팅한다.
 * read 중인 파일의 다음 파일이 있는지 찾고, 있을 경우 file write offset을 체크하여
 * 해당 파일에 write가 수행 되었는지 체크하는 것을 반복해 가장 마지막에 write가 수행된 파일을 찾는다.
 * 마지막에 write된 파일을 찾으면 해당 파일을 메모리에 올리고 write 대상으로 연결한다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::findLastWrittenFileAndSetWriteInfo()
{
    UInt                sFileNo;
    iduFile             sFile;
    UInt                sFileWriteOffset = 0;
    SChar               sFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    idBool              sIsEnd = ID_FALSE;
    rpdXLogfile *       sWriteFile = NULL;
    SInt                sLengthCheck = 0;
    rpXLogLSN           sNewWriteXLogLSN;
    rpdXLogfileHeader * sFileHeaderPtr = NULL;
    rpdXLogfileHeader   sFileHeader;

    sFileNo = getReadFileNoWithLock() + 1;

    IDE_TEST( sFile.initialize( IDU_MEM_OTHER, //차후 xlogfile용 추가할 것
                                1, /* Max Open FD Count*/
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    do{
        sLengthCheck = idlOS::snprintf( sFilePath,
                                        XLOGFILE_PATH_MAX_LENGTH,
                                        "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                        mXLogDirPath,
                                        IDL_FILE_SEPARATOR,
                                        mRepName,
                                        sFileNo );
        IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );
        
        if( rpdXLogfileCreater::isFileExist( sFilePath ) == ID_TRUE )
        {
            IDE_TEST( sFile.setFileName( sFilePath ) != IDE_SUCCESS );

            /* 파일을 open한다. */
            IDE_TEST( sFile.openUntilSuccess( ID_FALSE, O_RDWR ) != IDE_SUCCESS );

            /* header(file write offset) 만 read을 수행한다. */
            IDE_TEST( sFile.read( NULL,
                                  0,
                                  &sFileHeader,
                                  XLOGFILE_HEADER_SIZE_INIT ) != IDE_SUCCESS );

            if( sFileHeader.mFileWriteOffset > XLOGFILE_HEADER_SIZE_INIT )
            {
                /* 해당하는 파일은 최소 1번 쓰인 상태이므로 다음 파일을 체크해야 한다. */
                sFileNo++;
                sIsEnd = ID_FALSE;
            }
            else
            {
                /* 해당하는 파일은 생성만 되고 사용되지 않은 파일이다. */
                sIsEnd = ID_TRUE;
            }
        }
        else
        {
            /* 다음 파일이 없으므로 마지막으로 쓰인 파일은 sFile이다 */
            sIsEnd = ID_TRUE;
        }

        /* 파일을 닫는다. */
        IDE_TEST( sFile.close() != IDE_SUCCESS );

    }while( sIsEnd == ID_FALSE );

    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    sFileNo--;

    if( sFileNo == getReadFileNoWithLock() )
    {
        /* read 중인 파일 외에 다른 파일이 없거나 사용하지 않은 경우    *
         * read 중인 파일부터 write를 수행하면 된다.                    */

        mCurWriteXLogfile = mCurReadXLogfile;
        mWriteBase        = mReadBase;

        incFileRefCnt( mCurWriteXLogfile );
    }
    else
    {
        /* read중인 파일 외에 write된 파일이 있는 경우 가장 마지막 write된 파일부터 write를 시작한다. */
        IDE_DASSERT( sFileNo > getReadFileNoWithLock() );

        /* 새 파일을 open 및 mapping한다. */
        IDE_TEST( openAndMappingFile( sFileNo, &sWriteFile ) != IDE_SUCCESS );

        /* 새로 open한 파일을 file list에 연결한다. */
        addToFileListinLast( sWriteFile );

        mXLogfileListTail = sWriteFile;
        mCurWriteXLogfile = sWriteFile;
        mWriteBase        = sWriteFile->mBase;
    }

    /* write info를 세팅한다. */
    sFileHeaderPtr = (rpdXLogfileHeader*)mWriteBase;
    sFileWriteOffset = sFileHeaderPtr->mFileWriteOffset;

    sNewWriteXLogLSN = RP_SET_XLOGLSN( sFileNo, sFileWriteOffset );
    setWriteInfoWithLock( sNewWriteXLogLSN, sFileNo );   

    mCurWriteXLogfile->mIsWriteEnd = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_file_path )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_FILE_PATH, sFilePath ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::sleepReader                *
 * ------------------------------------------------------------------*
 * read(receiver)가 write(transfer)를 따라잡았을 때 잠시 대기한다.
 * 기본적으로 1초 대기하나 write가 signal을 보내 깨울 수도 있다.
 * 대기 시간이 일정이상 될 경우에는 timeout이 발생한다.
 *********************************************************************/
void rpdXLogfileMgr::sleepReader()
{
    PDL_Time_Value  sCheckTime;
    PDL_Time_Value  sSleepSec;

    sCheckTime.initialize();
    sSleepSec.initialize();

    sSleepSec.set( 1 );

    IDE_ASSERT( mReadWaitMutex.lock( NULL ) == IDE_SUCCESS );
    sCheckTime = idlOS::gettimeofday() + sSleepSec;

    IDU_FIT_POINT( "rpdXLogfileMgr::sleepReader" );

    mIsReadSleep = ID_TRUE;
    (void)mReadWaitCV.timedwait( &mReadWaitMutex, &sCheckTime );
    mIsReadSleep = ID_FALSE;

    IDE_ASSERT( mReadWaitMutex.unlock() == IDE_SUCCESS );

    IDE_EXCEPTION_END;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::wakeupReader               *
 * ------------------------------------------------------------------*
 * 대기중인 read(receiver)를 깨운다.
 * 해당 함수는 write(transfer)에서 호출된다.
 *********************************************************************/
void rpdXLogfileMgr::wakeupReader()
{
    IDU_FIT_POINT( "rpdXLogfileMgr::wakeupReader" );

    IDE_ASSERT( mReadWaitMutex.lock( NULL ) == IDE_SUCCESS );

    if( mIsReadSleep == ID_TRUE )
    {
        IDE_ASSERT( mReadWaitCV.signal() == IDE_SUCCESS );
    }

    IDE_ASSERT( mReadWaitMutex.unlock() == IDE_SUCCESS );

    IDE_EXCEPTION_END;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::sleepWriter                 *
 * ------------------------------------------------------------------*
 * write(transfer)가 create thread를 따라잡았을 때 잠시 대기한다.
 * 기본적으로 1초 대기하나 create thread가 signal을 보내 깨울 수도 있다.
 *********************************************************************/
void rpdXLogfileMgr::sleepWriter()
{
    PDL_Time_Value  sCheckTime;
    PDL_Time_Value  sSleepSec;

    sCheckTime.initialize();
    sSleepSec.initialize();

    sSleepSec.set( 1 );

    IDE_ASSERT( mWriteWaitMutex.lock( NULL ) == IDE_SUCCESS );
    sCheckTime = idlOS::gettimeofday() + sSleepSec;

    mIsWriteSleep = ID_TRUE;
    (void)mWriteWaitCV.timedwait( &mWriteWaitMutex, &sCheckTime );
    mIsWriteSleep = ID_FALSE;

    IDE_ASSERT( mWriteWaitMutex.unlock() == IDE_SUCCESS );

    return; 
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::wakeupWriter              *
 * ------------------------------------------------------------------*
 * 대기중인 write(transfer)를 깨운다.
 * 해당 함수는 create thread에서 호출된다.
 *********************************************************************/
void rpdXLogfileMgr::wakeupWriter()
{
    IDE_ASSERT( mWriteWaitMutex.lock( NULL ) == IDE_SUCCESS );

    if( mIsWriteSleep == ID_TRUE )
    {
        IDE_ASSERT( mWriteWaitCV.signal() == IDE_SUCCESS );
    }

    IDE_ASSERT( mWriteWaitMutex.unlock() == IDE_SUCCESS );

    return; 
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::stopSubThread              *
 * ------------------------------------------------------------------*
 * create thread와 flush thread를 정지시킨다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::stopSubThread()
{
    /* 현재 write 중인 파일을 flush list에 삽입한다.*/
    IDE_TEST_RAISE( mXLFFlusher->isFlusherAlive() == ID_FALSE, flusher_die );
    mXLFFlusher->addFileToFlushList( mCurWriteXLogfile );

    /* flush thread를 정리한다. */
    if( mXLFFlusher->isFlusherAlive() == ID_TRUE )
    {
        mXLFFlusher->mExitFlag = ID_TRUE;
        mXLFFlusher->wakeupFlusher();
        IDE_TEST( mXLFFlusher->join() != IDE_SUCCESS );
        mXLFFlusher->finalize();

        ideLog::log( IDE_RP_0, "[Receiver] Replication %s finalized XLogfile Flusher Thread", mRepName );
    }

    /* create thread를 정리한다. */
    if( mXLFCreater->isCreaterAlive() == ID_TRUE )
    {
        mXLFCreater->mExitFlag = ID_TRUE;
        mXLFCreater->wakeupCreater();
        IDE_TEST( mXLFCreater->join() != IDE_SUCCESS );
        mXLFCreater->finalize();

        ideLog::log( IDE_RP_0, "[Receiver] Replication %s finalized XLogfile Creater Thread", mRepName );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( flusher_die )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_FLUSHER_THREAD_NOT_ALIVE, mRepName ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::destroySubThread           *
 * ------------------------------------------------------------------*
 * create thread와 flush thread의 수행을 종료한다.
 *********************************************************************/
void rpdXLogfileMgr::destroySubThread()
{
    mXLFFlusher->destroy();
    mXLFCreater->destroy();

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getFileRefCnt              *
 * ------------------------------------------------------------------*
 * 해당 xlogfile의 reference count를 가져온다.
 *
 * aFile     - [IN] reference count를 가져올 rpdXlogfile의 포인터
 *********************************************************************/
UInt rpdXLogfileMgr::getFileRefCnt( rpdXLogfile * aFile )
{
    SInt    sRefCnt;

    IDE_ASSERT( aFile != NULL );

    IDE_ASSERT( aFile->mRefCntMutex.lock( NULL ) == IDE_SUCCESS );
    sRefCnt = aFile->mFileRefCnt;
    IDE_ASSERT( aFile->mRefCntMutex.unlock() == IDE_SUCCESS );

    return sRefCnt;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::incFileRefCnt              *
 * ------------------------------------------------------------------*
 * 해당 xlogfile의 reference count를 증가시킨다.
 * 증가시키지 전 값이 0일 경우 다른 thread에 의해 제거되는 중이므로 
 * aIsSuccess에 false를 리턴한다.
 *
 * aFile      - [IN]  reference count를 증가시킬 rpdXlogfile의 포인터
 *********************************************************************/
void rpdXLogfileMgr::incFileRefCnt( rpdXLogfile * aFile )
{
    IDE_ASSERT( aFile != NULL );

    IDE_ASSERT( aFile->mRefCntMutex.lock( NULL ) == IDE_SUCCESS );

    aFile->mFileRefCnt++;

    IDE_ASSERT( aFile->mRefCntMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::decFileRefCnt              *
 * ------------------------------------------------------------------*
 * 해당 xlogfile의 reference count를 감소시킨다.
 * 만약 감소시킨 후 0이라면 해당 파일을 정리하고 리스트에서 제거한다.
 *
 * aFile     - [IN] reference count를 감소시킬 rpdXlogfile의 포인터
 *********************************************************************/
void rpdXLogfileMgr::decFileRefCnt( rpdXLogfile * aFile )
{
    IDE_DASSERT( aFile != NULL );

    IDE_ASSERT( aFile->mRefCntMutex.lock( NULL ) == IDE_SUCCESS );

    IDE_DASSERT( aFile->mFileRefCnt > 0 );

    aFile->mFileRefCnt--;

    IDE_ASSERT( aFile->mRefCntMutex.unlock() == IDE_SUCCESS );

    if( aFile->mFileRefCnt == 0 )
    {
        /* reference count가 0이 되었다면 해당 파일을 정리한다. */
        removeFile( aFile );
    }
    else
    {
        /* reference count가 0이 아니라면 해당 파일을 유지한다. */
    }

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getReadXLogLSNWithLock     *
 * ------------------------------------------------------------------*
 * read lock을 잡은 상태에서 read xlogLSN을 가져온다.
 * 해당 함수는 read외에서 read의 정보에 접근하기 위해 사용된다.
 *********************************************************************/
rpXLogLSN rpdXLogfileMgr::getReadXLogLSNWithLock()
{
    rpXLogLSN sReadXLogLSN = RP_XLOGLSN_INIT;

    IDE_ASSERT( mReadInfoMutex.lock( NULL ) == IDE_SUCCESS );
    sReadXLogLSN = mReadXLogLSN;
    IDE_ASSERT( mReadInfoMutex.unlock() == IDE_SUCCESS );

    return sReadXLogLSN;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getReadFileNoWithLock     *
 * ------------------------------------------------------------------*
 * read lock을 잡은 상태에서 read중인 파일의 번호를 가져온다.
 * 해당 함수는 read외에서 read의 정보에 접근하기 위해 사용된다.
 *********************************************************************/
UInt rpdXLogfileMgr::getReadFileNoWithLock()
{
    UInt    sReadFileNo;

    IDE_ASSERT( mReadInfoMutex.lock( NULL ) == IDE_SUCCESS );
    sReadFileNo = mReadFileNo;
    IDE_ASSERT( mReadInfoMutex.unlock() == IDE_SUCCESS );

    return sReadFileNo;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::setReadInfoWithLock        *
 * ------------------------------------------------------------------*
 * read lock을 잡은 상태에서 read info를 갱신한다.
 * 해당 함수는 read에서 read info를 변경할때 사용된다.
 * 주의 : 기본적으로 read info는 read에서만 변경이 가능하다.
 *********************************************************************/
void rpdXLogfileMgr::setReadInfoWithLock( rpXLogLSN aLSN,
                                          UInt      aFileNo )
{
    IDE_ASSERT( mReadInfoMutex.lock( NULL ) == IDE_SUCCESS );
    mReadXLogLSN = aLSN;
    mReadFileNo = aFileNo; 
    IDE_ASSERT( mReadInfoMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getWriteXLogLSNWithLock    *
 * ------------------------------------------------------------------*
 * write lock을 잡은 상태에서 write xlogLSN을 가져온다.
 * 해당 함수는 write외에서 write의 정보에 접근하기 위해 사용된다.
 *********************************************************************/
rpXLogLSN rpdXLogfileMgr::getWriteXLogLSNWithLock()
{
    rpXLogLSN sWriteXLogLSN = RP_XLOGLSN_INIT;

    IDE_ASSERT( mWriteInfoMutex.lock( NULL ) == IDE_SUCCESS );
    sWriteXLogLSN = mWriteXLogLSN;
    IDE_ASSERT( mWriteInfoMutex.unlock() == IDE_SUCCESS );

    return sWriteXLogLSN;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getWriteFileNoWithLock     *
 * ------------------------------------------------------------------*
 * read lock을 잡은 상태에서 write중인 파일의 번호를 가져온다.
 * 해당 함수는 write외에서 write의 정보에 접근하기 위해 사용된다.
 *********************************************************************/
UInt rpdXLogfileMgr::getWriteFileNoWithLock()
{
    UInt    sWriteFileNo;

    IDE_ASSERT( mWriteInfoMutex.lock( NULL ) == IDE_SUCCESS );
    sWriteFileNo = mWriteFileNo;
    IDE_ASSERT( mWriteInfoMutex.unlock() == IDE_SUCCESS );

    return sWriteFileNo;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::setWriteInfoWithLock        *
 * ------------------------------------------------------------------*
 * write lock을 잡은 상태에서 write info를 갱신한다.
 * 해당 함수는 write에서 write info를 변경할때 사용된다.
 * 주의 : 기본적으로 write info는 write에서만 변경이 가능하다.
 *********************************************************************/
void rpdXLogfileMgr::setWriteInfoWithLock( rpXLogLSN aLSN,
                                           UInt      aFileNo )
{
    IDE_ASSERT( mWriteInfoMutex.lock( NULL ) == IDE_SUCCESS );
    mWriteXLogLSN = aLSN;
    mWriteFileNo = aFileNo;
    IDE_ASSERT( mWriteInfoMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::addToFileListinLast        *
 * ------------------------------------------------------------------*
 * xlogfile list의 마지막(tail)에 새 파일을 추가한다.
 * 해당 함수는 create thread에서 새로 create/open한 파일을 list에 연결할때 사용한다.
 *
 * aNewFile - [IN] xlogfile list에 추가할 xlogfile(rpdXLogfile)
 *********************************************************************/
void rpdXLogfileMgr::addToFileListinLast( rpdXLogfile * aNewFile )
{
    IDE_DASSERT( aNewFile != NULL );

    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    if( mXLogfileListTail == NULL )
    {
        mXLogfileListHeader = aNewFile;
        mXLogfileListTail = aNewFile;
    }
    else
    {
        IDE_DASSERT( mXLogfileListHeader != NULL );

        aNewFile->mPrevXLogfile = mXLogfileListTail;
        mXLogfileListTail->mNextXLogfile = aNewFile;
        mXLogfileListTail = aNewFile;
    }

    IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::openAndMappingFile         *
 * ------------------------------------------------------------------*
 * 새 파일을 open/mapping을 수행하고 xlogfile list에 연결한다.
 * 연결 위치는 인자로 주어진 파일 바로 다음이다.
 *
 * aXLogfileNo - [IN] 새로 열고 mapping할 파일의 번호
 * aBeforeFile - [IN] xlogfile list에 추가하기 위한 위치
                      해당 파일의 바로 다음에 새 파일을 연결한다.
 * aNewFile   -  [OUT] 새로 추가한 파일의 포인터
 *********************************************************************/
IDE_RC rpdXLogfileMgr::openAndMappingFile( UInt             aXLogfileNo,
                                           rpdXLogfile **   aNewFile )
{
    rpdXLogfile *   sNewFile;
    SChar           sBuffer[IDU_MUTEX_NAME_LEN];
    SChar           sNewFilePath[XLOGFILE_PATH_MAX_LENGTH];
    SInt            sLengthCheck = 0;

    /* 다음 파일이 열려고 한 파일이 아니라면 새 파일을 열어 사용해야 한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                 ID_SIZEOF( rpdXLogfile ),
                                 (void**)&(sNewFile) )
              != IDE_SUCCESS );

    /* xlogfile meta(rpdXLogfile)관리용 mutex를 초기화 한다. */
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "REF_xlogfile%s_%"ID_UINT32_FMT,
                                    mRepName,
                                    aXLogfileNo );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );

    IDE_TEST( sNewFile->mRefCntMutex.initialize( sBuffer,
                                                 IDU_MUTEX_KIND_POSIX,
                                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( sNewFile->mFile.initialize( IDU_MEM_RP_RPD, //차후 xlogfile용 추가할 것
                                          1, /* Max Open FD Count*/
                                          IDU_FIO_STAT_OFF,
                                          IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    /* 새로 열 파일의 이름을 세팅한다. */
    sLengthCheck = idlOS::snprintf( sNewFilePath,
                                    XLOGFILE_PATH_MAX_LENGTH,
                                    "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                    mXLogDirPath,
                                    IDL_FILE_SEPARATOR,
                                    mRepName,
                                    aXLogfileNo );
    IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

    IDE_TEST( sNewFile->mFile.setFileName( sNewFilePath ) != IDE_SUCCESS );

    /* 새 파일을 open한다. */
    IDE_TEST( sNewFile->mFile.openUntilSuccess( ID_FALSE, O_RDWR ) != IDE_SUCCESS );

    /* mapping을 수행한다. */
    IDE_TEST( sNewFile->mFile.mmap( NULL,
                                    RPU_REPLICATION_XLOGFILE_SIZE,
                                    ID_TRUE,
                                    &( sNewFile->mBase ) ) != IDE_SUCCESS );

    /* rpdXLogfile의 메타 정보를 갱신한다. */
    setFileMeta( sNewFile, aXLogfileNo ); 

    *aNewFile = sNewFile;

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_mutex_name )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_MUTEX_NAME, sBuffer ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION( too_long_file_path )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_FILE_PATH, sNewFilePath ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdXLogfileMgr::openAndReadHeader( SChar * aRepName,
                                          SChar * aPath,
                                          UInt    aXLogfileNo,
                                          rpdXLogfileHeader * aOutHeader )
{
    rpdXLogfile     sNewFile;
    SChar           sNewFilePath[XLOGFILE_PATH_MAX_LENGTH];
    SInt            sLengthCheck = 0;

    /* 새로 열 파일의 이름을 세팅한다. */
    sLengthCheck = idlOS::snprintf( sNewFilePath,
                                    XLOGFILE_PATH_MAX_LENGTH,
                                    "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                    aPath,
                                    IDL_FILE_SEPARATOR,
                                    aRepName,
                                    aXLogfileNo );
    IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );
    IDE_TEST( sNewFile.mFile.initialize( IDU_MEM_RP_RPD, //차후 xlogfile용 추가할 것
                                          1, /* Max Open FD Count*/
                                          IDU_FIO_STAT_OFF,
                                          IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    IDE_TEST( sNewFile.mFile.setFileName( sNewFilePath ) != IDE_SUCCESS );

    /* 새 파일을 open한다. */
    IDE_TEST( sNewFile.mFile.openUntilSuccess( ID_FALSE, O_RDWR ) != IDE_SUCCESS );

    IDE_TEST(sNewFile.mFile.read(NULL,
                                  0,
                                  (void*)aOutHeader,
                                  ID_SIZEOF(rpdXLogfileHeader)) != IDE_SUCCESS);
    IDE_TEST( sNewFile.mFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_file_path )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_FILE_PATH, sNewFilePath ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::addNewFileToFileList       *
 * ------------------------------------------------------------------*
 * 새 파일을 xlogfile list에 추가한다.
 * 추가하는 위치는 인자로 넘긴 파일 다음이다.
 * aBeforeFile - [IN] xlogfile list에 추가하기 위한 위치
                      해당 파일의 바로 다음에 새 파일을 연결한다.
 * aNewFile   -  [IN] 새로 추가한 파일의 포인터
 *********************************************************************/
void rpdXLogfileMgr::addNewFileToFileList( rpdXLogfile *  aBeforeFile,
                                           rpdXLogfile *  aNewFile )
{
    IDE_DASSERT( aNewFile != NULL );

    /* xlogfile list를 변경시켜야 하므로 list lock을 잡는다. */
    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    aNewFile->mPrevXLogfile = aBeforeFile;

    if( aBeforeFile != NULL )
    {
        aNewFile->mNextXLogfile = aBeforeFile->mNextXLogfile;

        if( aBeforeFile->mNextXLogfile != NULL )
        {
            aBeforeFile->mNextXLogfile->mPrevXLogfile = aNewFile;
        }

        aBeforeFile->mNextXLogfile = aNewFile;
    }

    if( mXLogfileListTail == NULL )
    {
        mXLogfileListTail = aNewFile;
    }

    if( mXLogfileListHeader == NULL )
    {
        mXLogfileListHeader = aNewFile;
    }

    IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::removeFile                 *
 * ------------------------------------------------------------------*
 * 해당하는 xlogfile을 메모리에서 제거한다.(파일이 지워지지는 않는다.)
 * read나 flush등에서 사용이 끝난 파일을 정리할때 호출한다.
 * 해당 파일의 file reference count는 0이어야 하며 만약 lock을 잡는 사이 
 * 다른 thread가 접근해 reference count를 증가시켰을 경우에는
 * 해당 파일의 제거를 취소한다.
 * aFile   -  [IN] 제거할 파일의 포인터 
 *********************************************************************/
void rpdXLogfileMgr::removeFile( rpdXLogfile * aFile )
{
    IDE_DASSERT( aFile != NULL );

    /* file list를 수정해야 하므로 list lock을 잡는다. */
    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    IDE_ASSERT( aFile->mRefCntMutex.lock( NULL ) == IDE_SUCCESS );

    if( aFile->mFileRefCnt > 0 )
    {
        /* list mutex를 잡는 사이에 다른 thread가 접근하여  *
         * ref cnt를 증가시킨 경우 제거를 중지한다.         */
        IDE_ASSERT( aFile->mRefCntMutex.unlock() == IDE_SUCCESS );
        IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

        IDE_CONT( normal_Exit );
    }

    if( aFile->mPrevXLogfile != NULL )
    {
        aFile->mPrevXLogfile->mNextXLogfile = aFile->mNextXLogfile;
    }

    if( aFile->mNextXLogfile != NULL )
    {
        aFile->mNextXLogfile->mPrevXLogfile = aFile->mPrevXLogfile;
    }

    if( mXLogfileListHeader == aFile )
    {
        mXLogfileListHeader = aFile->mNextXLogfile;
    }

    if( mXLogfileListTail == aFile )
    {
        mXLogfileListTail = aFile->mPrevXLogfile;
    }

    IDE_ASSERT( aFile->mRefCntMutex.unlock() == IDE_SUCCESS );

    IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

    if( idlOS::munmap( aFile->mBase, RPU_REPLICATION_XLOGFILE_SIZE ) == IDE_SUCCESS )
    {
        (void)aFile->mRefCntMutex.destroy();
        (void)aFile->mFile.close();
        (void)aFile->mFile.destroy();
        (void)iduMemMgr::free( aFile );
    }
    else
    {
        IDE_ERRLOG(IDE_RP_0);
        (void)aFile->mRefCntMutex.destroy();
    }

    RP_LABEL( normal_Exit );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::setFileMeta                *
 * ------------------------------------------------------------------*
 * 새로 mapping한 파일의 메타를 초기값으로 세팅한다.
 *
 * aFile    - [IN] 새로 mapping한 파일의 포인터
 * aFileNo  - [IN] 새로 mapping한 파일에 부여할 파일 번호
 *********************************************************************/
void rpdXLogfileMgr::setFileMeta( rpdXLogfile *  aFile,
                                  UInt           aFileNo )
{
    IDE_DASSERT( aFile != NULL );

    aFile->mFileNo = aFileNo;
    aFile->mFileRefCnt = 1;
    aFile->mPrevXLogfile = NULL;
    aFile->mNextXLogfile = NULL;
    aFile->mIsWriteEnd = ID_FALSE;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::rpdXLogfileEmergency       *
 * ------------------------------------------------------------------*
 * sync, create등에서 예외가 발생했을때 smiSetEmergency를 대체하기 위한 함수
 * 현재는 아무런 역할을 하지 않으나 차후 I/O 관련 작업 중 예외 발생시 
 * 후속 작업을 추가하고 싶을 경우 여기에 추가하도록 한다.
 *
 * 주의 : 예외 상황에서는 인자가 true가 들어온다.
 *********************************************************************/
void rpdXLogfileMgr::rpdXLogfileEmergency( idBool /* aFlag */ )
{
    return ;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::byteToString               *
 * ------------------------------------------------------------------*
 * xlogfile에 read/write를 확인하기 위해 trc로그에 출력하기 위해 사용하는 함수
 * qcsModule::byteToString 함수에서 가져와 사용한다.
 * 해당 함수는 테스트용으로만 사용되어야 하며 일반적인 작업시에는 사용되서는 안된다.
 *********************************************************************/
IDE_RC rpdXLogfileMgr::byteToString( UChar   * aByte,
                                     UShort    aByteSize,
                                     UChar   * aString,
                                     UShort  * aStringSize )
{
    SChar   sByteValue;
    SChar   sStringValue;
    SInt    sByteIterator;
    SInt    sStringIterator;

    for( sByteIterator = 0, sStringIterator = 0;
         sByteIterator < (SInt) aByteSize;
         sByteIterator++, sStringIterator += 2)
    {
        sByteValue = aByte[sByteIterator];

        sStringValue = (sByteValue & 0xF0) >> 4;
        aString[sStringIterator+0] =
            (sStringValue < 10) ? (sStringValue + '0') : (sStringValue + 'A' - 10);

        sStringValue = (sByteValue & 0x0F);
        aString[sStringIterator+1] =
            (sStringValue < 10) ? (sStringValue + '0') : (sStringValue + 'A' - 10);
    }
            
    *aStringSize = (UShort) sStringIterator;

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getXLogfileNumberInternal  *
 * ------------------------------------------------------------------*
 * 특정 XSN이 어느 xlogfile에 있는지 체크하는 함수
 * 주어진 파일 번호부터 역방향으로 진행하면서 xlogfile header의 mFileFirstXSN을 가져와 비교한다.
 * 탐색에 성공할 경우 해당 파일의 번호를 리턴하고 탐색에 실패할 경우 -1을 리턴한다.
 * 
 * aXSN         - [IN]  탐색할 XSN
 * aStartFileNo - [IN]  탐색을 시작할 파일번호
 * aRepName     - [IN]  해당 replication의 이름
 * aFileNo      - [OUT] 탐색한 파일의 번호
 *********************************************************************/
IDE_RC rpdXLogfileMgr::getXLogfileNumberInternal( smSN        aXSN,
                                                  SInt        aStartFileNo,
                                                  SChar *     aReplName,
												  rpXLogLSN * aXLogLSN )
{
    SInt                sFileNo = aStartFileNo;
    SChar               sFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    iduFile             sFile;
    SChar *             sXLogDirPath = NULL;
    idBool              sIsFind = ID_FALSE;
    UInt                sLengthCheck = 0;
    rpdXLogfileHeader   sFileHeader;
    rpXLogLSN           sFirstXLogLSN = RP_XLOGLSN_INIT;
    idBool              sIsInitializedFile = ID_FALSE;
    idBool              sIsOpenedFile = ID_FALSE;
#if defined(DEBUG)
    rpdXLogfileHeader   sFileHeaderForCheck;
#endif

    IDE_DASSERT( aStartFileNo >= 0 );

    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&sXLogDirPath ) != IDE_SUCCESS );

    IDE_TEST( sFile.initialize( IDU_MEM_OTHER,
                                1, /* Max Open FD Count*/
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sIsInitializedFile = ID_TRUE;

    while( ( sFileNo >= -1 ) && ( sIsFind == ID_FALSE ) )
    {
        idlOS::memset( sFilePath, 0, XLOGFILE_PATH_MAX_LENGTH );

        sLengthCheck = idlOS::snprintf( sFilePath,
                                        XLOGFILE_PATH_MAX_LENGTH,
                                        "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                        sXLogDirPath,
                                        IDL_FILE_SEPARATOR,
                                        aReplName,
                                        sFileNo );

        IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

        if( rpdXLogfileCreater::isFileExist( sFilePath ) == ID_FALSE )
        {
            /* 맨 앞 까지 다 탐색했는데 해당하는 파일이 없는 경우(탐색실패) */
            break;
        }
        else
        {
            IDE_TEST( sFile.setFileName( sFilePath ) != IDE_SUCCESS );

            /* 파일을 open한다. */
            IDE_TEST( sFile.openUntilSuccess( ID_FALSE, O_RDONLY ) != IDE_SUCCESS );
            sIsOpenedFile = ID_TRUE;

            /* header(file write offset) 만 read을 수행한다. */
            IDE_TEST( sFile.read( NULL,
                                  0,
                                  &sFileHeader,
                                  XLOGFILE_HEADER_SIZE_INIT ) != IDE_SUCCESS );

            if( ( sFileHeader.mFileFirstXSN <= aXSN ) && ( sFileHeader.mFileFirstXSN != SM_SN_NULL ) )
            {
                sIsFind = ID_TRUE;

                IDE_DASSERT( sFileHeader.mFileFirstXLogOffset != RP_XLOGLSN_INIT );
                sFirstXLogLSN = RP_SET_XLOGLSN( sFileNo, sFileHeader.mFileFirstXLogOffset );

                sIsOpenedFile = ID_FALSE;
                IDE_TEST( sFile.close() != IDE_SUCCESS );

#if defined(DEBUG)
                /* 혹시나 해서 넣어두는 테스트 *
                 * 다음 파일을 체크해서 XSN 비교가 실패하는지 체크한다.  */
                sLengthCheck = idlOS::snprintf( sFilePath,
                                                XLOGFILE_PATH_MAX_LENGTH,
                                                "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                                sXLogDirPath,
                                                IDL_FILE_SEPARATOR,
                                                aReplName,
                                                sFileNo + 1);
                
                if( rpdXLogfileCreater::isFileExist( sFilePath ) == ID_TRUE )
                {
                    IDE_TEST( sFile.setFileName( sFilePath ) != IDE_SUCCESS );
                    IDE_TEST( sFile.openUntilSuccess( ID_FALSE, O_RDWR ) != IDE_SUCCESS );
                    sIsOpenedFile = ID_FALSE;
                    IDE_TEST( sFile.read( NULL,
                                          0,
                                          &sFileHeaderForCheck,
                                          XLOGFILE_HEADER_SIZE_INIT ) != IDE_SUCCESS );
                    ideLog::log(IDE_RP_0,"[CHECK][nextfile] fileno = %d, aXSN = %lu, fileFirstXSN = %lu",
                                sFileNo+1, aXSN, sFileHeaderForCheck.mFileFirstXSN );
                    sIsOpenedFile = ID_FALSE;
                    IDE_TEST( sFile.close() != IDE_SUCCESS );
                }
                else
                {
                    ideLog::log(IDE_RP_0,"[CHECK] nextfile is not exist" );
                }

#endif
                break;
            }
            else
            {
                /* 해당 파일에 탐색 XSN에 해당하는 xlog가 없을 경우 앞 파일에 대해 반복한다. */
                sIsFind = ID_FALSE;
                sFileNo--;

                sIsOpenedFile = ID_FALSE;
                IDE_TEST( sFile.close() != IDE_SUCCESS );
            }
        }
    }

    sIsInitializedFile = ID_FALSE;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

     /* 단 sFileNo가 -1 이라면, 파일은 존재하나 xlog가 한번도 셋팅이 안된 경우다.
      * 이때는 XLOGFILE_HEADER_SIZE_INIT 값으로 결정한다.
      */
     if ( sFirstXLogLSN == RP_XLOGLSN_INIT )
     {
         sFirstXLogLSN = RP_SET_XLOGLSN( 0, XLOGFILE_HEADER_SIZE_INIT );
     }

    *aXLogLSN = sFirstXLogLSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_file_path )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_FILE_PATH, sFilePath ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsOpenedFile == ID_TRUE )
    {
        (void)sFile.close();
    }

    if ( sIsInitializedFile == ID_TRUE )
    {
        (void)sFile.destroy();
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getXLogfileNumber          *
 * ------------------------------------------------------------------*
 * 특정 XSN이 어느 xlogfile에 있는지 체크하는 함수
 * 해당 함수는 xlogfileMgr이 떠 있지 않은 상태에서만 사용해야 한다.
 * 메타에 저장된 read info의 갱신이 늦어 탐색 대상이 메타보다 뒤에 있을 가능성이 있으므로
 * 가장 최신 파일을 찾아 역방향으로 진행하면서 xlogfile header의 mFileFirstXSN을 가져와 비교한다.
 * 
 * aXSN         - [IN]  탐색할 XSN
 * aRepName     - [IN]  해당 replication의 이름
 * aFileNo      - [OUT] 탐색한 파일의 LSN (FileNo, Offset)
 *********************************************************************/
IDE_RC rpdXLogfileMgr::getXLogLSNFromXSNAndReplName( smSN         aXSN,
                                                     SChar     * aReplName,
                                                     rpXLogLSN * aXLogLSN )
{
    UInt    sFirstFileNo = 0;
    UInt    sLastFileNo = 0;

    if( findFirstAndLastFileNo( aReplName, &sFirstFileNo, &sLastFileNo) == IDE_SUCCESS )
    {
        IDE_TEST( getXLogfileNumberInternal( aXSN,
                                             sLastFileNo,
                                             aReplName,
                                             aXLogLSN )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( ideGetErrorCode() != rpERR_ABORT_RP_CANNOT_FIND_XLOGFILE );
        aXLogLSN = RP_XLOGLSN_INIT;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileMgr::getXLogfileNumber          *
 * ------------------------------------------------------------------*
 * 특정 XSN이 어느 xlogfile에 있는지 체크하는 함수
 * 해당 함수는 xlogfileMgr이 떠 있는 상태에서만 사용해야 한다.
 * file list에 존재하는 첫 파일부터 탐색을 시작하여
 * 역방향으로 진행하면서 xlogfile header의 mFileFirstXSN을 가져와 비교한다.
 * 
 * aXSN         - [IN]  탐색할 XSN
 * aFileNo      - [OUT] 탐색한 파일의 번호
 *********************************************************************/
IDE_RC rpdXLogfileMgr::getXLogLSNFromXSN( smSN        aXSN,
										  rpXLogLSN * aXLogLSN )
{
    SInt                  sFileNo = 0;

    IDE_ASSERT( mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );

    IDE_ASSERT( mXLogfileListHeader != NULL );

    sFileNo = mXLogfileListHeader->mFileNo;

    IDE_ASSERT( mXLogfileListMutex.unlock() == IDE_SUCCESS );

    IDE_TEST( getXLogfileNumberInternal( aXSN, 
                                         sFileNo,
                                         mRepName,
										 aXLogLSN ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt rpdXLogfileMgr::checkXLogFileAndGetFileNoFromRepName( SChar * aReplName,
                                                           SChar * aFileName,
                                                           idBool * aIsXLogfile )
{

    UInt   i;
    UInt   sFileNo = 0;
    SChar  sChar;
    UInt   sPrefixLen;
    idBool sIsLogFile;
    SChar sPrefix[100];


    sIsLogFile = ID_TRUE;

    sPrefixLen = idlOS::snprintf( sPrefix,
                                  100,
                                  "xlogfile%s_",
                                  aReplName );

    if ( idlOS::strncmp(aFileName, sPrefix, sPrefixLen) == 0 )
    {
        for ( i = sPrefixLen; i < idlOS::strlen(aFileName); i++)
        {
            if ( (aFileName[i] >= '0') && (aFileName[i] <= '9'))
            {
                continue;
            }
            else
            {
                sIsLogFile = ID_FALSE;
            }
        }
    }
    else
    {
        sIsLogFile = ID_FALSE;
    }

    if ( sIsLogFile == ID_TRUE )
    {
        for ( i = sPrefixLen; i < idlOS::strlen(aFileName); i++)
        {
            sChar   = aFileName[i];
            sFileNo = sFileNo * 10;
            sFileNo = sFileNo + (sChar - '0');
        }
    }

    *aIsXLogfile = sIsLogFile;

    return sFileNo;
}

IDE_RC rpdXLogfileMgr::findFirstAndLastFileNo( SChar * aReplName,
                                               UInt * aFirstXLogFileNo,
                                               UInt * aLastXLogFileNo )
{
    DIR           * sDirp = NULL;
    struct dirent * sDir  = NULL;
    UInt            sCurLogFileNo;
    idBool          sIsSetTempLogFileName = ID_FALSE;
    idBool          sIsLogFile;
    UInt            sFirstFileNo = 0;
    UInt            sEndFileNo = 0;


    SChar * sXLogDirPath = NULL;

    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&sXLogDirPath ) != IDE_SUCCESS );

    sDirp = idf::opendir( sXLogDirPath );
    IDE_TEST_RAISE(sDirp == NULL, error_open_dir);

    while(1)
    {
        sDir = idf::readdir(sDirp);
        if ( sDir == NULL )
        {
            break;
        }

        sCurLogFileNo = checkXLogFileAndGetFileNoFromRepName(aReplName,
                                                             sDir->d_name,
                                                             &sIsLogFile);

        if ( sIsLogFile == ID_FALSE)
        {
            continue;
        }

        if ( sIsSetTempLogFileName == ID_FALSE)
        {
            sFirstFileNo = sCurLogFileNo;
            sEndFileNo = sCurLogFileNo;
            sIsSetTempLogFileName = ID_TRUE;
            continue;
        }

        if ( sFirstFileNo > sCurLogFileNo)
        {
            sFirstFileNo = sCurLogFileNo;
        }

        if ( sEndFileNo < sCurLogFileNo)
        {
            sEndFileNo = sCurLogFileNo;
        }

    };/*End of While*/

    idf::closedir(sDirp);

    IDE_TEST_RAISE(sIsSetTempLogFileName != ID_TRUE, ERR_CANNOT_FIND_ANY_XLOGFILE);

    *aFirstXLogFileNo = sFirstFileNo;
    *aLastXLogFileNo = sEndFileNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_open_dir);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_CannotOpenDir));
    }
    IDE_EXCEPTION(ERR_CANNOT_FIND_ANY_XLOGFILE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_CANNOT_FIND_XLOGFILE, aReplName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdXLogfileMgr::findRemainFirstXLogLSN( SChar     * aReplName,
                                               rpXLogLSN * aFirstXLogLSN )
{
    UInt                sFirstFileNo = 0;
    UInt                sDummyFileNo = 0;
    SChar               sFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    iduFile             sFile;
    SChar *             sXLogDirPath = NULL;
    UInt                sLengthCheck = 0;
    rpdXLogfileHeader   sFileHeader;
    rpXLogLSN           sFirstXLogLSN      = RP_XLOGLSN_INIT;
    idBool              sIsInitializedFile = ID_FALSE;
    idBool              sIsOpenedFile      = ID_FALSE;

    IDE_TEST( findFirstAndLastFileNo( aReplName, &sFirstFileNo, &sDummyFileNo) != IDE_SUCCESS );

    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&sXLogDirPath ) != IDE_SUCCESS );

    IDE_TEST( sFile.initialize( IDU_MEM_OTHER,
                                1, /* Max Open FD Count*/
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sIsInitializedFile = ID_TRUE;

    idlOS::memset( sFilePath, 0, XLOGFILE_PATH_MAX_LENGTH );
    sLengthCheck = idlOS::snprintf( sFilePath,
                                    XLOGFILE_PATH_MAX_LENGTH,
                                    "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                    sXLogDirPath,
                                    IDL_FILE_SEPARATOR,
                                    aReplName,
                                    sFirstFileNo );
    IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

    if ( rpdXLogfileCreater::isFileExist( sFilePath ) != ID_TRUE )
    {
        ideLog::log(IDE_RP_0,"[CHECK] nextfile is not exist" );
    }
    else
    {
        IDE_TEST( sFile.setFileName( sFilePath ) != IDE_SUCCESS );

        /* 파일을 open한다. */
        IDE_TEST( sFile.openUntilSuccess( ID_FALSE, O_RDONLY ) != IDE_SUCCESS );
        sIsOpenedFile = ID_TRUE;

        /* header(file write offset) 만 read을 수행한다. */
        IDE_TEST( sFile.read( NULL,
                              0,
                              &sFileHeader,
                              XLOGFILE_HEADER_SIZE_INIT ) != IDE_SUCCESS );
        IDE_DASSERT( sFileHeader.mFileFirstXLogOffset != RP_XLOGLSN_INIT );
        
        sFirstXLogLSN = RP_SET_XLOGLSN( sFirstFileNo, sFileHeader.mFileFirstXLogOffset );

        sIsOpenedFile = ID_FALSE;
        IDE_TEST( sFile.close() != IDE_SUCCESS );
    }

    sIsInitializedFile = ID_FALSE;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    *aFirstXLogLSN = sFirstXLogLSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_file_path )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_FILE_PATH, sFilePath ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    if ( sIsOpenedFile == ID_TRUE )
    {
        (void)sFile.close();
    }

    if ( sIsInitializedFile == ID_TRUE )
    {
        (void)sFile.destroy();
    }

    return IDE_FAILURE;
}


