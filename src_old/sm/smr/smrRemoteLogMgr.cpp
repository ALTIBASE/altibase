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
 * PROJ-1915
 * Off-line 센더를 위한 LFGMgr 오직 로그 읽기 기능 만을 수행 한다.
 *
 **********************************************************************/
#include <smuProperty.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smr.h>
#include <smrReq.h>
#include <smiMain.h>

#define SMR_LOG_BUFFER_CHUNK_LIST_COUNT (1)
#define SMR_LOG_BUFFER_CHUNK_ITEM_COUNT (5)

smrRemoteLogMgr::smrRemoteLogMgr()
{
}

smrRemoteLogMgr::~smrRemoteLogMgr()
{
}

/***********************************************************************
 * Description : 로그 그룹 관리자 초기화
 *
 * aLogFileSize - [IN] off-line Log File Size
 * aLogDirPath  - [IN] LogDirPath array
 * aNotDecomp   - [IN] 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환)
 **********************************************************************/
IDE_RC smrRemoteLogMgr::initialize(ULong    aLogFileSize,
                                   SChar ** aLogDirPath,
                                   idBool   aNotDecomp )
{
    UInt sStage = 0;

    mNotDecomp = aNotDecomp;

    setLogFileSize(aLogFileSize);

    mLogDirPath = NULL;

    sStage = 1;
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                 SM_MAX_FILE_NAME,
                                 (void **)&mLogDirPath,
                                 IDU_MEM_FORCE) != IDE_SUCCESS );

    idlOS::memset(mLogDirPath, 0x00, SM_MAX_FILE_NAME);

    setLogDirPath( *aLogDirPath );

    IDE_TEST( checkLogDirExist() != IDE_SUCCESS );

    IDE_TEST( mMemPool.initialize( IDU_MEM_SM_SMR,
                                   (SChar *)"OPEN_REMOTE_LOGFILE_MEM_LIST",
                                   1,/* List Count */
                                   ID_SIZEOF(smrLogFile),
                                   SMR_LOG_FULL_SIZE,
                                   IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                   ID_TRUE,								/* UseMutex */
                                   IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,		/* AlignByte */
                                   ID_FALSE,							/* ForcePooling */
                                   ID_TRUE,								/* GarbageCollection */
                                   ID_TRUE,                             /* HWCacheLine */
                                   IDU_MEMPOOL_TYPE_LEGACY              /* mempool type */ ) 
              != IDE_SUCCESS);			
    sStage = 2;

    IDE_TEST( setRemoteLogMgrsInfo() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch (sStage)
    {
        case 2:
            IDE_ASSERT( mMemPool.destroy() == IDE_SUCCESS );
        case 1:
            if ( mLogDirPath != NULL )
            {
                IDE_ASSERT( iduMemMgr::free(mLogDirPath)
                            == IDE_SUCCESS );
                mLogDirPath = NULL;
            }
            else
            {
                /* nothing to do ... */
            }

        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 로그 그룹 관리자 해제
 *
 * :initialize 의 역순으로 수행한다.
 *
 **********************************************************************/
IDE_RC smrRemoteLogMgr::destroy()
{
    IDE_TEST( mMemPool.destroy() != IDE_SUCCESS );

    if ( mLogDirPath != NULL )
    {
        IDE_TEST( iduMemMgr::free(mLogDirPath) != IDE_SUCCESS );
        mLogDirPath = NULL;
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aLSN이 가리키는 로그파일의 첫번째 Log 의 Head를 읽는다
 *
 * aLSN      - [IN]  특정 로그파일상의 첫번째 로그의 LSN
 * aLogHead  - [OUT] 읽어들인 Log의 Head를 넘겨줄 Parameter
 * aIsValid  - [OUT] 읽어들인 로그의 Valid 여부
 **********************************************************************/
IDE_RC smrRemoteLogMgr::readFirstLogHead(smLSN      * aLSN,
                                         smrLogHead * aLogHead,
                                         idBool     * aIsValid)
{
    IDE_DASSERT(aLSN     != NULL );
    IDE_DASSERT(aLogHead != NULL );
    IDE_DASSERT( aIsValid != NULL );

    // 로그파일상의 첫번째 로그이므로 Offset은 0이어야 한다.
    IDE_ASSERT( aLSN->mOffset == 0 );

    IDE_TEST( readFirstLogHeadFromDisk( aLSN, aLogHead, aIsValid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aFirstFileNo에서 aEndFileNo사이의
 *               aMinLSN.mFileNo를 aNeedFirstFileNo에 넣어준다.
 *
 * aMinLSN          - [IN]  Minimum Log Sequence Number
 * aFirstFileNo     - [IN]  check할 Logfile 중 첫번째 File No
 * aEndFileNo       - [IN]  check할 Logfile 중 마지막 File No
 * aNeedFirstFileNo - [OUT] aMinLSN값보다 큰 값을 가진 첫번째 로그 File No
 **********************************************************************/
IDE_RC smrRemoteLogMgr::getFirstNeedLFN( smLSN        aMinLSN,
                                         const UInt   aFirstFileNo,
                                         const UInt   aEndFileNo,
                                         UInt       * aNeedFirstFileNo )
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

/***********************************************************************
 * Description : 마지막으로 기록한 log의
 *               LSN값을 리턴 한다.
 **********************************************************************/
void smrRemoteLogMgr::getLstLSN( smLSN * aLstLSN )
{
    //setRemoteLogMgrsInfo() 에서 정해 진 값을 리턴 한다.
    *aLstLSN = mRemoteLogMgrs.mLstLSN;
};

/***********************************************************************
 * Description : aLogFile을 Close한다.
 *
 * aLogFile - [IN] close할 로그파일
 **********************************************************************/
IDE_RC smrRemoteLogMgr::closeLogFile( smrLogFile * aLogFile )
{
    IDE_DASSERT( aLogFile != NULL );

    IDE_ASSERT( aLogFile->close() == IDE_SUCCESS );

    IDE_ASSERT( aLogFile->destroy() == IDE_SUCCESS );

    IDE_TEST( mMemPool.memfree(aLogFile) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 특정 로그파일의 첫번째 로그레코드의 Head를 File로부터
 *               직접 읽는다
 *
 * aLSN     - [IN]  읽어들일 로그의 LSN ( Offset이 0으로 세팅 )
 * aLogHead - [OUT] 읽어들인 로그의 Header를 넣을 Output Parameter
 * aIsValid - [OUT] 읽어들인 로그의 Valid 여부
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::readFirstLogHeadFromDisk(smLSN      * aLSN,
                                                 smrLogHead * aLogHead,
                                                 idBool     * aIsValid )
{
    SChar   sLogFileName[SM_MAX_FILE_NAME];
    iduFile sFile;
    SInt    sState = 0;

    IDE_DASSERT(aLogHead != NULL );
    IDE_DASSERT( aIsValid != NULL );

    IDE_ASSERT( aLSN != NULL );
    IDE_ASSERT( aLSN->mOffset == 0 );

    IDE_TEST( sFile.initialize(IDU_MEM_SM_SMR,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    idlOS::snprintf( sLogFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s%"ID_UINT32_FMT,
                     getLogDirPath(),
                     IDL_FILE_SEPARATOR,
                     SMR_LOG_FILE_NAME,
                     aLSN->mFileNo );

    IDE_TEST( sFile.setFileName(sLogFileName) != IDE_SUCCESS );

    IDE_TEST( sFile.open(ID_FALSE, O_RDONLY) != IDE_SUCCESS );
    sState = 2;

    // 로그파일 전체가 아니고 첫번째 로그의 Head만 읽어 들인다.
    IDE_TEST( sFile.read( NULL,
                          0,
                          (void *)aLogHead,
                         ID_SIZEOF(smrLogHead) )
              != IDE_SUCCESS );

    /*
     *  BUG-39240
     */
    // 로그 파일의 첫번째 로그는 압축하지 않는다.
    // 이유 :
    //     File의 첫번째 Log의 LSN을 읽는 작업을
    //     빠르게 수행하기 위함
    if ( ( smrLogFile::isValidMagicNumber( aLSN, aLogHead ) == ID_TRUE ) &&
         ( smrLogComp::isCompressedLog((SChar *)aLogHead) == ID_FALSE ) )
    {
        *aIsValid = ID_TRUE;
    }
    else
    {
        *aIsValid = ID_FALSE;
    }

    sState = 1;
    IDE_TEST( sFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        switch(sState)
        {
            case 2:
                IDE_ASSERT( sFile.close() == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );
            default:
                break;
        }
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 특정 LSN의 log record와 해당 log record가 속한 로그
 *               파일을 리턴한다.
 *
 * aDecompBufferHandle  - [IN] 압축 해제 버퍼의 핸들
 * aLSN                 - [IN] log record를 읽어올 LSN.
 *                             LSN에는 Log File Group의 ID도 있으므로,
 *                             이를 통해 여러개의 Log File Group중 하나를
 *                             선택하고, 그 속에 기록된 log record를 읽어온다.
 * aIsCloseLogFile      - [IN] aLSN이 *aLogFile이 가리키는 LogFile에 없다면
 *                             aIsCloseLogFile이 TRUE일 경우 *aLogFile을
 *                             Close하고, 새로운 LogFile을 열어야 한다.
 *
 * aLogFile  - [IN-OUT] 로그 레코드가 속한 로그파일 포인터
 * aLogHead  - [OUT] 로그 레코드의 Head
 * aLogPtr   - [OUT] 로그 레코드가 기록된 로그 버퍼 포인터
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
IDE_RC smrRemoteLogMgr::readLog(iduMemoryHandle * aDecompBufferHandle,
                                smLSN           * aLSN,
                                idBool            aIsCloseLogFile,
                                smrLogFile     ** aLogFile,
                                smrLogHead      * aLogHead,
                                SChar          ** aLogPtr,
                                UInt            * aLogSizeAtDisk)
{
    smrLogFile * sLogFilePtr;

    // 비압축 로그를 읽는 경우 aDecompBufferHandle이 NULL로 들어온다
    IDE_ASSERT( aLSN     != NULL );
    IDE_ASSERT( aLogFile != NULL );
    IDE_ASSERT( aLogHead != NULL );
    IDE_ASSERT( aLogPtr  != NULL );
    IDE_ASSERT( aLogSizeAtDisk != NULL );    

    sLogFilePtr = *aLogFile;

    if ( sLogFilePtr != NULL )
    {
        if ( aLSN->mFileNo != sLogFilePtr->getFileNo() )
        {
            if ( aIsCloseLogFile == ID_TRUE )
            {
                IDE_TEST( closeLogFile( sLogFilePtr ) != IDE_SUCCESS );
                *aLogFile = NULL;
            }


            IDE_TEST( openLogFile( aLSN->mFileNo,
                                   ID_FALSE/*ReadOnly*/,
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
                               ID_FALSE /*ReadOnly*/,
                               &sLogFilePtr )
                  != IDE_SUCCESS );
        *aLogFile = sLogFilePtr;
    }

    if ( mNotDecomp == ID_FALSE )
    {
        /* 압축된 로그파일의 경우 압축을 풀어서 레코드를 읽어온다.  */
        IDE_TEST( smrLogComp::readLog( aDecompBufferHandle,
                                       sLogFilePtr,
                                       aLSN->mOffset,
                                       aLogHead,
                                       aLogPtr,
                                       aLogSizeAtDisk)
                 != IDE_SUCCESS );
    }
    else 
    {
        /* 압축된 로그파일의 경우 압축된 형태 그대로 반환한다. */
        IDE_TEST( smrLogComp::readLog4RP( sLogFilePtr,
                                          aLSN->mOffset,
                                          aLogHead,
                                          aLogPtr,
                                          aLogSizeAtDisk)
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aLSN이 가리키는 Log를 읽어서 Log가 위치한 Log Buffer의
 *                포인터를 aLogPtr에 Setting한다. 그리고 Log를 가지고 있는
 *                Log파일포인터를 aLogFile에 Setting한다.
 *
 *  aDecompBufferHandle - [IN] 로그 압축해제에 사용할 버퍼의 핸들
 *  aLSN                - [IN] 읽어들일 Log Record위치
 *  aIsRecovery         - [IN] Recovery시에 호출되었으면
 *                             ID_TRUE, 아니면 ID_FALSE
 *
 *  aLogFile    - [IN-OUT] 현재 Log레코드를 가지고 있는 LogFile
 *  aLogHeadPtr - [OUT] Log의 Header를 복사
 *  aLogPtr     - [OUT] Log Record가 위치한 Log버퍼의 Pointer
 *  aIsValid    - [OUT] Log가 Valid하면 ID_TRUE, 아니면 ID_FALSE
 *  aLogSizeAtDisk   - [OUT] 파일상에서 읽어낸 로그의 크기
 *                      ( 압축된 로그의 경우 로그의 크기와
 *                        파일상의 크기가 다를 수 있다 )
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::readLogAndValid(iduMemoryHandle * aDecompBufferHandle,
                                        smLSN           * aLSN,
                                        idBool            aIsCloseLogFile,
                                        smrLogFile     ** aLogFile,
                                        smrLogHead      * aLogHeadPtr,
                                        SChar          ** aLogPtr,
                                        idBool          * aIsValid,
                                        UInt            * aLogSizeAtDisk)
{
    static UInt sMaxLogOffset    = getLogFileSize()
                                   - ID_SIZEOF(smrLogHead)
                                   - ID_SIZEOF(smrLogTail);
    static UInt  sLogFileSize = smuProperty::getLogFileSize();

    // 비압축 로그를 읽는 경우 aDecompBufferHandle이 NULL로 들어온다
    IDE_DASSERT(aLSN            != NULL );
    IDE_DASSERT(aIsCloseLogFile == ID_TRUE ||
                aIsCloseLogFile == ID_FALSE);
    IDE_DASSERT(aLogFile        != NULL );
    IDE_DASSERT(aLogPtr         != NULL );
    IDE_DASSERT(aLogHeadPtr     != NULL );


    IDE_TEST_RAISE(aLSN->mOffset > sMaxLogOffset, error_invalid_lsn_offset);

    IDE_TEST( smrRemoteLogMgr::readLog( aDecompBufferHandle,
                                        aLSN,
                                        aIsCloseLogFile,
                                        aLogFile,
                                        aLogHeadPtr,
                                        aLogPtr,
                                        aLogSizeAtDisk)
             != IDE_SUCCESS );

    if ( aIsValid != NULL )
    {
        if ( mNotDecomp == ID_FALSE )
        {
            *aIsValid = smrLogFile::isValidLog( aLSN,
                                                aLogHeadPtr,
                                                *aLogPtr,
                                                *aLogSizeAtDisk);
        }
        else
        {
            // Log의 압축을 풀지 않았기 때문에 Size와 MagicNumber 만으로 검증해야 한다.
            if ( ( smrLogHeadI::getSize(aLogHeadPtr) < ( ID_SIZEOF(smrLogHead) + ID_SIZEOF(smrLogTail) ) ) ||
                 ( *aLogSizeAtDisk > (sLogFileSize - aLSN->mOffset) ) )
            {
                *aIsValid = ID_FALSE;
            }
            else
            {
                /* BUG-37018 There is some mistake on logfile Offset calculation 
                 * Dummy log가 valid한지 검사하는 방법은 MagicNumber검사밖에 없다.*/
                *aIsValid = smrLogFile::isValidMagicNumber( aLSN, aLogHeadPtr );
            }
        }
    }
    else
    {
        /* aIsValid가 NUll이면 Valid를 Check하지 않는다 */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_invalid_lsn_offset);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_InvalidLSNOffset,
                                aLSN->mFileNo,
                                aLSN->mOffset));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aFileNo가 가리키는 LogFile을 Open한다.
 *               aLogFilePtr에 Open된 Logfile Pointer를 Setting해준다.
 *
 * aFileNo     - [IN]  open할 LogFile No
 * aIsWrite    - [IN]  open할 logfile에 대해 write를 한다면 ID_TRUE, 아니면
 *                     ID_FALSE
 * aLogFilePtr - [OUT] open된 logfile를 가리킨다.
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::openLogFile( UInt          aFileNo,
                                     idBool        aIsWrite,
                                     smrLogFile ** aLogFilePtr )
{
    SChar        sLogFileName[SM_MAX_FILE_NAME];
    smrLogFile * sNewLogFile;
    idBool       sIsAlloc  = ID_FALSE;
    idBool       sIsInit   = ID_FALSE;
    idBool       sIsLocked = ID_FALSE;

    idlOS::snprintf(sLogFileName,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT,
                    getLogDirPath(),
                    IDL_FILE_SEPARATOR,
                    SMR_LOG_FILE_NAME,
                    aFileNo);

    IDE_ASSERT( mMemPool.alloc((void **)&sNewLogFile) == IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    new (sNewLogFile) smrLogFile();

    IDE_ASSERT( sNewLogFile->initialize() == IDE_SUCCESS );
    sIsInit = ID_TRUE;

    sNewLogFile->mFileNo = aFileNo;

    *aLogFilePtr         = sNewLogFile;

    // 로그파일 list의 Mutex를 풀고 로그파일을 open하는 작업을
    // 계속 수행하기 위해 해당 로그파일에 Mutex를 잡는다.
    IDE_ASSERT( sNewLogFile->lock() == IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    sNewLogFile->mRef++;

    IDE_TEST( sNewLogFile->open( aFileNo,
                                 sLogFileName,
                                 ID_FALSE, /* aIsMultiplexLogFile */
                                 getLogFileSize(),
                                 aIsWrite ) 
              != IDE_SUCCESS );

    sIsLocked = ID_FALSE;
    IDE_ASSERT( sNewLogFile->unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( sNewLogFile->unlock() == IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsInit == ID_TRUE )
    {
        IDE_ASSERT( sNewLogFile->destroy() == IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsAlloc == ID_TRUE )
    {
        (void)mMemPool.memfree( sNewLogFile );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Check Log DIR Exist
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::checkLogDirExist(void)
{
    const SChar * sLogDirPtr;

    sLogDirPtr = getLogDirPath();

    IDE_TEST_RAISE(idf::access(sLogDirPtr, F_OK) != 0,
                   err_logdir_not_exist)

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_logdir_not_exist);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, sLogDirPtr));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aIndex에 해당하는 로그 경로를 리턴 한다.
 *
 * aIndex - [IN] LogFile Group ID
 ***********************************************************************/
SChar * smrRemoteLogMgr::getLogDirPath()
{
    return mLogDirPath;
}

/***********************************************************************
 * Description : aIndex에 로그 경로를 세팅 한다.
 *
 * aIndex   - [IN] LogFile Group ID
 * aDirPath - [IN] LogFile Path
 ***********************************************************************/
void smrRemoteLogMgr::setLogDirPath(SChar * aDirPath)
{
    idlOS::memcpy(mLogDirPath, aDirPath, strlen(aDirPath));
}

/***********************************************************************
 * Description : 로그 파일 사이즈를 리턴 한다.
 ***********************************************************************/
ULong smrRemoteLogMgr::getLogFileSize(void)
{
    return mLogFileSize;
}

/***********************************************************************
 * Description : 로그 파일 사이즈를 설정 한다.
 *
 * aLogFileSize - [IN] LogFile size
 ***********************************************************************/
void smrRemoteLogMgr::setLogFileSize(ULong aLogFileSize)
{
    mLogFileSize = aLogFileSize;
}

/***********************************************************************
 * Description : Check Log File Exist
 *
 * aFileNo  - [IN]  LogFile Number
 * aIsExist - [OUT] 파일 존재 유무
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::isLogFileExist(UInt     aFileNo,
                                       idBool * aIsExist)
{
    SChar   sLogFilename[SM_MAX_FILE_NAME];
    iduFile sFile;
    ULong   sLogFileSize = 0;

    *aIsExist = ID_TRUE;

    idlOS::snprintf(sLogFilename,
                    SM_MAX_FILE_NAME,
                    "%s%c%s%"ID_UINT32_FMT,
                    getLogDirPath(),
                    IDL_FILE_SEPARATOR,
                    SMR_LOG_FILE_NAME,
                    aFileNo);

    if ( idf::access(sLogFilename, F_OK) != 0 )
    {
        *aIsExist = ID_FALSE;
    }
    else
    {
        IDE_TEST( sFile.initialize(IDU_MEM_SM_SMR,
                                  1, /* Max Open FD Count */
                                  IDU_FIO_STAT_OFF,
                                  IDV_WAIT_INDEX_NULL )
                 != IDE_SUCCESS );

        IDE_TEST( sFile.setFileName(sLogFilename)  != IDE_SUCCESS );

        IDE_TEST( sFile.open(ID_FALSE, O_RDONLY) != IDE_SUCCESS );
        IDE_TEST( sFile.getFileSize(&sLogFileSize) != IDE_SUCCESS );
        IDE_TEST( sFile.close()                    != IDE_SUCCESS );
        IDE_TEST( sFile.destroy()                  != IDE_SUCCESS );

        if ( sLogFileSize != getLogFileSize() )
        {
            *aIsExist = ID_FALSE;
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 첫번째 파일번호로 부터 마지막
 *               파일 번호를 얻는다.
 *               opendir 를 이용 하여 로그 파일 리스트를 구하고 이중에
 *               가장 작은 파일 번호, 가장 큰 파일 번호를
 *               얻고 마지막 LSN을 구한다.
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::setRemoteLogMgrsInfo()
{
    idBool          sIsExist;
    UInt            sFileNo;
    smLSN           sReadLSN;
    smrLogHead      sLogHead;

    smrLogFile    * sLogFilePtr;
    SChar         * sLogPtr;
    UInt            sLogSizeAtDisk;
    UInt            sStage = 0;
    
    SChar           sLogFilename[SM_MAX_FILE_NAME];

    idBool          sIsValid = ID_FALSE;
#ifdef DEBUG
    smLSN           sDebugLSN;
#endif

    static UInt  sLogFileSize = smuProperty::getLogFileSize();

    /* 로그 압축해제를 위한 버퍼의 핸들 */
    iduReusedMemoryHandle sDecompBufferHandle;
    IDE_TEST( sDecompBufferHandle.initialize(IDU_MEM_SM_SMR)
              != IDE_SUCCESS );
    sStage = 1;

    //가장 작은 파일 번호를 찾는다 / 가장 큰 파일 번호를 찾는다.
    IDE_TEST( setFstFileNoAndEndFileNo( &mRemoteLogMgrs.mFstFileNo,
                                        &mRemoteLogMgrs.mEndFileNo )
              != IDE_SUCCESS );

    /*마지막 파일에서 유효한 로그 파일 찾아 마지막 LSN을 구한다. */
    sLogFilePtr = NULL;
    sFileNo     = mRemoteLogMgrs.mEndFileNo;

    while (1)
    {
       IDE_TEST( isLogFileExist(sFileNo, &sIsExist) != IDE_SUCCESS );

        if ( sIsExist == ID_TRUE )
        {
            SM_SET_LSN(sReadLSN, sFileNo, 0 );
            IDE_TEST( smrRemoteLogMgr::readFirstLogHead( &sReadLSN, 
                                                         &sLogHead,
                                                         &sIsValid )
                      !=IDE_SUCCESS );

            if ( sIsValid == ID_TRUE )
            {
                sReadLSN = smrLogHeadI::getLSN( &sLogHead ); 
                if ( SM_IS_LSN_INIT( sReadLSN ) )
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
                sFileNo--;
            }
        }
        else
        {
            //로그 파일이 없다.
            IDE_RAISE(ERR_FILE_NOT_FOUND);
        }
    }
    mRemoteLogMgrs.mEndFileNo = sFileNo; //로그가 유효한 마지막 파일 번호

    /*마지막 파일에서 SN과  LSN을 찾는다. */
    SM_SET_LSN(sReadLSN, mRemoteLogMgrs.mEndFileNo, 0 );
    while (1)
    {
            IDE_TEST( readLog( &sDecompBufferHandle,
                               &sReadLSN,
                               ID_TRUE /* Close Log File When aLogFile doesn't include aLSN */,
                               &sLogFilePtr,
                               &sLogHead,
                               &sLogPtr,
                               &sLogSizeAtDisk )
                      != IDE_SUCCESS );

        if ( mNotDecomp == ID_FALSE )
        {
            if ( smrLogFile::isValidLog( &sReadLSN,
                                         &sLogHead,
                                         sLogPtr,
                                         sLogSizeAtDisk ) == ID_TRUE )
            {
                // BUG-29115
                // log file의 마지막 FILE_END 로그는 무시한다.
                if ( smrLogHeadI::getType(&sLogHead) != SMR_LT_FILE_END )
                {   
#ifdef DEBUG 
                    sDebugLSN = smrLogHeadI::getLSN( &sLogHead ); 
                    IDE_ASSERT( smrCompareLSN::isEQ( &sReadLSN, &sDebugLSN ) );
#endif
                    SM_GET_LSN( mRemoteLogMgrs.mLstLSN, sReadLSN );
                    sReadLSN.mOffset += sLogSizeAtDisk;
                }
                else
                {
                    /* SMR_LT_FILE_END */
                    break;
                }
            }
            else
            {
                /* invalid Log */
                break;
            }
        } 
        else
        {
            sIsValid = ID_TRUE;
            /* Log의 압축을 풀지 않았기 때문에 Size와 MagicNumber 만으로 검증해야 한다.  */
            if ( ( smrLogHeadI::getSize( &sLogHead ) < ( ID_SIZEOF(smrLogHead) + ID_SIZEOF(smrLogTail) ) ) ||
                 ( sLogSizeAtDisk > ( sLogFileSize - sReadLSN.mOffset) ) )
            {
                sIsValid = ID_FALSE;
            }
            else
            {
                sIsValid = smrLogFile::isValidMagicNumber( &sReadLSN, &sLogHead );
            }
            
            if( sIsValid == ID_TRUE )
            {
                if ( ( smrLogHeadI::getFlag( &sLogHead ) & SMR_LOG_COMPRESSED_MASK )
                       == SMR_LOG_COMPRESSED_OK )
                {
#ifdef DEBUG 
                    sDebugLSN = smrLogHeadI::getLSN( &sLogHead ); 
                    IDE_ASSERT( smrCompareLSN::isEQ( &sReadLSN, &sDebugLSN ) );
#endif
                    SM_GET_LSN( mRemoteLogMgrs.mLstLSN, sReadLSN )
                    sReadLSN.mOffset += sLogSizeAtDisk;
                }
                else
                {
                    if ( smrLogHeadI::getType( &sLogHead ) != SMR_LT_FILE_END )
                    {
#ifdef DEBUG 
                        sDebugLSN = smrLogHeadI::getLSN( &sLogHead ); 
                        IDE_ASSERT( smrCompareLSN::isEQ( &sReadLSN, &sDebugLSN ) );
#endif
                        SM_GET_LSN( mRemoteLogMgrs.mLstLSN, sReadLSN );
                        sReadLSN.mOffset += sLogSizeAtDisk;
                    }
                    else
                    {
                        /* SMR_LT_FILE_END */
                        break;
                    }
                }
            }
            else
            {
                /* invalod Log */
                break;
            }
        }
    }// while

    closeLogFile(sLogFilePtr);

    sStage = 0;
    IDE_TEST( sDecompBufferHandle.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_FILE_NOT_FOUND);
    {
        idlOS::snprintf(sLogFilename,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        getLogDirPath(),
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME,
                        sFileNo);
        
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistFile, 
                                sLogFilename));
                                
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 1 :
            IDE_ASSERT( sDecompBufferHandle.destroy() == IDE_SUCCESS );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : setRemoteLogMgrsInfo 함수 에서 호출 된다.
 *               해당 하는 경로에서 파일 리스트 중 로그 파일 에서
 *               최소 파일 번호 최대 파이 번호를 구한다.
 *
 * aFstFileNo - [OUT] Log File Group 내에 가장 작은 로그 파일 번호
 * aEndFileNo - [OUT] Log File Group 내에 가장 큰 로그 파일 번호
 ***********************************************************************/
IDE_RC smrRemoteLogMgr::setFstFileNoAndEndFileNo(UInt * aFstFileNo,
                                                 UInt * aEndFileNo)
{
    DIR           * sDirp = NULL;
    struct dirent * sDir  = NULL;
    UInt            sCurLogFileNo;
    idBool          sIsSetTempLogFileName = ID_FALSE;
    idBool          sIsLogFile;
    UInt            sFirstFileNo = 0;
    UInt            sEndFileNo = 0;

    sDirp = idf::opendir(getLogDirPath());
    IDE_TEST_RAISE(sDirp == NULL, error_open_dir);

    while(1)
    {
        sDir = idf::readdir(sDirp);
        if ( sDir == NULL )
        {
            break;
        }

        sCurLogFileNo = chkLogFileAndGetFileNo(sDir->d_name, &sIsLogFile);
        
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

    IDE_TEST_RAISE(sIsSetTempLogFileName != ID_TRUE, error_no_log_file);

    *aFstFileNo = sFirstFileNo;
    *aEndFileNo = sEndFileNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_open_dir);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_CannotOpenDir));
    }
    IDE_EXCEPTION(error_no_log_file);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_FOUND_LOGFILE, getLogDirPath() ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든 로그 파일 번호에서 최초 파일 번호를 리턴 한다.
 *
 * aFileNo - [OUT] 첫번째 로그 파일 번호 
 ***********************************************************************/
void smrRemoteLogMgr::getFirstFileNo(UInt * aFileNo)
{
    *aFileNo = mRemoteLogMgrs.mFstFileNo;

    return;
}

/***********************************************************************
 * Description : logfile에서 로그 파일 prefix를 제거 하고 번호를 반환 한다.
 *
 * aFileName  - [IN]  로그 파일 이름
 * aIsLogFile - [OUT] 파일 이름이 로그 파일 형식 이 아닐경우 ID_FALSE
 ***********************************************************************/
UInt smrRemoteLogMgr::chkLogFileAndGetFileNo(SChar  * aFileName,
                                             idBool * aIsLogFile)
{
    UInt   i;
    UInt   sFileNo = 0;
    SChar  sChar;
    UInt   sPrefixLen;
    idBool sIsLogFile; 

    sIsLogFile = ID_TRUE;

    sPrefixLen  = idlOS::strlen(SMR_LOG_FILE_NAME);

    if ( idlOS::strncmp(aFileName, SMR_LOG_FILE_NAME, sPrefixLen) == 0 )
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

    *aIsLogFile = sIsLogFile;

    return sFileNo;
}
