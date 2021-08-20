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
 * $Id: smrLogFile.cpp 91389 2021-08-01 23:36:10Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smu.h>
#include <smm.h>
#include <sct.h>
#include <smr.h>
#include <smrReq.h>

#ifdef ALTIBASE_ENABLE_SMARTSSD
#include <sdm_mgnt_public.h>
#endif

#define SMR_LOG_BUFFER_CHUNK_LIST_COUNT (1)
#define SMR_LOG_BUFFER_CHUNK_ITEM_COUNT (5)

iduMemPool smrLogFile::mLogBufferPool;

/*
 * TASK-6198 Samsung Smart SSD GC control
 */
#ifdef ALTIBASE_ENABLE_SMARTSSD
extern sdm_handle_t * gLogSDMHandle;
#endif

smrLogFile::smrLogFile()
{

}

smrLogFile::~smrLogFile()
{

}

IDE_RC smrLogFile::initializeStatic( UInt aLogFileSize )
{
    // 만약 Direct I/O를 한다면 Log Buffer의 시작 주소 또한
    // Direct I/O Page크기에 맞게 Align을 해주어야 한다.
    // 이에 대비하여 로그 버퍼 할당시 Direct I/O Page 크기만큼
    // 더 할당한다.
    if ( smuProperty::getLogIOType() == 1 )
    {
        aLogFileSize += iduProperty::getDirectIOPageSize();
    }

    IDE_TEST( mLogBufferPool.initialize(
                                 IDU_MEM_SM_SMR,
                                 (SChar *)"Log File Buffer Memory Pool",
                                 SMR_LOG_BUFFER_CHUNK_LIST_COUNT, 
                                 aLogFileSize, 
                                 SMR_LOG_BUFFER_CHUNK_ITEM_COUNT,
                                 IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                 ID_TRUE,							/* UseMutex */
                                 IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                 ID_FALSE,							/* ForcePooling */
                                 ID_TRUE,							/* GarbageCollection */
                                 ID_TRUE,                           /* HWCacheLine */
                                 IDU_MEMPOOL_TYPE_LEGACY            /* mempool type */) 
              != IDE_SUCCESS);			

    /* BUG-35392 
     * 압축/비압축 로그에서 아래 멤버는 같은 위치에 있어야 한다. */
    IDE_ASSERT( SMR_LOG_FLAG_OFFSET    == SMR_COMP_LOG_FLAG_OFFSET );
    IDE_ASSERT( SMR_LOG_LOGSIZE_OFFSET == SMR_COMP_LOG_SIZE_OFFSET );
    IDE_ASSERT( SMR_LOG_LSN_OFFSET     == SMR_COMP_LOG_LSN_OFFSET );
    IDE_ASSERT( SMR_LOG_MAGIC_OFFSET   == SMR_COMP_LOG_MAGIC_OFFSET );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogFile::destroyStatic()
{
    IDE_TEST( mLogBufferPool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smrLogFile::initialize()
{
    UInt sMultiplexIdx;

    mEndLogFlush = ID_FALSE;

    mFileNo        = ID_UINT_MAX;
    mFreeSize      = 0;
    mOffset        = 0;
    mBaseAlloced   = NULL;
    mBase          = NULL;
    mRef           = 0;
    mSyncOffset    = 0;
    mPreSyncOffset = 0;
    mSwitch        = ID_FALSE;
    mWrite         = ID_FALSE;
    mMemmap        = ID_FALSE;
    mOnDisk        = ID_FALSE;
    mIsOpened      = ID_FALSE;
    
    mPrvLogFile = NULL;
    mNxtLogFile = NULL;

    for( sMultiplexIdx = 0; 
         sMultiplexIdx < SM_LOG_MULTIPLEX_PATH_MAX_CNT; 
         sMultiplexIdx++ )
    {
        mMultiplexSwitch[ sMultiplexIdx ]  = ID_FALSE;
    }

    IDE_TEST( mFile.initialize( IDU_MEM_SM_SMR,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );

    IDE_TEST( mMutex.initialize( (SChar*)"SMR_LOGFILE_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smrLogFile::destroy()
{
    /* 현재 Open되어 있는 FD의 개수가 0이 아니면 */
    if ( mFile.getCurFDCnt() != 0 )
    {
        IDE_TEST(close() != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST(mFile.destroy() != IDE_SUCCESS);
    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    mFileNo   = ID_UINT_MAX;
    mFreeSize = 0;
    mOffset   = 0;
    mBaseAlloced = NULL;
    mBase     = NULL;
    mRef      = 0;

    mPrvLogFile = NULL;
    mNxtLogFile = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* ------------------------------------------------
 * Description : LogFile을 준비 한다.(BUG-48409)
 *               logfile 생성 중에 서버가 비정상 종료 하면 
 *               생성 중이던 size가 맞지 않는 logfile 이 생긴다.
 *               prepare 중에 임시 파일명으로 생성해서
 *               생성 완료된 경우에 logfile name으로 변경한다.
 *
 * aLogFileName  [IN] - 생성하려는 logfile의 name
 * aTempFileName [IN] - 임시 파일명
 * aInitBuffer   [IN] - logfile 초기화 데이터가 있는 버퍼
 * aSize         [IN] - logFile의 크기
 * ----------------------------------------------*/
IDE_RC smrLogFile::prepare(SChar   * aLogFileName,
                           SChar   * aTempFileName,
                           SChar   * aInitBuffer,
                           SInt      aSize)
{
    UInt sState = 0;

    /* logfile.tmp가 존재 할 경우 먼저 삭제합니다.*/
    if ( idf::access( aTempFileName, F_OK ) == 0 )
    {
        (void)idf::unlink( aTempFileName );
    }
    /* BUG-49202 target LogFile 이 있으면 삭제 합니다. */ 
    if ( idf::access( aLogFileName, F_OK ) == 0 )
    {
        (void)idf::unlink( aLogFileName );
    }

    IDE_TEST( create( aTempFileName,
                      aInitBuffer ,
                      aSize ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( mFile.setFileName( aLogFileName ) != IDE_SUCCESS );

    IDE_TEST( idf::rename( aTempFileName,
                           aLogFileName ) != 0 );
    sState = 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void)idf::unlink( aLogFileName );
            break;
        case 1:
            (void)idf::unlink( aTempFileName );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smrLogFile::create( SChar   * aStrFilename,
                           SChar   * aInitBuffer,
                           UInt      aSize )
{

    SInt       sFilePos;
    ULong      sFileSize;
    SInt       sState = 0;
    UInt       sFileInitBufferSize;
    SChar    * sBufPtr ;
    SChar    * sBufFence ;
    idBool     sUseDirectIO;

    // read properties
    sFileInitBufferSize = smuProperty::getFileInitBufferSize();

    //create log file
    mSize = aSize;

    IDE_TEST(mFile.setFileName(aStrFilename) != IDE_SUCCESS);

    /*---------------------------------------------------------------------- 
     * If disk space is insufficient, wait until disk space is sufficient. 
     * During waiting, User can enlarge the disk space.
     * If SYSTEM SHUTDOWN command is accepted,
     * server is shutdown immediately without trasaction rollback
     * because of insufficient log space for CLR!!  
     *-------------------------------------------------------------------- */
    IDE_TEST( mFile.createUntilSuccess( smLayerCallback::setEmergency,
                                        smrRecoveryMgr::isFinish() )
              != IDE_SUCCESS );
    sState = 1;

    /* BUG-15962: LOG_IO_TYPE이 DirectIO가 아닐때 logfille생성시
     * direct io를 사용하고 있습니다. */
    if ( smuProperty::getLogIOType() == 1 )
    {
        sUseDirectIO = ID_TRUE;
    }
    else
    {
        sUseDirectIO = ID_FALSE;
    }

    IDE_TEST( mFile.open(sUseDirectIO) != IDE_SUCCESS );
    sState = 2;

    if ( smuProperty::getCreateMethod() == SMU_LOG_FILE_CREATE_FALLOCATE )
    {
        IDE_TEST( mFile.fallocate( aSize ) != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( smuProperty::getCreateMethod() == SMU_LOG_FILE_CREATE_WRITE );

        switch( smuProperty::getSyncCreate() )
        {
        case 2:

            /* For BUG-13353 [LFG] SYNC_CREATE_=0일때
             *                     invalid log읽을 확률을 0에 가깝게 낮추어야 함
             *
             * SYNC_CREATE=1 과 동일하지만, 로그파일을 random값으로 채운다.
             * SYNC_CREATE=0 으로 실행했을때 OS가 memset을 해서 올려주는 시스템
             * 에서는 이를 이용하여 SYNC_CREATE=0이 안전한지 시뮬레이션 할 수 있다. */
            {
                idlOS::srand( idlOS::getpid() );

                sBufFence = aInitBuffer + sFileInitBufferSize;

                for ( sBufPtr = aInitBuffer ;
                      sBufPtr < sBufFence   ;
                      sBufPtr ++ )
                {
                    * sBufPtr = (SChar) idlOS::rand();
                }
            }
        case 1:
            /* 로그파일 전체를 aInitBuffer 의 내용으로 초기화한다. */
            {
                sFilePos = 0;

                while ( (ULong)(aSize - sFilePos) > sFileInitBufferSize )
                {
                    IDE_TEST( write( sFilePos, aInitBuffer, sFileInitBufferSize )
                              != IDE_SUCCESS );

                    sFilePos += sFileInitBufferSize;
                }


                if(aSize - sFilePos != 0)
                {
                    IDE_TEST( write( sFilePos, aInitBuffer, ( aSize - sFilePos ) )
                             != IDE_SUCCESS );
                }
            }

            break;

        case 0 :
            {
                /* 로그파일을 통채로 초기화하지 않고
                 * 맨 끝 한바이트만 초기화 하여 파일을 생성한다.
                 *
                 * BMT와 같이 일시적으로 성능을 높여야 하는 경우에만
                 * SyncCreate프로퍼티가 0으로 설정되어 여기로 들어오도록 한다.
                 * 즉, SyncCreate 프로퍼티가 고객에게 나가서는 안된다.
                 *
                 * 이유 1 :  이렇게 하면 대부분의 OS에서는 sSize만큼 File이 extend된다.
                 *           그러나, 어떤 OS는 File의 메타정보만 변경하고
                 *           extend를 안하는 경우도 있다.
                 *
                 * 이유 2 :  로그파일이 초기화가 되지 않아서,
                 *           파일상에 쓰레기 값이 존재하게 되는데,
                 *           그 값이, 운좋게도 로그 헤더의 로그타입과 로그 테일이
                 *           같은 값으로 기록된것처럼 되어서
                 *           리커버리 매니저가 로그레코드가 아닌데도
                 *           로그레코드로 인식할 수가 있다.
                 *           이 경우 리커버리 매니저의 정상 동작을 보장할 수 없다.
                 */

                IDE_TEST( write( aSize - iduProperty::getDirectIOPageSize(),
                                 aInitBuffer,
                                 iduProperty::getDirectIOPageSize() )
                          != IDE_SUCCESS );
            }

            break;

        default:
            break;
        } //switch 
    }// else

    /* BUG-48409 생성한 Logfile의 size를 확인한다. */    
    IDE_TEST( mFile.getFileSize( &sFileSize ) != IDE_SUCCESS );
    IDE_ERROR( sFileSize == (ULong)aSize );

    sState = 1;
    IDE_TEST( mFile.close() != IDE_SUCCESS );


    mFreeSize = aSize;
    mOffset   = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 2 )
    {
        IDE_PUSH();
        IDE_ASSERT( mFile.close() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

IDE_RC smrLogFile::backup( SChar*   aBackupFullFilePath )
{
    iduFile         sDestFile;
    ULong           sLogFileSize;
    idBool          sUseDirectIO;
    ULong           sCurFileSize = 0;
    UInt            sState = 0;

    // read properties
    sLogFileSize = smuProperty::getLogFileSize();

    IDE_TEST( mFile.getFileSize( &sCurFileSize ) != IDE_SUCCESS ); 

    IDE_TEST_RAISE( sCurFileSize != sLogFileSize,
                    error_filesize_wrong);

    IDE_TEST( sDestFile.initialize( IDU_MEM_SM_SMR,
                                    1, /* Max Open FD Count */
                                    IDU_FIO_STAT_OFF,
                                    IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sDestFile.setFileName(aBackupFullFilePath)
              != IDE_SUCCESS );

    IDE_TEST( sDestFile.createUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    /* BUG-15962: LOG_IO_TYPE이 DirectIO가 아닐때 logfille생성시 direct io를
     * 사용하고 있습니다. */
    if ( smuProperty::getLogIOType() == 1 )
    {
        sUseDirectIO = ID_TRUE;
    }
    else
    {
        sUseDirectIO = ID_FALSE;
    }

    IDE_TEST( sDestFile.open(sUseDirectIO) != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sDestFile.write( NULL, /* idvSQL* */
                               0,
                               mBase,
                               sLogFileSize )
              != IDE_SUCCESS);

    sState = 1;
    IDE_TEST( sDestFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sDestFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_filesize_wrong);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_WrongLogFileSize, mFile.getFileName()));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT(sDestFile.close() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(sDestFile.destroy() == IDE_SUCCESS);

            if ( idf::access( aBackupFullFilePath, F_OK ) == 0 )
            {
                (void)idf::unlink(aBackupFullFilePath);
            }
        case 0:
            break;
    }

    return IDE_FAILURE;

}

IDE_RC smrLogFile::mmap( UInt aSize, idBool aWrite )
{
    IDE_TEST( mFile.mmap( NULL /* idvSQL* */,
                          aSize,
                          aWrite,
                          &mBase )
              != IDE_SUCCESS );

    mMemmap = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mMemmap = ID_FALSE;

    return IDE_FAILURE;
}

/*
 * 로그파일을 open한다.
 * aFileNo             [IN] - 로그파일 번호
 *                            로그파일 헤더에 기록된 값과 일치하지 않으면 에러.
 * aStrFilename        [IN] - 로그파일 이름
 * aIsMultiplexLogFile [IN] - 다중화로그파일로 사용하기위해 open하는것인지 여부
 * aSize               [IN] - 로그파일 크기
 * aWrite              [IN] - 쓰기모드인지 여부
 */
IDE_RC smrLogFile::open( UInt       aFileNo,
                         SChar    * aStrFilename,
                         idBool     aIsMultiplexLogFile,
                         UInt       aSize,
                         idBool     aWrite )
{
    ULong       sCurFileSize = 0;
    idBool      sUseDirectIO = ID_FALSE;
    UInt        sState       = 0;

    IDE_ASSERT( mIsOpened == ID_FALSE );

    mWrite              = aWrite;
    mIsMultiplexLogFile = aIsMultiplexLogFile;

    IDE_TEST(mFile.setFileName(aStrFilename) != IDE_SUCCESS);

    // BUG-15065 Direct I/O를 지원하지 않는 시스템에서는
    // Direct I/O를 사용하면 안된다.
    sUseDirectIO = ID_FALSE;

    // Log 기록시 IO타입
    // 0 : buffered IO, 1 : direct IO
    if ( smuProperty::getLogIOType() == 1 )
    {
        /* LOG Buffer Type이 Memory이거나 Log File을 Read를 위해서 Open할 경우는
           IO를 Direct IO로 수행한다. */
        if ( ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY ) || 
             ( aWrite == ID_FALSE ) )
        {
            sUseDirectIO = ID_TRUE;
        }
        else
        {
            /* nothing to do ... */
        }
    }

    if( aWrite == ID_TRUE )
    {
        IDE_TEST( mFile.openUntilSuccess( sUseDirectIO, O_RDWR ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mFile.openUntilSuccess( sUseDirectIO, O_RDONLY ) != IDE_SUCCESS );
    }

    sState = 1;

    if ( aWrite == ID_TRUE )
    {
        IDE_TEST( mFile.getFileSize( &sCurFileSize ) != IDE_SUCCESS );
        IDE_TEST_RAISE( sCurFileSize != aSize, error_filesize_wrong );
    }

    mOnDisk = ID_TRUE;

    mSize = aSize;
    mBase = NULL;

    if ( ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY ) ||
         ( ( aWrite == ID_FALSE ) && 
           ( smuProperty::getLogReadMethodType() == 0 ) ) )
    {
        // mBase에 Direct I/O Page크기로 시작주소가
        // Align되도록 로그버퍼 할당
        IDE_TEST( allocAndAlignLogBuffer() != IDE_SUCCESS );

        if (aWrite == ID_TRUE)
        {
            idlOS::memset(mBase, 0, mSize);
        }
        else
        {
            IDE_TEST(mFile.read( NULL, /* idvSQL* */
                                 0,
                                 mBase,
                                 mSize) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST( mmap( aSize, aWrite ) != IDE_SUCCESS );
        sState = 2;
    }

    if ( aWrite == ID_FALSE ) // 읽기 모드인 경우
    {
        // Read Only이기 때문에 내릴 로그가 없다.
        mEndLogFlush = ID_TRUE;

        /*
         * To Fix BUG-11450  LOG_DIR, ARCHIVE_DIR 의 프로퍼티 내용이 변경되면
         *                   DB가 깨짐
         *
         * 로그파일의 File Begin Log가 정상인지 체크한다.
         */
        IDE_TEST( checkFileBeginLog( aFileNo ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    mIsOpened = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_filesize_wrong );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_WrongLogFileSize, mFile.getFileName() ));
    }
    IDE_EXCEPTION_END;

    /* 
     * BUG-21209 [SM: smrLogFileMgr] logfile open에서 에러발생시 에러처리가
     *           잘 못 되고 있습니다.
     */
    switch ( sState )
    {
        case 2:
            IDE_ASSERT( unmap() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mFile.close() == IDE_SUCCESS );
        default:
            break;
    }

    mBase = NULL;

    return IDE_FAILURE;

}

/*
 * Direct I/O를 사용할 경우 Direct I/O Page크기로 Align된 주소를 mBase에 세팅
 */
IDE_RC smrLogFile::allocAndAlignLogBuffer()
{
    IDE_DASSERT( mBaseAlloced == NULL );
    IDE_DASSERT( mBase == NULL );
    IDE_DASSERT( mMemmap == ID_FALSE );

    /* PROJ-1915 : off-line 로그의 경우
     * 사이즈 를 확인하여 같은 경우 mLogBufferPool을 이용 하고 그렇지 않은 경우 malloc을 하여 사용 한다.
     */
    if ( mSize == smuProperty::getLogFileSize() )
    {
        /* smrLogFile_allocAndAlignLogBuffer_alloc_BaseAlloced.tc */
        IDU_FIT_POINT("smrLogFile::allocAndAlignLogBuffer::alloc::BaseAlloced");
        IDE_TEST(mLogBufferPool.alloc(&mBaseAlloced) != IDE_SUCCESS);
    }
    else 
    {
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SMR,
                                   mSize + iduProperty::getDirectIOPageSize(),
                                   (void **)&mBaseAlloced,
                                   IDU_MEM_FORCE) != IDE_SUCCESS);
    }

    if ( smuProperty::getLogIOType() == 1 )
    {
        // Direct I/O를 사용할 경우
        mBase = idlOS::align( mBaseAlloced,
                              iduProperty::getDirectIOPageSize() );
    }
    else
    {
        // Direct I/O를 사용하지 않을 경우
        mBase = mBaseAlloced;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *  allocAndAlignLogBuffer 로 할당한 로그버퍼를 Free한다.
 */
IDE_RC smrLogFile::freeLogBuffer()
{
    IDE_DASSERT( mBaseAlloced != NULL );
    IDE_DASSERT( mBase != NULL );

    /* PROJ-1915 */
    if ( mSize == smuProperty::getLogFileSize() )
    {
        IDE_TEST(mLogBufferPool.memfree(mBaseAlloced) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::free(mBaseAlloced) != IDE_SUCCESS);
    }
    
    mBaseAlloced = NULL;
    mBase = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 로그파일의 File Begin Log가 정상인지 체크한다.
 *
 * To Fix BUG-11450  LOG_DIR, ARCHIVE_DIR 의 프로퍼티 내용이 변경되면
 *                   DB가 깨짐
 *
 * 로그파일의 맨 처음에 smrFileBeginLog가 Valid할 경우 다음을 체크한다.
 * FileNo 체크 => 로그파일 이름이 rename되었는지 여부를 체크
 *
 * aFileNo      [IN] - 로그파일 번호
 *                     로그파일 헤더에 기록된 값과 일치하지 않으면 에러.
 */
IDE_RC smrLogFile::checkFileBeginLog( UInt aFileNo )
{
    smrFileBeginLog  sFileBeginLog;
    smLSN            sFileBeginLSN;

    idlOS::memcpy( &sFileBeginLog, mBase, SMR_LOGREC_SIZE( smrFileBeginLog ) );

    sFileBeginLSN.mFileNo = aFileNo;
    sFileBeginLSN.mOffset = 0;

    
    if ( isValidLog( & sFileBeginLSN,
                     (smrLogHead * ) & sFileBeginLog,
                     (SChar * )   mBase,
                     // File Begin Log의 경우 압축하지 않는다.
                     // 메모리상의 로그의 크기가 곧
                     // 디스크상의 로그 크기이다.
                     smrLogHeadI::getSize(& sFileBeginLog.mHead ) )
         == ID_TRUE )
    {
        IDE_ASSERT( smrLogHeadI::getType(&sFileBeginLog.mHead) == SMR_LT_FILE_BEGIN );

        IDE_TEST_RAISE( sFileBeginLog.mFileNo  != aFileNo,
                        error_mismatched_fileno );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_mismatched_fileno);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_MISMATCHED_FILENO_IN_LOGFILE,
                                mFile.getFileName(),
                                aFileNo,
                                sFileBeginLog.mFileNo ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description: mmap되었는지 확인후 mapping을 해제한다.
 ***********************************************************************/
IDE_RC smrLogFile::unmap(void)
{
    if ( mBase != NULL )
    {
        if( mMemmap == ID_TRUE )
        {
            IDE_TEST_RAISE(mFile.munmap(mBase, mSize) != 0,
                           err_memory_unmap);
        }
        else
        {
            // allocAndAlignLogBuffer로 할당한 버퍼 해제
            IDE_TEST( freeLogBuffer() != IDE_SUCCESS );
        }

        mBase = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_memory_unmap);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_MunmapFail));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smrLogFile::close()
{
    if ( mIsOpened == ID_TRUE )
    {
        IDE_TEST( unmap() != IDE_SUCCESS );

        if ( mOnDisk == ID_TRUE )
        {
            IDE_TEST( mFile.close() != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
        mIsOpened = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    mFileNo   = ID_UINT_MAX;
    mFreeSize = 0;
    mOffset   = 0;
    mBase     = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogFile::readFromDisk( UInt    aOffset,
                                 SChar * aStrBuffer,
                                 UInt    aSize )
{
    return mFile.read( NULL, /* idvSQL* */
                       aOffset,
                       aStrBuffer,
                       aSize );
}

void smrLogFile::read( UInt      aOffset,
                       SChar   * aStrBuffer,
                       UInt      aSize )
{
    idlOS::memcpy(aStrBuffer, (SChar *)mBase + aOffset, aSize);
    
}

void smrLogFile::read( UInt      aOffset,
                       SChar  ** aLogPtr )
{
    *aLogPtr = (SChar *)mBase + aOffset;
}

IDE_RC smrLogFile::remove( SChar   * aStrFileName,
                           idBool    aIsCheckpoint )
{
    SInt rc;

    //ignore file unlink error during restart recovery
    rc = idf::unlink(aStrFileName);

    // 체크 포인트 도중 로그파일 삭제가 실패하면 에러발생
    /* BUG-42589: 로그 파일 삭제 할 때 체크포인트 상황이 아니어도 (restart recovery)
     * 삭제가 실패하면 트레이스 로그를 남긴다. */
    if ( rc != 0 )
    {
        ideLog::log(IDE_SM_0, " %s Remove Fail (errno=<%u>) \n", aStrFileName, (UInt)errno );

        if ( aIsCheckpoint == ID_TRUE )
        {
            IDE_RAISE(err_file_unlink);
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        // do nothing
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_unlink);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FileDelete, aStrFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void smrLogFile::clear(UInt   aBegin)
{
    if ( aBegin < mSize )
    {
        idlOS::memset( (SChar *)mBase + aBegin, 0, mSize - aBegin );
    }
    else
    {
        /* nothing to do */
    }
}


// 2의 배수의 값을 지니는 Page로 Align을 위한 PAGE_MASK계산
//    
// Page Size       =>       256 ( 0x00000100 ) 일때
// Page Size -1    =>       255 ( 0x000000FF ) 이고
// ~(Page Size -1) => PAGE_MASK ( 0xFFFFFF00 ) 이다
//
// => PAGE_MASK로 Bit And연산하는 것만으로 Align Down이 된다.
//
#define PAGE_MASK(aPageSize)   (~((aPageSize)-1))
// sSystemPageSize 단위로 Align하는 함수 ( ALIGN DOWN )
#define PAGE_ALIGN_DOWN(aToAlign, aPageSize)                     \
           ( (aToAlign              ) & (PAGE_MASK(aPageSize)) ) 
// sSystemPageSize 단위로 Align하는 함수 ( ALIGN UP )
#define PAGE_ALIGN_UP(aToAlign, aPageSize)                       \
           ( (aToAlign + aPageSize-1) & (PAGE_MASK(aPageSize)) )

/*
 * 로그 기록시 Align할 Page의 크기 리턴
 */
UInt smrLogFile::getLogPageSize()
{
    UInt sLogPageSize ;

    if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MMAP )
    {
        // log buffer type이 mmap인 경우,
        // System Page Size로 설정
        sLogPageSize = idlOS::getpagesize();
    }
    else
    {
        // log buffer type이 memroy인 경우,
        // Direct I/O 시 사용되는 page 크기로 설정
        sLogPageSize = iduProperty::getDirectIOPageSize();
    }    
    
    // Align Up/Down을 Bit Mask로 처리하기 위한 체크
    IDE_ASSERT( (sLogPageSize == 512)  || (sLogPageSize == 1024)  ||
                (sLogPageSize == 2048) || (sLogPageSize == 4096)  ||
                (sLogPageSize == 8192) || (sLogPageSize == 16384) ||
                (sLogPageSize == 32768)|| (sLogPageSize == 65536) );

    return sLogPageSize;
}
               

/* ================================================================= *
 * aSyncLastPage : sync 대상이 되는 마지막 페이지를 sync할 것인지의  *
 *                 여부                                              *
 *                 == a_bEnd가 ID_TRUE인 경우 ======                 *
 *             (1) commit 로그 기록시 sync 수행하는 경우             *
 *             (2) full log file sync 시                             *
 *             (3) checkpoint 수행 시                                *
 *             (4) server shutdown 시                                *
 *             (5) 이전 sync시 동일 페이지가 sync되지 않은 경우      *
 *                 (mPreSyncOffset == mSyncOffset)                   *
 *             (6) FOR A4 : 버퍼 매니저가 PAGE를 FLUSH하기 전 해당   *
 *                          PAGE의 변경로그를 반드시 SYNC            *
 * aOffsetToSync : sync하고자 하는 마지막 로그의 offset              *
 * ================================================================= */
IDE_RC smrLogFile::syncLog( idBool   aSyncLastPage,
                            UInt     aOffsetToSync )
{
    UInt   sInvalidSize;
    UInt   sSyncSize;
    UInt   sEndOffset;
    UInt   sLastSyncOffset = mSyncOffset;
    UInt   sSyncBeginOffset;
    SInt   rc;

    // Log Page Size to Align
    static UInt sLogPageSize= 0;
    
    /* DB가 Consistent하지 않으면, Log Sync를 막음 */
    IDE_TEST_CONT( ( smrRecoveryMgr::getConsistency() == ID_FALSE ) &&
                   ( smuProperty::getCrashTolerance() != 2 ),
                   SKIP );

    // Align의 기준이 될 Page 크기 결정
    if ( sLogPageSize == 0 )
    {
        sLogPageSize = getLogPageSize();
    }
    else
    {
        /* nothing to do ... */
    }

    IDE_ASSERT( mOffset >= mSyncOffset );

    /* BUG-35392 */
    /* sync 대상이 되는 마지막 로그 결정 */
    sEndOffset = getLastValidOffset();

    IDE_ASSERT( sEndOffset >= mSyncOffset );

    sInvalidSize = sEndOffset - mSyncOffset;

    if ( sInvalidSize > 0 )
    {
        IDE_ASSERT( sEndOffset >= sLastSyncOffset );

        // sync 대상이 되는 첫번째 페이지 결정
        sSyncBeginOffset = PAGE_ALIGN_DOWN( mSyncOffset, sLogPageSize );
        sSyncSize = sEndOffset - sSyncBeginOffset;

        // sync 대상이 되는 마지막 페이지를 sync할 것인지 여부 결정
        if ( (aOffsetToSync > (sSyncBeginOffset + sSyncSize) ) &&
             (aOffsetToSync != ID_UINT_MAX) )
        {
            aSyncLastPage = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        // 이전에 이 함수가 불렸었지만, 마지막 페이지라서 sync하지 못 한 경우,
        // 이번에는 마지막 페이지라도 sync를 실시 한다.
        if ( mPreSyncOffset == mSyncOffset )
        {
            aSyncLastPage = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        /* 실제 sync할 로그 페이지 갯수 결정한다.
         * sSyncSize에는 페이지 갯수가 아닌, sync될 바이트수가 저장된다. */
        if ( (aSyncLastPage == ID_TRUE) ||
             (smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY) )
        {
            // BUG-14424
            // Log Buffer Type이 memory인 경우에는
            // mmap에서의 LastPage에 sync하기 위한
            // contention이 없으므로, LastPage까지 항상 sync한다.
            sSyncSize = PAGE_ALIGN_UP( sSyncSize, sLogPageSize );

            // 다음번에 sync를 할 offset을 세팅한다.
            mSyncOffset = sEndOffset;
        }
        else
        {
            sSyncSize = PAGE_ALIGN_DOWN( sSyncSize, sLogPageSize );
            // 다음번에 sync를 할 offset을 세팅한다.
            mSyncOffset = sSyncBeginOffset + sSyncSize;
        }

        if ( sSyncSize != 0 )
        {
            if ( mMemmap == ID_TRUE )
            {
                while (1)
                {
                    rc = idlOS::msync( (SChar *)mBase + sSyncBeginOffset,
                                       sSyncSize ,
                                       MS_SYNC);
                    if ( rc == 0 )
                    {
                        break;
                    }
                    else
                    {
                        IDE_TEST_RAISE( ( (errno != EAGAIN) && (errno != ENOMEM) ),
                                        err_memory_sync);
                    }
                }
            }
            else
            {
                IDE_ASSERT( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY );

                // BUG-14424
                // log buffer type이 memory인 경우에는,
                // mmap에서의 LastPage에 sync하기 위한
                // contention이 없으므로, LastPage까지 항상 sync한다.
                if ( smuProperty::getLogIOType() == 0 )
                {
                    // Direct I/O 사용 안함.
                    // Direct I/O를 사용하지 않을 경우
                    // Kernel의 File cache에 대한 memcpy를 줄이기 위해서
                    IDE_TEST( write( sLastSyncOffset,
                                     (SChar *)mBase + sLastSyncOffset,
                                     sEndOffset - sLastSyncOffset )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Direct I/O 사용
                    // Kernel의 File cache를 거치지 않고 Disk로 바로
                    // 기록하도록 OS에게 힌트를 제공한다.
                    // 모든 OS가 Direct I/O시 File Cache를
                    // 거치지 않음을 보장하지 못한다.
                    // Ex> Case-4658에 기술한 Sun의 Direct I/O매뉴얼 참고
                    IDE_ASSERT( smuProperty::getLogIOType() == 1 );

                    IDE_TEST( write( sSyncBeginOffset,
                                     // Offset을 Page경계에 맞춤
                                     (SChar *)mBase + sSyncBeginOffset,
                                     // Page크기 단위로 내림
                                     PAGE_ALIGN_UP( sEndOffset -
                                                    sSyncBeginOffset,
                                                    sLogPageSize ) ) 
                              != IDE_SUCCESS );
                }

                // Direct I/O를 하더라도 Sync를 호출해야함.
                // Direct IO는 기본적으로 Sync가 불필요하나 SM 버그로
                // 인해 다음과 같은 Direct IO조건을 지키지 않을 경우
                //  1. read나 write의 offset, buffer, size가 특정 값으
                //    로 align되어야 한다.
                //  2. 위 특정값은 OS마다 다르다.
                // 어떤 OS는 에러를 내는 경우도 있고 SUN같이 그냥 Buffered
                // IO로 처리하는 경우도 있습니다. 때문에 무조건 Sync합니다.
                IDE_TEST( sync() != IDE_SUCCESS );
            }

            /*
             * TASK-6198 Samsung Smart SSD GC control
             */
#ifdef ALTIBASE_ENABLE_SMARTSSD
            if ( smuProperty::getSmartSSDLogRunGCEnable() != 0 )
            {
                if ( sdm_run_gc( gLogSDMHandle, 
                                 smuProperty::getSmartSSDGCTimeMilliSec() ) != 0 )
                {
                    ideLog::log( IDE_SM_0,
                                 "sdm_run_gc failed" );
                }
            }
#endif
        }
        else // sSyncSize == 0
        {
            // sSyncSize가 한 페이지 안에 들어가는 작은 크기인 경우,
            // aSyncLastPage == ID_FALSE 이면
            // 마지막 페이지를 sync하지 않게 되어
            // sSyncSize가 0보다 큰 값에서 0으로 바뀐 경우이다.
            //
            // 그러나, 이러한 요청, 즉,
            // 작은 크기의 sSyncSize만큼 sync하라는 요청이
            // 다음번에 aSyncLastPage == ID_FALSE 로 한번 더 들어오게 되면,
            // 그때는 aSyncLastPage를 ID_TRUE로 바꾸어서,
            // 마지막 페이지를 sync하도록 해야 한다.
            //
            // 다음번에 이 함수가 불렸을 때 이전에 Sync요청이 들어왔으나
            // 마지막 페이지라서 sync하지 못 한 offset인지 체크하기 위해,

            mPreSyncOffset = mSyncOffset;
        }
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_memory_sync);
    {
        IDE_SET(ideSetErrorCode(smERR_IGNORE_SyncFail));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smrLogFile::write( SInt      aWhere,
                          void    * aBuffer,
                          SInt      aSize )
{

    IDE_ASSERT(mMemmap == ID_FALSE);

    IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                       aWhere,
                                       aBuffer,
                                       aSize,
                                       smLayerCallback::setEmergency,
                                       smrRecoveryMgr::isFinish() )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

/***********************************************************************
 * Description : aStartOffset에서 aEndOffset까지 mBase의 내용을 디스크에 반영시킨다.
 *
 * aStartOffset - [IN] Disk에 기록할 mBase의 시작 Offset
 * aEndOffset   - [IN] Disk에 기록할 mBase의 끝 Offset
 ***********************************************************************/
IDE_RC smrLogFile::syncToDisk( UInt aStartOffset, UInt aEndOffset )
{
    SInt   rc;
    
    UInt   sSyncSize;
    UInt   sSyncBeginOffset;

    /* ===================================
     * Log Page Size to Align
     * =================================== */
    static UInt sLogPageSize  = 0;

    /* DB가 Consistent하지 않으면, Log Sync를 막음 */
    IDE_TEST_CONT( ( smrRecoveryMgr::getConsistency() == ID_FALSE ) &&
                    ( smuProperty::getCrashTolerance() != 2 ),
                    SKIP );

    // Align의 기준이 될 Page 크기 결정
    if ( sLogPageSize == 0 )
    {
        sLogPageSize = getLogPageSize();
    }

    
    IDE_DASSERT( (aStartOffset <= smuProperty::getLogFileSize()) &&
                 (aEndOffset <= smuProperty::getLogFileSize()));

    IDE_ASSERT( aStartOffset <= aEndOffset);

    if ( aStartOffset != aEndOffset )
    {
        sSyncBeginOffset = PAGE_ALIGN_DOWN( aStartOffset, sLogPageSize );

        sSyncSize = aEndOffset - sSyncBeginOffset;

        sSyncSize = PAGE_ALIGN_UP( sSyncSize, sLogPageSize );

        if ( mMemmap == ID_TRUE )
        {
            rc = idlOS::msync( (SChar*)mBase + sSyncBeginOffset,
                               sSyncSize,
                               MS_SYNC );

            IDE_TEST_RAISE( (rc != 0) && (errno != EAGAIN) && (errno != ENOMEM),
                            err_memory_sync);
        }
        else
        {
            // Diect I/O를 사용하지 않는 경우
            if ( smuProperty::getLogIOType() == 0 )
            {
                IDE_TEST( write( aStartOffset,
                                 (SChar*)mBase + aStartOffset,
                                 aEndOffset - aStartOffset )
                          != IDE_SUCCESS );
            }
            else
            {
                // Direct I/O 사용
                IDE_ASSERT( smuProperty::getLogIOType() == 1 );

                IDE_TEST( write( sSyncBeginOffset,
                                 (SChar*)mBase + sSyncBeginOffset,
                                 sSyncSize )
                          != IDE_SUCCESS );
            }

            // Direct I/O는 fsync가 불필요하나
            // 특정 OS의 경우 Direct I/O 요청을 때에 따라서 Buffer IO로
            // 수행하는 경우가 발생하기때문에 예비차원에서 fsync를 수행함(ex: sun)
            IDE_TEST(sync() != IDE_SUCCESS);
        }
    }

    IDE_EXCEPTION_CONT( SKIP );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_memory_sync);
    {
        IDE_SET(ideSetErrorCode(smERR_IGNORE_SyncFail));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 하나의 로그 레코드가 Valid한지 여부를 판별한다.
 *
 * aLSN        - [IN]  Log가 위치한 LSN
 * aLogHeadPtr - [IN]  Log의 헤더 ( Align 된 메모리 )
 * aLogPtr     - [IN] Log의 Log Buffer의 Log Pointer
 * aLogSizeAtDisk - [IN] 로그파일상에 기록된 로그의 크기
 *                       (압축된 로그의 경우 실제로그크기보다 작다)
 **********************************************************************/
idBool smrLogFile::isValidLog( smLSN       * aLSN,
                               smrLogHead  * aLogHeadPtr,
                               SChar       * aLogPtr,
                               UInt          aLogSizeAtDisk )
{
    smrLogType   sLogType;
    idBool       sIsValid = ID_TRUE;
    static UInt  sLogFileSize = smuProperty::getLogFileSize();

    IDE_ASSERT( aLSN->mOffset < sLogFileSize );
    
    /* Fix BUG-4926 */
    if ( ( smrLogHeadI::getSize(aLogHeadPtr) < ( ID_SIZEOF(smrLogHead) + ID_SIZEOF(smrLogTail) ) ) ||
         ( aLogSizeAtDisk > (sLogFileSize - aLSN->mOffset) ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        idlOS::memcpy( (SChar*)&sLogType,
                       aLogPtr + smrLogHeadI::getSize(aLogHeadPtr) - ID_SIZEOF(smrLogTail),
                       ID_SIZEOF(smrLogTail) );

        if (( sLogType == SMR_LT_NULL ) || // BUG-47754
            ( smrLogHeadI::getType(aLogHeadPtr) != sLogType ))
        {
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG1);
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG2,
                        smrLogHeadI::getType(aLogHeadPtr),
                        sLogType);
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG3);

            idlOS::memcpy((SChar*)&sLogType,
                          aLogPtr + smrLogHeadI::getSize(aLogHeadPtr) - ID_SIZEOF(smrLogTail),
                          ID_SIZEOF(smrLogTail));

            if (( sLogType == SMR_LT_NULL ) || // BUG-47754
                ( smrLogHeadI::getType(aLogHeadPtr) != sLogType ))
            {
                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG4);
                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                            SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG5,
                            smrLogHeadI::getType(aLogHeadPtr),
                            sLogType);
                sIsValid = ID_FALSE;
            }
            else
            {
                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                            SM_TRC_MRECOVERY_LOGFILE_INVALID_LOG6);

                sIsValid = isValidMagicNumber( aLSN, aLogHeadPtr );
            }
        }
        else
        {
            // Log를 기록할때 만들었던 MAGIC NUMBER 검사
            sIsValid = isValidMagicNumber( aLSN, aLogHeadPtr );
        }
    }

    return sIsValid ;
}

 /***********************************************************************
 * Description : logfile이 mmap되었을 경우 file cache에 올려놓기 위해 메모리에
 *               logfile데이타를 읽어들인다. 하지만 모든 데이타는 읽을 필요가 없기
 *               때문에 mmap영역의 메모리를 Page Size로 나우어서 각각의 Page 의 첫
 *               Byte만을 읽어들인다.
 **********************************************************************/
#if defined(IA64_HP_HPUX)
SInt smrLogFile::touchMMapArea()
#else
void smrLogFile::touchMMapArea()
#endif
{
    SChar   sData;
    SInt    i;
    SInt    sum = 0;

    static SInt sLoopCnt = smuProperty::getLogFileSize() / 512;

    if ( mMemmap == ID_TRUE )
    {
        for ( i = 0 ; i < sLoopCnt ; i++ )
        {
            sData = *((SChar*)mBase + 512 * i);
            sum += (SInt)sData;
        }
    }
    else
    {
        /* nothing to do */
    }
#if defined(IA64_HP_HPUX)
    /* sum에 대한 연산은 Compiler가 최적화시 이 function을
       dead code로 간주하는 것을 방지하기 위해 추가된 것으로
       아무 의미 없음.*/
    return sum;
#endif
}

/***********************************************************************
 * BUG-35392
 * Description : 마지막으로 Copy된(logfile에 저장된) Log의 마지막 Offset을 받아온다.
 *               현재 File을 넘어서서 다음 Log File을 기록 중이라면
 *               mOffset을 넘긴다.
 **********************************************************************/
UInt  smrLogFile::getLastValidOffset()
{
    smLSN    sLstLSN;
    UInt     sOffset;

    SM_LSN_MAX( sLstLSN );
    sOffset = ID_UINT_MAX;

    smrLogMgr::getLstLogOffset( &sLstLSN );

    if ( sLstLSN.mFileNo == mFileNo )
    {
        /* 
         * BUG-37018 There is some mistake on logfile Offset calculation
         * 다중화 로그파일경우 로그가 기록된 mOffset이 sLstLSN보다 작을 수 있다.
         * 따라서 자기자신의 offset까지만 sync해야한다.
         */
        if ( mIsMultiplexLogFile == ID_TRUE )
        {
            sOffset = mOffset;
        }
        else
        {
            sOffset = sLstLSN.mOffset;
        }  
    }
    else
    {
        if ( sLstLSN.mFileNo > mFileNo )
        {
            /* Copy 완료 LSN이 다음 Log File로 넘어간 경우
             * 현재 log file은 모두 sync 할 수 있다. */
            sOffset = mOffset;
        }
        else
        {
            /* 이전 Log File의 Copy가 아직 완료되지 않은 경우
             * mSyncOffset을 확장하지 않는다. */
            sOffset = mSyncOffset;

            /* 그런데, 이럴수가 있나?
             * 일단 처음 시작하면 발생 할 수 있는 것 같다. */
            IDE_ASSERT( mSyncOffset == 0 );
        }
    }

    return sOffset;
}
