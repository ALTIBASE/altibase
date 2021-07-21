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
 * $$Id: smrLogMgr.cpp 91226 2021-07-14 08:01:24Z lswhh $
 *
 * 로그관리자 구현파일입니다.
 *
 * 로그는 한 방향으로 계속 자라나는, Durable한 저장공간이다.
 * 하지만 Durable한 매체로 보편적으로 사용하는 Disk의 경우,
 * 그 용량이 한정되어 있어서, 로그를 무한정 자라나게 허용할 수는 없다.
 *
 * 그래서 여러개의 물리적인 로그파일들을 논리적으로 하나의 로그로
 * 사용할 수 있도록 구현을 하는데,
 * 이 때 필요한 물리적인 로그파일을 smrLogFile 로 표현한다.
 * 여러개의 로그파일들의 리스트를 관리하는 역할을 smrLogFileMgr이 담당하고,
 * 로그파일의 Durable한 속성을 충족시키는 역할을 smrLFThread가 담당한다.
 * 여러개의 물리적인 로그파일들을
 * 하나의 논리적인 로그로 추상화 하는 역할을 smrLogMgr가 담당한다.
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smu.h>
#include <smr.h>
#include <smrReq.h>
#include <sct.h>
#include <smxTrans.h>

iduMutex smrLogMgr::mMtxLoggingMode;
UInt     smrLogMgr::mMaxLogOffset;

smrLFThread                smrLogMgr::mLFThread;
smrArchThread              smrLogMgr::mArchiveThread;
const SChar*               smrLogMgr::mLogPath ;
const SChar*               smrLogMgr::mArchivePath ;
iduMutex                   smrLogMgr::mMutex;
smrLogFile*                smrLogMgr::mCurLogFile;
smrLSN4Union               smrLogMgr::mLstLSN;
smrLSN4Union               smrLogMgr::mLstWriteLSN;
smrLogFileMgr              smrLogMgr::mLogFileMgr;
smrFileBeginLog            smrLogMgr::mFileBeginLog;
smrFileEndLog              smrLogMgr::mFileEndLog;
UInt                       smrLogMgr::mUpdateTxCount;
UInt                       smrLogMgr::mLogSwitchCount;
iduMutex                   smrLogMgr::mLogSwitchCountMutex;
idBool                     smrLogMgr::mAvailable;
/* BUG-35392 */
iduMutex                   smrLogMgr::mMutex4NullTrans;
smrUCSNChkThread           smrLogMgr::mUCSNChkThread;
smrUncompletedLogInfo      smrLogMgr::mUncompletedLSN;
smrUncompletedLogInfo    * smrLogMgr::mFstChkLSNArr;
UInt                       smrLogMgr::mFstChkLSNArrSize;
UInt                     * smrLogMgr::mFstChkLSNUpdateCnt;

smrCompResPool             smrLogMgr::mCompResPool;

/***********************************************************************
 * Description : 로깅모듈 초기화
 **********************************************************************/
