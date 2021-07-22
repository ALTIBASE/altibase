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
 * $Id: rpdXLogfileCreater.cpp
 ******************************************************************************/

#include <mtc.h>
#include <rpdXLogfileCreater.h>
#include <rpdXLogfileMgr.h>
#include <rpxReceiver.h>
#include <rpuProperty.h>

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::initialize             *
 * ------------------------------------------------------------------*
 * xlogfile create thread의 초기화를 위해 호출되는 함수
 * 플레그를 초기값으로 세팅한 후 mutex와 conditional variable을 초기화 한다.
 *
 * aReplName         - [IN] replication의 이름
 * aReadXLogLSN      - [IN] 기존 xlogfile의 시작 위치
 * aXLogfileMgr      - [IN] xlogfile manager
 *********************************************************************/
IDE_RC rpdXLogfileCreater::initialize( SChar *          aReplName,
                                       rpXLogLSN        aReadXLogLSN,
                                       rpdXLogfileMgr * aXLogfileMgr,
                                       void *           aReceiverPtr )
{
    SChar           sBuffer[IDU_MUTEX_NAME_LEN] = {0,};
    UInt            sState = 0;
    UInt            sFileNo = 0;
    SInt            sLengthCheck = 0;

    IDE_DASSERT( aReceiverPtr != NULL );

    /* 변수 초기화 */
    mXLogfileMgr = aXLogfileMgr;
    mReceiverPtr = aReceiverPtr;

    /* 파일 생성 관련 변수 */
    mLastCreatedFileNo = ID_UINT_MAX;
    mPrepareXLogfileCnt = 0;
    idlOS::strncpy( mRepName, aReplName, QCI_MAX_NAME_LEN + 1 );
    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&mXLogDirPath ) != IDE_SUCCESS );

    /* 파일 삭제 관련 변수 */
    mFileCreateCount = 0;

    /* thread 관련 변수 */
    mExitFlag = ID_FALSE;
    mEndFlag = ID_FALSE;
    mIsThreadSleep = ID_FALSE;

    /* mutex 및 CV 초기화 */
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "PREOPENFILE_COUNT_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );

    IDE_TEST( mFileCntMutex.initialize( sBuffer,
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "CREATER_CV_%s",
                                    aReplName );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );

    IDE_TEST( mThreadWaitMutex.initialize( sBuffer,
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mThreadWaitCV.initialize() != IDE_SUCCESS );
    sState = 3;

    if( aReadXLogLSN == RP_XLOGLSN_INIT )
    {
        /* 기존 meta data가 없을 경우 새 파일을 만들어 사용해야 한다. */
        IDE_TEST( createXLogfile( 0 ) != IDE_SUCCESS );

        mOldestExistXLogfileNo = 0;

        /* read 중인 파일을 새로 생성한 파일로 세팅한다. */
        mXLogfileMgr->mCurReadXLogfile = mXLogfileMgr->mXLogfileListTail;
    }
    else
    {
        /* 기존 파일이 있을 경우 가장 오래된 xlogfile 번호를 세팅한다. */
        RP_GET_FILENO_FROM_XLOGLSN( sFileNo, aReadXLogLSN );

        IDE_TEST( getOldestXLogfileNo( sFileNo ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_mutex_name )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_TOO_LONG_MUTEX_NAME, sBuffer ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 3:
            (void)mThreadWaitCV.destroy();
        case 2:
            (void)mThreadWaitMutex.destroy();
        case 1:
            (void)mFileCntMutex.destroy();
        case 0:
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::finalize               *
 * ------------------------------------------------------------------*
 * xlogfile create thread를 정지시키는 함수
 *********************************************************************/
void rpdXLogfileCreater::finalize()
{
    idBool              sIsLock = ID_FALSE;
    rpdXLogfileHeader * sHeader = NULL;

    IDE_ASSERT( mXLogfileMgr->mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );
    sIsLock = ID_TRUE;

    /* flusher의 정리가 creater보다 먼저 이루어지므로 flush 대상은 파일은 이미 내려가있다. *
     * 남은 파일은 read중인 파일과 생성만 하고 사용하지 않은 파일뿐이므로                  *
     * 여기서는 생성만 하고 사용하지 않은 파일만 찾아 닫도록 한다.                         */
    while( mXLogfileMgr->mXLogfileListTail != NULL )
    {
        sHeader = (rpdXLogfileHeader*)mXLogfileMgr->mXLogfileListTail->mBase;

        /* 해당 파일의 write offset이 초기값이라면 해당 파일은 create만 되고 사용되지 않았다. */
        if( sHeader->mFileWriteOffset == XLOGFILE_HEADER_SIZE_INIT )
        {
            IDE_DASSERT( mXLogfileMgr->mXLogfileListTail->mFileRefCnt == 1 );

            sIsLock = ID_FALSE;
            IDE_ASSERT( mXLogfileMgr->mXLogfileListMutex.unlock() == IDE_SUCCESS );

            mXLogfileMgr->decFileRefCnt( mXLogfileMgr->mXLogfileListTail );

            sIsLock = ID_TRUE;
            IDE_ASSERT( mXLogfileMgr->mXLogfileListMutex.lock( NULL ) == IDE_SUCCESS );
        }
        else
        {
            sIsLock = ID_FALSE;
            IDE_ASSERT( mXLogfileMgr->mXLogfileListMutex.unlock() == IDE_SUCCESS );

            break;
        }
    }

    if( sIsLock == ID_TRUE )
    {
        IDE_ASSERT( mXLogfileMgr->mXLogfileListMutex.unlock() == IDE_SUCCESS );
    }

    mEndFlag = ID_TRUE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::destroy                *
 * ------------------------------------------------------------------*
 * xlogfile create thread를 정리하는 함수
 *********************************************************************/
void rpdXLogfileCreater::destroy()
{
    /* flush thread에서 사용하는 CV와 mutex를 정리한다. */
    if( mThreadWaitCV.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if( mThreadWaitMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if( mFileCntMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::run                    *
 * ------------------------------------------------------------------*
 * xlogfile create thread가 실제 작업을 수행하는 함수
 *********************************************************************/
void rpdXLogfileCreater::run()
{
    UInt            sPrepareFileCnt = 0;
    time_t          sStartTime;
    time_t          sNowTime;
    //test
    UInt            sFileCntCheck = RPU_REPLICATION_XLOGFILE_PREPARE_COUNT;

    sStartTime = idlOS::time();

    while( mExitFlag == ID_FALSE )
    {
        if( mFileCreateCount > RPU_REPLICATION_XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE )
        {
            /* XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE 프로퍼티 값 이상의 파일을 생성했을 경우  *
             * 대기시간과 관계없이 불필요한 파일을 삭제하도록 한다.                             */

            IDE_TEST( checkAndRemoveOldXLogfiles() != IDE_SUCCESS );

            sStartTime = idlOS::time();
            mFileCreateCount = 0;
        }

        getPrepareFileCnt( &sPrepareFileCnt );

        if( sPrepareFileCnt < sFileCntCheck )
        {
            IDE_TEST( createXLogfiles() != IDE_SUCCESS );
        }
        else
        {
            sNowTime = idlOS::time();

            if( ( sNowTime - sStartTime ) > RPU_REPLICATION_XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE )
            {
                /* 파일을 제거한다. */
                IDE_TEST( checkAndRemoveOldXLogfiles() != IDE_SUCCESS );

                sStartTime = idlOS::time();
                mFileCreateCount = 0;
            }
            else
            {
                /* 새 파일을 만들 필요가 없을 경우 잠시 대기한다. */
                (void)sleepCreater();
            }
        }
    }

    return;

    IDE_EXCEPTION_END;

    /* 예외 발생시 finalize만 수행하고 destroy는 수행하지 않도록 하여           * 
     * 다른 thread에서 접근시 creater thread가 정지되었다는 것을 알리도록 한다. */
    finalize();

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::createXLogfiles        *
 * ------------------------------------------------------------------*
 * 다수의 xlogfile을 생성한다.
 *********************************************************************/
IDE_RC rpdXLogfileCreater::createXLogfiles()
{
    UInt            sNewFileNo = 0;
    UInt            sPrepareFileCnt = 0;

    IDE_DASSERT( mLastCreatedFileNo != ID_UINT_MAX );

    getPrepareFileCnt( &sPrepareFileCnt );

    while( sPrepareFileCnt < RPU_REPLICATION_XLOGFILE_PREPARE_COUNT )
    {
        sNewFileNo = mLastCreatedFileNo + 1;

        /* 새 파일을 생성한다. */
        IDE_TEST( createXLogfile( sNewFileNo ) != IDE_SUCCESS );

        /* xlogfile count를 증가시킨다. */
        incPrepareFileCnt();

        /* write(transfer)에게 signal을 보내 대기중일 경우 깨운다. */
        mXLogfileMgr->wakeupWriter();

        sPrepareFileCnt++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::incPrepareFileCnt      *
 * ------------------------------------------------------------------*
 * xlogfile creater의 현재 준비된 xlogfile count를 증가시킨다.
 * 해당 함수는 파일 생성시에만 호출된다.
 *********************************************************************/
void rpdXLogfileCreater::incPrepareFileCnt()
{
    IDE_ASSERT( mFileCntMutex.lock( NULL ) == IDE_SUCCESS );
    mPrepareXLogfileCnt++;
    IDE_ASSERT( mFileCntMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::decPrepareFileCnt      *
 * ------------------------------------------------------------------*
 * xlogfile creater의 현재 준비된 xlogfile count를 감소시킨다.
 * 해당 함수는 write(transfer)에서 새 파일을 열었을 때 호출된다.
 *********************************************************************/
void rpdXLogfileCreater::decPrepareFileCnt()
{
    IDE_ASSERT( mFileCntMutex.lock( NULL ) == IDE_SUCCESS );
    mPrepareXLogfileCnt--;
    IDE_ASSERT( mFileCntMutex.unlock() == IDE_SUCCESS );

    return;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::getPrepareFileCnt      *
 * ------------------------------------------------------------------*
 * xlogfile creater에서 미리 준비해둔 xlogfile의 수를 가져온다.
 *********************************************************************/
void rpdXLogfileCreater::getPrepareFileCnt( UInt *  aResult )
{
    IDE_ASSERT( aResult != NULL );

    IDE_ASSERT( mFileCntMutex.lock( NULL ) == IDE_SUCCESS );
    *aResult = mPrepareXLogfileCnt;
    IDE_ASSERT( mFileCntMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileFlusher::wakeupFileCreater      *
 * ------------------------------------------------------------------*
 * xlogfile을 생성하는 thread를 깨우는 작업을 수행하는 함수
 *********************************************************************/
void rpdXLogfileCreater::wakeupCreater()
{
    IDE_ASSERT( mThreadWaitMutex.lock( NULL ) == IDE_SUCCESS );

    IDU_FIT_POINT( "rpdXLogfileCreater::wakeupCreater" );

    if( mIsThreadSleep == ID_TRUE )
    {
        IDE_ASSERT( mThreadWaitCV.signal() == IDE_SUCCESS );
    }

    IDE_ASSERT( mThreadWaitMutex.unlock() == IDE_SUCCESS );

    IDE_EXCEPTION_END;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::sleepCreater           *
 * ------------------------------------------------------------------*
 * 충분한 pre pen file이 준비되었을 때 임시로 짧은 시간 create thread를 재운다.
 * 대기중인 thread는 따로  signal을 보내 깨우지 않아도 1초마다 자동적으로 일어난다.
 *********************************************************************/
void rpdXLogfileCreater::sleepCreater()
{
    PDL_Time_Value    sCheckTime;
    PDL_Time_Value    sSleepSec;

    sCheckTime.initialize();
    sSleepSec.initialize();

    sSleepSec.set( 1 );

    IDE_ASSERT( mThreadWaitMutex.lock( NULL ) == IDE_SUCCESS );
    sCheckTime = idlOS::gettimeofday() + sSleepSec;

    mIsThreadSleep = ID_TRUE;
    (void)mThreadWaitCV.timedwait( &mThreadWaitMutex, &sCheckTime );
    mIsThreadSleep = ID_FALSE;

    IDE_ASSERT( mThreadWaitMutex.unlock() == IDE_SUCCESS );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::
 * ------------------------------------------------------------------*
 * Active 에서 반영되어서 제거해도 되는 xlogfile no와 (sRemoteCheckpointedFileNo)
 * Standby 에서 반영되어서 제거해도 되는 xlogfile no (sUsingFileNo) 을 비교해서
 * 더 작은 파일 번호까지 xlogfile을 삭제한다.
 *********************************************************************/
IDE_RC rpdXLogfileCreater::checkAndRemoveOldXLogfiles( )
{
    rpxReceiver *   sReceiver = (rpxReceiver*)mReceiverPtr;
    
    UInt            sUsingFileNo = 0;
    UInt            sRemoteCheckpointedFileNo = 0;
    UInt            sAbleToRemoveFileNo = 0;

    IDE_DASSERT( sReceiver != NULL );

    IDE_TEST( sReceiver->findXLogfileNoByRemoteCheckpointSN( &sRemoteCheckpointedFileNo ) 
              != IDE_SUCCESS );
    sUsingFileNo = sReceiver->getMinimumUsingXLogFileNo();

    if ( sRemoteCheckpointedFileNo > sUsingFileNo )
    {
        sAbleToRemoveFileNo = sUsingFileNo;
    }
    else
    {
        sAbleToRemoveFileNo = sRemoteCheckpointedFileNo;
    }
    
    if ( sAbleToRemoveFileNo > mOldestExistXLogfileNo )
    {
        IDE_TEST( removeOldXLogfiles( sAbleToRemoveFileNo ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::removeOldXLogfiles         *
 * ------------------------------------------------------------------*
 * 불필요한 xlogfile을 모두 제거한다.
 * 인자로 받은 파일 번호보다 오래된 xlogfile을 모두 제거한다.
 * 해당 파일은 제거하지 않는다.
 * 만약, 인자로 받은 파일 번호보다 read 중인 파일 번호가 작을 경우
 * (read가 더 앞에 있을 경우)에는 read 중인 파일 바로 전까지만 제거한다.
 *
 * aUsingXLogfileNo - [IN] 유지해야 할 가장 오래된 xlogfile의 번호
 *********************************************************************/
IDE_RC rpdXLogfileCreater::removeOldXLogfiles( UInt aUsingXLogfileNo )
{
    SChar   sFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    UInt    i;
    UInt    sFileNo;
    SInt    sLengthCheck = 0;

    IDE_DASSERT( mOldestExistXLogfileNo <= aUsingXLogfileNo );

    if( mXLogfileMgr->mXLogfileListHeader != NULL )
    {
        sFileNo = mXLogfileMgr->mXLogfileListHeader->mFileNo;
    }
    else
    {
        sFileNo = ID_UINT_MAX;
    }

    for( i = mOldestExistXLogfileNo; i < aUsingXLogfileNo; i++ )
    {
        if( sFileNo <= i )
        {
            /* 현재 메모리에 올라와 있는 파일은 지우면 안된다. */
            break;
        }

        sLengthCheck = idlOS::snprintf( sFilePath,
                                        XLOGFILE_PATH_MAX_LENGTH,
                                        "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                        mXLogDirPath,
                                        IDL_FILE_SEPARATOR,
                                        mRepName,
                                        i );
        IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

        if( removeXLogfile ( sFilePath ) != IDE_SUCCESS )
        {
            IDE_ERRLOG(IDE_RP_0);
        }

        mOldestExistXLogfileNo = i + 1;
    }

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
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::removeXLogfile         *
 * ------------------------------------------------------------------*
 * 입력받은 path의 xlogfile을 제거한다.
 *
 * aFilePath - [IN] 제거할 xlogfile의 path
 *********************************************************************/
IDE_RC rpdXLogfileCreater::removeXLogfile( SChar * aFilePath )
{
    SInt    rc;

    if( isFileExist( aFilePath ) == ID_FALSE )
    {
        /* 제거하려는 파일이 이미 없음 */
        ideLog::log( IDE_RP_0, "%s file is not exist.", aFilePath );
    }
    else
    {
        rc = idf::unlink( aFilePath );

        IDE_TEST_RAISE( rc != 0, unlink_fail );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( unlink_fail )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_FAILURE_UNLINK_FILE, aFilePath, rc ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::getOldestXLogfileNo    *
 * ------------------------------------------------------------------*
 * 가장 오래된 xlogfile의 번호를 찾는다.
 * 여기서 찾은 번호는 오래된 xlogfile 정리시 시작 위치가 된다.
 * 오래된 xlogfile 정리시 매번 호출될 경우 부담이 너무 크므로 initialize시에만 한번 호출된다.
 *********************************************************************/
IDE_RC rpdXLogfileCreater::getOldestXLogfileNo( UInt aFileNo )
{
    UInt    sFileNo = 0;
    SChar   sFilePath[XLOGFILE_PATH_MAX_LENGTH];
    UInt    i = 0;
    SInt    sLengthCheck = 0;

    for( i = aFileNo; i > 0; i-- )
    {
        idlOS::memset( sFilePath, 0 , XLOGFILE_PATH_MAX_LENGTH );

        sLengthCheck = idlOS::snprintf( sFilePath,
                                        XLOGFILE_PATH_MAX_LENGTH,
                                        "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                        mXLogDirPath,
                                        IDL_FILE_SEPARATOR,
                                        mRepName,
                                        i );
        IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

        if( isFileExist( sFilePath ) == ID_FALSE )
        {
            sFileNo = i + 1;

            break;
        }
    }

    mOldestExistXLogfileNo = sFileNo;

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
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::createXLogfile         *
 * ------------------------------------------------------------------*
 * 새 xlogfile을 생성한 후 open 및 mapping을 수행한다.
 * 생성되는 파일은 xlogfile[replication이름]_[번호]의 형태를 가진다.
 *
 * aFileNo      - [IN] 생성할 파일의 번호
 *********************************************************************/
IDE_RC rpdXLogfileCreater::createXLogfile( UInt aFileNo )
{
    rpdXLogfile *       sNewFile;
    SChar               sNewFilePath[XLOGFILE_PATH_MAX_LENGTH] = {0,};
    SChar               sBuffer[IDU_MUTEX_NAME_LEN];
    SInt                sLengthCheck = 0;
    rpdXLogfileHeader * sFileHeader = NULL;

    /* 새 파일의 path를 설정한다. */
    idlOS::memset( sNewFilePath, 0, XLOGFILE_PATH_MAX_LENGTH );
    sLengthCheck = idlOS::snprintf( sNewFilePath,
                                    XLOGFILE_PATH_MAX_LENGTH,
                                    "%s%cxlogfile%s_%"ID_UINT32_FMT,
                                    mXLogDirPath,
                                    IDL_FILE_SEPARATOR,
                                    mRepName,
                                    aFileNo );
    IDE_TEST_RAISE( sLengthCheck == XLOGFILE_PATH_MAX_LENGTH, too_long_file_path );

    /* 새 파일을 생성한다. */
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                 ID_SIZEOF( rpdXLogfile ),
                                 (void**)&(sNewFile),
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );

    IDE_TEST( sNewFile->mFile.initialize( IDU_MEM_RP_RPD,
                                          1, /* Max Open FD Count*/
                                          IDU_FIO_STAT_OFF,
                                          IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST( sNewFile->mFile.setFileName( sNewFilePath ) != IDE_SUCCESS );

    if( isFileExist( sNewFilePath ) == ID_FALSE )
    {
        IDE_TEST( sNewFile->mFile.createUntilSuccess( rpdXLogfileMgr::rpdXLogfileEmergency ) != IDE_SUCCESS );
    }
    else
    {
        /* 생성하려는 파일이 이미 있는 경우에는 해당 파일을 만들 필요가 없다. */
    }

    /* mapping 할 수 있도록 크기를 세팅한다. */
    IDE_TEST( sNewFile->mFile.open( ID_FALSE ) != IDE_SUCCESS );
    IDE_TEST( sNewFile->mFile.truncate( RPU_REPLICATION_XLOGFILE_SIZE ) != IDE_SUCCESS );
    IDE_TEST( sNewFile->mFile.close() != IDE_SUCCESS );

    /* xlogfile meta(rpdXLogfile)관리용 mutex를 초기화 한다. */
    idlOS::memset( sBuffer, 0, IDU_MUTEX_NAME_LEN );
    sLengthCheck = idlOS::snprintf( sBuffer,
                                    IDU_MUTEX_NAME_LEN,
                                    "xlogfile%s_%"ID_UINT32_FMT,
                                    mRepName,
                                    aFileNo );
    IDE_TEST_RAISE( sLengthCheck == IDU_MUTEX_NAME_LEN, too_long_mutex_name );

    IDE_TEST( sNewFile->mRefCntMutex.initialize( sBuffer,
                                                 IDU_MUTEX_KIND_POSIX,
                                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    /* 생성한 파일을 open 한다. */
    IDE_TEST( sNewFile->mFile.openUntilSuccess( ID_FALSE, O_RDWR ) != IDE_SUCCESS );

    /* mapping을 수행한다. */
    IDE_TEST( sNewFile->mFile.mmap( NULL,
                                    RPU_REPLICATION_XLOGFILE_SIZE,
                                    ID_TRUE,
                                    &( sNewFile->mBase ) ) != IDE_SUCCESS );

    /* 메타정보를 갱신한다. */
    mXLogfileMgr->setFileMeta( sNewFile, aFileNo );

    mLastCreatedFileNo = aFileNo;

    /* 새로 연 파일의 file header를 초기화한다. */
    sFileHeader = (rpdXLogfileHeader*)(sNewFile->mBase);
    sFileHeader->mFileWriteOffset = XLOGFILE_HEADER_SIZE_INIT;
    sFileHeader->mFileFirstXLogOffset = XLOGFILE_HEADER_SIZE_INIT;
    sFileHeader->mFileFirstXSN = SM_SN_NULL;

    /* sync를 수행한다. */
    sNewFile->mFile.syncUntilSuccess( rpdXLogfileMgr::rpdXLogfileEmergency );

    /* xlogfile list에 추가한다. */
    mXLogfileMgr->addToFileListinLast( sNewFile );

    mFileCreateCount++;

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


/*********************************************************************
 * FUNCTION DESCRIPTION : rpdXLogfileCreater::isFileExist            *
 * ------------------------------------------------------------------*
 * 입력받은 path에 해당하는 파일이 이미 존재하는지 확인한다.
 * 주의 : 해당 함수는 static 함수이다.
 *
 * aFilePath - [IN] 제거할 xlogfile의 path
 *********************************************************************/
idBool rpdXLogfileCreater::isFileExist( SChar * aFilePath )
{
    idBool  sResult = ID_FALSE;

    if( idf::access( aFilePath, F_OK ) == 0 )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}