IDE_RC smrLogMgr::initialize()
{
    UInt        sState  = 0;
    UInt        i;
    UInt        sFstChkLSNUpdateCntSize;
    

    mLogPath        = smuProperty::getLogDirPath();
    mArchivePath    = smuProperty::getArchiveDirPath();
    mLogSwitchCount = 0;

    IDE_TEST( checkLogDirExist() != IDE_SUCCESS );
 
    IDE_TEST( mLogSwitchCountMutex.initialize((SChar*)"LOG_SWITCH_COUNT_MUTEX",
                                              IDU_MUTEX_KIND_NATIVE,
                                              IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );

    // static멤버들 초기화
    IDE_TEST( initializeStatic() != IDE_SUCCESS );

    // 현재 로그파일 초기화
    mCurLogFile = NULL;

    mUpdateTxCount = 0;
    
    // 마지막 LSN초기화
    SM_LSN_INIT( mLstWriteLSN.mLSN );
    // 마지막 LSN초기화
    SM_LSN_INIT( mLstLSN.mLSN );

    // 현재 로그파일의 동시성 제어를 위한 Mutex초기화
    IDE_TEST( mMutex.initialize((SChar*)"LOG_ALLOCATION_MUTEX",
                                (iduMutexKind)smuProperty::getLogAllocMutexType(),
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    /* BUG-35392 */
    IDE_TEST( mMutex4NullTrans.initialize((SChar *)"LOG_FILE_GROUP_MUTEX_FOR_NULL_TRANS",
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    sState = 2;

    /* BUG-35392 */
    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        /* Null Transaction용 1 추가 */
        mFstChkLSNArrSize = smLayerCallback::getCurTransCnt() + 1; 

        /* smrLogMgr_initialize_malloc_mFstChkLSNArr.tc */
        IDU_FIT_POINT("smrLogMgr::initialize::malloc::mFstChkLSNArr");
        IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SMR,
                                   (ULong)ID_SIZEOF( smrUncompletedLogInfo ) * mFstChkLSNArrSize,
                                   (void **)&mFstChkLSNArr) != IDE_SUCCESS );
        sState = 3;

        for ( i = 0 ; i < mFstChkLSNArrSize ; i++)
        {
            SM_LSN_MAX( mFstChkLSNArr[i].mLstLSN.mLSN );
            SM_LSN_MAX( mFstChkLSNArr[i].mLstWriteLSN.mLSN );
        }
    }    

    // 로그파일의 맨 처음에 기록할 FileBeginLog를 초기화
    initializeFileBeginLog ( &mFileBeginLog );
    // 로그파일의 맨 끝에 기록할 FileEndLog를 초기화
    initializeFileEndLog   ( &mFileEndLog );

    /******* 로그 파일 매니저가 지닌 쓰레드들을 시작  ********/

    // 로그파일 관리자 초기화 및 Prepare 쓰레드 시작
    // 주의! initialize함수가 불리어도 쓰레드를 시작시키지 않는다.
    IDE_TEST( mLogFileMgr.initialize( mLogPath,
                                      mArchivePath, 
                                      &mLFThread )
              != IDE_SUCCESS );
    sState = 4;

    // 아카이브 쓰레드 객체를 항상 초기화한다.
    // ( mLFThread가 아카이브 쓰레드를 참조하기 때문 )
    // 로그파일 아카이브 쓰레드 객체 초기화
    // 주의! 아카이브 쓰레드는 initialize함수가 불리어도
    // 쓰레드를 시작시키지 않는다.
    //
    // smiMain.cpp에서 쓰레드 자동시작 프로퍼티인
    // smuProperty::getArchiveThreadAutoStart() 이 설정된 경우에만
    // 별도로 아카이브 쓰레드를 startup시킨다.
    IDE_TEST( mArchiveThread.initialize( mArchivePath,
                                         &mLogFileMgr,
                                         smrRecoveryMgr::getLstDeleteLogFileNo() )
              != IDE_SUCCESS );
    sState = 5;

    /* BUG-35392 */
    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        sFstChkLSNUpdateCntSize = ( mFstChkLSNArrSize / SMR_CHECK_LSN_UPDATE_GROUP ) + 1; 

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                     (ULong)ID_SIZEOF( UInt ) * sFstChkLSNUpdateCntSize,
                                     (void **)&mFstChkLSNUpdateCnt ) != IDE_SUCCESS );
        sState = 6;
        
        idlOS::memset( mFstChkLSNUpdateCnt, 0x00, ID_SIZEOF( UInt ) * sFstChkLSNUpdateCntSize );
    }
    else
    {
        /* nothing to do */
    }

    // 로그파일 Flush 쓰레드 초기화후 시작
    ideLog::log(IDE_SERVER_0," [SM-PREPARE] Log Flush Thread Startup");
    IDE_TEST( mLFThread.initialize( &mLogFileMgr,
                                    &mArchiveThread )
              != IDE_SUCCESS );

    /* BUG-35392 */
    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        /* Uncompleted LSN를 갱신하는 Thread를 Init & Start 한다. */
        IDE_TEST( mUCSNChkThread.initialize() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    mAvailable = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 6:
            if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
            {
                IDE_ASSERT( iduMemMgr::free(mFstChkLSNUpdateCnt) == IDE_SUCCESS ); 
            }
        case 5:
            IDE_ASSERT( mArchiveThread.destroy() == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( mLogFileMgr.destroy() == IDE_SUCCESS );
        case 3:
            /* BUG-35392 */
            if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
            {
                IDE_ASSERT( iduMemMgr::free(mFstChkLSNArr) == IDE_SUCCESS );
            }
        case 2:
            IDE_ASSERT( mMutex4NullTrans.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로깅모듈 해제
 **********************************************************************/
IDE_RC smrLogMgr::destroy()
{
    mAvailable = ID_FALSE;

    /* BUG-35392 */
    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        /* UncompletedLSN을 구하는 Thread Stop & Destroy */
        IDE_TEST( mUCSNChkThread.destroy() != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free(mFstChkLSNUpdateCnt) != IDE_SUCCESS ); 
    }
    else
    {
        /* nothing to do ... */
    }

    // To Fix BUG-14185
    // initialize에서 아카이브 모드와 상관없이
    // 아카이브 쓰레드가 항상 초기화 되어있기 때문에
    // 항상 destroy 해주어야 함
    IDE_TEST( mArchiveThread.destroy() != IDE_SUCCESS );

    // 로그파일 Flush 쓰레드 해제
    IDE_TEST( mLFThread.destroy() != IDE_SUCCESS );

    // 로그파일 관리자 겸 Prepare 쓰레드 해제
    IDE_TEST( mLogFileMgr.destroy() != IDE_SUCCESS );

    // 현재 로그파일의 동시성 제어를 위한 Mutex 해제
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    /* BUG-35392 */
    IDE_TEST( mMutex4NullTrans.destroy() != IDE_SUCCESS );

    /* BUG-35392 */
    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        IDE_TEST( iduMemMgr::free( mFstChkLSNArr ) != IDE_SUCCESS );
    }

    IDE_TEST( destroyStatic() != IDE_SUCCESS );

    IDE_TEST( mLogSwitchCountMutex.destroy() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smrLogMgr::initializeStatic()
{
    const  SChar* sLogDirPtr;
    // 로그 디렉토리가 있는지 확인  
    sLogDirPtr = smuProperty::getLogDirPath();
        
    IDE_TEST_RAISE( idf::access(sLogDirPtr, F_OK) != 0,
                    err_logdir_not_exist )
    
    /* ------------------------------------------------
     * 로그관리자의 mutex 초기화
     * - 로깅 모드 mutex 초기화
     * ----------------------------------------------*/
    IDE_TEST( mMtxLoggingMode.initialize((SChar*)"LOG_MODE_MUTEX",
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    IDE_TEST( smuDynArray::initializeStatic(SMU_DA_BASE_SIZE)
             != IDE_SUCCESS );

    // BUG-29329 에서 Codding Convention문제로
    // static 지역 변수에서 static 멤버변수로 수정
    mMaxLogOffset = smuProperty::getLogFileSize() - ID_SIZEOF(smrLogHead)
                    - ID_SIZEOF(smrLogTail);


    /* BUG-31114 mismatch between real log type and display log type
     *           in dumplf.
     * LogType검증을 위해, 한번 초기화 해본다. 어차피 확보된 메모리 이기
     * 때문에, 초기화 해준다고 해서 추가적인 메모리를 먹진 않는다. */

    IDE_TEST( smrLogFileDump::initializeStatic() != IDE_SUCCESS );

    IDE_TEST( smrLogFile::initializeStatic( (UInt)smuProperty::getLogFileSize())
              != IDE_SUCCESS );

    IDE_TEST( mCompResPool.initialize(
                                  (SChar*)"NONTRANS_LOG_COMP_RESOURCE_POOL",
                                  1,   // aInitialResourceCount
                                  1,   // aMinimumResourceCount
                                  smuProperty::getCompressionResourceGCSecond() )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION(err_logdir_not_exist);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, sLogDirPtr));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogMgr::destroyStatic()
{
    IDE_TEST( mCompResPool.destroy() != IDE_SUCCESS );

    // Do nothing
    IDE_TEST( smrLogFile::destroyStatic() != IDE_SUCCESS );

    // Do nothing
    IDE_TEST( smrLogFileDump::destroyStatic() != IDE_SUCCESS );

    IDE_TEST( smuDynArray::destroyStatic() != IDE_SUCCESS );

    IDE_TEST( mMtxLoggingMode.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :  이 함수는 createdb시에 부르게 된다.
 **********************************************************************/
IDE_RC smrLogMgr::create()
{
    IDE_TEST( checkLogDirExist() != IDE_SUCCESS );
    
    IDE_TEST( smrLogFileMgr::create( smuProperty::getLogDirPath() ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그 파일 매니저가 지닌 쓰레드들을 중지
 *
 * smrLogMgr::initialize에서 해준 것과 반대의 순서로 중지한다.
 **********************************************************************/
IDE_RC smrLogMgr::shutdown()
{
    if ( mLFThread.isStarted() == ID_TRUE )
    {
        // 로그파일 Flush 쓰레드 중지

        ideLog::log(IDE_SERVER_0,"      [SM-PREPARE] Log Flush Thread Shutdown...");
        {
            IDE_TEST( mLFThread.shutdown() != IDE_SUCCESS );
        }
        ideLog::log(IDE_SERVER_0,"[SUCCESS]\n");
    }
    else
    {
        /* nothing to do */
    }

    // 로그파일 관리자 겸 Prepare 쓰레드 중지

    if ( mLogFileMgr.isStarted() == ID_TRUE )
    {
        ideLog::log(IDE_SERVER_0,"      [SM-PREPARE] Log Prepare Thread Shutdown...");
        {
            IDE_TEST( mLogFileMgr.shutdown() != IDE_SUCCESS );
        }
        ideLog::log(IDE_SERVER_0,"[SUCCESS]\n");
    }
    else
    {
        /* nothing to do */
    }

    // 아카이브 쓰레드 중지
    if ( smrRecoveryMgr::getArchiveMode() == SMI_LOG_ARCHIVE )
    {
        if ( mArchiveThread.isStarted() == ID_TRUE )
        {

            ideLog::log(IDE_SERVER_0,"      [SM-PREPARE] Log Archive Thread Shutdown...");
            {
                IDE_TEST( mArchiveThread.shutdown() != IDE_SUCCESS );
            }
            ideLog::log(IDE_SERVER_0,"[SUCCESS]\n");

            // 아카이브를 마지막으로 한번 더 실시하여
            // 모든 아카이브가 끝나도록 한다.
            IDE_TEST( mArchiveThread.archLogFile() != IDE_SUCCESS );
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Check Log DIR Exist
 *
 ***********************************************************************/
IDE_RC smrLogMgr::checkLogDirExist()
{
    const  SChar* sLogDirPtr;
    
    sLogDirPtr = smuProperty::getLogDirPath();
        
    IDE_TEST_RAISE( idf::access(sLogDirPtr, F_OK) != 0,
                    err_logdir_not_exist )
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_logdir_not_exist);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, sLogDirPtr));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/********************************************************************
 * aLSN이 가리키는 Log를 읽어서 Log가 위치한 Log Buffer의
 * 포인터를 aLogPtr에 Setting한다. 그리고 Log를 가지고 있는
 * Log파일포인터를 aLogFile에 Setting한다.
 *
 * [IN]  aDecompBufferHandle - 로그 압축해제에 사용할 버퍼의 핸들
 * [IN]  aLSN        - 읽어들일 Log Record위치
 * [IN]  aIsRecovery - Recovery시에 호출되었으면
 *                     ID_TRUE, 아니면 ID_FALSE
 * [INOUT] aLogFile  - 현재 Log레코드를 가지고 있는 LogFile
 * [OUT] aLogHeadPtr - Log의 Header를 복사
 * [OUT] aLogPtr     - Log Record가 위치한 Log버퍼의 Pointer
 * [OUT] aIsValid    - Log가 Valid하면 ID_TRUE, 아니면 ID_FALSE
 * [OUT] aLogSizeAtDisk     - 파일상에서 읽어낸 로그의 크기
 *                      ( 압축된 로그의 경우 로그의 크기와
 *                        파일상의 크기가 다를 수 있다 )
 *******************************************************************/
IDE_RC smrLogMgr::readLog( iduMemoryHandle  * aDecompBufferHandle,
                           smLSN            * aLSN,
                           idBool             aIsCloseLogFile,
                           smrLogFile      ** aLogFile,
                           smrLogHead       * aLogHeadPtr,
                           SChar           ** aLogPtr,
                           idBool           * aIsValid,
                           UInt             * aLogSizeAtDisk)
{
    UInt      sOffset   = 0;

    // 비압축 로그를 읽는 경우 aDecompBufferHandle이 NULL로 들어온다

    IDE_ASSERT( aLSN        != NULL );
    IDE_ASSERT( (aIsCloseLogFile == ID_TRUE) ||
                (aIsCloseLogFile == ID_FALSE) );
    IDE_ASSERT( aLogFile    != NULL );
    IDE_ASSERT( aLogPtr     != NULL );
    IDE_ASSERT( aLogHeadPtr != NULL );
    IDE_ASSERT( aLogSizeAtDisk != NULL );
     
    // BUG-29329 로 인한 디버깅 코드 추가
    // aLSN->mOffset이 sMaxLogOffset보다 크다고 판단하여 Fatal발생.
    // 하지만, FATAL err Msg에 기록된 Offset은 정상이었음.
    // 처리 도중에 값이 변경되는지를 확인합니다.
    sOffset = aLSN->mOffset;

    // BUG-20062
    IDE_TEST_RAISE(aLSN->mOffset > getMaxLogOffset(), error_invalid_lsn_offset);

    IDE_TEST( readLogInternal( aDecompBufferHandle,
                               aLSN,
                               aIsCloseLogFile,
                               aLogFile,
                               aLogHeadPtr,
                               aLogPtr,
                               aLogSizeAtDisk )
              != IDE_SUCCESS );

    if ( aIsValid != NULL )
    {
        /* BUG-35392 */
        if ( smrLogHeadI::isDummyLog( aLogHeadPtr ) == ID_FALSE )
        {
            *aIsValid = smrLogFile::isValidLog( aLSN,
                                                aLogHeadPtr,
                                                *aLogPtr,
                                                *aLogSizeAtDisk );
        }
        else
        {
            /* BUG-37018 There is some mistake on logfile Offset calculation 
             * Dummy log가 valid한지 검사하는 방법은 MagicNumber검사밖에 없다.*/
            *aIsValid = smrLogFile::isValidMagicNumber( aLSN, aLogHeadPtr );
        }
    }
    else
    {
        /* aIsValid가 NULL이면 Valid를 Check하지 않는다 */
    }

    // BUG-29329 로 인한 디버깅 코드 추가
    IDE_TEST_RAISE( ( smuProperty::getLogFileSize() 
                      - ID_SIZEOF(smrLogHead)
                      - ID_SIZEOF(smrLogTail) ) 
                    != getMaxLogOffset(),
                    error_invalid_lsn_offset);

    IDE_TEST_RAISE(sOffset != aLSN->mOffset, error_invalid_lsn_offset);

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_invalid_lsn_offset);
    {
        // BUG-29329 로 인한 디버깅 코드 추가
        ideLog::log(IDE_SERVER_0,
                    SM_TRC_DRECOVER_INVALID_LOG_OFFSET,
                    sOffset,
                    getMaxLogOffset(),
                    smuProperty::getLogFileSize());

        IDE_SET( ideSetErrorCode( smERR_FATAL_InvalidLSNOffset,
                                  aLSN->mFileNo,
                                  aLSN->mOffset) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogMgr::readLog4RP( smLSN            * aLSN,
                              idBool             aIsCloseLogFile,
                              smrLogFile      ** aLogFile,
                              smrLogHead       * aLogHeadPtr,
                              SChar           ** aLogPtr,
                              idBool           * aIsValid,
                              UInt             * aLogSizeAtDisk )
{
    UInt         sOffset      = 0;
    static UInt  sLogFileSize = smuProperty::getLogFileSize();


    IDE_ASSERT( aLSN        != NULL );
    IDE_ASSERT( (aIsCloseLogFile == ID_TRUE) ||
                (aIsCloseLogFile == ID_FALSE) );
    IDE_ASSERT( aLogFile    != NULL );
    IDE_ASSERT( aLogPtr     != NULL );
    IDE_ASSERT( aLogHeadPtr != NULL );
    IDE_ASSERT( aLogSizeAtDisk != NULL );
     
    // BUG-29329 로 인한 디버깅 코드 추가
    // aLSN->mOffset이 sMaxLogOffset보다 크다고 판단하여 Fatal발생.
    // 하지만, FATAL err Msg에 기록된 Offset은 정상이었음.
    // 처리 도중에 값이 변경되는지를 확인합니다.
    sOffset = aLSN->mOffset;

    // BUG-20062
    IDE_TEST_RAISE(aLSN->mOffset > getMaxLogOffset(), error_invalid_lsn_offset);

    IDE_TEST( readLogInternal4RP( aLSN,
                                  aIsCloseLogFile,
                                  aLogFile,
                                  aLogHeadPtr,
                                  aLogPtr,
                                  aLogSizeAtDisk )
              != IDE_SUCCESS );

    if ( aIsValid != NULL )
    {
        // Log의 압축을 풀지 않았기 때문에 Size와 MagicNumber 만으로 검증해야 한다.
        if ( ( smrLogHeadI::getSize(aLogHeadPtr) < ( ID_SIZEOF(smrLogHead) + ID_SIZEOF(smrLogTail) ) ) ||
             ( *aLogSizeAtDisk > (sLogFileSize - aLSN->mOffset) ) )
        {
            *aIsValid = ID_FALSE;
        }

        /* BUG-37018 There is some mistake on logfile Offset calculation 
         * Dummy log가 valid한지 검사하는 방법은 MagicNumber검사밖에 없다.*/
        *aIsValid = smrLogFile::isValidMagicNumber( aLSN, aLogHeadPtr );
    }
    else
    {
        /* aIsValid가 NULL이면 Valid를 Check하지 않는다 */
    }

    // BUG-29329 로 인한 디버깅 코드 추가
    IDE_TEST_RAISE( ( smuProperty::getLogFileSize() 
                      - ID_SIZEOF(smrLogHead)
                      - ID_SIZEOF(smrLogTail) ) 
                    != getMaxLogOffset(),
                    error_invalid_lsn_offset);

    IDE_TEST_RAISE(sOffset != aLSN->mOffset, error_invalid_lsn_offset);

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_invalid_lsn_offset);
    {
        // BUG-29329 로 인한 디버깅 코드 추가
        ideLog::log(IDE_SERVER_0,
                    SM_TRC_DRECOVER_INVALID_LOG_OFFSET,
                    sOffset,
                    getMaxLogOffset(),
                    smuProperty::getLogFileSize());

        IDE_SET( ideSetErrorCode( smERR_FATAL_InvalidLSNOffset,
                                  aLSN->mFileNo,
                                  aLSN->mOffset) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogMgr::readLogInternal4RP( smLSN            * aLSN,
                                      idBool             aIsCloseLogFile,
                                      smrLogFile      ** aLogFile,
                                      smrLogHead       * aLogHead,
                                      SChar           ** aLogPtr,
                                      UInt             * aLogSizeAtDisk )
{
    IDE_DASSERT( aLSN     != NULL );
    IDE_DASSERT( aLogFile != NULL );
    IDE_DASSERT( aLogHead != NULL );
    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aLogSizeAtDisk != NULL );
    
    smrLogFile    * sLogFilePtr;

    sLogFilePtr = *aLogFile;
   
    if ( sLogFilePtr != NULL )
    {
        if ( (aLSN->mFileNo != sLogFilePtr->getFileNo()) )
        {
            if ( aIsCloseLogFile == ID_TRUE )
            {
                IDE_TEST( closeLogFile(sLogFilePtr) != IDE_SUCCESS );
                *aLogFile = NULL; 
            }
            else
            {
                /* nothing to do ... */
            }

            /* FIT/ART/rp/Bugs/BUG-17185/BUG-17185.tc */
            IDU_FIT_POINT( "1.BUG-17185@smrLFGMgr::readLog" );

            IDE_TEST( openLogFile( aLSN->mFileNo,
                                   ID_FALSE,
                                   &sLogFilePtr )
                      != IDE_SUCCESS );
            *aLogFile = sLogFilePtr;
        }
        else
        {
            /* aLSN이 가리키는 로그는 *aLogFile에 있다.*/
        }
    }
    else
    {
        IDE_TEST( openLogFile( aLSN->mFileNo,
                               ID_FALSE,
                               &sLogFilePtr )
                  != IDE_SUCCESS );
        *aLogFile = sLogFilePtr;
    }

    IDE_TEST( smrLogComp::readLog4RP( sLogFilePtr,
                                      aLSN->mOffset,
                                      aLogHead,
                                      aLogPtr,
                                      aLogSizeAtDisk )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : disk 로그를 로그버퍼에 기록한다.
 *
 * mtx의 commit과정에서 NTA가 설정되지 않은 경우에 호출되는
 * 함수로써, SMR_DLT_REDOONLY이나 SMR_DLT_UNDOABLE 타입의
 * 로그를 기록한다.
 *
 * 로그 header에 설정하기 위한 멤버 aIsDML, aReplicate, aUndable
 * 들의 상태에 따른 로그 헤더에 설정될 값이 다르다.
 * 또, 트랜잭션의 logFlag에 따라서도 다르다.
 * replication이 동작할 때, isDML이 true인 경우에 N에 대해서는
 * replicator가 판독해야 할 로그를 나타낸다.
 *
 * - 로그헤더에 설정될 값 (N:NORMAL, R:REPL_NONE, C:REPL_RECOVERY)
 *   ___________________________________________________
 *             | insert   | delete   | update  | etc.
 *   __________|__________|__________|_________|_______
 *   mIsDML    | T        | T        | T       |   F
 *             |          |          |         |
 *   mReplicate| T   F    | T   F    | T   F   |  (F)
 *             |          |          |         |
 *   mUndoable | T F T F  | T F T F  | T F T F |  (F)
 *  ___________|__________|__________|_________|_______
 *  N-Trans경우  R N R R    R N R R    N N R R     R
 *  C-Trans경우  R C R R    R C R R    C C R R     R
 *  R-Trans경우  R R R R    R R R R    R R R R     R
 *
 * - 2nd. code design
 *   + 트랜잭션 로그 버퍼 초기화
 *   + 로그 header 설정
 *   + 설정된 로그 header를 트랜잭션 로그버퍼에 기록
 *   + mtx 로그 버퍼에 저장된 로그를 트랜잭션 로그버퍼에 기록
 *   + 로그의 tail을 설정한다.
 *   + 마지막으로 트랜잭션 로그 버퍼에 작성된 로그를 모두
 *     로그파일에 기록한다.
 **********************************************************************/
IDE_RC smrLogMgr::writeDiskLogRec( idvSQL           * aStatistics,
                                   void             * aTrans,
                                   smLSN            * aPrvLSN,
                                   smuDynArrayBase  * aLogBuffer,
                                   UInt               aWriteOption,
                                   UInt               aLogAttr,
                                   UInt               aContType,
                                   UInt               aRefOffset,
                                   smOID              aTableOID,
                                   UInt               aRedoType,
                                   smLSN            * aBeginLSN,
                                   smLSN            * aEndLSN )
{

    UInt         sLength;
    UInt         sLogTypeFlag;
    smTID        sTransID;
    smrDiskLog   sDiskLog;
    smrLogType   sLogType;
    void       * sTableHeader;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aBeginLSN != NULL );
    IDE_DASSERT( aEndLSN != NULL );

    idlOS::memset(&sDiskLog, 0x00, SMR_LOGREC_SIZE(smrDiskLog));

    smLayerCallback::initLogBuffer( aTrans );

    sLogType =
        ((aLogAttr & SM_DLOG_ATTR_LOGTYPE_MASK) == SM_DLOG_ATTR_UNDOABLE) ?
        SMR_DLT_UNDOABLE : SMR_DLT_REDOONLY;

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    smrLogHeadI::setType(&sDiskLog.mHead, sLogType);
    sLength  = smuDynArray::getSize(aLogBuffer);

    smrLogHeadI::setSize(&sDiskLog.mHead,
                         SMR_LOGREC_SIZE(smrDiskLog) +
                         sLength + ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sDiskLog.mHead, sTransID);

    if ((aLogAttr & SM_DLOG_ATTR_DML_MASK) == SM_DLOG_ATTR_DML)
    {
        //PROJ-1608 recovery From Replication
        //REPL_RECOVERY는 Recovery Sender가 봐야한다.
        if ( ( (aLogAttr & SM_DLOG_ATTR_TRANS_MASK) == SM_DLOG_ATTR_REPLICATE) &&
            ( (sLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
              (sLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY) ) )
        {
            smrLogHeadI::setFlag( &sDiskLog.mHead, sLogTypeFlag );

            /* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
             * Partial Rollback을 지원해야 합니다. */
            if ( smLayerCallback::checkAndSetImplSVPStmtDepth4Repl( aTrans )
                 == ID_FALSE )
            {
                smrLogHeadI::setFlag(&sDiskLog.mHead,
                                     smrLogHeadI::getFlag(&sDiskLog.mHead) | SMR_LOG_SAVEPOINT_OK);

                smrLogHeadI::setReplStmtDepth( &sDiskLog.mHead,
                                               smLayerCallback::getLstReplStmtDepth( aTrans ) );
            }

            // To Fix BUG-12512
            if ( smLayerCallback::isPsmSvpReserved( aTrans ) == ID_TRUE )
            {
                IDE_TEST( smLayerCallback::writePsmSvp( aTrans ) != IDE_SUCCESS );
            }
            else
            {
                // do nothing
            }
        }
        else
        {
            smrLogHeadI::setFlag(&sDiskLog.mHead, SMR_LOG_TYPE_REPLICATED);
        }
    }
    else
    {
        /* ------------------------------------------------
         * DML과 관련없는 로그일경우에, SMR_LOG_TYPE_NORMAL을 설정할 경우에
         * replacator가 판독할 우려가 있다. 왜냐하면, 로그 헤더만 봐서는 어떤
         * disk 로그 타입인지를 판단하지 못하기 때문이다.
         * 그렇기 때문에 N 에 대하여 R로 표기하여 etc의
         * disk 로그의 경우는 모두 판독하지 못하도록 만든다.
         * 결정: (N->R??) 아니면 로그 플래그 타입을 확장하는 방법을 고려해볼
         * 필요가 있다.
         * ----------------------------------------------*/
        smrLogHeadI::setFlag(&sDiskLog.mHead, sLogTypeFlag);
    }

    /* TASK-5030 
     * 로그 헤드에 FXLog 플래그를 설정 한다 */
    IDE_TEST( smLayerCallback::getTableHeaderFromOID( aTableOID, (void**)(&sTableHeader) )
              != IDE_SUCCESS );

    if ( smcTable::isSupplementalTable((smcTableHeader *)sTableHeader) == ID_TRUE )
    {
        smrLogHeadI::setFlag( &sDiskLog.mHead,
                              smrLogHeadI::getFlag(&sDiskLog.mHead) | SMR_LOG_FULL_XLOG_OK);
    }

    IDE_TEST( decideLogComp( aWriteOption, &sDiskLog.mHead )
              != IDE_SUCCESS );

    sDiskLog.mTableOID    = aTableOID;
    sDiskLog.mContType    = aContType;
    sDiskLog.mRedoLogSize = sLength;
    sDiskLog.mRefOffset   = aRefOffset;
    sDiskLog.mRedoType    = aRedoType;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sDiskLog,
                                   SMR_LOGREC_SIZE(smrDiskLog))
              != IDE_SUCCESS );

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBufferUsingDynArr( aLogBuffer,
                                                                sLength )
              != IDE_SUCCESS );

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sLogType,
                                                     ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        aPrvLSN,  // Previous LSN Ptr
                        aBeginLSN,// Log LSN Ptr 
                        aEndLSN,  // End LSN Ptr
                        sDiskLog.mTableOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : disk NTA 로그(SMR_DLT_NTA)를 로그버퍼에 기록한다.
 *
 * disk NTA 로그는 MRDB와 DRDB에 걸친 연산에 대한 actomic 연산을
 * 보장하기 위해 기록하는 로그이다. table segment 생성이나, index
 * segment 생성등에 사용된다.
 * mtx의 commit과정에서 NTA가 설정되어 있는 경우에 호출되는
 * 함수로써, SMR_DLT_NTA타입의 로그를 기록한다.
 *
 * - 2nd. code design
 *   + 트랜잭션 로그 버퍼 초기화
 *   + 로그 header 설정
 *   + 설정된 로그 header를 트랜잭션 로그버퍼에 기록
 *   + mtx 로그 버퍼에 저장된 로그를 트랜잭션 로그버퍼에 기록
 *   + 로그의 tail을 설정한다.
 *   + 마지막으로 트랜잭션 로그 버퍼에 작성된 로그를 모두
 *     로그파일에 기록한다.
 **********************************************************************/
IDE_RC smrLogMgr::writeDiskNTALogRec( idvSQL          * aStatistics,
                                      void            * aTrans,
                                      smuDynArrayBase * aLogBuffer,
                                      UInt              aWriteOption,
                                      UInt              aOPType,
                                      smLSN           * aPPrevLSN,
                                      scSpaceID         aSpaceID,
                                      ULong           * aArrData,
                                      UInt              aDataCount,
                                      smLSN           * aBeginLSN,
                                      smLSN           * aEndLSN,
                                      smOID             aTableOID )
{

    UInt           sLength;
    UInt           sLogTypeFlag;
    smTID          sTransID;
    smrDiskNTALog  sNTALog;
    smrLogType     sLogType;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aBeginLSN != NULL );
    IDE_DASSERT( aEndLSN != NULL ); 
    IDE_ASSERT(  aDataCount < SM_DISK_NTALOG_DATA_COUNT );/*BUG-27516 Klocwork*/

    idlOS::memset(&sNTALog, 0x00, SMR_LOGREC_SIZE(smrDiskNTALog));

    smLayerCallback::initLogBuffer( aTrans );

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    sLogType = SMR_DLT_NTA;

    smrLogHeadI::setType(&sNTALog.mHead, sLogType);
    sLength  = smuDynArray::getSize(aLogBuffer);

    smrLogHeadI::setSize(&sNTALog.mHead,
                         SMR_LOGREC_SIZE(smrDiskNTALog) +
                         sLength + ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sNTALog.mHead, sTransID);

    /* ------------------------------------------------
     * DML과 관련없는 로그이지만, SMR_LOG_TYPE_NORMAL을 설정할 경우에
     * replacator가 판독할 우려가 있다.
     * 결정: (N->R??) 아니면 로그 플래그 타입을 확장하는 방법을 고려해볼
     * 필요가 있다.
     * ----------------------------------------------*/
    smrLogHeadI::setFlag(&sNTALog.mHead, sLogTypeFlag);

    if ( (smrLogHeadI::getFlag(&sNTALog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sNTALog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sNTALog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    IDE_TEST( decideLogComp( aWriteOption, &sNTALog.mHead )
              != IDE_SUCCESS );

    sNTALog.mOPType     = aOPType;  // 연산 타입을 설정한다.
    sNTALog.mSpaceID    = aSpaceID;
    sNTALog.mDataCount  = aDataCount;

    idlOS::memcpy( sNTALog.mData,
                   aArrData,
                   (size_t)ID_SIZEOF(ULong)*aDataCount );

    sLength = smuDynArray::getSize(aLogBuffer);
    sNTALog.mRedoLogSize = sLength;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sNTALog,
                                   SMR_LOGREC_SIZE(smrDiskNTALog))
              != IDE_SUCCESS );

    if ( sLength != 0 )
    {
        IDE_TEST( ((smxTrans*)aTrans)->writeLogToBufferUsingDynArr(
                                       aLogBuffer,
                                       sLength)
                != IDE_SUCCESS );
    }

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        aPPrevLSN,
                        aBeginLSN,
                        aEndLSN,
                        aTableOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : Referenced NTA 로그(SMR_DLT_REF_NTA)를 로그버퍼에
 * 기록한다.
 *
 **********************************************************************/
IDE_RC smrLogMgr::writeDiskRefNTALogRec( idvSQL          * aStatistics,
                                         void            * aTrans,
                                         smuDynArrayBase * aLogBuffer,
                                         UInt              aWriteOption,
                                         UInt              aOPType,
                                         UInt              aRefOffset,
                                         smLSN           * aPPrevLSN,
                                         scSpaceID         aSpaceID,
                                         smLSN           * aBeginLSN,
                                         smLSN           * aEndLSN,
                                         smOID             aTableOID )
{

    UInt              sLength;
    UInt              sLogTypeFlag;
    smTID             sTransID;
    smrDiskRefNTALog  sNTALog;
    smrLogType        sLogType;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aBeginLSN  != NULL );
    IDE_DASSERT( aEndLSN    != NULL );

    idlOS::memset(&sNTALog, 0x00, SMR_LOGREC_SIZE(smrDiskRefNTALog));

    smLayerCallback::initLogBuffer( aTrans );

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    sLogType = SMR_DLT_REF_NTA;
    smrLogHeadI::setType(&sNTALog.mHead, sLogType);
    sLength  = smuDynArray::getSize(aLogBuffer);

    smrLogHeadI::setSize(&sNTALog.mHead,
                         SMR_LOGREC_SIZE(smrDiskRefNTALog) +
                         sLength + ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sNTALog.mHead, sTransID);

    /* ------------------------------------------------
     * DML과 관련없는 로그이지만, SMR_LOG_TYPE_NORMAL을 설정할 경우에
     * replacator가 판독할 우려가 있다.
     * 결정: (N->R??) 아니면 로그 플래그 타입을 확장하는 방법을 고려해볼
     * 필요가 있다.
     * ----------------------------------------------*/
    smrLogHeadI::setFlag(&sNTALog.mHead, sLogTypeFlag);

    if ( (smrLogHeadI::getFlag(&sNTALog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sNTALog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sNTALog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    IDE_TEST( decideLogComp( aWriteOption, &sNTALog.mHead )
              != IDE_SUCCESS );

    sNTALog.mOPType      = aOPType;  // 연산 타입을 설정한다.
    sNTALog.mSpaceID     = aSpaceID;
    sNTALog.mRefOffset   = aRefOffset;
    sNTALog.mRedoLogSize = sLength;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( 
                             &sNTALog,
                             SMR_LOGREC_SIZE(smrDiskRefNTALog))
              != IDE_SUCCESS );

    if ( sLength != 0 )
    {
        IDE_TEST( ((smxTrans*)aTrans)->writeLogToBufferUsingDynArr(
                                      aLogBuffer,
                                      sLength )
                != IDE_SUCCESS );
    }

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        aPPrevLSN,
                        aBeginLSN,
                        aEndLSN,
                        aTableOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
  PRJ-1548 User Memory Tablespace
  테이블스페이스 UPDATE 연산 대한 CLR 로그를 로그버퍼에 기록
*/
IDE_RC smrLogMgr::writeCMPSLogRec4TBSUpt( idvSQL*        aStatistics,
                                          void*          aTrans,
                                          smLSN*         aPrvUndoLSN,
                                          smrTBSUptLog*  aUpdateLog,
                                          SChar*         aBeforeImage )
{
    smrCMPSLog     sCSLog;
    smTID          sSMTID;
    UInt           sLogFlag;
    smrLogType     sLogType;

    IDE_DASSERT(aPrvUndoLSN != NULL);
    IDE_DASSERT(aUpdateLog != NULL);
    IDE_DASSERT(aBeforeImage != NULL);

    idlOS::memset(&sCSLog, 0x00, SMR_LOGREC_SIZE(smrDiskCMPSLog));
    smLayerCallback::initLogBuffer( aTrans );

    sLogType = SMR_LT_COMPENSATION;

    smrLogHeadI::setType(&sCSLog.mHead, sLogType);

    //get tx id and logtype flag from a transaction
    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sSMTID,
                                       &sLogFlag );

    smrLogHeadI::setTransID(&sCSLog.mHead, sSMTID);

    smrLogHeadI::setFlag(&sCSLog.mHead, sLogFlag);

    if ( (smrLogHeadI::getFlag(&sCSLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sCSLog.mHead, 
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sCSLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    smrLogHeadI::setSize (&sCSLog.mHead,
                          SMR_LOGREC_SIZE(smrCMPSLog) +
                          aUpdateLog->mBImgSize +
                          ID_SIZEOF(smrLogTail) );

    SC_MAKE_GRID(sCSLog.mGRID, aUpdateLog->mSpaceID, 0, 0);

    sCSLog.mFileID      = aUpdateLog->mFileID;
    sCSLog.mTBSUptType  = aUpdateLog->mTBSUptType;
    sCSLog.mBImgSize    = aUpdateLog->mBImgSize;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sCSLog,
                                   SMR_LOGREC_SIZE(smrCMPSLog))
              != IDE_SUCCESS );

    if (aUpdateLog->mBImgSize > 0)
    {
        IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                       aBeforeImage,
                                       aUpdateLog->mBImgSize)
                  != IDE_SUCCESS );
    }

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        aPrvUndoLSN,
                        NULL,  // Log LSN Ptr
                        NULL,  // End LSN Ptr
                        SM_NULL_OID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : disk CLR 로그(SMR_DLT_COMPENSATION)를 로그버퍼에 기록한다.
 * 1. 트랜잭션 로그 버퍼 초기화
 * 2. 로그의 header 설정
 * 3. disk logs들의 총 길이를 기록한다.
 * 4. 설정된 로그 header를 트랜잭션 로그버퍼에 기록
 * 5. mtx 로그버퍼에 저장된 disk log 들을 기록한다.
 * 6. 로그의 tail을 설정한다.
 **********************************************************************/
IDE_RC smrLogMgr::writeDiskCMPSLogRec( idvSQL          * aStatistics,
                                       void            * aTrans,
                                       smuDynArrayBase * aLogBuffer,
                                       UInt              aWriteOption,
                                       smLSN           * aPrevLSN,
                                       smLSN           * aBeginLSN,
                                       smLSN           * aEndLSN,
                                       smOID             aTableOID )
{

    UInt           sLength;
    UInt           sLogTypeFlag;
    smTID          sTransID;
    smrDiskCMPSLog sCSLog;
    smrLogType     sLogType;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aBeginLSN  != NULL );
    IDE_DASSERT( aEndLSN    != NULL );

    idlOS::memset(&sCSLog, 0x00, SMR_LOGREC_SIZE(smrDiskCMPSLog));

    smLayerCallback::initLogBuffer( aTrans );
    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    sLogType = SMR_DLT_COMPENSATION;

    smrLogHeadI::setType(&sCSLog.mHead, sLogType);
    sLength = smuDynArray::getSize(aLogBuffer);

    smrLogHeadI::setSize(&sCSLog.mHead,
                         SMR_LOGREC_SIZE(smrDiskCMPSLog) +
                         sLength + ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sCSLog.mHead, sTransID);

    /* ------------------------------------------------
     * DML과 관련없는 로그이지만, SMR_LOG_TYPE_NORMAL을 설정할 경우에
     * replacator가 판독할 우려가 있다.
     * 결정: (N->R??) 아니면 로그 플래그 타입을 확장하는 방법을 고려해볼
     * 필요가 있다.
     * ----------------------------------------------*/
    smrLogHeadI::setFlag(&sCSLog.mHead, sLogTypeFlag);

    if ( (smrLogHeadI::getFlag(&sCSLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sCSLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sCSLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    IDE_TEST( decideLogComp( aWriteOption, &sCSLog.mHead )
              != IDE_SUCCESS );

    sCSLog.mRedoLogSize = sLength;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sCSLog,
                                   SMR_LOGREC_SIZE(smrDiskCMPSLog))
              != IDE_SUCCESS );

    if ( sLength != 0 )
    {
        IDE_TEST( ((smxTrans*)aTrans)->writeLogToBufferUsingDynArr( 
                                                          aLogBuffer,
                                                          sLength )
                != IDE_SUCCESS );
    }
    else
    {
        /* nothing do to */
    }

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        aPrevLSN,
                        aBeginLSN,
                        aEndLSN,
                        aTableOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : tx와 관계없는 disk 로그를 로깅
 * tx commit 과정에서 발생하는 commit SCN 로깅시 호출됨.
 **********************************************************************/
IDE_RC smrLogMgr::writeDiskDummyLogRec( idvSQL           * aStatistics,
                                        smuDynArrayBase  * aLogBuffer,
                                        UInt               aWriteOption,
                                        UInt               aContType,
                                        UInt               aRedoType,
                                        smOID              aTableOID,
                                        smLSN            * aBeginLSN,
                                        smLSN            * aEndLSN )
{

    smrDiskLog sDiskLog;
    UInt       sLength;
    ULong      sBuffer[(SD_PAGE_SIZE * 2)/ID_SIZEOF(ULong)];
    SChar*     sWritePtr;

    IDE_DASSERT( aLogBuffer != NULL );
    IDE_DASSERT( aBeginLSN  != NULL );
    IDE_DASSERT( aEndLSN    != NULL );

    idlOS::memset(&sDiskLog, 0x00, SMR_LOGREC_SIZE(smrDiskLog));

    smrLogHeadI::setType(&sDiskLog.mHead, SMR_DLT_REDOONLY);
    sLength  = smuDynArray::getSize(aLogBuffer);
    IDE_ASSERT( sLength > 0 );

    /* BUG-32623 When index statistics is rebuilded, the log's size
     * can exceed stack log buffer's size in mini-transaction. */
    IDE_TEST_RAISE(
            (SMR_LOGREC_SIZE(smrDiskLog) + sLength + ID_SIZEOF(smrLogTail))
            > ID_SIZEOF(sBuffer), error_exceed_stack_buffer_size);

    smrLogHeadI::setSize(&sDiskLog.mHead,
                         SMR_LOGREC_SIZE(smrDiskLog) +
                         sLength + ID_SIZEOF(smrLogTail) );

    smrLogHeadI::setTransID(&sDiskLog.mHead, SM_NULL_TID);

    smrLogHeadI::setFlag(&sDiskLog.mHead, SMR_LOG_TYPE_NORMAL);

    smrLogHeadI::setReplStmtDepth( &sDiskLog.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    IDE_TEST( decideLogComp( aWriteOption, &sDiskLog.mHead )
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * !!] 트랜잭션이 할당받은 tss의 RID를 설정한다.
     * 트랜잭션이 TSS를 할당받지 않은 경우는 SD_NULL_SID가
     * 반환된다.
     * ----------------------------------------------*/
    sDiskLog.mTableOID    = aTableOID;
    sDiskLog.mContType    = aContType;
    sDiskLog.mRedoType    = aRedoType;

    /* 3. disk logs들의 총 길이를 기록한다. */
    sDiskLog.mRedoLogSize = sLength;

    sWritePtr = (SChar*)sBuffer;

    idlOS::memset(sWritePtr, 0x00, smrLogHeadI::getSize(&sDiskLog.mHead));
    idlOS::memcpy(sWritePtr, &sDiskLog, SMR_LOGREC_SIZE(smrDiskLog));
    sWritePtr += SMR_LOGREC_SIZE(smrDiskLog);

    smuDynArray::load(aLogBuffer, (void*)sWritePtr, sLength );
    sWritePtr += sLength;

    smrLogHeadI::copyTail(sWritePtr, &(sDiskLog.mHead));

    IDE_TEST( writeLog( aStatistics,
                        NULL,      // aTrans
                        (SChar*)sBuffer,
                        NULL,      // Previous LSN Ptr
                        aBeginLSN, // Log LSN Ptr
                        aEndLSN,   // End LSN Ptr
                        aTableOID ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION(error_exceed_stack_buffer_size)
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, 
                                 "exceed stack buffer size") );
    }
    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;

}
/***********************************************************************
 * Description : tablespace BACKUP 과정중에
 * tx로부터 flsuh되는 Page에 대한 image 로깅
 **********************************************************************/
IDE_RC smrLogMgr::writeDiskPILogRec( idvSQL*           aStatistics,
                                     UChar*            aBuffer,
                                     scGRID            aPageGRID )
{
    IDE_TEST( writePageImgLogRec( aStatistics,
                                  aBuffer,
                                  aPageGRID,
                                  SDR_SDP_WRITE_PAGEIMG,
                                  NULL) // end LSN
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 디스크 공간 할당, 해제 및 변경에 대한 로그 타입
 **********************************************************************/
IDE_RC smrLogMgr::writeTBSUptLogRec( idvSQL           * aStatistics,
                                     void             * aTrans,
                                     smuDynArrayBase  * aLogBuffer,
                                     scSpaceID          aSpaceID,
                                     UInt               aFileID,
                                     sctUpdateType      aTBSUptType,
                                     UInt               aAImgSize,
                                     UInt               aBImgSize,
                                     smLSN            * aBeginLSN )
{
    UInt            sLength;
    UInt            sLogTypeFlag;
    smTID           sTransID;
    smrTBSUptLog    sTBSUptLog;
    smrLogType      sLogType;
    smLSN           sEndLSN;

    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aLogBuffer != NULL );

    idlOS::memset(&sTBSUptLog, 0x00, SMR_LOGREC_SIZE(smrTBSUptLog));

    smLayerCallback::initLogBuffer( aTrans );

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    sLogType = SMR_LT_TBS_UPDATE;

    smrLogHeadI::setType(&sTBSUptLog.mHead, sLogType );
    sLength = smuDynArray::getSize(aLogBuffer);

    smrLogHeadI::setSize(&sTBSUptLog.mHead,
                         SMR_LOGREC_SIZE(smrTBSUptLog) +
                         sLength + ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sTBSUptLog.mHead, sTransID);

    smrLogHeadI::setFlag(&sTBSUptLog.mHead, sLogTypeFlag);

    if ( (smrLogHeadI::getFlag(&sTBSUptLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sTBSUptLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sTBSUptLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sTBSUptLog.mSpaceID    = aSpaceID;
    sTBSUptLog.mFileID     = aFileID;
    sTBSUptLog.mTBSUptType = aTBSUptType;
    sTBSUptLog.mAImgSize   = aAImgSize;
    sTBSUptLog.mBImgSize   = aBImgSize;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sTBSUptLog,
                                   SMR_LOGREC_SIZE(smrTBSUptLog))
              != IDE_SUCCESS );

    if ( sLength != 0 )
    {
        IDE_TEST( ((smxTrans*)aTrans)->writeLogToBufferUsingDynArr(
                                 aLogBuffer,
                                 sLength )
                != IDE_SUCCESS );
    }

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,      // Prev LSN Ptr
                        aBeginLSN, 
                        &sEndLSN,  // END LSN Ptr
                        SM_NULL_OID )
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_TRX,
                                       &sEndLSN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : MRDB NTA 로그(SMR_LT_NTA)를 로그버퍼에 기록
 **********************************************************************/
IDE_RC smrLogMgr::writeNTALogRec(idvSQL   * aStatistics,
                                 void     * aTrans,
                                 smLSN    * aLSN,
                                 scSpaceID  aSpaceID,
                                 smrOPType  aOPType,
                                 vULong     aData1,
                                 vULong     aData2)
{

    smrNTALog sNTALog;
    smTID     sTID;
    UInt      sLogTypeFlag;

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTID,
                                       &sLogTypeFlag );
    smrLogHeadI::setType(&sNTALog.mHead, SMR_LT_NTA);
    smrLogHeadI::setSize(&sNTALog.mHead, SMR_LOGREC_SIZE(smrNTALog));
    smrLogHeadI::setTransID(&sNTALog.mHead, sTID);
    smrLogHeadI::setFlag(&sNTALog.mHead, sLogTypeFlag);

    if ( (smrLogHeadI::getFlag(&sNTALog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sNTALog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sNTALog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }
    sNTALog.mSpaceID           = aSpaceID;
    sNTALog.mData1             = aData1;
    sNTALog.mData2             = aData2;
    sNTALog.mOPType            = aOPType;
    sNTALog.mTail              = SMR_LT_NTA;

    return writeLog( aStatistics,
                     aTrans,
                     (SChar*)&sNTALog,
                     aLSN, // Previous LSN Ptr
                     NULL, // Log LSN Ptr
                     NULL, // END LSN Ptr
                     SM_NULL_OID );
}

/***********************************************************************
 * Description : savepoint 설정 로그를 로그버퍼에 기록
 **********************************************************************/
IDE_RC smrLogMgr::writeSetSvpLog(idvSQL*      aStatistics,
                                 void*        aTrans,
                                 const SChar* aSVPName)
{

    ULong        sLogBuffer[SM_PAGE_SIZE / ID_SIZEOF(ULong)];
    SChar*       sLogPtr;
    smrLogHead*  sLogHead;
    UInt         sLen;
    smTID        sTID;
    UInt         sLogFlag;

    sLogPtr         = (SChar*)sLogBuffer;
    sLogHead        = (smrLogHead*)sLogPtr;

    smrLogHeadI::setType(sLogHead, SMR_LT_SAVEPOINT_SET);

    //get tx id and logtype flag from a transaction
    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTID,
                                       &sLogFlag );

    smrLogHeadI::setTransID(sLogHead, sTID);
    smrLogHeadI::setFlag(sLogHead, sLogFlag);
    if ( (smrLogHeadI::getFlag(sLogHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        sLogHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( sLogHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    // get m_lstUndoNxtLSN of a transaction
    smrLogHeadI::setPrevLSN( sLogHead, smLayerCallback::getLstUndoNxtLSN( aTrans ) );
    sLogPtr += ID_SIZEOF(smrLogHead);

    if ( aSVPName != NULL)
    {
        sLen = idlOS::strlen(aSVPName);
        *((UInt*)sLogPtr) = sLen;
        sLogPtr += ID_SIZEOF(UInt);

        idlOS::memcpy(sLogPtr, aSVPName, sLen);
    }
    else
    {
        sLen = SMR_IMPLICIT_SVP_NAME_SIZE;

        SMR_LOG_APPEND_4( sLogPtr, sLen );

        idlOS::memcpy(sLogPtr, SMR_IMPLICIT_SVP_NAME, SMR_IMPLICIT_SVP_NAME_SIZE);
    }

    sLogPtr += sLen;
    smrLogHeadI::setSize(sLogHead, ID_SIZEOF(smrLogHead) + ID_SIZEOF(UInt) + sLen  + ID_SIZEOF(smrLogTail));

    smrLogHeadI::copyTail(sLogPtr, sLogHead);

    return writeLog( aStatistics,
                     aTrans,
                     (SChar*)sLogBuffer,
                     NULL,   // Previous LSN Ptr
                     NULL,   // Log LSN Ptr
                     NULL,   // End LSN Ptr
                     SM_NULL_OID );
}

/***********************************************************************
 * Description : savepoint 해제 로그를 로그버퍼에 기록
 **********************************************************************/
IDE_RC smrLogMgr::writeAbortSvpLog( idvSQL*      aStatistics,
                                    void*        aTrans,
                                    const SChar* aSVPName)
{

    ULong        sLogBuffer[SM_PAGE_SIZE / ID_SIZEOF(ULong)];
    SChar*       sLogPtr;
    smrLogHead*  sLogHead;
    UInt         sLen;
    smTID        sTID;
    UInt         sLogFlag;

    sLogPtr         = (SChar*)sLogBuffer;
    sLogHead        = (smrLogHead*)sLogPtr;

    smrLogHeadI::setType(sLogHead, SMR_LT_SAVEPOINT_ABORT);

    //get tx id and logtype flag from a transaction
    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTID,
                                       &sLogFlag );

    smrLogHeadI::setTransID(sLogHead, sTID);
    smrLogHeadI::setFlag(sLogHead, sLogFlag);
    if ( (smrLogHeadI::getFlag(sLogHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        sLogHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( sLogHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    // get m_lstUndoNxtLSN of a transaction
    smrLogHeadI::setPrevLSN( sLogHead, smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    sLogPtr += ID_SIZEOF(smrLogHead);

    if ( aSVPName != NULL)
    {
        sLen = idlOS::strlen(aSVPName);

        SMR_LOG_APPEND_4( sLogPtr, sLen );

        idlOS::memcpy(sLogPtr, aSVPName, sLen);
    }
    else
    {
        sLen = SMR_IMPLICIT_SVP_NAME_SIZE;

        SMR_LOG_APPEND_4( sLogPtr, sLen );

        idlOS::memcpy(sLogPtr, SMR_IMPLICIT_SVP_NAME, SMR_IMPLICIT_SVP_NAME_SIZE);
    }

    sLogPtr += sLen;
    smrLogHeadI::setSize(sLogHead, ID_SIZEOF(smrLogHead) + ID_SIZEOF(UInt) + sLen  + ID_SIZEOF(smrLogTail));

    smrLogHeadI::copyTail(sLogPtr, sLogHead);

    return writeLog( aStatistics,
                     aTrans,
                     (SChar*)sLogBuffer,
                     NULL,   // Previous LSN Ptr
                     NULL,   // Log LSN Ptr
                     NULL,   // End LSN Ptr
                     SM_NULL_OID );
}

/***********************************************************************
 * Description : CLR 로그를 로그버퍼에 기록
 **********************************************************************/
IDE_RC smrLogMgr::writeCMPSLogRec( idvSQL*       aStatistics,
                                   void*         aTrans,
                                   smrLogType    aType,
                                   smLSN*        aPrvUndoLSN,
                                   smrUpdateLog* aUpdateLog,
                                   SChar*        aBeforeImage )
{

    smrCMPSLog sCSLog;
    smTID      sTID;
    UInt       sLogFlag;
    SChar*     sLogPtr;                 
    UInt       sFlagLogSize = 0;        
    UInt       sColCnt = 0;
    UInt       sPieceCnt = 0;
    smOID      sVCPieceOID;
    UShort     sBefFlag;
    UInt       sFullXLogSize;
    UInt       sPrimaryKeySize;
    UInt       sOffset = 0;
    UInt       sFlag;
    UInt       i;
    UInt       j;

    idlOS::memset(&sCSLog, 0x00, SMR_LOGREC_SIZE(smrCMPSLog));
    smLayerCallback::initLogBuffer( aTrans );

    smrLogHeadI::setType(&sCSLog.mHead, aType);

    //get tx id and logtype flag from a transaction
    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTID,
                                       &sLogFlag );

    smrLogHeadI::setTransID(&sCSLog.mHead, sTID);
    smrLogHeadI::setFlag(&sCSLog.mHead, sLogFlag);
    
    if ( (smrLogHeadI::getFlag(&sCSLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sCSLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sCSLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    if (aUpdateLog != NULL)
    {
        /* BUG-46854: 로그 헤더의 size 셋팅 뒤로 이동함. VCPiece 플래그
         * 변경 로그량을 계산해야하기 때문이다. 
         */
        
        smrLogHeadI::setFlag(&sCSLog.mHead,
                             smrLogHeadI::getFlag(&aUpdateLog->mHead) & SMR_LOG_TYPE_MASK);
        sCSLog.mGRID       = aUpdateLog->mGRID;
        sCSLog.mBImgSize   = aUpdateLog->mBImgSize;
        sCSLog.mUpdateType = aUpdateLog->mType;
        sCSLog.mData       = aUpdateLog->mData;

        // PRJ-1548 User Memory Tablespace
        // 테이블스페이스 UPDATE 연산에 대한 CLR이 아니므로 다음과 같이
        // 초기화해준다.
        sCSLog.mTBSUptType = SCT_UPDATE_MAXMAX_TYPE;
    }
    else
    {
        // dummy CLR의 경우

        smrLogHeadI::setSize( &sCSLog.mHead,
                              SMR_LOGREC_SIZE(smrCMPSLog) +
                              ID_SIZEOF(smrLogTail) );
        // PRJ-1548 User Memory Tablespace
        // 테이블스페이스 UPDATE 연산에 대한 CLR이 아니므로 다음과 같이
        // 초기화해준다.
        sCSLog.mTBSUptType = SCT_UPDATE_MAXMAX_TYPE;
    }

    // BUG-46854i: CMPS로그 헤더 write 뒤로 이동

    sOffset += SMR_LOGREC_SIZE(smrCMPSLog);

    if ( aUpdateLog != NULL )
    {
        if ( aUpdateLog->mBImgSize > 0 )
        {
            IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( aBeforeImage,
                                                             sOffset,
                                                             aUpdateLog->mBImgSize )
                      != IDE_SUCCESS );
            
            // BUG-46854
            sOffset += aUpdateLog->mBImgSize;
        }

        // BUG-47366
        if ( ( aUpdateLog->mType == SMR_SMC_PERS_INSERT_ROW ) ||
             ( aUpdateLog->mType == SMR_SMC_PERS_UPDATE_VERSION_ROW ) ||
             ( aUpdateLog->mType == SMR_SMC_PERS_UPDATE_INPLACE_ROW ) )
        {
            IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( aBeforeImage + aUpdateLog->mBImgSize,
                                                             sOffset,
                                                             aUpdateLog->mAImgSize )
                      != IDE_SUCCESS );
            
            sOffset      += aUpdateLog->mAImgSize;
            sFlagLogSize += aUpdateLog->mAImgSize;
        }
     
        // BUG-46854: delete version 일 때 플래그 로그 남기기
        if ( aUpdateLog->mType == SMR_SMC_PERS_DELETE_VERSION_ROW ) 
        {
            // BUG-46854: Var 피스 플래그 로그 처리 
            // before + after image 크기 만큼 포인터 이동
            sLogPtr = aBeforeImage;
            sLogPtr += ( aUpdateLog->mBImgSize + aUpdateLog->mAImgSize );  
            
            // 프라이머리 키 있을 때
            if( ( smrLogHeadI::getFlag(&aUpdateLog->mHead) & SMR_LOG_RP_INFO_LOG_MASK ) 
                == SMR_LOG_RP_INFO_LOG_OK )
            {
                idlOS::memcpy(&sPrimaryKeySize, sLogPtr, ID_SIZEOF(UInt));
                sLogPtr += sPrimaryKeySize;
            }

            // Full XLog있을 때
            if( ( smrLogHeadI::getFlag(&aUpdateLog->mHead) & SMR_LOG_FULL_XLOG_MASK ) 
                == SMR_LOG_FULL_XLOG_OK )
            {
                idlOS::memcpy(&sFullXLogSize, sLogPtr, ID_SIZEOF(UInt));
                sLogPtr += sFullXLogSize;
            }    

            // col cnt 기져오기 
            idlOS::memcpy(&sColCnt, sLogPtr, ID_SIZEOF(UInt));
            sLogPtr += ID_SIZEOF(UInt);

            // col cnt 로깅 
            IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sColCnt,
                                                             sOffset,
                                                             ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );
            sFlagLogSize += ID_SIZEOF(UInt);
            sOffset += ID_SIZEOF(UInt);

            for ( i = 0 ; i < sColCnt ; i++ )
            {
                // 피스 개수 가져오기 
                idlOS::memcpy(&sPieceCnt, sLogPtr, ID_SIZEOF(UInt));
                sLogPtr += ID_SIZEOF(UInt);

                // 피스 개수 로깅
                IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sPieceCnt,
                                                                 sOffset,
                                                                 ID_SIZEOF(UInt) )
                      != IDE_SUCCESS );
                sFlagLogSize += ID_SIZEOF(UInt);
                sOffset += ID_SIZEOF(UInt);

                for ( j = 0 ; j < sPieceCnt ; j++ )
                {
                    // OID 가져오기
                    idlOS::memcpy(&sVCPieceOID, sLogPtr, ID_SIZEOF(smOID));
                    sLogPtr += ID_SIZEOF(smOID);

                    // OID 로깅 
                    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sVCPieceOID,
                                                                     sOffset,
                                                                     ID_SIZEOF(smOID) )
                              != IDE_SUCCESS );
                    sFlagLogSize += ID_SIZEOF(smOID);
                    sOffset += ID_SIZEOF(smOID);

                    // before 플래그 가져오기 
                    idlOS::memcpy(&sBefFlag, sLogPtr, ID_SIZEOF(UShort));
                    sLogPtr += ID_SIZEOF(UShort);

                    // before 플래그 로깅
                    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sBefFlag,
                                                                     sOffset,
                                                                     ID_SIZEOF(UShort) )
                              != IDE_SUCCESS );
                    sFlagLogSize += ID_SIZEOF(UShort);
                    sOffset += ID_SIZEOF(UShort);

                    // after 플래그 넘어가기 
                    sLogPtr += ID_SIZEOF(UShort);
                }
            }
            
            // BUG-46854: CMPS log 셋팅.
            sFlag = smrLogHeadI::getFlag(&sCSLog.mHead);
            sFlag &= SMR_LOG_CMPS_LOG_MASK;
            sFlag |= SMR_LOG_CMPS_LOG_OK;
            smrLogHeadI::setFlag( &sCSLog.mHead, sFlag );
        }
    
        // BUG-46854: size 셋팅 옮겨짐    
        smrLogHeadI::setSize(&sCSLog.mHead, SMR_LOGREC_SIZE(smrCMPSLog) 
                                            + aUpdateLog->mBImgSize 
                                            + sFlagLogSize   /* flag 로그 크기 */ 
                                            + ID_SIZEOF(smrLogTail));
    }
   
    // BUG-46854: CMPS 로그 헤더 write 
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( (SChar*)&sCSLog,
                                                     0,
                                                     SMR_LOGREC_SIZE(smrCMPSLog))
              != IDE_SUCCESS );

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &aType,
                                                     sOffset,
                                                     ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        aPrvUndoLSN,
                        NULL,  // Log LSN Ptr
                        NULL,  // End LSN Ptr
                        SM_NULL_OID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// PROJ-1362 LOB CURSOR OPEN LOG for replication.
// lob cursor open log
// smrLogHead| lob locator | mOp(lob operation) | tableOID | column id | pk info
IDE_RC smrLogMgr::writeLobCursorOpenLogRec( idvSQL*           aStatistics,
                                            void*             aTrans,
                                            smrLobOpType      aLobOp,
                                            smOID             aTable,
                                            UInt              aLobColID,
                                            smLobLocator      aLobLocator,
                                            const void       *aPKInfo)
{
    UInt                      sLogTypeFlag;
    smTID                     sTransID;
    smrLobLog                 sLobCursorLog;
    smrLogType                sLogType;
    const sdcPKInfo          *sPKInfo = (const sdcPKInfo*)aPKInfo;
    const sdcColumnInfo4PK   *sColumnInfo;
    UShort                    sPKInfoSize;
    UShort                    sPKColCount;
    UShort                    sPKColSeq;
    UInt                      sColumnId;
    UChar                     sPrefix;
    UChar                     sSmallLen;
    UShort                    sLargeLen;

    IDE_DASSERT( aTrans  != NULL );
    IDE_DASSERT( aPKInfo != NULL );

    IDE_ASSERT(aLobOp == SMR_DISK_LOB_CURSOR_OPEN);

    idlOS::memset(&sLobCursorLog, 0x00, SMR_LOGREC_SIZE(smrLobLog));
    smLayerCallback::initLogBuffer( aTrans );


    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    IDE_DASSERT (sLogTypeFlag == SMR_LOG_TYPE_NORMAL);
    sLogType =  SMR_LT_LOB_FOR_REPL;

    smrLogHeadI::setType(&sLobCursorLog.mHead,sLogType);
    smrLogHeadI::setSize(&sLobCursorLog.mHead,
                         SMR_LOGREC_SIZE(smrLobLog) +
                         ID_SIZEOF(smOID)+   // table
                         ID_SIZEOF(UInt) +   // lob column id
                         sdcRow::calcPKLogSize(sPKInfo) +
                         ID_SIZEOF(smrLogTail) );

    smrLogHeadI::setTransID(&sLobCursorLog.mHead, sTransID);

    smrLogHeadI::setPrevLSN( &sLobCursorLog.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag(&sLobCursorLog.mHead, SMR_LOG_TYPE_NORMAL);

    if ( (smrLogHeadI::getFlag(&sLobCursorLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sLobCursorLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLobCursorLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sLobCursorLog.mOpType = aLobOp;
    sLobCursorLog.mLocator = aLobLocator;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLobCursorLog,
                                   SMR_LOGREC_SIZE(smrLobLog))
              != IDE_SUCCESS );
    // table
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aTable ,
                                   ID_SIZEOF(smOID))
              != IDE_SUCCESS );
    // column-id
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aLobColID ,
                                   ID_SIZEOF(UInt))
              != IDE_SUCCESS );

    /***************************************************/
    /*            write pk info                        */
    /*                                                 */
    /***************************************************/
    sPKColCount = sPKInfo->mTotalPKColCount;
    sPKInfoSize = sdcRow::calcPKLogSize(sPKInfo) - (2);

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sPKInfoSize,
                                   ID_SIZEOF(sPKInfoSize))
              != IDE_SUCCESS );

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sPKColCount,
                                   ID_SIZEOF(sPKColCount))
              != IDE_SUCCESS );

    for ( sPKColSeq = 0; sPKColSeq < sPKColCount; sPKColSeq++ )
    {
        sColumnInfo = sPKInfo->mColInfoList + sPKColSeq;
        sColumnId   = (UInt)sColumnInfo->mColumn->id;

        IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                       &sColumnId,
                                       ID_SIZEOF(sColumnId))
                  != IDE_SUCCESS );

        if ( sColumnInfo->mValue.length == 0)
        {
            sPrefix = (UChar)SDC_NULL_COLUMN_PREFIX;
            IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                           &sPrefix,
                                           ID_SIZEOF(sPrefix))
                      != IDE_SUCCESS );
        }
        else
        {
            if ( sColumnInfo->mValue.length >
                 SDC_COLUMN_LEN_STORE_SIZE_THRESHOLD )
            {
                sPrefix = (UChar)SDC_LARGE_COLUMN_PREFIX;
                IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                               &sPrefix,
                                               ID_SIZEOF(sPrefix))
                          != IDE_SUCCESS );

                sLargeLen = (UShort)sColumnInfo->mValue.length;
                IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                               &sLargeLen,
                                               ID_SIZEOF(sLargeLen))
                          != IDE_SUCCESS );
            }
            else
            {
                sSmallLen = (UChar)sColumnInfo->mValue.length;
                IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                               &sSmallLen,
                                               ID_SIZEOF(sSmallLen))
                          != IDE_SUCCESS );
            }

            IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                          sColumnInfo->mValue.value,
                          sColumnInfo->mValue.length)
                      != IDE_SUCCESS );
        }
    }

    // tail
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,    // Previous LSN Ptr
                        NULL,    // Log LSN Ptr 
                        NULL,    // End LSN Ptr
                        aTable ) // TableOID
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogMgr::writeLobCursorCloseLogRec( idvSQL*         aStatistics,
                                             void*           aTrans,
                                             smLobLocator    aLobLocator,
                                             smOID           aTableOID )
{
    IDE_TEST( writeLobOpLogRec(aStatistics,
                               aTrans,
                               aLobLocator,
                               SMR_LOB_CURSOR_CLOSE,
                               aTableOID )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// TODO: aOldSize 제거하기
// PROJ-1362 LOB PREPARE4WRITE  LOG for replication.
// smrLogHead| lob locator | mOp | offset(4) | old size(4) | newSize
IDE_RC smrLogMgr::writeLobPrepare4WriteLogRec(idvSQL*          aStatistics,
                                              void*            aTrans,
                                              smLobLocator     aLobLocator,
                                              UInt             aOffset,
                                              UInt             aOldSize,
                                              UInt             aNewSize,
                                              smOID            aTableOID )
{
    UInt       sLogTypeFlag;
    smTID      sTransID;
    smrLobLog  sLobCursorLog;
    smrLogType sLogType;


    IDE_DASSERT( aTrans != NULL );
    idlOS::memset(&sLobCursorLog, 0x00, SMR_LOGREC_SIZE(smrLobLog));
    smLayerCallback::initLogBuffer( aTrans );


    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    IDE_DASSERT (sLogTypeFlag == SMR_LOG_TYPE_NORMAL);
    sLogType =  SMR_LT_LOB_FOR_REPL;

    smrLogHeadI::setType(&sLobCursorLog.mHead,sLogType);
    smrLogHeadI::setSize(&sLobCursorLog.mHead,
                         SMR_LOGREC_SIZE(smrLobLog) +
                         ID_SIZEOF(UInt) +   // offset
                         ID_SIZEOF(UInt) +   // old size
                         ID_SIZEOF(UInt) +   // new size
                         ID_SIZEOF(smrLogTail) );

    smrLogHeadI::setTransID(&sLobCursorLog.mHead, sTransID);

    smrLogHeadI::setPrevLSN( &sLobCursorLog.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag(&sLobCursorLog.mHead, SMR_LOG_TYPE_NORMAL);

    if ( (smrLogHeadI::getFlag(&sLobCursorLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sLobCursorLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLobCursorLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sLobCursorLog.mOpType = SMR_PREPARE4WRITE;
    sLobCursorLog.mLocator = aLobLocator;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLobCursorLog,
                                   SMR_LOGREC_SIZE(smrLobLog))
              != IDE_SUCCESS );
    // offset
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aOffset ,
                                   ID_SIZEOF(UInt))
              != IDE_SUCCESS );

    // old size
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aOldSize ,
                                   ID_SIZEOF(UInt))
              != IDE_SUCCESS );

    // new size
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aNewSize ,
                                   ID_SIZEOF(UInt))
              != IDE_SUCCESS );


    // tail
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,  // Previous LSN Ptr
                        NULL,  // Log LSN Ptr 
                        NULL,  // End LSN Ptr  
                        aTableOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smrLogMgr::writeLobFinish2WriteLogRec( idvSQL*       aStatistics,
                                              void*         aTrans,
                                              smLobLocator  aLobLocator,
                                              smOID         aTableOID )
{
    IDE_TEST( writeLobOpLogRec(aStatistics,
                               aTrans,
                               aLobLocator,
                               SMR_FINISH2WRITE,
                               aTableOID )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1362 LOB CURSOR CLOSE LOG,SMR_FINISH2WRITE  for replication.
IDE_RC smrLogMgr::writeLobOpLogRec( idvSQL*           aStatistics,
                                    void*             aTrans,
                                    smLobLocator      aLobLocator,
                                    smrLobOpType      aLobOp,
                                    smOID             aTableOID )
{

    UInt       sLogTypeFlag;
    smTID      sTransID;
    smrLobLog  sLobCursorLog;
    smrLogType sLogType;


    IDE_DASSERT( aTrans != NULL );
    idlOS::memset(&sLobCursorLog, 0x00, SMR_LOGREC_SIZE(smrLobLog));
    smLayerCallback::initLogBuffer( aTrans );

    IDE_ASSERT(( aLobOp == SMR_LOB_CURSOR_CLOSE)  ||
               (aLobOp ==  SMR_FINISH2WRITE ));

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    IDE_DASSERT (sLogTypeFlag == SMR_LOG_TYPE_NORMAL);


    sLogType =  SMR_LT_LOB_FOR_REPL;

    smrLogHeadI::setType(&sLobCursorLog.mHead,sLogType);
    smrLogHeadI::setSize(&sLobCursorLog.mHead,
                         SMR_LOGREC_SIZE(smrLobLog) +ID_SIZEOF(smrLogTail) );
    smrLogHeadI::setTransID(&sLobCursorLog.mHead, sTransID);

    smrLogHeadI::setPrevLSN( &sLobCursorLog.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag(&sLobCursorLog.mHead, SMR_LOG_TYPE_NORMAL);

    if ( (smrLogHeadI::getFlag(&sLobCursorLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sLobCursorLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLobCursorLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sLobCursorLog.mOpType = aLobOp;
    sLobCursorLog.mLocator = aLobLocator;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLobCursorLog,
                                   SMR_LOGREC_SIZE(smrLobLog))
              != IDE_SUCCESS );

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,  // Previous LSN Ptr
                        NULL,  // Log LSN Ptr 
                        NULL,  // End LSN Ptr  
                        aTableOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2047 Strengthening LOB
// smrLogHead| lob locator | mOp | offset(8) | size(8) | value(1)
IDE_RC smrLogMgr::writeLobEraseLogRec( idvSQL       * aStatistics,
                                       void         * aTrans,
                                       smLobLocator   aLobLocator,
                                       ULong          aOffset,
                                       ULong          aSize,
                                       smOID          aTableOID )
{
    UInt       sLogTypeFlag;
    smTID      sTransID;
    smrLobLog  sLobCursorLog;
    smrLogType sLogType;


    IDE_DASSERT( aTrans != NULL );
    idlOS::memset(&sLobCursorLog, 0x00, SMR_LOGREC_SIZE(smrLobLog));
    smLayerCallback::initLogBuffer( aTrans );


    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    IDE_DASSERT (sLogTypeFlag == SMR_LOG_TYPE_NORMAL);
    sLogType =  SMR_LT_LOB_FOR_REPL;

    smrLogHeadI::setType( &sLobCursorLog.mHead,sLogType);
    smrLogHeadI::setSize( &sLobCursorLog.mHead,
                          SMR_LOGREC_SIZE(smrLobLog) +
                          ID_SIZEOF(ULong) +   // offset
                          ID_SIZEOF(ULong) +   // size
                          ID_SIZEOF(smrLogTail) );

    smrLogHeadI::setTransID( &sLobCursorLog.mHead, sTransID );

    smrLogHeadI::setPrevLSN( &sLobCursorLog.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag(&sLobCursorLog.mHead, SMR_LOG_TYPE_NORMAL);

    if ( (smrLogHeadI::getFlag(&sLobCursorLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sLobCursorLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLobCursorLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sLobCursorLog.mOpType = SMR_LOB_ERASE;
    sLobCursorLog.mLocator = aLobLocator;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLobCursorLog,
                                   SMR_LOGREC_SIZE(smrLobLog))
              != IDE_SUCCESS );
    // offset
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aOffset ,
                                   ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    // size
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &aSize ,
                                   ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    // tail
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer(
                                   &sLogType,
                                   ID_SIZEOF(smrLogType))
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,  // Previous LSN Ptr
                        NULL,  // Log LSN Ptr 
                        NULL,  // End LSN Ptr  
                        aTableOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// PROJ-2047 Strengthening LOB
// smrLogHead| lob locator | mOp | offset(8)
IDE_RC smrLogMgr::writeLobTrimLogRec(idvSQL         * aStatistics,
                                     void*            aTrans,
                                     smLobLocator     aLobLocator,
                                     ULong            aOffset,
                                     smOID            aTableOID )
{
    UInt       sLogTypeFlag;
    smTID      sTransID;
    smrLobLog  sLobCursorLog;
    smrLogType sLogType;


    IDE_DASSERT( aTrans != NULL );
    idlOS::memset(&sLobCursorLog, 0x00, SMR_LOGREC_SIZE(smrLobLog));
    smLayerCallback::initLogBuffer( aTrans );


    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    IDE_DASSERT (sLogTypeFlag == SMR_LOG_TYPE_NORMAL);
    sLogType =  SMR_LT_LOB_FOR_REPL;

    smrLogHeadI::setType( &sLobCursorLog.mHead,sLogType);
    smrLogHeadI::setSize( &sLobCursorLog.mHead,
                          SMR_LOGREC_SIZE(smrLobLog) +
                          ID_SIZEOF(ULong) +    // offset
                          ID_SIZEOF(smrLogTail) );

    smrLogHeadI::setTransID( &sLobCursorLog.mHead, sTransID );

    smrLogHeadI::setPrevLSN( &sLobCursorLog.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag( &sLobCursorLog.mHead, SMR_LOG_TYPE_NORMAL );

    if ( (smrLogHeadI::getFlag(&sLobCursorLog.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sLobCursorLog.mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLobCursorLog.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sLobCursorLog.mOpType = SMR_LOB_TRIM;
    sLobCursorLog.mLocator = aLobLocator;

    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sLobCursorLog,
                                                     SMR_LOGREC_SIZE(smrLobLog) )
              != IDE_SUCCESS );
    // offset
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &aOffset ,
                                                     ID_SIZEOF(ULong) )
              != IDE_SUCCESS );

    // tail
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sLogType,
                                                     ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,  // Previous LSN Ptr
                        NULL,  // Log LSN Ptr 
                        NULL,  // End LSN Ptr  
                        aTableOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1665
 * Description : Direct-Path Insert가 수행된 Page 전체를 Logging
 * Implementation :
 *    Direct-Path Buffer Manager가 flush 할때,
 *    Page 전체에 대하여 Log를 남길때 사용
 *
 *   +--------------------------------------------------+
 *   | smrDiskLog | sdrLogHdr | Page 내용 | smrLogTail |
 *   +--------------------------------------------------+
 *
 *    - In
 *      aStatistics : statistics
 *      aTrans      : Page Log를 쓰려고 하는 transaction
 *      aBuffer     : buffer
 *      aPageGRID   : Page GRID
 *      aLSN        : LSN
 *    - Out
 **********************************************************************/
IDE_RC smrLogMgr::writeDPathPageLogRec( idvSQL * aStatistics,
                                        UChar  * aBuffer,
                                        scGRID   aPageGRID,
                                        smLSN  * aEndLSN )
{
    return writePageImgLogRec( aStatistics,
                               aBuffer,
                               aPageGRID,
                               SDR_SDP_WRITE_DPATH_INS_PAGE,
                               aEndLSN );
}



/***********************************************************************
 * PROJ-1867
 * Page Img Log를 기록한다.
 **********************************************************************/
IDE_RC smrLogMgr::writePageImgLogRec( idvSQL     * aStatistics,
                                      UChar      * aBuffer,
                                      scGRID       aPageGRID,
                                      sdrLogType   aLogType,
                                      smLSN      * aEndLSN )
{
    smrDiskPILog sPILog;

    IDE_DASSERT( aBuffer != NULL );

    //------------------------
    // smrDiskLog 설정
    //------------------------

    idlOS::memset( &sPILog, 0x00, SMR_LOGREC_SIZE(smrDiskPILog) );

    smrLogHeadI::setType( &sPILog.mHead, SMR_DLT_REDOONLY );

    smrLogHeadI::setSize( &sPILog.mHead,
                          SMR_LOGREC_SIZE(smrDiskPILog) );

    smrLogHeadI::setTransID( &sPILog.mHead, SM_NULL_TID );
    smrLogHeadI::setFlag( &sPILog.mHead, SMR_LOG_TYPE_NORMAL );

    smrLogHeadI::setReplStmtDepth( &sPILog.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    sPILog.mTableOID    = SM_NULL_OID;     // not use
    sPILog.mContType    = SMR_CT_END;      // 단일 로그임
    sPILog.mRedoLogSize = ID_SIZEOF(sdrLogHdr) + SD_PAGE_SIZE;
    sPILog.mRefOffset   = 0;               // replication 관련
    sPILog.mRedoType    = SMR_RT_DISKONLY; // runtime memory에 대하여 함께 재수행 여부

    //------------------------
    // sdrLogHdr 설정
    //------------------------
    sPILog.mDiskLogHdr.mGRID   = aPageGRID;
    sPILog.mDiskLogHdr.mLength = SD_PAGE_SIZE;
    sPILog.mDiskLogHdr.mType   = aLogType;

    //------------------------
    // page data
    //------------------------
    idlOS::memcpy( sPILog.mPage,
                   aBuffer,
                   SD_PAGE_SIZE );

    //------------------------
    // tail ( smrLogType == smrLogTail )
    //------------------------
    sPILog.mTail = SMR_DLT_REDOONLY;

    //------------------------
    // write log
    //------------------------
    IDE_TEST( writeLog( aStatistics,
                        NULL,         // transaction
                        (SChar*)&sPILog,
                        NULL,         // Previous LSN Ptr
                        NULL,         // Log LSN Ptr 
                        aEndLSN,      // End LSN Ptr  
                        SM_NULL_OID ) // TableOID
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * PROJ-1864
 * Page Consistent Log를 기록한다.
 **********************************************************************/
IDE_RC smrLogMgr::writePageConsistentLogRec( idvSQL     * aStatistics,
                                             scSpaceID    aSpaceID,
                                             scPageID     aPageID,
                                             UChar        aIsPageConsistent)
{
    scGRID               sPageGRID;
    smrPageCinsistentLog sPCLog;

    SC_MAKE_GRID( sPageGRID, aSpaceID, aPageID, 0 ) ;

    //------------------------
    // smrDiskLog 설정
    //------------------------

    idlOS::memset( &sPCLog, 0x00, SMR_LOGREC_SIZE(smrPageCinsistentLog) );

    smrLogHeadI::setType( &sPCLog.mHead, SMR_DLT_REDOONLY );

    smrLogHeadI::setSize( &sPCLog.mHead,
                          SMR_LOGREC_SIZE(smrPageCinsistentLog) );

    smrLogHeadI::setTransID( &sPCLog.mHead, SM_NULL_TID );
    smrLogHeadI::setFlag( &sPCLog.mHead, SMR_LOG_TYPE_NORMAL );

    smrLogHeadI::setReplStmtDepth( &sPCLog.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    sPCLog.mTableOID    = SM_NULL_OID;     // not use
    sPCLog.mContType    = SMR_CT_END;      // 단일 로그임
    sPCLog.mRedoLogSize = ID_SIZEOF(sdrLogHdr) + ID_SIZEOF(aIsPageConsistent);
    sPCLog.mRefOffset   = 0;               // replication 관련
    sPCLog.mRedoType    = SMR_RT_DISKONLY; // runtime memory에 대하여 함께 재수행 여부
    //------------------------
    // sdrLogHdr 설정
    //------------------------
    sPCLog.mDiskLogHdr.mGRID   = sPageGRID;
    sPCLog.mDiskLogHdr.mLength = ID_SIZEOF(aIsPageConsistent);
    sPCLog.mDiskLogHdr.mType   = SDR_SDP_PAGE_CONSISTENT;

    //------------------------
    // page data
    //------------------------
    sPCLog.mPageConsistent = aIsPageConsistent;

    //------------------------
    // tail ( smrLogType == smrLogTail )
    //------------------------
    sPCLog.mTail = SMR_DLT_REDOONLY;

    //------------------------
    // write log
    //------------------------
    IDE_TEST( writeLog( aStatistics,
                        NULL,         // transaction
                        (SChar*)&sPCLog,
                        NULL,         // Previous LSN Ptr
                        NULL,         // Log LSN Ptr 
                        NULL,         // End LSN Ptr  
                        SM_NULL_OID ) // TableOID
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : SMR_OP_NULL 타입의 NTA 로그 기록
 **********************************************************************/
IDE_RC smrLogMgr::writeNullNTALogRec( idvSQL* aStatistics,
                                      void  * aTrans,
                                      smLSN * aLSN )
{

    return writeNTALogRec( aStatistics,
                           aTrans,
                           aLSN,
                           0,             // meaningless
                           SMR_OP_NULL );

}

/***********************************************************************
 * Description : SMR_OP_SMM_PERS_LIST_ALLOC 타입의 NTA 로그 기록
 **********************************************************************/
IDE_RC smrLogMgr::writeAllocPersListNTALogRec( idvSQL*    aStatistics,
                                               void     * aTrans,
                                               smLSN    * aLSN,
                                               scSpaceID  aSpaceID,
                                               scPageID   aFirstPID,
                                               scPageID   aLastPID )
{

    return writeNTALogRec( aStatistics,
                           aTrans,
                           aLSN,
                           aSpaceID,
                           SMR_OP_SMM_PERS_LIST_ALLOC,
                           aFirstPID,
                           aLastPID );

}


IDE_RC smrLogMgr::writeCreateTbsNTALogRec( idvSQL*    aStatistics,
                                           void     * aTrans,
                                           smLSN    * aLSN,
                                           scSpaceID  aSpaceID)
{
    return writeNTALogRec( aStatistics,
                           aTrans,
                           aLSN,
                           aSpaceID,
                           SMR_OP_SMM_CREATE_TBS,
                           0,
                           0 );

}


/* Disk로그의 Log 압축 여부를 결정한다

   [IN] aDiskLogWriteOption - 로그의 압축 여부가 들어있는 Option변수
   [IN] aLogHead - 로그압축 여부가 설정될 Log의 Head
 */
IDE_RC smrLogMgr::decideLogComp( UInt         aDiskLogWriteOption,
                                 smrLogHead * aLogHead )
{
    UChar  sFlag;

    if ( ( aDiskLogWriteOption & SMR_DISK_LOG_WRITE_OP_COMPRESS_MASK )
         == SMR_DISK_LOG_WRITE_OP_COMPRESS_TRUE )
    {
        // Log Head에 아무런 플래그도 설정하지 않으면
        // 기본적으로 로그를 압축한다.
    }
    else
    {
        // 로그를 압축하지 않도록 Log Head에 Flag를 설정한다.
        sFlag = smrLogHeadI::getFlag( aLogHead );
        smrLogHeadI::setFlag( aLogHead, sFlag|SMR_LOG_FORBID_COMPRESS_OK );
    }

    return IDE_SUCCESS;

}

/* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발

   Table/Index/Sequence의
   Create/Alter/Drop DDL에 대해 Query String을 로깅한다.

   로깅내용 ================================================
   4/8 byte - smOID
   4 byte - UserName Length
   n byte - UserName
   4 byte - Statement Length
   n byte - Statement
 */
IDE_RC smrLogMgr::writeDDLStmtTextLog( idvSQL         * aStatistics,
                                       void           * aTrans,
                                       smrDDLStmtMeta * aDDLStmtMeta,
                                       SChar          * aStmtText,
                                       SInt             aStmtTextLen )
{

    UInt       sLogSize;
    UInt       sLogTypeFlag;
    smTID      sTransID;
    smrLogHead sLogHead;
    smrLogType sLogType;


    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aStmtText != NULL );
    IDE_DASSERT( aStmtTextLen > 0 );

    idlOS::memset(&sLogHead, 0x00, ID_SIZEOF(sLogHead));

    smLayerCallback::initLogBuffer( aTrans );

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag );

    sLogSize = (ID_SIZEOF( smrLogHead )
                + ID_SIZEOF( smrDDLStmtMeta )
                + ID_SIZEOF( aStmtTextLen ) + aStmtTextLen + 1
                + ID_SIZEOF( smrLogType )) ;

    sLogType = SMR_LT_DDL_QUERY_STRING;
    smrLogHeadI::setType(&sLogHead, sLogType);
    smrLogHeadI::setSize(&sLogHead, sLogSize );
    smrLogHeadI::setTransID(&sLogHead, sTransID);

    smrLogHeadI::setPrevLSN( &sLogHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag(&sLogHead, SMR_LOG_TYPE_NORMAL);

    if ( (smrLogHeadI::getFlag(&sLogHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sLogHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLogHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    // Log Head 기록
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sLogHead,
                                                     ID_SIZEOF(sLogHead) )
              != IDE_SUCCESS );

    // Log Body 기록 : DDLStmtMeta
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( aDDLStmtMeta,
                                                     ID_SIZEOF(smrDDLStmtMeta) )
              != IDE_SUCCESS );

    // Log Body 기록 : 4 byte - Statement Text Length
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &aStmtTextLen,
                                                     ID_SIZEOF(aStmtTextLen) )
              != IDE_SUCCESS );

    // Log Body 기록 : aStmtTextLen bytes - Statement Text
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( aStmtText,
                                                     aStmtTextLen )
              != IDE_SUCCESS );
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer("", 1)
              != IDE_SUCCESS );

    // Log Tail 기록
    IDE_TEST( ((smxTrans*)aTrans)->writeLogToBuffer( &sLogType,
                                                     ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );

    IDE_TEST( writeLog( aStatistics,
                        aTrans,
                        smLayerCallback::getLogBufferOfTrans( aTrans ),
                        NULL,  // Previous LSN Ptr
                        NULL,  // Log LSN Ptr 
                        NULL,  // End LSN Ptr  
                        SM_NULL_OID )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그 Header(smrLogHead)의 previous undo LSN을 설정
 *
 * 로그의 header를 previous undo LSN을 설정하고, 로그의 body
 * 사이즈를 반환한다.
 *
 * + 2nd. code design
 *   - 주어진 aPPrvLSN이 NULL아니면, 로그 header의 prvUndoLSN을 주어진
 *     aPPrvLSN으로 설정한다.
 *   - 만약 NULL 이면, 트랜잭션이 마지막 로깅한 LSN을 얻어 로그 header의
 *     prvUndoLSN을 설정하며, 트랜잭션이 없는 경우에는 ID_UINT_MAX로
 *     설정한다.
 **********************************************************************/
void smrLogMgr::setLogHdrPrevLSN( void       * aTrans,
                                  smrLogHead * aLogHead,
                                  smLSN      * aPPrvLSN )
{
    UInt sLogFlag;
    UInt sLogTypeFlag;
    IDE_DASSERT( aLogHead != NULL );

    if ( aTrans != NULL )
    {
        // 이전에 로깅된 적이 한번도 없고,
        if ( smLayerCallback::getIsFirstLog( aTrans ) == ID_TRUE )
        {
            // Transaction ID가 부여되어 있다면
            if ( smLayerCallback::getTransID( aTrans ) != SM_NULL_TID )
            {
                // 로그 헤더에 BEGIN 플래그를 달아준다.
                smrLogHeadI::setFlag(aLogHead,
                                     smrLogHeadI::getFlag(aLogHead) | SMR_LOG_BEGINTRANS_OK);

                // BUG-15109
                // normal tx begin된 후 첫번째 log인 경우
                // 무조건 replication flag를 0으로 세팅한다.
                //PROJ-1608 Recovery From Replication
                sLogTypeFlag = smLayerCallback::getLogTypeFlagOfTrans(aTrans);
                if ( (sLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
                     (sLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY) )
                {
                    sLogFlag = smrLogHeadI::getFlag(aLogHead) & ~SMR_LOG_TYPE_MASK;
                    sLogFlag = sLogFlag | (sLogTypeFlag & SMR_LOG_TYPE_MASK);
                    smrLogHeadI::setFlag(aLogHead, sLogFlag);
                }

                smLayerCallback::setIsFirstLog( aTrans, ID_FALSE );
            }
        }
    }

    /* ------------------------------------------------
     * 1. prev LSN 설정
     * ----------------------------------------------*/
    if ( aPPrvLSN != NULL )
    {
        /* ------------------------------------------------
         *  prev LSN을 직접 지정하기 위해 인자로 받는 경우는
         *  NTA 로그의 preUndoLSN을 설정하기 위해서이다.
         * ----------------------------------------------*/
        smrLogHeadI::setPrevLSN( aLogHead, *aPPrvLSN );
    }
    else
    {
        /* ------------------------------------------------
         * 일반적으로 preUndoLSN은 해당 트랜잭션이 마지막으로
         * 작성했던 로그의 LSN을 얻어서 설정한다.
         * 하지만, 트랜잭션이 아닌경우는 ID_UINT_MAX로 설정한다.
         * ----------------------------------------------*/
        if ( aTrans != NULL )
        {
            smrLogHeadI::setPrevLSN( aLogHead, smLayerCallback::getLstUndoNxtLSN(aTrans) );
        }
        else
        {
            // To fix PR-3562
            smrLogHeadI::setPrevLSN( aLogHead,
                                     ID_UINT_MAX,  // FILEID
                                     ID_UINT_MAX ); // OFFSET
        }
    }

    return;
}

/* SMR_LT_FILE_BEGIN 로그 기록
 *
 * 이 함수는 다음과 같은 경로로 호출되며, 두가지 모두 lock() 호출후 들어온다.
 *
 * 1. writeLog 에서 lock() 잡고 -> reserveLogSpace -> writeFileBeginLog
 * 2. switchLogFileByForce 에서 lock() 잡고  -> writeFileBeginLog */
void smrLogMgr::writeFileBeginLog()
{
    smLSN       sLSN;

    IDE_ASSERT( mCurLogFile != NULL );

    mFileBeginLog.mFileNo = mCurLogFile->mFileNo ;

    IDE_DASSERT( mLstLSN.mLSN.mOffset == 0 );
    // 나중에 로그를 읽을때 Log의 Validity check를 위해
    // 로그가 기록되는 파일번호와 로그레코드의 파일내 Offset을 이용하여
    // Magic Number를 생성해둔다.
    smrLogHeadI::setMagic(&mFileBeginLog.mHead,
                          smrLogFile::makeMagicNumber(mLstLSN.mLSN.mFileNo,
                                                      mLstLSN.mLSN.mOffset));
    SM_SET_LSN( sLSN,
                mFileBeginLog.mFileNo,
                mLstLSN.mLSN.mOffset );
    
    smrLogHeadI::setLSN( &mFileBeginLog.mHead, sLSN );

    // File Begin Log는 항상 로그파일의 맨 처음에 기록된다.
    IDE_ASSERT( mCurLogFile->mOffset == 0 );

    // File Begin Log는 압축하지 않고 바로 기록한다.
    // 이유 :
    //     File의 첫번째 Log의 LSN을 읽는 작업을
    //     빠르게 수행하기 위함
    mCurLogFile->append( (SChar *)&mFileBeginLog,
                         smrLogHeadI::getSize(&mFileBeginLog.mHead) );

    // 현재 어느 LSN까지 로그를 기록했는지를 Setting한다.
    setLstWriteLSN( sLSN );

    // 현재 어느 LSN까지 로그를 기록했는지를 Setting한다.
    setLstLSN( mCurLogFile->mFileNo,
               mCurLogFile->mOffset );
}

/* SMR_LT_FILE_END 로그 기록
 *
 * 이 함수는 다음과 같은 경로로 호출되며, 두가지 모두 lock() 호출후 들어온다.
 *
 * 1. writeLog 에서 lock() 잡고 -> reserveLogSpace -> writeFileEndLog
 * 2. switchLogFileByForce 에서 lock() 잡고  -> writeFileEndLog */
void smrLogMgr::writeFileEndLog()
{
    smLSN       sLSN;

    // 나중에 로그를 읽을때 Log의 Validity check를 위해
    // 로그가 기록되는 파일번호와 로그레코드의 파일내 Offset을 이용하여
    // Magic Number를 생성해둔다.
    smrLogHeadI::setMagic (&mFileEndLog.mHead,
                           smrLogFile::makeMagicNumber( mLstLSN.mLSN.mFileNo,
                                                        mLstLSN.mLSN.mOffset ) );

    // 현재 로그가 기록될 LSN
    SM_SET_LSN( sLSN,
                mLstLSN.mLSN.mFileNo,
                mLstLSN.mLSN.mOffset );

    smrLogHeadI::setLSN( &mFileEndLog.mHead, sLSN );

    // 로그파일의 FILE END LOG는 압축하지 않은채로 기록한다.
    // 이유 : 로그파일의 Offset범위 체크를 쉽게 할 수 있도록 하기 위함
    mCurLogFile->append( (SChar *)&mFileEndLog,
                         smrLogHeadI::getSize(&mFileEndLog.mHead) );

    // 현재 어느 LSN까지 로그를 기록했는지를 Setting한다.
    setLstWriteLSN( sLSN );

    if ( smrRecoveryMgr::mSendXLogFunc != NULL )
    {
        smrRecoveryMgr::mSendXLogFunc( (const SChar*) &mFileEndLog );
    }
    else
    {
        /* Nothing to do */
    }
    /* BUG-35392 */
    if ( smrRecoveryMgr::mCopyToRPLogBufFunc != NULL )
    {
        /* BUG-32137     [sm-disk-recovery] The setDirty operation in DRDB causes
         * contention of LOG_ALLOCATION_MUTEX.
         * writeEndFileLog에서는 setLsnLSN을 하지 않는다.
         * FileEndLog 찍고 switchLogFile 수행한 후에 setLstLSN해야 올바른 LSN이
         * 설정된다. */

        // 압축하지 않은 원본로그를 Replication Log Buffer로 복사
        copyLogToReplBuffer( NULL,
                             (SChar *)&mFileEndLog,
                             smrLogHeadI::getSize(&mFileEndLog.mHead),
                             sLSN );
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : 로그를 기록할 공간 확보
 *
 * 로그를 기록할 공간을 현재 로그파일을 검사하여  없을 경우 로그파일을
 * switch한다. 이 때, mLstLSN을 새로운 로그파일의 LSN으로 재설정한다.
 * 또한, switch 횟수가 checkpoint interval에 만족하면, checkpoint를
 * 수행한다.
 * !!) 로그 관리자의 mutex를 획득한 이후에 호출되어야 한다.
 *
 * + 2nd. code design
 *   - 로그파일의 free 공간이 충분하지 않다면 로그파일을 switch시킨다.
 *
 * aLogSize           - [IN]  새로 기록하려는 로그 레코드의 크기
 * aIsLogFileSwitched - [OUT] aLogSize만큼 기록할만한 공간을 확보하던 중에
 *                            로그파일 Switch가 발생했는지의 여부
 **********************************************************************/
IDE_RC smrLogMgr::reserveLogSpace( UInt     aLogSize,
                                   idBool * aIsLogFileSwitched )
{
    static UInt    sLogFileEndSize;

    IDE_DASSERT( aLogSize > 0 );

    *aIsLogFileSwitched = ID_FALSE;

    /* 로그파일에 대한 마무리 로그의 길이 */
    sLogFileEndSize = SMR_LOGREC_SIZE(smrFileEndLog);

    /* ------------------------------------------------
     * 로그파일의 free 공간이 현재 기록할 로그길이와
     * 마무리 로그길이를 모두 기록할 수 있는지를 판단하여
     * 부족하면 switch 시킨다.
     * ----------------------------------------------*/
    if ( mCurLogFile->mFreeSize < ((UInt)aLogSize + sLogFileEndSize) )
    {
        writeFileEndLog();

        //switch log file
        IDE_TEST( mLogFileMgr.switchLogFile(&mCurLogFile) != IDE_SUCCESS );

        // 새 로그파일로 switch가 발생했기 때문에
        // 로그레코드가 기록될 위치인 로그파일의 offset은 0이어야 함.
        IDE_ASSERT( mCurLogFile->mOffset == 0 );

        /* BUG-32137 [sm-disk-recovery] The setDirty operation in DRDB causes
         * contention of LOG_ALLOCATION_MUTEX. */

        // 로그파일 Swtich가 발생하였으므로, 새로 로그가 기록될 LSN도 변경
        setLstLSN( mCurLogFile->mFileNo,
                   mCurLogFile->mOffset );

        *aIsLogFileSwitched = ID_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    // 아직 로그가 하나도 기록되지 않은 상태.
    // 파일의 첫번째 로그레코드로 File Begin Log를 기록한다.

    // 로그파일이 생성될 때 File Begin Log를 기록하지 않으며,
    // 이는 0번째 로그파일의 경우에도 마찬가지이다.
    // 일반 로그 기록중에 0번째 로그파일에 대해서도 기록하기 위해,
    // 위의 log file switch루틴과 별개로 수행한다.
    if ( mCurLogFile->mOffset == 0 )
    {
        writeFileBeginLog();
                                             
        // BUG-24701 설정된 로그 파일 사이즈 보다 aLogSize가 클경우 Error 처리한다.
        IDE_TEST_RAISE( ((UInt)aLogSize + sLogFileEndSize) >
                        mCurLogFile->mFreeSize, ERROR_INVALID_LOGSIZE );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_LOGSIZE );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_LogSizeExceedLogFileSize,
                                smuProperty::getLogFileSize(),
                                aLogSize));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 작성된 MRDB/DRDB 로그를 로그파일에 기록
 *
 * 로그파일에 로그를 기록하고, DRDB 로깅에 대해서는
 * Begin LSN과 End LSN을 반환하고, MRDB 로깅에 대해서는 Begin LSN을
 * 반환한다.
 *
 * - 2nd. code design
 *   + smrLogHead의 prev undo LSN을 설정한다.
 *   + 로깅을 하기 위해 로그 관리자의 lock을 획득하고,
 *     로그를 기록할 공간을 확인한다.
 *   + 로그의 begin LSN을 out 인자에 설정한다.
 *   + current 로그파일에 로그를 기록한다.
 *   + 기록한 로그의 vaildation을 검사한다.
 *   + 새로운 LSN을 last LSN 설정에 하고,
 *     DRDB 로그일 경우에는 aEndLSN을 설정한 다음
 *     로그 관리자의 lock을 푼다
 *   + 트랜재션의 경우 기록한 로그의 begin LSN을
 *     트랜잭션에 설정한다.
 *   + durability Type에 따라 commit 로그는 sync를
 *     보장하기도 한다.
 *
 **********************************************************************
 *   PROJ-1464 패러랠 로깅 관련 인자
 *
 *   aIsLogFileSwitched - [OUT] 로그를 기록하는 도중에
 *                              로그파일 Switch가 발생했는지의 여부
 *
 **********************************************************************/
IDE_RC smrLogMgr::writeLog( idvSQL   * aStatistics,
                            void     * aTrans,
                            SChar    * aRawLog,
                            smLSN    * aPPrvLSN,
                            smLSN    * aBeginLSN,
                            smLSN    * aEndLSN,
                            smOID      aTableOID )
{
    /* aTrans는 NULL일 수 있다. */
    IDE_DASSERT( aRawLog != NULL );
    /* aBeginLSN, aEndLSN은 NULL로 들어올 수 있다. */

    smrLogHead    * sLogHead;
    UInt            sRawLogSize;
    SChar         * sLogToWrite     = NULL;     /* 로그파일에 기록할 로그 */
    UInt            sLogSizeToWrite = 0;
    smLSN           sWrittenLSN;
    smrCompRes    * sCompRes    = NULL;
    UInt            sStage      = 0;
    UInt            sMinLogRecordSize;
    idBool          sIsLogFileSwitched;

    sIsLogFileSwitched = ID_FALSE;

    /* ------------------------------------------------
     * 주어진 버퍼를 smrLogHead로 casting한다.
     * ----------------------------------------------*/
    sLogHead = (smrLogHead *)aRawLog;

    sRawLogSize = smrLogHeadI::getSize(sLogHead);

    /* 로그 기록전에 수행할 작업들 처리 */
    onBeforeWriteLog( aTrans,
                      aRawLog,
                      aPPrvLSN );

    sMinLogRecordSize = smuProperty::getMinLogRecordSizeForCompress();

    if ( (sRawLogSize >= sMinLogRecordSize) && (sMinLogRecordSize != 0) )
    {
        /* 압축 리소스를 가져온다 */
        IDE_TEST( allocCompRes( aTrans, & sCompRes ) != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( tryLogCompression( sCompRes,
                                     aRawLog,
                                     sRawLogSize,
                                     &sLogToWrite,
                                     &sLogSizeToWrite,
                                     aTableOID ) != IDE_SUCCESS );
    }
    else
    {
        sLogToWrite     = aRawLog;
        sLogSizeToWrite = sRawLogSize;
    }

    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_FALSE )
    {
        IDE_TEST( lockAndWriteLog ( aStatistics,  /* for replication */
                                    aTrans,
                                    sLogToWrite,
                                    sLogSizeToWrite,
                                    aRawLog,      /* for replication */
                                    sRawLogSize,  /* for replication */
                                    &sWrittenLSN,
                                    aEndLSN,
                                    &sIsLogFileSwitched )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( lockAndWriteLog4FastUnlock ( aStatistics,  /* for replication */
                                               aTrans,
                                               sLogToWrite,
                                               sLogSizeToWrite,
                                               aRawLog,      /* for replication */
                                               sRawLogSize,  /* for replication */
                                               &sWrittenLSN,
                                               aEndLSN,
                                               &sIsLogFileSwitched )
                  != IDE_SUCCESS );
    }

    /* 이전에 로깅된 적이 한번도 없고,
     * 로그 기록후에 수행할 작업들 처리 */
    onAfterWriteLog( aStatistics,
                     aTrans,
                     sLogHead,
                     sWrittenLSN,
                     sLogSizeToWrite );

    if ( sStage == 1 )
    {
        /* 압축 리소스를 반납한다.
         * - 로그 기록중에 다른 Thread가 사용하지 못하도록
         *   로그 기록이 완료된 후에 반납해야 한다. */
        sStage = 0;
        IDE_TEST( freeCompRes( aTrans, sCompRes ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( aBeginLSN != NULL )
    {
       *aBeginLSN = sWrittenLSN;
    }
    else
    {
        /* nothing to do */
    }

    // 만약 로그파일 Switch가 발생했다면 로그파일 Switch횟수를 증가하고
    // 체크포인트를 수행해야 할 지의 여부를 결정한다.
    if ( sIsLogFileSwitched == ID_TRUE )
    {
        IDE_TEST( onLogFileSwitched() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch (sStage)
        {
            case 1:
                IDE_ASSERT( freeCompRes( aTrans, sCompRes )
                            == IDE_SUCCESS );
                break;

            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*
    압축 리소스를 가져온다
    - Transaction이 존재할 경우 Transaction에 매달려있는
      압축 리소스를 사용
    - Transaction이 존재하지 않을 경우
      공유하는 압축 리소스 풀인 mCompResPool을 사용

   [IN] aTrans - 트랜잭션
   [OUT] aCompRes - 할당된 압축 리소스
 */
IDE_RC smrLogMgr::allocCompRes( void        * aTrans,
                                smrCompRes ** aCompRes )
{
    IDE_DASSERT( aCompRes != NULL );

    smrCompRes * sCompRes ;

    if ( aTrans != NULL )
    {
        // Transaction의 압축 Resource를 가져온다.
        IDE_TEST( smLayerCallback::getTransCompRes( aTrans,
                                                    & sCompRes )
                  != IDE_SUCCESS );

        IDE_DASSERT( sCompRes != NULL );
    }
    else
    {
        IDE_TEST( mCompResPool.allocCompRes( & sCompRes ) != IDE_SUCCESS );
        IDE_DASSERT( sCompRes != NULL );
    }

    *aCompRes = sCompRes;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    압축 리소스를 반납한다.
    - Transaction이 존재할 경우 반납하지 않아도 된다.
      ( 추후 Transaction commit/rollback시 반납됨 )
    - Transaction이 존재하지 않을 경우
      공유하는 압축 리소스 풀인 mCompResPool로 반납

   [IN] aTrans - 트랜잭션
   [OUT] aCompRes - 반납할 압축 리소스
 */
IDE_RC smrLogMgr::freeCompRes( void       * aTrans,
                               smrCompRes * aCompRes )
{
    IDE_DASSERT( aCompRes != NULL );

    if ( aTrans != NULL )
    {
        // Transaction Rollback/Commit시 자동 반납됨
        // 아무것도 하지 않는다.
    }
    else
    {
        IDE_TEST( mCompResPool.freeCompRes( aCompRes ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   로그의 압축이 가능할 경우 압축 실시

   [IN] aCompRes           - 압축 리소스
   [IN] aRawLog            - 압축되기 전의 원본 로그
   [IN] aRawLogSize        - 압축되기 전의 원본 로그의 크기
   [OUT] aLogToWrite       - 로그파일에 기록할 로그
   [OUT] aLogSizeToWrite   - 로그파일에 기록할 로그의 크기
 */
IDE_RC smrLogMgr::tryLogCompression( smrCompRes    * aCompRes,
                                     SChar         * aRawLog,
                                     UInt            aRawLogSize,
                                     SChar        ** aLogToWrite,
                                     UInt          * aLogSizeToWrite,
                                     smOID           aTableOID )
{
    idBool       sDoCompLog;

    SChar      * sCompLog;        /* 압축된 로그 */
    UInt         sCompLogSize;

    SChar      * sLogToWrite;
    UInt         sLogSizeToWrite;

    // aTrans는 NULL일 수 있다.
    IDE_DASSERT( aRawLog != NULL );
    IDE_DASSERT( aRawLogSize > 0 );
    IDE_DASSERT( aLogToWrite != NULL );
    IDE_DASSERT( aLogSizeToWrite != NULL );

    // 기본적으로 비압축 로그를 사용
    sLogToWrite = aRawLog;
    sLogSizeToWrite = aRawLogSize;

    // 로그 압축해야하는지 여부 결정
    IDE_TEST( smrLogComp::shouldLogBeCompressed( aRawLog,
                                                 aRawLogSize,
                                                 &sDoCompLog )
              != IDE_SUCCESS );

    // 로그 압축 시도
    if ( sDoCompLog == ID_TRUE )
    {
        /* BUG-31009 - [SM] Compression buffer allocation need
         *                  exception handling.
         * 로그 압축에 실패한 경우 예외가 아니라 그냥 비압축 로그를 사용하도록
         * 한다. */
        if ( smrLogComp::createCompLog( & aCompRes->mCompBufferHandle,
                                        aRawLog,
                                        aRawLogSize,
                                        & sCompLog,
                                        & sCompLogSize,
                                        aTableOID ) == IDE_SUCCESS )
        {
            // 압축후 로그가 더 작아진 경우에만 압축로그로 기록
            if ( sCompLogSize < aRawLogSize  )
            {
                sLogToWrite = sCompLog;
                sLogSizeToWrite = sCompLogSize;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* 실패했으면 비압축 로그 사용 */
        }
    }
    else
    {
        /* nothing to do */
    }

    *aLogToWrite     = sLogToWrite;
    *aLogSizeToWrite = sLogSizeToWrite;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Log의 끝단 Mutex를 잡은 상태로 로그 기록 */
IDE_RC smrLogMgr::lockAndWriteLog( idvSQL   * aStatistics,
                                   void     * aTrans,
                                   SChar    * aRawOrCompLog,
                                   UInt       aRawOrCompLogSize,
                                   SChar    * aRawLog4Repl,
                                   UInt       aRawLogSize4Repl,
                                   smLSN    * aBeginLSN,
                                   smLSN    * aEndLSN,
                                   idBool   * aIsLogFileSwitched )
{
    smLSN        sLSN;
    idBool       sIsLocked          = ID_FALSE;
    idBool       sIsLock4NullTrans  = ID_FALSE;

    /* aTrans 는 NULL로 들어올 수 있다. */
    IDE_DASSERT( aRawOrCompLog      != NULL );
    IDE_DASSERT( aRawOrCompLogSize   > 0 );
    IDE_DASSERT( aRawLog4Repl       != NULL );
    IDE_DASSERT( aRawLogSize4Repl    > 0 );
    IDE_DASSERT( aBeginLSN          != NULL );
    /* aEndLSN은 NULL로 들어올 수 있다. */
    IDE_DASSERT( aIsLogFileSwitched != NULL );

    IDE_DASSERT( smuProperty::isFastUnlockLogAllocMutex() == ID_FALSE );

    /* 1. 로깅을 하기 위해 로그 관리자의 lock을 획득하고,
     *    로그를 기록할 공간을 확인한다.
     *  - 만약 로그파일 공간이 부족하다면, END 로그파일 로그를
     *      남기고 다음 로그파일로 switch 한다.
     *  - last LSN을 새로운 로그파일의 LSN으로 재설정한다.
     *  - 로그파일 switch 횟수가 checkpoint interval에 만족하면
     *      checkpoint를 수행한다. */

    IDE_TEST( lock() != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /* 1.check log size */
    /* 이 속에서 File End Log를 찍으면서 Log File Switch가 발생할 수 있다.
     * LSN값을 이보다 먼저 따게 되면, LSN값이 거꾸로 기록되는 현상이 발생한다. */
    IDE_TEST( reserveLogSpace( aRawOrCompLogSize,
                               aIsLogFileSwitched ) != IDE_SUCCESS );

    /* 2.로그의 출력 Begin LSN에 설정한다. */
    SM_SET_LSN( *aBeginLSN,
                mCurLogFile->mFileNo,
                mCurLogFile->mOffset );

    /* 3.현재 로그가 기록될 LSN */
    SM_SET_LSN( sLSN,
                mLstLSN.mLSN.mFileNo,
                mLstLSN.mLSN.mOffset );

    /* Log File을 통해 얻은 LSN과 mLstLSN을 통해 얻은 LSN이 같아야 한다 */
    IDE_DASSERT( smrCompareLSN::isEQ( aBeginLSN, &sLSN )
                 == ID_TRUE );

    /* 로그파일의 맨 처음에는 FIle Begin 로그가 찍히므로,
     * 일반로그가 로그파일의 Offset : 0 에 기록되어서는 안된다. */
    IDE_ASSERT( sLSN.mOffset != 0 );
    IDE_ASSERT( sLSN.mFileNo != ID_UINT_MAX );

    /* 4.Log에 LSN을 기록한다. 
     * smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
    smrLogHeadI::setLSN( (smrLogHead*)aRawOrCompLog, sLSN );

    /* 5.Log에 Magic Number를 기록한다.
     * smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
    smrLogHeadI::setMagic( (smrLogHead *)aRawOrCompLog,
                           smrLogFile::makeMagicNumber( sLSN.mFileNo, sLSN.mOffset) );
 
    /* 6.로그파일에 로그를 기록한다. */
    mCurLogFile->append( aRawOrCompLog, aRawOrCompLogSize );

    /* 7.현재 어느 LSN까지 로그를 기록했는지를 Setting한다. */
    setLstWriteLSN( sLSN );
 
    /* 8.새로운 end offset을 last LSN 설정에 하고, 
      lstLSN을 설정한 다음 로그 관리자의 lock을 푼다. */
    setLstLSN( mCurLogFile->mFileNo,
               mCurLogFile->mOffset );
  
    /* BUG-35392 */
    if ( smrRecoveryMgr::mCopyToRPLogBufFunc != NULL )
    {
        /* 압축하지 않은 원본로그를 Replication Log Buffer로 복사 */
        copyLogToReplBuffer( aStatistics,
                             aRawLog4Repl,
                             aRawLogSize4Repl,
                             sLSN );
    }
    else
    {
        /* nothing to do */
    }

    sIsLocked = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    // 9.여기 까지 sync 해야 한다. 
    if ( aEndLSN != NULL )
    {
        SM_SET_LSN( *aEndLSN,
                    mLstLSN.mLSN.mFileNo,
                    mLstLSN.mLSN.mOffset);
    }
    else
    {
        /* nothing to do */
    }

    if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY )
    {
        /* Log Buffer Type이 memory인 경우,
         * Update Transaction 수를
         * 증가시켜야 하는지 체크후 증가시킴 */
        checkIncreaseUpdateTxCount(aTrans);
    }
    else
    {
        /* nothing to do ... */
    }

    /* PROJ-2453 Eager Replication performance enhancement */
    if ( smrRecoveryMgr::mSendXLogFunc != NULL )
    {
        smrRecoveryMgr::mSendXLogFunc( aRawLog4Repl );
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_PUSH();
        IDE_ASSERT( unlock() == IDE_SUCCESS );
        IDE_POP();
    }

    if ( sIsLock4NullTrans == ID_TRUE )
    {
        IDE_DASSERT( aTrans == NULL );

        sIsLock4NullTrans = ID_FALSE;
        IDE_ASSERT( mMutex4NullTrans.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* Log의 끝단 Mutex를 잡은 상태로 로그 기록 */
IDE_RC smrLogMgr::lockAndWriteLog4FastUnlock( idvSQL   * aStatistics,
                                              void     * aTrans,
                                              SChar    * aRawOrCompLog,
                                              UInt       aRawOrCompLogSize,
                                              SChar    * aRawLog4Repl,
                                              UInt       aRawLogSize4Repl,
                                              smLSN    * aBeginLSN,
                                              smLSN    * aEndLSN,
                                              idBool   * aIsLogFileSwitched )
{
    smLSN        sLSN;
    UInt         sSlotID            = 0;
    idBool       sIsLocked          = ID_FALSE;
    idBool       sIsSetFstChkLSN    = ID_FALSE;
    idBool       sIsLock4NullTrans  = ID_FALSE;
    smrLogFile * sCurLogFile        = NULL;
    smLSN        sEndLSN;
#ifdef DEBUG
    smLSN        sEndLSN4Debug;
#endif

    /* aTrans 는 NULL로 들어올 수 있다. */
    IDE_DASSERT( aRawOrCompLog      != NULL );
    IDE_DASSERT( aRawOrCompLogSize   > 0 );
    IDE_DASSERT( aRawLog4Repl       != NULL );
    IDE_DASSERT( aRawLogSize4Repl    > 0 );
    IDE_DASSERT( aBeginLSN          != NULL );
    /* aEndLSN은 NULL로 들어올 수 있다. */
    IDE_DASSERT( aIsLogFileSwitched != NULL );

    IDE_DASSERT( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE );
    /* 1. 로깅을 하기 위해 로그 관리자의 lock을 획득하고,
     *    로그를 기록할 공간을 확인한다.
     *  - 만약 로그파일 공간이 부족하다면, END 로그파일 로그를
     *      남기고 다음 로그파일로 switch 한다.
     *  - last LSN을 새로운 로그파일의 LSN으로 재설정한다.
     *  - 로그파일 switch 횟수가 checkpoint interval에 만족하면
     *      checkpoint를 수행한다. */

    /* BUG-35392 */
    /* aTrans는 NULL로 들어올 수 있다. */
    if ( aTrans != NULL )
    {
        sSlotID = (UInt)smLayerCallback::getTransSlot( aTrans );
    }
    else
    {   /* 시스템 트랜잭션은 한번에 하나만 실행 가능 */
        sSlotID = mFstChkLSNArrSize - 1 ;

        IDE_TEST( mMutex4NullTrans.lock( NULL ) != IDE_SUCCESS );
        sIsLock4NullTrans = ID_TRUE;
    }

    /* 안전하게 쓰여진 최대 LSN을 구하기 위해 체크 */
    setFstCheckLSN( sSlotID );
    sIsSetFstChkLSN = ID_TRUE;

    IDE_TEST( lock() != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /* 1.check log size */
    /* 이 속에서 File End Log를 찍으면서 Log File Switch가 발생할 수 있다.
     * LSN값을 이보다 먼저 따게 되면, LSN값이 거꾸로 기록되는 현상이 발생한다. */
    IDE_TEST( reserveLogSpace( aRawOrCompLogSize,
                               aIsLogFileSwitched ) != IDE_SUCCESS );

    /* 2.확보된 로그 크기 만큼 로그 파일의 크기를 변경한다. */
    mCurLogFile->mFreeSize -= aRawOrCompLogSize;
    mCurLogFile->mOffset   += aRawOrCompLogSize;

    /* 3.현재 로그가 기록될 LSN */
    SM_SET_LSN( sLSN,
                mLstLSN.mLSN.mFileNo,
                mLstLSN.mLSN.mOffset );

    /* 4.현재 로그의 마지막 offset */
    SM_SET_LSN( sEndLSN,
                mLstLSN.mLSN.mFileNo,
                mLstLSN.mLSN.mOffset+aRawOrCompLogSize );

#ifdef DEBUG
    SM_SET_LSN( sEndLSN4Debug,
                mCurLogFile->mFileNo,
                mCurLogFile->mOffset );

    /* Log File을 통해 얻은 LSN과 mLstLSN을 통해 얻은 LSN이 같아야 한다 */
    IDE_DASSERT( smrCompareLSN::isEQ( &sEndLSN4Debug, &sEndLSN )
                 == ID_TRUE );
#endif   

    /* 5.현재 어느 LSN까지 로그를 기록했는지를 Setting한다. */
    setLstWriteLSN( sLSN );
 
    /* 6.새로운 end offset을 last LSN 설정에 하고, 
      lstLSN을 설정한 다음 로그 관리자의 lock을 푼다. */
    setLstLSN( sEndLSN.mFileNo,
               sEndLSN.mOffset );

    /* BUG-35392 */
    /* 압축하지 않은 원본로그를 Replication Log Buffer로 복사 */
    if ( smrRecoveryMgr::mCopyToRPLogBufFunc != NULL )
    {
        copyLogToReplBuffer( aStatistics,
                             aRawLog4Repl,
                             aRawLogSize4Repl,
                             sLSN );
    }
    else
    {
        /* nothing to do */
    }

    /* 7.AllocMutex를 unlock하면 mCurLogFile 값이 변경 될 수 있다.
     *    이후 처리를 위해 복사해둔다. */
    sCurLogFile = mCurLogFile;

    sIsLocked = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    /* 로그파일의 맨 처음에는 FIle Begin 로그가 찍히므로,
     * 일반로그가 로그파일의 Offset : 0 에 기록되어서는 안된다. */
    IDE_ASSERT( sLSN.mOffset != 0 );

    /* 1.Log에 LSN을 기록한다.
     * smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
    smrLogHeadI::setLSN( (smrLogHead*)aRawOrCompLog, sLSN );

    /* 2.Log에 Magic Number를 기록 한다.
     * smrCompResPool은 8Byte align 이 맞는 상태 : memcpy 없이 설정한다 */
    smrLogHeadI::setMagic( (smrLogHead *)aRawOrCompLog,
                           smrLogFile::makeMagicNumber( sLSN.mFileNo, sLSN.mOffset) );

    /* 3.로그파일에 로그를 기록한다. */
    sCurLogFile->scribble( aRawOrCompLog,
                           aRawOrCompLogSize,
                           sLSN.mOffset );

    /* 4.로그의 출력 Begin LSN에 설정한다. */
    SM_GET_LSN( *aBeginLSN, sLSN );

    /* 5.여기 까지 sync 해야 한다. */
    if ( aEndLSN != NULL )
    {
        SM_GET_LSN( *aEndLSN, sEndLSN );
    }
    else
    {
        /* nothing to do */
    }

    sIsSetFstChkLSN = ID_FALSE;
    unsetFstCheckLSN( sSlotID );

    if ( sIsLock4NullTrans == ID_TRUE )
    {
        IDE_DASSERT( aTrans == NULL );

        sIsLock4NullTrans = ID_FALSE;
        IDE_TEST( mMutex4NullTrans.unlock() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY )
    {
        /* Log Buffer Type이 memory인 경우,
         * Update Transaction 수를
         * 증가시켜야 하는지 체크후 증가시킴 */
        checkIncreaseUpdateTxCount(aTrans);
    }
    else
    {
        /* nothing to do ... */
    }

    /* PROJ-2453 Eager Replication performance enhancement */
    if ( smrRecoveryMgr::mSendXLogFunc != NULL )
    {
        smrRecoveryMgr::mSendXLogFunc( aRawLog4Repl );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_PUSH();
        IDE_ASSERT( unlock() == IDE_SUCCESS );
        IDE_POP();
    }

    if ( sIsSetFstChkLSN == ID_TRUE )
    {
        sIsSetFstChkLSN = ID_FALSE;
        unsetFstCheckLSN( sSlotID );
    }

    if ( sIsLock4NullTrans == ID_TRUE )
    {
        IDE_DASSERT( aTrans == NULL );

        sIsLock4NullTrans = ID_FALSE;
        IDE_ASSERT( mMutex4NullTrans.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 *  압축하지 않은 원본로그를 Replication Log Buffer로 복사
 *
 *  - Replication Log Buffer는 Sender의 성능 최적화를 위해
 *    로그를 Memory로부터 즉시 읽기 위한 버퍼이다.
 *
 *  - 압축되지 않은 원본 로그를 기록 하여 빠른 처리를 보장한다.
 *
 * [IN] aRawLog     - 압축되지 않은 원본로그
 * [IN] aRawLogSize - 압축되지 않은 원본로그의 크기
 * [IN] aLSN        - 로그의 LSN
 *********************************************************************/
void smrLogMgr::copyLogToReplBuffer( idvSQL * aStatistics,
                                     SChar  * aRawLog,
                                     UInt     aRawLogSize,
                                     smLSN    aLSN )
{
    IDE_DASSERT( aRawLog    != NULL );
    IDE_DASSERT( aRawLogSize > 0 );
    IDE_DASSERT( smrLogComp::isCompressedLog( aRawLog ) == ID_FALSE );

    /* 원본 Log에 LSN을 기록한다. */
    smrLogHeadI::setLSN( (smrLogHead*)aRawLog, aLSN );

    /* 원본 Log에 Magic Number를 기록한다. */
    smrLogHeadI::setMagic( (smrLogHead *)aRawLog,
                           smrLogFile::makeMagicNumber( aLSN.mFileNo, aLSN.mOffset) );

    /* Replication log Buffer manager에게 처리를 요청한다. */
    smrRecoveryMgr::mCopyToRPLogBufFunc( aStatistics,
                                         aRawLogSize,
                                         aRawLog,
                                         aLSN );
}


/*
    로그 기록전에 수행할 작업 처리
      - Prev LSN기록
      - Commit Log에 시각 기록
 */
void smrLogMgr::onBeforeWriteLog( void     * aTrans,
                                  SChar    * aRawLog,
                                  smLSN    * aPPrvLSN )
{
    // aTrans 는 NULL로 들어올 수 있다.
    IDE_DASSERT( aRawLog != NULL );
    // aPPrvLSN 은 NULL로 들어올 수 있다.

    smrLogHead*              sLogHead;
    smrTransCommitLog      * sCommitLog;
    smrTransGroupCommitLog * sGroupCommitLog;

    sLogHead = (smrLogHead*)aRawLog;

    // 원본로그는 압축되지 않은 로그여야 한다.
    IDE_DASSERT( (smrLogHeadI::getFlag(sLogHead) & SMR_LOG_COMPRESSED_MASK)
                 ==  SMR_LOG_COMPRESSED_NO );

    /* ------------------------------------------------
     * 1. smrLogHead의 prev undo LSN을 설정한다.
     * ----------------------------------------------*/
    setLogHdrPrevLSN(aTrans, sLogHead, aPPrvLSN);

    /* 만약 tx commit 로그라면 time value를 저장한다. */
    if ( (smrLogHeadI::getType(sLogHead) == SMR_LT_MEMTRANS_COMMIT) ||
         (smrLogHeadI::getType(sLogHead) == SMR_LT_DSKTRANS_COMMIT) )
    {
        sCommitLog = (smrTransCommitLog*)aRawLog;
        sCommitLog->mTxCommitTV = smLayerCallback::getCurrTime();
    }
    else
    {
        /* BUG-47525 Group Commit도 time value를 저장해야한다. */
        if ( smrLogHeadI::getType(sLogHead) == SMR_LT_MEMTRANS_GROUPCOMMIT )
        {
            sGroupCommitLog = (smrTransGroupCommitLog*)aRawLog;
            sGroupCommitLog->mTxCommitTV = smLayerCallback::getCurrTime();
        }
    }

#ifdef DEBUG
   /* ------------------------------------------------
    * 2. 버퍼에 기록된 로그의 Head와 Tail이 일치하는지 비교
    * ----------------------------------------------*/
    validateLogRec( aRawLog );
#endif
}

/*
    로그 기록후에 수행할 작업들 처리

    [IN] aTrans   - Transaction
    [IN] aLogHead - 기록한 로그의 Head
    [IN] aLSN     - 기록한 로그의 LSN
    [IN] aWrittenLogSize - 기록한 로그의 크기
 */
void smrLogMgr::onAfterWriteLog( idvSQL     * aStatistics,
                                 void       * aTrans,
                                 smrLogHead * aLogHead,
                                 smLSN        aLSN,
                                 UInt         aWrittenLogSize )
{

    /* 1. 트랜재션의 경우 기록한 로그의 begin LSN과 Last LogLSN을
     *    트랜잭션에 설정한다. */
    updateTransLSNInfo( aStatistics,
                        aTrans,
                        &aLSN,
                        aWrittenLogSize );

    if ( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MEMORY )
    {
        // BUG-15396
        // Log Buffer Type이 memory일 경우, Update Transaction 수를
        // 감소시켜야 하는지 체크후 감소시킴
        // Group Commit 시 필요한 정보로, mmap인 경우에는 속도가
        // 빠르기 때문에 group commit 할 필요가 없어서 설정하지 않음
        checkDecreaseUpdateTxCount(aTrans, aLogHead);
    }
    else
    {
        /* nothing to do ... */
    }
}



/*
  Transaction이 로그를 기록할 때
  Update Transaction의 수를 증가시켜야 하는지 체크한다.

  - 하나의 트랜잭션에 대해 로그가 최초 기록될 때 1 증가
  - Restart Recovery중에는 Count하지 않음
  - 여기서 계산한 Update Transaction의 수로 Group Commit을 실시할지의 여부결정

  aTrans   [IN] 로그를 기록하려는 트랜잭션 객체
  aLogHead [IN] 기록하려는 로그의 Head
*/
inline void smrLogMgr::checkIncreaseUpdateTxCount( void       * aTrans )
{
    // Restart Recovery중에는 Active Transaction들의 Rollback시
    // 로그를 한번도 기록하지 않고 Abort Log를 기록할 수 있다.
    //
    // Normal Processing일때만 Update Transaction을 Count한다.
    if ( smrRecoveryMgr::isRestartRecoveryPhase() == ID_FALSE )
    {
        if ( aTrans != NULL )
        {
            // Log를 기록하는 시점까지 ReadOnly라면,
            // 이 트랜잭션은 최초로 로그를 기록하는 것이다.
            if ( smLayerCallback::isReadOnly( aTrans ) == ID_TRUE )
            {
                // Update Transaction 수를 하나 증가시킨다.
                incUpdateTxCount();

                // smrLogMgr::writeLog에서
                // smLayerCallback::setLstUndoNxtLSN(aTrans, &sLSN)
                // 을 호출할 때 트랜잭션이 Update Transaction으로 설정된다.
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
}

/*
  Transaction이 로그를 기록할 때
  Update Transaction의 수를 감소시켜야 하는지 체크한다.

  - Commit이나 Abort로그가 기록될 때 1감소
  - Restart Recovery중에는 Count하지 않음
  - 여기서 계산한 Update Transaction의 수로 Group Commit을 실시할지의 여부결정

  aTrans   [IN] 로그를 기록하려는 트랜잭션 객체
  aLogHead [IN] 기록하려는 로그의 Head
*/
inline void smrLogMgr::checkDecreaseUpdateTxCount( void       * aTrans,
                                                   smrLogHead * aLogHead )
{
    UInt sCnt;
    UInt sGroupCnt;
    smrTransGroupCommitLog * sGroupCommitLog;

    // Restart Recovery중에는 Active Transaction들의 Rollback시
    // 로그를 한번도 기록하지 않고 Abort Log를 기록할 수 있다.
    //
    // Normal Processing일때만 Update Transaction을 Count한다.
    if ( smrRecoveryMgr::isRestartRecoveryPhase() == ID_FALSE )
    {
        if ( aTrans != NULL )
        {
            // Log를 기록하는 시점까지 ReadOnly라면,
            // 이 트랜잭션은 최초로 로그를 기록하는 것이다.
            if ( smLayerCallback::isReadOnly( aTrans ) == ID_FALSE )
            {
                // Update Transaction이 마지막으로 찍는 로그라면?
                if ( (smrLogHeadI::getType( aLogHead ) == SMR_LT_MEMTRANS_COMMIT) ||
                     (smrLogHeadI::getType( aLogHead ) == SMR_LT_DSKTRANS_COMMIT) ||
                     (smrLogHeadI::getType( aLogHead ) == SMR_LT_MEMTRANS_ABORT)  ||
                     (smrLogHeadI::getType( aLogHead ) == SMR_LT_DSKTRANS_ABORT) )
                {
                    // Update Transaction의 수를 하나 감소한다.
                    decUpdateTxCount();
                }
                else
                {
                    /* BUG-47525 Group Commit의 경우 Group 화 된 Tx 수만큼
                     * Update Transaction 수를 줄여주어야 한다. 
                     * Group화된 나머지 Tx들은 writeLog 를 호출하지 않기 때문 */
                    if ( smrLogHeadI::getType( aLogHead ) == SMR_LT_MEMTRANS_GROUPCOMMIT )
                    {
                        sGroupCommitLog = (smrTransGroupCommitLog*) aLogHead;
                        sGroupCnt =  sGroupCommitLog->mGroupCnt;
                        for ( sCnt = 0; sCnt < sGroupCnt; sCnt++)
                        {
                            decUpdateTxCount();
                        }
                    }
                }
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
// assertion code
#if defined(DEBUG)
        // Update Transaction이 마지막으로 찍는 로그라면?
        if ( (smrLogHeadI::getType( aLogHead ) == SMR_LT_MEMTRANS_COMMIT) ||
             (smrLogHeadI::getType( aLogHead ) == SMR_LT_DSKTRANS_COMMIT) ||
             (smrLogHeadI::getType( aLogHead ) == SMR_LT_MEMTRANS_ABORT)  ||
             (smrLogHeadI::getType( aLogHead ) == SMR_LT_DSKTRANS_ABORT)  ||
             (smrLogHeadI::getType( aLogHead ) == SMR_LT_MEMTRANS_GROUPCOMMIT) )
        {
            // Transaction 없이 Commit/Abort 불가
            IDE_DASSERT( aTrans != NULL );
            // Readonly Transaction은 Commit/Abort로그를 찍을 수 없음
            IDE_DASSERT( smLayerCallback::isReadOnly( aTrans ) == ID_FALSE );
        }
        else
        {
            /* nothing to do */
        }
#endif /* DEBUG */
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : 특정 로그파일의 첫번째 로그의 Head를 읽는다.
 *
 * aFileNo  - [IN] 읽어들일 로그파일의 번호
 * aLogHead - [OUT] 읽어들인 로그의 Header를 넣을 Output Parameter
 ***********************************************************************/
IDE_RC smrLogMgr::readFirstLogHeadFromDisk( UInt         aFileNo,
                                            smrLogHead * aLogHead )
{
    SChar    sLogFileName[SM_MAX_FILE_NAME];
    iduFile  sFile;
    SInt     sState = 0;

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    idlOS::snprintf( sLogFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s%"ID_UINT32_FMT,
                     getLogPath(),
                     IDL_FILE_SEPARATOR,
                     SMR_LOG_FILE_NAME,
                     aFileNo );

    IDE_TEST( sFile.setFileName( sLogFileName )
              != IDE_SUCCESS );

    IDE_TEST( sFile.open( ID_FALSE ) != IDE_SUCCESS );
    sState = 2;

    // 로그파일 전체가 아니고 첫번째 로그의 Head만 읽어 들인다.
    IDE_TEST( sFile.read( NULL,
                          0,
                          (void*)aLogHead,
                          ID_SIZEOF(smrLogHead) )
              != IDE_SUCCESS );

    // 로그 파일의 첫번째 로그는 압축하지 않는다.
    // 이유 :
    //     File의 첫번째 Log의 LSN을 읽는 작업을
    //     빠르게 수행하기 위함
    IDE_ASSERT( smrLogComp::isCompressedLog( (SChar*)aLogHead ) == ID_FALSE );

    sState = 1;
    IDE_TEST( sFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        switch( sState )
        {
            case 2:
                (void)sFile.close();
            case 1:
                (void)sFile.destroy();
            default:
                break;
        }
        IDE_POP();
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :
 * 강제로 현재 로그파일을 switch시킨다.
 * Switch된 로그파일을 끝까지 sync한 후, archive 시킨다.
 **********************************************************************/
IDE_RC smrLogMgr::switchLogFileByForce()
{

    UInt               sSwitchedLogFileNo;
    UInt               sSwitchedLogOffset;
    UInt               sLogFileEndSize;
    UInt               sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    sLogFileEndSize = SMR_LOGREC_SIZE(smrFileEndLog);

    IDE_ASSERT( mCurLogFile->mFreeSize >= sLogFileEndSize );

    // 로그파일의 끝이므로 File End Log를 기록한다.
    writeFileEndLog();

    sSwitchedLogFileNo = mCurLogFile->mFileNo;
    sSwitchedLogOffset = mCurLogFile->mOffset;

    IDE_TEST( mLogFileMgr.switchLogFile( &mCurLogFile ) != IDE_SUCCESS );

    // 새 로그파일로 switch가 발생했기 때문에
    // 로그레코드가 기록될 위치인 로그파일의 offset은 0이어야 함.
    IDE_ASSERT( mCurLogFile->mOffset == 0 );
    IDE_DASSERT( sSwitchedLogFileNo + 1 == mCurLogFile->mFileNo );

    /* BUG-32137 [sm-disk-recovery] The setDirty operation in DRDB causes
     * contention of LOG_ALLOCATION_MUTEX. */
    // 로그파일 Swtich가 발생하였으므로, 새로 로그가 기록될 LSN도 변경
    setLstLSN( mCurLogFile->mFileNo,
               mCurLogFile->mOffset );

    // 아직 로그가 하나도 기록되지 않은 상태.
    // 파일의 첫번째 로그레코드로 File Begin Log를 기록한다.
    writeFileBeginLog();

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    // 해당 로그파일까지 sync시키고 archive list에 추가한다.
    IDE_TEST( mLFThread.syncOrWait4SyncLogToLSN(
                                      SMR_LOG_SYNC_BY_SYS,
                                      sSwitchedLogFileNo,
                                      sSwitchedLogOffset,
                                      NULL )   // aSyncedLFCnt
              != IDE_SUCCESS );

    // 해당 로그파일이 archive 될때까지 기다린다.
    IDE_TEST( mArchiveThread.wait4EndArchLF( sSwitchedLogFileNo )
              != IDE_SUCCESS );

    // lock/unlock사이에서 에러가 Raise될 수 없으므로, stage 처리 안한다.
    IDE_TEST( lockLogSwitchCount() != IDE_SUCCESS );

    // 로그파일 switch를 수행하였으므로,
    // 로그파일 switch counter를 증가해준다.
    mLogSwitchCount++;

    IDE_TEST( unlockLogSwitchCount() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        (void)unlock();
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든 LogFile을 조사해서 aMinLSN보다 작거나 같은 LSN을 가지는 로그를
 *               첫번째로 가지는 LogFile No를 구해서 aNeedFirstFileNo에 넣어준다.
 *
 * aMinLSN          - [IN]  Minimum Log Sequence Number
 * aFirstFileNo     - [IN]  check할 Logfile 중 첫번째 File No들
 * aEndFileNo       - [IN]  check할 Logfile 중 마지막 File No들
 * aNeedFirstFileNo - [OUT] aMinLSN값보다 큰 값을 가진 첫번째 로그 File No
 ***********************************************************************/
IDE_RC smrLogMgr::getFirstNeedLFN( smLSN          aMinLSN,
                                   const UInt     aFirstFileNo,
                                   const UInt     aEndFileNo,
                                   UInt         * aNeedFirstFileNo )
{
    IDE_DASSERT( aFirstFileNo <= aEndFileNo );
  
    if ( aFirstFileNo <= aMinLSN.mFileNo )
    {
        if ( aEndFileNo >= aMinLSN.mFileNo )
        {
            *aNeedFirstFileNo = aMinLSN.mFileNo; 
        }
        else
        {
            /* BUG-43974 EndLSN보다 큰 LSN을 요청하였을 경우
             * EndLSN을 넘겨주어야 한다. */
            *aNeedFirstFileNo = aEndFileNo;
        }
    }
    else
    {
        /* BUG-15803: Replication이 보내야할 로그의 위치를 찾을때
         *  자신의 mLSN보다 작은 값을 가진 logfile이 없을때 
         *  첫번째 파일을 선택한다.*/
        *aNeedFirstFileNo = aFirstFileNo;
    }

    return IDE_SUCCESS;
}

/*
 * 마지막 LSN까지 Sync한다.
 */
IDE_RC smrLogMgr::syncToLstLSN( smrSyncByWho   aWhoSyncLog )
{
    smLSN sSyncLstLSN ;
    SM_LSN_INIT( sSyncLstLSN );
    getLstLSN( &sSyncLstLSN );

    IDE_TEST( syncLFThread( aWhoSyncLog, & sSyncLstLSN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * File Begin Log를 구성한다.
 *
 * aFileBeginLog [IN] - 초기화할 File Begin Log의 주소
 */
void smrLogMgr::initializeFileBeginLog( smrFileBeginLog * aFileBeginLog )
{
    IDE_DASSERT( aFileBeginLog != NULL );

    smrLogHeadI::setType( &aFileBeginLog->mHead, SMR_LT_FILE_BEGIN );
    smrLogHeadI::setTransID( &aFileBeginLog->mHead, SM_NULL_TID );
    smrLogHeadI::setSize( &aFileBeginLog->mHead, SMR_LOGREC_SIZE(smrFileBeginLog) );
    smrLogHeadI::setFlag( &aFileBeginLog->mHead, SMR_LOG_TYPE_NORMAL );
    aFileBeginLog->mFileNo = ID_UINT_MAX;
    smrLogHeadI::setPrevLSN( &aFileBeginLog->mHead,
                             ID_UINT_MAX,  // FILENO
                             ID_UINT_MAX ); // OFFSET
    aFileBeginLog->mTail = SMR_LT_FILE_BEGIN;
    smrLogHeadI::setReplStmtDepth( &aFileBeginLog->mHead,
                                   SMI_STATEMENT_DEPTH_NULL );
}

/*
 * File End Log를 구성한다.
 *
 * aFileEndLog [IN] - 초기화할 File End Log의 주소
 */
void smrLogMgr::initializeFileEndLog( smrFileEndLog * aFileEndLog )
{
    IDE_DASSERT( aFileEndLog != NULL );

    smrLogHeadI::setType( &aFileEndLog->mHead, SMR_LT_FILE_END );
    smrLogHeadI::setTransID( &aFileEndLog->mHead, SM_NULL_TID );
    smrLogHeadI::setSize( &aFileEndLog->mHead, SMR_LOGREC_SIZE(smrFileEndLog) );
    smrLogHeadI::setFlag( &aFileEndLog->mHead, SMR_LOG_TYPE_NORMAL );
    smrLogHeadI::setPrevLSN( &aFileEndLog->mHead,
                             ID_UINT_MAX,  // FILENO
                             ID_UINT_MAX); // OFFSET
    aFileEndLog->mTail = SMR_LT_FILE_END;
    smrLogHeadI::setReplStmtDepth( &aFileEndLog->mHead,
                                   SMI_STATEMENT_DEPTH_NULL );
}

/* Transaction이 로그를 기록후에 다음과 같은 정보를 갱신한다.
 * 1. Transaction의 첫번째 로그라면 Transaction의 Begin LSN정보를 갱신
 * 2. Transaction의 마지막으로 기록한 로그 LSN
 *
 * aTrans  - [IN] Transaction 포인터
 * aLstLSN - [IN] Log의 LSN
 */
void smrLogMgr::updateTransLSNInfo( idvSQL  * aStatistics,
                                    void    * aTrans,
                                    smLSN   * aLstLSN,
                                    UInt      aLogSize )
{
    idvSQL     *sSqlStat;
    idvSession *sSession;

    if ( aTrans != NULL )
    {
        sSqlStat = aStatistics;

        if (sSqlStat != NULL )
        {
            sSession = sSqlStat->mSess;

            IDV_SESS_ADD( sSession, IDV_STAT_INDEX_REDOLOG_COUNT, 1 );
            IDV_SESS_ADD( sSession, IDV_STAT_INDEX_REDOLOG_SIZE,  (ULong)aLogSize );
        }
        else
        {
            /* nothing to do */
        }

        IDE_ASSERT( aLstLSN != NULL );
        smLayerCallback::setLstUndoNxtLSN( aTrans, *aLstLSN );
    }
    else
    {
        /* nothing to do */
    }
}

/* 현재 로그가 Valid한지 조사한다. 로그 Header에 있는 Type과
 * Tail값이 동일한지 조사 같지 않으면 서버를 돌아가시게 한다. */
void smrLogMgr::validateLogRec( SChar * aRawLog )
{
    smrLogHead    * sTmpLogHead;
    smrLogType      sLogTypeInTail;
    smrLogType      sLogTypeInHead;

    sTmpLogHead = (smrLogHead *)aRawLog;

    idlOS::memcpy( &sLogTypeInTail,
                   ((SChar *)(aRawLog + smrLogHeadI::getSize(sTmpLogHead) -
                            (UInt)ID_SIZEOF(smrLogTail))),
                   ID_SIZEOF(smrLogTail) );

    sLogTypeInHead = smrLogHeadI::getType(sTmpLogHead);

    /* invalid log 일 경우 */
    if ( sLogTypeInHead != sLogTypeInTail )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_DEBUG,
                     SM_TRC_MRECOVERY_LOGFILEGROUP_INVALID_LOGTYPE,
                     smrLogHeadI::getType(sTmpLogHead),
                     sLogTypeInTail,
                     mCurLogFile->mFileNo,
                     mCurLogFile->mOffset );

        IDE_ASSERT(0);
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : interval에 의한 checkpoint 수행후 switch count 초기화
 *
 ***********************************************************************/
IDE_RC smrLogMgr::clearLogSwitchCount()
{
    
    IDE_TEST( lockLogSwitchCount() != IDE_SUCCESS );

    mLogSwitchCount = mLogSwitchCount % (smuProperty::getChkptIntervalInLog());

    IDE_TEST( unlockLogSwitchCount() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  로그파일이 Switch될 때마다 불리운다.로그파일
 *                switch Count를 1 증가시키고 체크포인트를 수행해야
 *                할 지의 여부를 결정한다.
 *
 **********************************************************************/
IDE_RC smrLogMgr::onLogFileSwitched()
{
    UInt              sStage  = 0;

    IDE_TEST( lockLogSwitchCount() != IDE_SUCCESS );
    sStage = 1;

    // 로그 파일 Switch가 발생했으므로, Count를 1증가
    mLogSwitchCount++;

    /* ------------------------------------------------
     * 로그파일 switch 횟수가 checkpoint interval에 만족하면
     * checkpoint를 수행한다.
     * ----------------------------------------------*/
    if ( smuProperty::getChkptEnabled() != 0 )
    {
        /* BUG-18740 server shutdown시 checkpoint실패하여 비정상종료
         * 할 수 있습니다. smrRecoveryMgr::destroy시에는 이미 checkpoint
         * thread는 종료 되었다. 때문에 checkpoint thread에 대한 resumeAndnoWait
         * 를 호출하면 안된다. */
        if ( ( smrRecoveryMgr::isRestart() == ID_FALSE ) &&
             ( smrRecoveryMgr::isFinish() == ID_FALSE ) )
        {
            if ( (mLogSwitchCount >= smuProperty::getChkptIntervalInLog() ) &&
                 (smuProperty::getChkptEnabled() != 0))
            {
                IDE_TEST( gSmrChkptThread.resumeAndNoWait(
                                             SMR_CHECKPOINT_BY_LOGFILE_SWITCH,
                                             SMR_CHKPT_TYPE_BOTH )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do ... */
            }
        }
        else
        {
            /* nothing to do ... */
        }
    }
    else
    {
        /* nothing to do ... */
    }

    sStage = 0;
    IDE_TEST( unlockLogSwitchCount() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1 :
            (void) unlockLogSwitchCount();
    }
    
    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 * Log FlushThread가 aLSNToSync에 지정된 LSN까지 Sync수행.
 *
 * 현재 이 함수는 입력으로 들어온 LSN보다 더 많이 sync되도록
 * 구현되어 있으며, 이는 DBMS의 consistency를 해치지 않는다.
 *
 * aLSNToSync    - [IN] sync시킬 LSN
 */
IDE_RC smrLogMgr::syncLFThread( smrSyncByWho   aWhoSync,
                                smLSN        * aLSNToSync )
{
    // 현재 smrLFThread는 특정 LSN까지 Log를 Flush하는 interface를
    // 지원하지 않으며, 특정 Log File의 끝까지 Log를 Flush하는
    // 인터페이스만 지원한다.
    //
    // 현재 이 함수는 입력으로 들어온 LSN보다 더 많이 sync되도록
    // 구현되어 있으며, 이는 DBMS의 consistency를 해치지 않는다.
    //
    // 성능을 위해서는 해당 로그파일의 해당 offset까지, 즉, 특정 LSN까지
    // sync를 하도록 구현할 필요가 있다.

    /* BUG-17702:[MCM] Sync Thread가 의한 모든 log를 Flush할때까지
     * Checkpint Thread가 기다리는 경우가 발생함.
     *
     * 현재 Sync가 필요한 LSN까지만 Sync되면 나머지 checkpoint작업을
     * 수행하여야 합니다.
     */
    IDE_TEST( mLFThread.syncOrWait4SyncLogToLSN(
                                          aWhoSync,
                                          aLSNToSync->mFileNo,
                                          aLSNToSync->mOffset,
                                          NULL)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : sync된 파일의 처음 LSN (file begin의 LSN) 을 반환한다. 
 * aLSN - [OUT] sync된 file begin의 LSN
 **********************************************************************/
IDE_RC smrLogMgr::getSyncedMinFirstLogLSN( smLSN *aLSN )
{
    smLSN           sSyncedLSN;
    smLSN           sLSN;
    smrLogHead      sLogHead;
    smLSN           sMinLSN;
    smLSN           sTmpLSN;
    smrLogFile    * sLogFilePtr = NULL;
    SChar         * sLogPtr     = NULL;
    UInt            sLogSizeAtDisk;
    idBool          sIsValid    = ID_FALSE;
    idBool          sIsFileOpen = ID_FALSE;

    IDE_TEST( mLFThread.getSyncedLSN (&sSyncedLSN ) != IDE_SUCCESS );
    
    SM_LSN_MAX( sMinLSN );

    sLSN.mFileNo = sSyncedLSN.mFileNo;
    sLSN.mOffset = 0;

    sIsFileOpen = ID_TRUE;
    IDE_TEST( readLogInternal( NULL,
                               &sLSN,
                               ID_TRUE,
                               &sLogFilePtr,
                               &sLogHead,
                               &sLogPtr,
                               &sLogSizeAtDisk )
              != IDE_SUCCESS );

    /*
     * fix BUG-21725
     */
    sIsValid = smrLogFile::isValidLog( &sLSN,
                                       &sLogHead,
                                       sLogPtr,
                                       sLogSizeAtDisk );

    sIsFileOpen = ID_FALSE;
    IDE_TEST( closeLogFile( sLogFilePtr ) != IDE_SUCCESS );
    sLogFilePtr = NULL;


    /* getSyncedLSN를 통해 반환되는 값 중, 아직 아무것도 sync되지 않은
     * 파일이 offset 0으로 반환될 수 있다.
     * 그러므로 각 파일의 첫 번째 로그를 읽었을 때 invalid한 상태가 될 수 있다.
     */
    if ( sIsValid == ID_TRUE )
    {
        sTmpLSN = smrLogHeadI::getLSN( &sLogHead );
        if ( SM_IS_LSN_MAX( sMinLSN ) )
        {
            SM_GET_LSN( sMinLSN, sTmpLSN );
        }
        else
        {
            /* 이럴수 있나 ? 
             * 어쩌튼 디버그에서만 확인하자 */
            IDE_DASSERT_MSG( SM_IS_LSN_MAX( sMinLSN ),
                             "Invalid Log LSN \n"
                             "LogLSN : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                             "TmpLSN : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                             "MinLSN : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n",
                             sLSN.mFileNo, 
                             sLSN.mOffset,
                             sTmpLSN.mFileNo,
                             sTmpLSN.mOffset,
                             sMinLSN.mFileNo,
                             sMinLSN.mOffset );
                             
            if ( smrCompareLSN::isLT( &sTmpLSN, &sMinLSN ) )
            {
                SM_GET_LSN( sMinLSN, sTmpLSN );
            }
            else
            {
                /* nothing to do ... */
            }
        }
    }
    else
    {
        /* 아직 file begin 로그가 기록되지 않았다.
         * Sync된 file의 begin로그의 LSN을 알 수 없다면
         * 디스크에 sync되었다는 것을 보장할 수 있는 최소 LSN을 알 수 없으므로
         * SM_SN_NULL로 반환해 준다.*/
        SM_LSN_MAX( sMinLSN );
    }

    SM_GET_LSN( *aLSN, sMinLSN );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    if ( ( sIsFileOpen != ID_FALSE ) && ( sLogFilePtr != NULL ) )
    {
        IDE_PUSH();
        (void)closeLogFile( sLogFilePtr );
        IDE_POP();
    }
    else
    {
        /* nothing to do ... */
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 특정 LSN의 log record와 해당 log record가 속한 로그
 *               파일을 리턴한다.
 *
 * aDecompBufferHandle  - [IN] 압축 해제 버퍼의 핸들
 * aLSN            - [IN] log record를 읽어올 LSN.
 *                   LSN에는 Log File Group의 ID도 있으므로, 이를 통해
 *                   여러개의 Log File Group중 하나를 선택하고,
 *                   그 속에 기록된 log record를 읽어온다.
 * aIsCloseLogFile - [IN] aLSN이 *aLogFile이 가리키는 LogFile에 없다면
 *                   aIsCloseLogFile이 TRUE일 경우 *aLogFile을 Close하고,
 *                   새로운 LogFile을 열어야 한다.
 * aLogFile - [IN-OUT] 로그 레코드가 속한 로그파일 포인터
 * aLogHead - [OUT] 로그 레코드의 Head
 * aLogPtr  - [OUT] 로그 레코드가 기록된 로그 버퍼 포인터
 * aReadSize - [OUT] 파일상에서 읽어낸 로그의 크기
 *                   ( 압축된 로그의 경우 로그의 크기와
 *                     파일상의 크기가 다를 수 있다 )
 *
 * 주의: 마지막으로 Read하고 난 후 aLogFile가 가리키는 LogFile을
 *       smrLogMgr::readLog을 호출한 쪽에서 반드시 Close해야합니다.
 *       그리고 aIsCloseLogFile가 ID_FALSE일 경우 여러개의 logfile이 open
 *       되어 있을 수 있기 때문에 반드시 자신이 열었던 파일을 close해
 *       줘야 합니다. 예를 들면 redo시에 ID_FALSE를 넘기는데 여기서는
 *       closeAllLogFile를 이용해서 file을 close합니다.
 *
 ***********************************************************************/
IDE_RC smrLogMgr::readLogInternal( iduMemoryHandle  * aDecompBufferHandle,
                                   smLSN            * aLSN,
                                   idBool             aIsCloseLogFile,
                                   smrLogFile      ** aLogFile,
                                   smrLogHead       * aLogHead,
                                   SChar           ** aLogPtr,
                                   UInt             * aLogSizeAtDisk )
{
    // 비압축 로그를 읽는 경우 aDecompBufferHandle이 NULL로 들어온다
    IDE_DASSERT( aLSN     != NULL );
    IDE_DASSERT( aLogFile != NULL );
    IDE_DASSERT( aLogHead != NULL );
    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aLogSizeAtDisk != NULL );
    
    smrLogFile    * sLogFilePtr;

    sLogFilePtr = *aLogFile;
   
    if ( sLogFilePtr != NULL )
    {
        if ( (aLSN->mFileNo != sLogFilePtr->getFileNo()) )
        {
            if ( aIsCloseLogFile == ID_TRUE )
            {
                IDE_TEST( closeLogFile(sLogFilePtr) != IDE_SUCCESS );
                *aLogFile = NULL; 
            }
            else
            {
                /* nothing to do ... */
            }

            /* FIT/ART/rp/Bugs/BUG-17185/BUG-17185.tc */
            IDU_FIT_POINT( "1.BUG-17185@smrLFGMgr::readLog" );

            IDE_TEST( openLogFile( aLSN->mFileNo,
                                   ID_FALSE,
                                   &sLogFilePtr )
                      != IDE_SUCCESS );
            *aLogFile = sLogFilePtr;
        }
        else
        {
            /* aLSN이 가리키는 로그는 *aLogFile에 있다.*/
        }
    }
    else
    {
        IDE_TEST( openLogFile( aLSN->mFileNo,
                               ID_FALSE,
                               &sLogFilePtr )
                  != IDE_SUCCESS );
        *aLogFile = sLogFilePtr;
    }

    IDE_TEST( smrLogComp::readLog( aDecompBufferHandle,
                                   sLogFilePtr,
                                   aLSN->mOffset,
                                   aLogHead,
                                   aLogPtr,
                                   aLogSizeAtDisk )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aLSN이 가리키는 로그파일의 첫번째 Log 의 Head를 읽는다
 *
 * aDecompBufferHandle  - [IN] 압축 해제 버퍼의 핸들
 * aLSN      - [IN] 특정 로그파일상의 첫번째 로그의 LSN
 * aLogHead  - [OUT] 읽어들인 Log의 Head를 넘겨줄 Parameter
 **********************************************************************/
IDE_RC smrLogMgr::readFirstLogHead( smLSN      *aLSN,
                                    smrLogHead *aLogHead )
{
    IDE_DASSERT( aLSN != NULL );
    IDE_DASSERT( aLogHead != NULL );
    
    smrLogFile *sLogFilePtr = NULL;
    idBool      sIsOpen     = ID_FALSE;
    SChar      *sLogPtr     = NULL;
    SInt        sState      = 0;

    // 로그파일상의 첫번째 로그이므로 Offset은 0이어야 한다.
    IDE_ASSERT( aLSN->mOffset == 0 );
    
    /* aLSN에 해당하는 Log를 가진 LogFile이 open되어 있으면 Reference
       Count값만을 증가시키고 없으면 그냥 리턴한다. */
    IDE_TEST( mLogFileMgr.checkLogFileOpenAndIncRefCnt( aLSN->mFileNo,
                                                        &sIsOpen,
                                                        &sLogFilePtr )
              != IDE_SUCCESS );

    if ( sIsOpen == ID_FALSE )
    {
        /* open되어있지 않기때문에 */
        IDE_TEST( readFirstLogHeadFromDisk( aLSN->mFileNo,
                                            aLogHead )
                 != IDE_SUCCESS );
    }
    else
    {
        sState = 1;
        
        IDE_ASSERT(sLogFilePtr != NULL);
        
        sLogFilePtr->read( 0, &sLogPtr );
        
        /* 로그파일의 첫번째 로그는 압축하지 않는다.
           압축 고려하지 않고 바로 읽는다.
           
           이유 :
              File의 첫번째 Log의 LSN을 읽는 작업을,
              빠르게 수행하기 위함
         */
        IDE_ASSERT( smrLogComp::isCompressedLog( sLogPtr ) == ID_FALSE );

        idlOS::memcpy( aLogHead, sLogPtr, ID_SIZEOF( *aLogHead ) );
        
        sState = 0;
        IDE_TEST( closeLogFile(sLogFilePtr) != IDE_SUCCESS );
        sLogFilePtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_PUSH();
        (void)closeLogFile(sLogFilePtr);
        IDE_POP();
    }
    else
    {
        /* nothing to do ... */
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Dummy Log를 기록한다.
 *
 ***********************************************************************/
IDE_RC smrLogMgr::writeDummyLog()
{
    smrDummyLog       sDummyLog;

    smrLogHeadI::setType( &sDummyLog.mHead, SMR_LT_DUMMY );
    smrLogHeadI::setTransID( &sDummyLog.mHead, SM_NULL_TID );
    smrLogHeadI::setSize( &sDummyLog.mHead, SMR_LOGREC_SIZE(smrDummyLog) );
    smrLogHeadI::setFlag( &sDummyLog.mHead, SMR_LOG_TYPE_NORMAL );
    smrLogHeadI::setPrevLSN( &sDummyLog.mHead,
                             ID_UINT_MAX,  // FILENO
                             ID_UINT_MAX ); // OFFSET
    sDummyLog.mTail = SMR_LT_DUMMY;
    smrLogHeadI::setReplStmtDepth( &sDummyLog.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    // 해당 Transaction의 Log File Group에 로그를 기록한다.
    IDE_TEST( writeLog( NULL,  // idvSQL* 
                        NULL,  // Transaction Ptr
                        (SChar*)&sDummyLog, 
                        NULL,  // Previous LSN Ptr
                        NULL,  // Log LSN Ptr
                        NULL,  // End LSN Ptr
                        SM_NULL_OID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2118 BUG Reporting
 *               Server Fatal 시점에 Signal Handler 가 호출할
 *               Debugging 정보 기록함수
 *
 *               이미 altibase_dump.log 에 lock을 잡고 들어오므로
 *               lock을 잡지않는 trace 기록 함수를 사용해야 한다.
 *
 **********************************************************************/
void smrLogMgr::writeDebugInfo()
{
    smLSN sDiskLstLSN;
    SM_LSN_INIT( sDiskLstLSN );

    if ( isAvailable() == ID_TRUE )
    {
        getLstLSN( &sDiskLstLSN );

        ideLog::logMessage( IDE_DUMP_0,
                            "====================================================\n"
                            " Storage Manager - Last Disk Log LSN\n"
                            "====================================================\n"
                            "LstRedoLSN        : [ %"ID_UINT32_FMT" , %"ID_UINT32_FMT" ]\n"
                            "====================================================\n",
                            sDiskLstLSN.mFileNo,
                            sDiskLstLSN.mOffset );
    }
    else
    {
        /* nothing to do ... */
    }
}

/* BUG-35392
 * 지정된 LstLSN(Offset) 까지 sync가 완료되기를 대기한다. */
void smrLogMgr::waitLogSyncToLSN( smLSN  * aLSNToSync,
                                  UInt     aSyncWaitMin,
                                  UInt     aSyncWaitMax )
{
    UInt            sTimeOutMSec    = aSyncWaitMin;
    smLSN           sMinUncompletedLstLSN;
    PDL_Time_Value  sTimeOut;

    if ( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE )
    {
        /* Sync 가능한(Dummy Log를 포함하지 않는) 최대의 LSN을(Offset) 찾는다. */
        getUncompletedLstLSN( &sMinUncompletedLstLSN );

        while ( smrCompareLSN::isGT( aLSNToSync, &sMinUncompletedLstLSN ) == ID_TRUE )
        {
            sTimeOut.set( 0, sTimeOutMSec );
            idlOS::sleep( sTimeOut );

            sTimeOutMSec <<= 1;

            if ( sTimeOutMSec > aSyncWaitMax )
            {
                sTimeOutMSec = aSyncWaitMax;
            }
            else
            {
                /* nothing to do ... */
            }

            getUncompletedLstLSN( &sMinUncompletedLstLSN );
        } // end of while
    }
    else
    {
        /* do nothing */
    }
}

/* BUG-35392 
 *   logfile0 에 20까지 로그가 기록되었다면 
 *   +---------------------------------------------
 *   + [FileNo, offset] | .....   [0,10] | [0,20] |        
 *   +----------------------------------------------
 *                                      (A)      (B) 
 *   (A) : mLstWriteLSN    (B) : mLstLSN
 */

void smrLogMgr::rebuildMinUCSN()
{
    UInt                    i;
    UInt                    j;
    UInt                    sIdx;
    UInt                    sStartIdx;
    UInt                    sEndtIdx;
    smrUncompletedLogInfo   sMinUncompletedLstLSN;
    smrUncompletedLogInfo   sFstChkLSN;

    IDE_DASSERT( smuProperty::isFastUnlockLogAllocMutex() == ID_TRUE );

    /* 마지막 로그 레코드의 offset (B) 을 가져온다. */
    sMinUncompletedLstLSN.mLstLSN.mSync = mLstLSN.mSync; 

    if ( sMinUncompletedLstLSN.mLstLSN.mSync == 0 )
    {
        SM_SYNC_LSN_MAX( sMinUncompletedLstLSN.mLstLSN.mSync );
    }
    else
    {
        /* nothing to do */
    }

    /* 마지막 로그 레코드가 기록된 LSN (A) 을 가져온다.*/
    sMinUncompletedLstLSN.mLstWriteLSN.mSync = mLstWriteLSN.mSync;

    if ( sMinUncompletedLstLSN.mLstWriteLSN.mSync == 0 )
    {
        SM_SYNC_LSN_MAX( sMinUncompletedLstLSN.mLstWriteLSN.mSync );
    }
    else
    {
        /* nothing to do */
    }

    /* 1. Last LSN을 먼저 구하지 않으면
     *    loop를 탐색하는 도중에 놓칠 수 있다.
     * 2. Last LSN도 log copy 중인 LSN일수도 있다.
     *   따로 구하여 비교한다. */

    sIdx = ( mFstChkLSNArrSize / SMR_CHECK_LSN_UPDATE_GROUP ) + 1;
    for ( i = 0 ; i < sIdx ; i++ )
    {
        if ( mFstChkLSNUpdateCnt[i] != 0 ) 
        {
            sStartIdx = i * SMR_CHECK_LSN_UPDATE_GROUP;
            sEndtIdx  = IDL_MIN( (i + 1) * SMR_CHECK_LSN_UPDATE_GROUP, mFstChkLSNArrSize ); 

            for ( j = sStartIdx ; j < sEndtIdx ; j++ )
            {
                //mLstLSN 구하기 
                sFstChkLSN.mLstLSN.mSync = mFstChkLSNArr[j].mLstLSN.mSync;

                if ( sFstChkLSN.mLstLSN.mLSN.mFileNo < sMinUncompletedLstLSN.mLstLSN.mLSN.mFileNo )
                {
                    sMinUncompletedLstLSN.mLstLSN.mSync = sFstChkLSN.mLstLSN.mSync;
                }
                else
                {
                    if ( ( sFstChkLSN.mLstLSN.mLSN.mFileNo == sMinUncompletedLstLSN.mLstLSN.mLSN.mFileNo ) &&
                         ( sFstChkLSN.mLstLSN.mLSN.mOffset < sMinUncompletedLstLSN.mLstLSN.mLSN.mOffset ) )
                    {
                        sMinUncompletedLstLSN.mLstLSN.mSync = sFstChkLSN.mLstLSN.mSync;
                    }
                    else
                    {
                        /* nothing to do */
                    }
                } //else


                sFstChkLSN.mLstWriteLSN.mSync = mFstChkLSNArr[j].mLstWriteLSN.mSync;
                if ( sFstChkLSN.mLstWriteLSN.mLSN.mFileNo < sMinUncompletedLstLSN.mLstWriteLSN.mLSN.mFileNo )
                {
                    sMinUncompletedLstLSN.mLstWriteLSN.mSync = sFstChkLSN.mLstWriteLSN.mSync;
                }
                else
                {
                    if ( ( sFstChkLSN.mLstWriteLSN.mLSN.mFileNo == sMinUncompletedLstLSN.mLstWriteLSN.mLSN.mFileNo ) &&
                         ( sFstChkLSN.mLstWriteLSN.mLSN.mOffset < sMinUncompletedLstLSN.mLstWriteLSN.mLSN.mOffset ) )
                    {
                        sMinUncompletedLstLSN.mLstWriteLSN.mSync = sFstChkLSN.mLstWriteLSN.mSync;
                    }
                    else
                    {
                        /* nothing to do */
                    }
                } //else

            } //for
        } //if
        else
        {
            /* nothing to do */
        }
    } //for

    /* 마지막 로그 레코드의 offset (B) 을 설정한다 */
    mUncompletedLSN.mLstLSN.mSync = sMinUncompletedLstLSN.mLstLSN.mSync;

    /* 마지막 로그 레코드가 기록된 LSN (A) 을 설정한다.*/
    mUncompletedLSN.mLstWriteLSN.mSync = sMinUncompletedLstLSN.mLstWriteLSN.mSync;
}
