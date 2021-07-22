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
 * $Id: smrLogAnchorMgr.cpp 90259 2021-03-19 01:22:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 loganchor 관리자의 구현파일이다.
 *
 * # Algorithm
 *
 * 1. 다중 loganchor의 생성
 *
 *  - createdb과정에서만 수행하며, hidden 프로퍼티에
 *    정의된 만큼 로그앵커 파일을 log DIR에 생성
 *
 * 2. loganchor 갱신 및 플러쉬 처리
 *
 *  - 기존에 있던 loganchor 자료에 대한 갱신
 *    mLogAnchor를 이용하여 mBuffer에 저장된 로그앵커
 *    정보가 수정되며 tablespace 정보는 그대로 사용
 *
 *  - tablespace 관련 DDL이 발생할 경우
 *    mBuffer를 tablespace 정보가 저장된
 *    부분만 reset하고 mLogAnchor와 tablespace정보를
 *    다시 기록
 *
 * 3. CheckSum 계산 100
 *
 *   다음과 같은 경우 (b)영역에 대해 foldBinary한 결과와
 *   (c)영역에 대한 foldBinary한 결과를 더해서 계산한다.
 *
 *    : (b) + (c) = CheckSum
 *
 *        __mLogAnchor(고정길이)
 *   _____|______________________
 *   | |       |                 |
 *   |_|__(b)__|________(c)______|
 *    |                     |
 *    |__ CheckSum(4bytes)  |__ tablespace정보(가변길이)
 *
 *
 * 4.loganchor의 validate 검증
 *
 *  검증단계는 다음과 같다
 *
 *  - 저장된 checksum과 다시 계산한 checksum을 비교
 *  - 위과정을 거쳐서 corrupt되지 않은 loganchor 파일들 중에
 *    저장된 LSN이 가장 크고, UpdateAndFlush 횟수가 가장 큰
 *    loganchor 파일 선정
 *
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ide.h>
#include <idp.h>
#include <smDef.h>
#include <smm.h>
#include <smErrorCode.h>
#include <sdd.h>
#include <smu.h>
#include <smrDef.h>
#include <smrLogAnchorMgr.h>
#include <smrCompareLSN.h>
#include <smrReq.h>
#include <sct.h>
#include <sdcTXSegMgr.h>
#include <smiMain.h>
#include <svm.h>
#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h>


smrLogAnchorMgr::smrLogAnchorMgr()
{

}

smrLogAnchorMgr::~smrLogAnchorMgr()
{

}

/***********************************************************************
 * Description: 여러 벌의 loganchor 파일 생성
 *
 * createdb과정에서만 수행한다.
 * SM _LOGANCHOR_FILE_COUNT : hidden property (3개 고정)
 * 정의되어 있는 개수만큼 loganchor 파일을 생성
 *
 * + 2nd. code design
 *   - if durability level 0이 아니라면 then
 *        - SMU_LOGANCHOR_FILE_COUNT 개수만큼의 loganchor 파일을 생성 (only create)
 *          및 loganchor 파일들을 open 한다.
 *        - mBuffer를 SM_PAGE_SIZE 크기로 초기화함
 *        - dummy loganchor를 초기화한다.
 *        - mBuffer를 SM_PAGE_SIZE 크기로 초기화함
 *        - loganchor buffer의 write offset 초기화 이후에 dummy loganchor를 기록
 *        - loganchor buffer를 모든 loganchor 파일에 flush한다.
 *        - loganchor 파일들을 close 하고 해제한다.
 *        - mBuffer를 해제한다.
 *   - endif
 **********************************************************************/
IDE_RC smrLogAnchorMgr::create()
{
    UInt          sWhich;
    UInt          sState = 0;
    smrLogAnchor  sLogAnchor;
    SChar         sFileName[SM_MAX_FILE_NAME];
    UInt          i;

    /* BUG-16214: SM에서 startup process, create db, startup service,
     * shutdown immediate할때 발생하는 UMR없애기 */
    idlOS::memset( &sLogAnchor, 0x00, ID_SIZEOF(smrLogAnchor));

    /* INITIALIZE PROPERTIES */

    IDE_TEST(checkLogAnchorDirExist() != IDE_SUCCESS );

    /* ------------------------------------------------
     * 여러개의 loganchor 파일을 생성 (only create)
     * ----------------------------------------------*/
    for ( sWhich = 0 ; sWhich < SM_LOGANCHOR_FILE_COUNT ; sWhich++ )
    {
        idlOS::memset( sFileName, 0x00, SM_MAX_FILE_NAME );
        /* 파일명은 loganchor1 .. loganchorN 형태로 붙임 */
        idlOS::snprintf( sFileName, SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT"",
                         smuProperty::getLogAnchorDir(sWhich),
                         IDL_FILE_SEPARATOR,
                         SMR_LOGANCHOR_NAME,
                         sWhich );

        IDE_TEST_RAISE(idf::access(smuProperty::getLogAnchorDir(sWhich),
                                     W_OK) !=0, err_no_write_perm_path);

        IDE_TEST_RAISE(idf::access(smuProperty::getLogAnchorDir(sWhich),
                                     X_OK) !=0, err_no_execute_perm_path);

        IDE_TEST_RAISE(idf::access(sFileName,
                                     F_OK) ==0, err_already_exist_file);

        IDE_TEST( mFile[sWhich].initialize( IDU_MEM_SM_SMR,
                                            1, /* Max Open FD Count */
                                            IDU_FIO_STAT_OFF,
                                            IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );

        IDE_TEST( mFile[sWhich].setFileName(sFileName)
                  != IDE_SUCCESS );

        IDE_TEST( mFile[sWhich].createUntilSuccess( smLayerCallback::setEmergency )
                  != IDE_SUCCESS );

        //====================================================================
        // To Fix BUG-13924
        //====================================================================

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_DISK_FILE_CREATE,
                     sFileName );
    }

    SM_LSN_INIT( sLogAnchor.mBeginChkptLSN );
    SM_LSN_INIT( sLogAnchor.mEndChkptLSN );

    SM_LSN_INIT( sLogAnchor.mMemEndLSN );

    sLogAnchor.mLstCreatedLogFileNo = 0;
    sLogAnchor.mFstDeleteFileNo     = 0;
    sLogAnchor.mLstDeleteFileNo     = 0;
    SM_LSN_MAX(sLogAnchor.mResetLSN);

    SM_LSN_INIT( sLogAnchor.mMediaRecoveryLSN );
    SM_LSN_INIT( sLogAnchor.mDiskRedoLSN );
    sLogAnchor.mCheckSum            = 0;
    sLogAnchor.mServerStatus        = SMR_SERVER_SHUTDOWN;
    sLogAnchor.mArchiveMode         = SMI_LOG_NOARCHIVE;

    IDE_TEST( sdcTXSegMgr::adjustEntryCount( smuProperty::getTXSEGEntryCnt(),
                                             &(sLogAnchor.mTXSEGEntryCnt) ) 
              != IDE_SUCCESS );

    sLogAnchor.mNewTableSpaceID     = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
    sLogAnchor.mSmVersionID         = smVersionID;

    sLogAnchor.mUpdateAndFlushCount = 0; // 생성시 0으로 초기화

    for( i = 0 ; i < SMR_LOGANCHOR_RESERV_SIZE ; i++ )
    {
        sLogAnchor.mReserveArea[i] = 0;
    }

    /* proj-1608 recovery from replication
     * BUG-31488, 초기화가 되지 않아 NULL로 설정되지 않고, 0으로 설정됨
     * RP에서는 mReplRecoveryLSN은 초기화 시 항상 SM_SN_NULL로 가정하고 있음..
     */
    SM_LSN_MAX( sLogAnchor.mReplRecoveryLSN );

    /* PROJ-2133 incremental backup */
    idlOS::memset( &sLogAnchor.mCTFileAttr, 0x00, ID_SIZEOF( smriCTFileAttr ));
    idlOS::memset( &sLogAnchor.mBIFileAttr, 0x00, ID_SIZEOF( smriBIFileAttr ));

    sLogAnchor.mCTFileAttr.mCTMgrState = SMRI_CT_MGR_DISABLED;
    sLogAnchor.mBIFileAttr.mBIMgrState = SMRI_BI_MGR_FILE_REMOVED;
    
    sLogAnchor.mBIFileAttr.mDeleteArchLogFileNo = ID_UINT_MAX;

    /* ------------------------------------------------
     * mBuffer를 SM_PAGE_SIZE 크기로 초기화함
     * ----------------------------------------------*/
    IDE_TEST( allocBuffer( SM_PAGE_SIZE ) != IDE_SUCCESS );
    sState = 1;

    initBuffer();

    IDE_TEST( writeToBuffer( (UChar*)&sLogAnchor,
                             (UInt)ID_SIZEOF(smrLogAnchor) )
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * 모든 loganchor 파일에 flush
     * ----------------------------------------------*/
    IDE_TEST(flushAll() != IDE_SUCCESS );

    for ( sWhich = 0 ; sWhich < SM_LOGANCHOR_FILE_COUNT ; sWhich++ )
    {
        IDE_TEST( mFile[sWhich].destroy() != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( freeBuffer() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( err_already_exist_file );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistLogAnchorFile ) );
    }
    IDE_EXCEPTION( err_no_write_perm_path );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_NoWritePermFile,
                                 smuProperty::getLogAnchorDir(sWhich)));
    }
    IDE_EXCEPTION( err_no_execute_perm_path );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_NoExecutePermFile,
                                 smuProperty::getLogAnchorDir(sWhich)));
    }
    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)freeBuffer();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : loganchor online backup
 *
 * loganchor backup시에는 backup 디렉토리에 한벌의 loganchor 파일을 유지하며,
 * restore시에 여러 loganchor 디렉토리에 복구한다.
 **********************************************************************/
IDE_RC smrLogAnchorMgr::backup( UInt   aWhich,
                                SChar* aBackupFilePath )
{
    UInt sState = 0;

    IDE_DASSERT( aBackupFilePath != NULL );

    IDE_TEST(mFile[aWhich].open() != IDE_SUCCESS );
    sState = 1;
    
    // source와 destination이 동일하면 안된다.
    IDE_TEST_RAISE( idlOS::strcmp( aBackupFilePath,
                                   mFile[aWhich].getFileName() )
                    == 0, error_self_copy );

    IDE_TEST( mFile[aWhich].copy( NULL, /* idvSQL* */
                                  aBackupFilePath,
                                  ID_FALSE )
             != IDE_SUCCESS );

   sState = 0;
   IDE_ASSERT(mFile[aWhich].close() == IDE_SUCCESS );
   
   return IDE_SUCCESS;

   IDE_EXCEPTION(error_self_copy);
   {
       IDE_SET( ideSetErrorCode(smERR_ABORT_SelfCopy));
   }
   IDE_EXCEPTION_END;

   if ( sState == 1 )
   {
       IDE_ASSERT( mFile[aWhich].close() == IDE_SUCCESS );
   }

   return IDE_FAILURE;
}

/***********************************************************************
 * Description : loganchor online backup
 * PRJ-1149, log anchor파일들을 backup 한다.
 **********************************************************************/
IDE_RC smrLogAnchorMgr::backup( idvSQL * aStatistics,
                                SChar  * aBackupPath )
{
    UInt   sState = 0;
    UInt   sWhich;
    UInt   sNameLen;
    UInt   sPathLen;
    SChar  sBackupFilePath[SM_MAX_FILE_NAME];

    IDE_TEST_RAISE( lock() != IDE_SUCCESS, error_mutex_lock);

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_LOGANC_BACKUP_START);

    sState =1;

    for ( sWhich = 0 ; sWhich < SM_LOGANCHOR_FILE_COUNT ; sWhich++ )
    {
        idlOS::memset( sBackupFilePath, 0x00, SM_MAX_FILE_NAME );

        /* 파일명은 loganchor1 .. loganchorN 형태로 붙임 */
        sPathLen = idlOS::strlen( aBackupPath );

        // BUG-11206와 같이 source와 destination이 같아서
        // 원본 데이타 파일이 유실되는 경우를 막기위하여
        // 다음과 같이 정확히 path를 구성함.
        if ( aBackupPath[sPathLen -1] == IDL_FILE_SEPARATOR )
        {
            idlOS::snprintf( sBackupFilePath, SM_MAX_FILE_NAME,
                             "%s%s%"ID_UINT32_FMT"",
                             aBackupPath,
                             SMR_LOGANCHOR_NAME,
                             sWhich );
        }
        else
        {
            idlOS::snprintf( sBackupFilePath, SM_MAX_FILE_NAME,
                             "%s%c%s%"ID_UINT32_FMT"",
                             aBackupPath,
                             IDL_FILE_SEPARATOR,
                             SMR_LOGANCHOR_NAME,
                             sWhich );
        }

        sNameLen = idlOS::strlen( sBackupFilePath );

        IDE_TEST( sctTableSpaceMgr::makeValidABSPath( ID_TRUE,
                                                      sBackupFilePath,
                                                      &sNameLen,
                                                      SMI_TBS_DISK )
                  != IDE_SUCCESS );

        IDE_TEST( backup( sWhich,sBackupFilePath ) != IDE_SUCCESS );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVERY_LOGANC_BACKUP_TO,
                     mFile[sWhich].getFileName(),
                     sBackupFilePath );

        IDE_TEST_RAISE( iduCheckSessionEvent(aStatistics) != IDE_SUCCESS,
                        error_backup_abort );

    }

    sState =0;
    IDE_TEST_RAISE( unlock() != IDE_SUCCESS, error_mutex_unlock);


    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_LOGANC_BACKUP_END);

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_mutex_lock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(error_mutex_unlock);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION(error_backup_abort);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_LOGANC_BACKUP_ABORT);
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_MRECOVERY_LOGANC_BACKUP_FAIL);

        if ( sState != 0 )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*
   PRJ-1548 User Memory Tablespace

   기능 : Loganchor 관리자 초기화하기 전에 유효한 loganchor 파일 선택

   Loganchor 파일을 순서대로 메모리버퍼에 로딩하여 checksum 검사 및
   Latest Update Loganchor를 선택한다.

   CheckSum 오류가 감지된 Loganchor는 Lasted Update Loganchor 선택할때
   고려하지 않는다. 모두 CheckSum 오류가 발생했다면 Exception을 반환하고,
   서버구동을 실패하게 한다.

   # Lasted Update Loganchor 조건
   1. LSN이 가장 큰 것(최신)을 선택
   2. 모두 같다면 UpdateAndFlushCount를 사용하여 선택

*/
IDE_RC smrLogAnchorMgr::checkAndGetValidAnchorNo(UInt*  aWhich)
{
    UInt          sWhich;
    UInt          sAllocSize;
    ULong         sFileSize;
    idBool        sFound;
    UChar*        sBuffer;
    SChar         sAnchorFileName[SM_MAX_FILE_NAME];
    iduFile       sFile[SM_LOGANCHOR_FILE_COUNT];
    smrLogAnchor* sLogAnchor;
    smLSN         sMaxLSN;
    smLSN         sLogAnchorLSN;
    ULong         sUpdateAndFlushCount;

    IDE_DASSERT( aWhich != NULL );

    sBuffer    = NULL;
    sLogAnchor = NULL;
    sFileSize  = 0;
    sAllocSize = 0;
    sFound     = ID_FALSE;
    sUpdateAndFlushCount = 0;
    SM_LSN_INIT( sMaxLSN );

    // 로그앵커버퍼 writeoffset 초기화 이후에 기록시작

    for ( sWhich = 0 ; sWhich < SM_LOGANCHOR_FILE_COUNT ; sWhich++ )
    {
        // 파일명은 loganchor1 .. loganchorN 형태로 붙임
        idlOS::snprintf( sAnchorFileName, SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT"",
                         smuProperty::getLogAnchorDir(sWhich),
                         IDL_FILE_SEPARATOR,
                         SMR_LOGANCHOR_NAME,
                         sWhich );

        IDE_TEST( sFile[sWhich].initialize(IDU_MEM_SM_SMR,
                                           1, /* Max Open FD Count */
                                           IDU_FIO_STAT_OFF,
                                           IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );
        IDE_TEST( sFile[sWhich].setFileName( sAnchorFileName )
                  != IDE_SUCCESS );

        if ( sFile[sWhich].open() != IDE_SUCCESS )
        {
            // 파일이 오픈되지 않으면 무시하고, 다음 파일 처리
            continue;
        }

        if ( sFile[sWhich].getFileSize( &sFileSize ) != IDE_SUCCESS )
        {
            // 파일크기를 얻을수 없으면 무시하고, 다음 파일 처리
            continue;
        }

        //fix BUG-27232 [CodeSonar] Null Pointer Dereference
        if ( sFileSize == 0 )
        {
            continue;
        }
        
        if ( sFileSize > sAllocSize )
        {
            // 임시버퍼보다 로딩할 파일크기가 큰 경우만 임시버퍼를
            // 재할당 한다.

            if ( sBuffer != NULL )
            {
                IDE_DASSERT( sAllocSize != 0);

                IDE_TEST( iduMemMgr::free(sBuffer) != IDE_SUCCESS );

                sBuffer = NULL;
            }

            sAllocSize = sFileSize;

            /* TC/FIT/Limit/sm/smr/smrLogAnchorMgr_checkAndGetValidAnchorNo_malloc.tc */
            IDU_FIT_POINT_RAISE( "smrLogAnchorMgr::checkAndGetValidAnchorNo::malloc",
                                  insufficient_memory );

            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                               sAllocSize,
                                               (void**)&sBuffer) != IDE_SUCCESS,
                            insufficient_memory );
        }
        
        // 임시버퍼에 로딩
        IDE_TEST( sFile[sWhich].read( NULL, /* idvSQL* */
                                      0,
                                      sBuffer,
                                      sFileSize )
                  != IDE_SUCCESS );

        sLogAnchor = (smrLogAnchor*)sBuffer;

        // Checksum 오류 검사
        if (sLogAnchor->mCheckSum == makeCheckSum(sBuffer, sFileSize))
        {
            sFound  = ID_TRUE;
            // Loganchor의 EndLSN은 
            // checkpoint시의 Memory RedoLSN 혹은 Shutdown시의 EndLSN임. 
            SM_GET_LSN( sLogAnchorLSN, sLogAnchor->mMemEndLSN );

            if (  smrCompareLSN::isLT( &sMaxLSN, &sLogAnchorLSN ) )
            {
                // 큰 LSN 저장하고, 해당 Loganchor를 선택한다.
                SM_GET_LSN( sMaxLSN, sLogAnchorLSN );

                /* BUG-31485  [sm-disk-recovery]when the server starts,  
                 *            a wrong loganchor file is selected.  
                 */ 
                sUpdateAndFlushCount = sLogAnchor->mUpdateAndFlushCount; 

                *aWhich = sWhich;
            }
            else
            {
                // LSN이 동일한 경우는 UpdateAndFlush 횟수를 비교한다.
                // STARTUP CONTROL 단계에서 DDL에 의해 Loganchor가 변경될수
                // 있는데 이때, LSN이 모두 동일할 수 있기 때문에 Count로
                // 최신 Loganchor를 선택한다.
                if ( smrCompareLSN::isEQ( &sMaxLSN, &sLogAnchorLSN ) )
                {
                    if ( sUpdateAndFlushCount < sLogAnchor->mUpdateAndFlushCount )
                    {
                        sUpdateAndFlushCount = sLogAnchor->mUpdateAndFlushCount;

                        *aWhich = sWhich;
                    }
                    else
                    {
                        // continue
                    }
                }
                else
                {

                }
                // continue..
            }
        }
        else
        {
            // CheckSum 오류 감지..
        }
    }

    for ( sWhich = 0 ; sWhich < SM_LOGANCHOR_FILE_COUNT ; sWhich++ )
    {
        IDE_TEST( sFile[sWhich].destroy() != IDE_SUCCESS );
    }

    // 임시메모리버퍼 해제
    if ( sBuffer != NULL )
    {
        IDE_TEST( iduMemMgr::free(sBuffer) != IDE_SUCCESS );

        sBuffer = NULL;
    }
    else
    {
        /* nothing to do */
    }

    // 모든 Loganchor가 CheckSum 오류가 발생한 경우에 Exception 처리
    IDE_TEST_RAISE( sFound != ID_TRUE, error_invalid_loganchor_file );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_loganchor_file );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidLogAnchorFile));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sBuffer != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( sBuffer ) == IDE_SUCCESS );
        sBuffer = NULL;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
  PRJ-1548 User Memory Tablespace

  Loganchor의 첫번째 Node Attribute Type을 판독한다.

  [IN ] aLogAnchorFile : 오픈된 loganchor 파일 객체
  [OUT] aBeginOffset   : 가변영역 시작 오프셋
  [OUT] aAttrType      : 첫번째 Node Attribute Type
*/
IDE_RC smrLogAnchorMgr::getFstNodeAttrType( iduFile         * aLogAnchorFile,
                                            UInt            * aBeginOffset,
                                            smiNodeAttrType * aAttrType )
{
    UInt             sBeginOffset;
    smiNodeAttrType  sAttrType;

    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aBeginOffset   != NULL );
    IDE_DASSERT( aAttrType      != NULL );

    sBeginOffset = ID_SIZEOF(smrLogAnchor);

    // attribute type 판독
    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    sBeginOffset,
                                    (SChar*)&sAttrType,
                                    ID_SIZEOF(smiNodeAttrType) )
              != IDE_SUCCESS );

    *aBeginOffset = sBeginOffset;
    *aAttrType    = sAttrType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   PRJ-1548 User Memory Tablespace

   aNextOffset에 저장된 Node Attribute 타입을 판독하여 반환한다.

  [IN ] aLogAnchorFile : 오픈된 loganchor 파일 객체
  [IN ] aCurrOffset    : 판독할 오프셋
  [OUT] aAttrType      : Node Attribute Type
*/
IDE_RC smrLogAnchorMgr::getNxtNodeAttrType( iduFile         * aLogAnchorFile,
                                            UInt              aNextOffset,
                                            smiNodeAttrType * aNextAttrType )
{
    smiNodeAttrType  sAttrType;

    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aNextAttrType  != NULL );

    // attribute type 판독
    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    aNextOffset,
                                    (SChar*)&sAttrType,
                                    ID_SIZEOF(smiNodeAttrType) )
              != IDE_SUCCESS );

    *aNextAttrType    = sAttrType;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : loganchor 관리자 초기화
 *
 * + 2nd. code design
 *   - mutex 초기화한다.
 *   - tablespace 속성 및 datafile 속성을 저장하기 위한 자료구조
 *     할당한다.
 *   - mBuffer를 SM_PAGE_SIZE 크기로 초기화한다.
 *   - if (durability level 0 가 아닌경우) then
 *         - 유효한 로그앵커파일을 선정
 *         - 모든 loganchor 파일을 오픈한다.
 *         - loganchor 파일로부터 smrLogAnchor를 판독하여
 *           loganchor 버퍼에 저장한다.
 *         - mLogAnchor에 assign한다.
 *         - while ()
 *           {
 *               - tablespace 속성 정보를 tablespace 개수만큼 판독하여
 *                 loganchor 버퍼에 기록한다.
 *               -  for (datafile 개수만큼)
 *                  datafile 속성 정보를 판독하여 loganchor 버퍼에 기록한다.
 *           }
 *         - 모든 loganchor 파일에 유효한 loganchor 버퍼를 flush 한다.
 *     else
 *         - loganchor 버퍼를 초기화한다.
 *     endif
 **********************************************************************/
IDE_RC smrLogAnchorMgr::initialize()
{
    UInt                  i;
    UInt                  sWhich    = 0;
    UInt                  sState    = 0;
    ULong                 sFileSize = 0;
    smrLogAnchor          sLogAnchor;
    SChar                 sAnchorFileName[SM_MAX_FILE_NAME];
    UInt                  sFileState = 0;

    // LOGANCHOR_DIR 확인
    IDE_TEST( checkLogAnchorDirExist() != IDE_SUCCESS );

    for ( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++ )
    {
        idlOS::snprintf( sAnchorFileName, SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT"",
                         smuProperty::getLogAnchorDir(i),
                         IDL_FILE_SEPARATOR,
                         SMR_LOGANCHOR_NAME,
                         i );

        // Loganchor 파일 접근 권한 확인
        IDE_TEST_RAISE( idf::access(sAnchorFileName, (F_OK|W_OK|R_OK) ) != 0,
                        error_file_not_exist );
    }

    IDE_TEST_RAISE( mMutex.initialize( (SChar*)"LOGANCHOR_MGR_MUTEX",
                                       IDU_MUTEX_KIND_NATIVE,
                                       IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS,
                    error_mutex_init );

    idlOS::memset( &mTableSpaceAttr,
                   0x00,
                   ID_SIZEOF(smiTableSpaceAttr) );

    idlOS::memset( &mDataFileAttr,
                   0x00,
                   ID_SIZEOF(smiDataFileAttr) );

    // 메모리 버퍼(mBuffer)를 SM_PAGE_SIZE 크기로 할당 및 초기화
    IDE_TEST( allocBuffer( SM_PAGE_SIZE ) != IDE_SUCCESS );
    sState = 1;

    // 유효한 loganchor파일을 선정한다.
    IDE_TEST( checkAndGetValidAnchorNo( &sWhich ) != IDE_SUCCESS );

    // 모든 loganchor 파일을 오픈한다.
    for ( i = 0; i < SM_LOGANCHOR_FILE_COUNT; i++ )
    {
        idlOS::snprintf( sAnchorFileName, SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT"",
                         smuProperty::getLogAnchorDir(i),
                         IDL_FILE_SEPARATOR,
                         SMR_LOGANCHOR_NAME,
                         i );
        IDE_TEST( mFile[i].initialize( IDU_MEM_SM_SMR,
                                       1, /* Max Open FD Count */
                                       IDU_FIO_STAT_OFF,
                                       IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );
        IDE_TEST( mFile[i].setFileName( sAnchorFileName ) != IDE_SUCCESS );
        IDE_TEST( mFile[i].open() != IDE_SUCCESS );
        sFileState++;
    }

    // 메모리버퍼 오프셋 초기화
    initBuffer();

    /*
       PRJ-1548 고정길이 영역을 메모리 버퍼에 로딩하는 단계
    */

    IDE_TEST( mFile[sWhich].getFileSize(&sFileSize) != IDE_SUCCESS );

    // CREATE DATABASE 과정중에 가변영역이 존재하지 않을 수는 있지만,
    // 고정영역은 저장되어 있어야 한다.
    IDE_ASSERT ( sFileSize >= ID_SIZEOF(smrLogAnchor) );

    // 파일로 부터 Loganchor 고정영역을 로딩한다.
    IDE_TEST( mFile[sWhich].read( NULL, /* idvSQL* */
                                  0,
                                  (SChar*)&sLogAnchor,
                                  ID_SIZEOF(smrLogAnchor) )
              != IDE_SUCCESS );

    // 메모리 버퍼에 Loganchor 고정역영을 로딩한다.
    IDE_TEST( writeToBuffer( (SChar*)&sLogAnchor,
                             ID_SIZEOF(smrLogAnchor) )
              != IDE_SUCCESS );

    mLogAnchor = (smrLogAnchor*)mBuffer;

    /* DISK REDO LSN과 MEMORY REDO LSN을 테이블스페이스 관리자에
       설정한다. Empty 데이타파일을 생성해서 미디어 복구를
       하는 경우에 파일헤더에 기록할 수 있도록 하기 위해 */
    sctTableSpaceMgr::setRedoLSN4DBFileMetaHdr( &mLogAnchor->mDiskRedoLSN,
                                                &mLogAnchor->mMemEndLSN );

    // 테이블스페이스 전체 갯수 설정
    sctTableSpaceMgr::setNewTableSpaceID( (scSpaceID)mLogAnchor->mNewTableSpaceID );

    // [ 메모리 테이블스페이스 ] STATE단계까지 초기화가 된다
    IDE_TEST( readAllTBSAttrs( sWhich, SMI_TBS_ATTR ) != IDE_SUCCESS );
    IDE_TEST( readAllTBSAttrs( sWhich, SMI_DBF_ATTR ) != IDE_SUCCESS );
    IDE_TEST( readAllTBSAttrs( sWhich, SMI_CHKPTPATH_ATTR ) != IDE_SUCCESS );

    // [ 메모리 테이블스페이스 ] MEDIA, PAGE단계까지 초기화가 된다
    //   Checkpoint Path Attribute가 로드된 상태에에서 MEDIA단계의
    //   초기화가 가능하다.
    IDE_TEST( smmTBSStartupShutdown::initFromStatePhase4AllTBS()
              != IDE_SUCCESS );

    // [ 메모리 테이블스페이스 ]
    //  MEDIA단계까지 초기화 된 상태에서 Checkpoint Image Attribute를
    // 로드할 수 있다.
    //
    // <이유>
    //   - MEDIA단계까지 초기화가 된 상태에서
    //     DB File 객체 사용이 가능하다
    //   - DB File객체 사용이 가능할 때
    //     Checkpoint Image Attribute들을 로드할 수 있다
    IDE_TEST( readAllTBSAttrs( sWhich, SMI_CHKPTIMG_ATTR ) != IDE_SUCCESS );

    //  [  Secondary Buffer 초기화  ]
    IDE_TEST( readAllTBSAttrs( sWhich, SMI_SBUFFER_ATTR ) != IDE_SUCCESS );

    /* ------------------------------------------------
     * 5. 유효한 loganchor 버퍼를 모든 anchor 파일에 flush 한다.
     * ----------------------------------------------*/
    IDE_TEST( readLogAnchorToBuffer( sWhich ) != IDE_SUCCESS );
    IDE_ASSERT( checkLogAnchorBuffer() == ID_TRUE );

    for ( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++ )
    {
        sFileState--;
        IDE_ASSERT(mFile[i].close() == IDE_SUCCESS );
    }

    mIsProcessPhase = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_file_not_exist );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidLogAnchorFile));
    }
    IDE_EXCEPTION( error_mutex_init );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;

    while ( sFileState > 0 )
    {
        sFileState--;
        IDE_ASSERT( mFile[sFileState].close() == IDE_SUCCESS ); 
    }
    
    if (sState != 0)
    {
        (void)freeBuffer();
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : loganchor 관리자 해제
 *
 * + 2nd. code design
 *   - 모든 loganchor 파일을 닫는다
 *   - loganchor 버퍼를 메모리 해제한다.
 *   - tablespace 속성 및 datafile 속성을 loganchor에
 *     저장하기 위한 자료구조를 해제한다.
 *   - mutex를 해제한다.
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::destroy()
{
    UInt   i;

    for (i = 0; i < SM_LOGANCHOR_FILE_COUNT; i++)
    {
        IDE_TEST(mFile[i].destroy() != IDE_SUCCESS );
    }

    IDE_TEST( freeBuffer() != IDE_SUCCESS );

    IDE_TEST_RAISE( mMutex.destroy() != IDE_SUCCESS, error_mutex_destroy );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_mutex_destroy );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*
    로그앵커로부터 Tablespace의 특정 타입의 Attribute들을 로드한다.

    [IN] aWhichAnchor    - 여러 벌의 Log Anchor File중 어떤 것인지
    [IN] aAttrTypeToLoad - 로드할 Attribute의 종류
 */
IDE_RC smrLogAnchorMgr::readAllTBSAttrs( UInt             aWhichAnchor,
                                         smiNodeAttrType  aAttrTypeToLoad )
{
    ULong             sFileSize   = 0;
    smiNodeAttrType   sAttrType   = (smiNodeAttrType)0;
    UInt              sReadOffset = 0;

    IDE_TEST( mFile[aWhichAnchor].getFileSize(&sFileSize) != IDE_SUCCESS );

    /*
       PRJ-1548 가변길이 영역을 메모리 버퍼에 로딩하는 단계
    */
    if ( sFileSize > ID_SIZEOF(smrLogAnchor) )
    {
        // 가변영역의 첫번째 Node Attribute Type을 판독한다.
        IDE_TEST( getFstNodeAttrType( &mFile[aWhichAnchor],
                                      &sReadOffset,
                                      &sAttrType ) != IDE_SUCCESS );
        while ( 1 )
        {
            // Log Anchor로부터 읽은 Attribute를
            // 테이블스페이스별 자료구조로 Load할지 여부를 결정
            if ( sAttrType == aAttrTypeToLoad )
            {
                IDE_TEST( readAttrFromLogAnchor( SMR_AAO_LOAD_ATTR,
                                                 sAttrType,
                                                 aWhichAnchor,
                                                 &sReadOffset ) != IDE_SUCCESS );
            }
            else
            {
                // 해당 Attribute를 읽지 않고 Skip한다.
                sReadOffset += getAttrSize( sAttrType );
            }

            // EOF 확인
            if ( sFileSize == ((ULong)sReadOffset) )
            {
                break;
            }

            IDE_ASSERT( sFileSize > ((ULong)sReadOffset) );

            // attribute type 판독
            IDE_TEST( getNxtNodeAttrType( &mFile[aWhichAnchor],
                                          sReadOffset,
                                          &sAttrType ) 
                      != IDE_SUCCESS );
        } // while
    }
    else
    {
        // CREATE DATABASE 과정중 Loganchor의 고정영역만 저장된 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
   로그앵커 버퍼에 로그앵커 Attribute들중
   Valid한 Attribute들 만드로 다시 기록한다.

   DROP된 Tablespace와 관련된 Attribute는 기록하지 않고 버린다.

   [IN] aWhichAnchor    - 여러 벌의 Log Anchor File중 어떤 것인지
 */
IDE_RC smrLogAnchorMgr::readLogAnchorToBuffer(UInt aWhichAnchor)
{
    ULong             sFileSize = 0;
    smiNodeAttrType   sAttrType = (smiNodeAttrType)0;
    UInt              sReadOffset;

    IDE_TEST( mFile[aWhichAnchor].getFileSize(&sFileSize) != IDE_SUCCESS );

    sReadOffset  = 0;

    /*
       PRJ-1548 가변길이 영역을 메모리 버퍼에 로딩하는 단계
    */
    if ( sFileSize > ID_SIZEOF(smrLogAnchor) )
    {
        // 가변영역의 첫번째 Node Attribute Type을 판독한다.
        IDE_TEST( getFstNodeAttrType( &mFile[aWhichAnchor],
                                      &sReadOffset,
                                      &sAttrType ) != IDE_SUCCESS );
        while ( 1 )
        {
            IDE_TEST( readAttrFromLogAnchor( SMR_AAO_REWRITE_ATTR,
                                             sAttrType,
                                             aWhichAnchor,
                                             &sReadOffset ) != IDE_SUCCESS );

            // EOF 확인
            if ( sFileSize == ((ULong)sReadOffset) )
            {
                break;
            }

            IDE_ASSERT( sFileSize > ( (ULong)sReadOffset ) );

            // attribute type 판독
            IDE_TEST( getNxtNodeAttrType( &mFile[aWhichAnchor],
                                          sReadOffset,
                                          &sAttrType ) 
                      != IDE_SUCCESS );
        } // while
    }
    else
    {
        // CREATE DATABASE 과정중 Loganchor의 고정영역만 저장된 경우
        // nothing to do ...
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : LogAnchor File로 부터 Attribute속성을 읽어온다.
 * aAttrOp 속성에 따라 SMR_AAO_REWRITE_ATTR이면 LogAnchor Buffer에
 * SMR_AAO_LOAD_ATTR 이면 space node에 기록한다.
 *
 *   aAttrOp      - [IN]     Attribute 기록 대상 (Buffer, Node)
 *   aAttrType    - [IN]     읽을 Attribute의 종류
 *   aWhichAnchor - [IN]     읽을 logAnchor File
 *   aReadOffset  - [IN/OUT] 읽을 offset, 이후 다음 offset 반환
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readAttrFromLogAnchor( smrAnchorAttrOption aAttrOp,
                                               smiNodeAttrType     aAttrType,
                                               UInt                aWhichAnchor,
                                               UInt              * aReadOffset )
{
    switch ( aAttrType )
    {
        case SMI_TBS_ATTR:
            // 테이블스페이스 초기화
            IDE_TEST( initTableSpaceAttr( aWhichAnchor,
                                          aAttrOp,
                                          aReadOffset )
                      != IDE_SUCCESS );
            break;

        case SMI_DBF_ATTR :
            // 디스크 데이타파일 초기화
            IDE_TEST( initDataFileAttr( aWhichAnchor,
                                        aAttrOp,
                                        aReadOffset )
                      != IDE_SUCCESS );
            break;

        case SMI_CHKPTPATH_ATTR :
            // 체크포인트 Path 초기화
            IDE_TEST( initChkptPathAttr( aWhichAnchor,
                                         aAttrOp,
                                         aReadOffset )
                      != IDE_SUCCESS );
            break;

        case SMI_CHKPTIMG_ATTR:
            IDE_TEST( initChkptImageAttr( aWhichAnchor,
                                          aAttrOp,
                                          aReadOffset )
                      != IDE_SUCCESS );
            break;

         case SMI_SBUFFER_ATTR:
             IDE_TEST( initSBufferFileAttr( aWhichAnchor,
                                            aAttrOp,
                                            aReadOffset )
                       != IDE_SUCCESS );
             break;

        default:
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_INVALID_LOGANCHOR_ATTRTYPE,
                        aAttrType);

            IDE_ASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
  메모리/디스크 테이블스페이스 초기화

  [IN] aWhich       - 유효한 로그앵커 번호
  [IN] aAttrOp      - Attribute초기화 도중 수행할 작업
  [IN] aReadOffset  - 로그앵커 파일상에서 현재 속성의 오프셋
  [OUT] aReadOffset - 로그앵커 파일상에서 다음 속성 시작 오프셋

  [ PRJ-1548 User Memory TableSpace 개념도입 ]

   * 디스크와 동일하게 미디어 복구를 지원하려면
     STARTUP CONTROL 단계에서 Memory TableSpace Node를 초기화해야 한다.

   * 미디어복구를 위한 지원사항
     [1] Checkpoint Image Hdr 검증이 가능해야함 ( Media Failure 검사 )
     [2] Empty Checkpoint Image 생성 가능해야함 ( Media Recovery 기능 )

     모든 Checkpoint Image의 초기화가 완료되어야 한다.

     이전에는 prepareTBS 단계에서 메모리 테이블스페이스의
     smmDatabaseFile들을 할당하고 초기화하였지만,

     로그앵커 초기화시점에서 그 해당 작업을 수행하기로 한다.
*/
IDE_RC smrLogAnchorMgr::initTableSpaceAttr( UInt                aWhich,
                                            smrAnchorAttrOption aAttrOp,
                                            UInt              * aReadOffset )
{
    UInt sAnchorOffset;

    IDE_DASSERT( aReadOffset != NULL );

    // 현재 속성의 오프셋을 백업한다.
    sAnchorOffset = *aReadOffset;

    // [1] 로그앵커로부터 현재 속성을 판독한다.
    IDE_TEST( readTBSNodeAttr( &mFile[aWhich],
                               aReadOffset,
                               &mTableSpaceAttr )
              != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    // [2] 백업상태가 존재할 경우 백업상태를 없앤다.
    mTableSpaceAttr.mTBSStateOnLA &= ~SMI_TBS_BACKUP;

    if ( aAttrOp == SMR_AAO_REWRITE_ATTR )
    {
        IDE_ASSERT( mWriteOffset == sAnchorOffset );
        // [3] 로그앵커 메모리 버퍼에 기록한다.
        IDE_TEST( writeToBuffer( (SChar*)&mTableSpaceAttr,
                                 ID_SIZEOF( smiTableSpaceAttr ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( aAttrOp == SMR_AAO_LOAD_ATTR )
    {
        // DROP된 경우만 log anchor로부터 Tablespace Load를 SKIP
        //
        // Discard된 Tablespace의 경우 => 로드실시
        //   추후 Drop시를 위해 Tablespace관련
        //   자료구조가 초기화되어있어야 함
        if ( ( mTableSpaceAttr.mTBSStateOnLA & SMI_TBS_DROPPED )
               != SMI_TBS_DROPPED )
        {
            // 테이블스페이스가 삭제되지 않았다면

            // [4] 테이블스페이스를 초기화한다.
            if ( ( mTableSpaceAttr.mType == SMI_MEMORY_SYSTEM_DICTIONARY ) ||
                 ( mTableSpaceAttr.mType == SMI_MEMORY_SYSTEM_DATA )       ||
                 ( mTableSpaceAttr.mType == SMI_MEMORY_USER_DATA ) )
            {
                // memory 매니저에 tablespace Node를 생성
                IDE_TEST( smmTBSStartupShutdown::loadTableSpaceNode(
                                                              &mTableSpaceAttr,
                                                              sAnchorOffset )
                          != IDE_SUCCESS );
            }
            /* PROJ-1594 Volatile TBS */
            else if ( mTableSpaceAttr.mType == SMI_VOLATILE_USER_DATA )
            {
                // volatile 매니저에 tablespace Node를 생성
                IDE_TEST( svmTBSStartupShutdown::loadTableSpaceNode(
                                                              &mTableSpaceAttr,
                                                              sAnchorOffset )
                          != IDE_SUCCESS );
            }
            else
            {
                // disk 매니저에
                // tablespace Node를 생성
                IDE_TEST( sddDiskMgr::loadTableSpaceNode( NULL, /* idvSQL* */
                                                          &mTableSpaceAttr,
                                                          sAnchorOffset )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // 삭제된 TBS는 초기화하지않는다.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  디스크 데이타파일 메타헤더 초기화

  [IN] aWhich        - 유효한 로그앵커 번호
  [IN] aAttrOp       - Attribute초기화 도중 수행할 작업
  [IN] aReadOffset   - 로그앵커 파일상에서 현재 속성의 오프셋
  [OUT] aReadOffset  - 로그앵커 파일상에서 다음 속성 시작 오프셋
*/
IDE_RC smrLogAnchorMgr::initDataFileAttr( UInt                aWhich,
                                          smrAnchorAttrOption aAttrOp,
                                          UInt              * aReadOffset )
{
    UInt  sAnchorOffset;

    IDE_DASSERT( aReadOffset != NULL );

    // 현재 속성의 오프셋을 백업한다.
    sAnchorOffset = *aReadOffset;

    // [1] 로그앵커로부터 현재 속성을 판독한다.
    IDE_TEST( readDBFNodeAttr( &mFile[aWhich],
                               aReadOffset,
                               &mDataFileAttr )  != IDE_SUCCESS );

    // [2] 백업상태가 존재할 경우 백업상태를 없앤다.
    mDataFileAttr.mState &= ~SMI_FILE_BACKUP;

    if ( aAttrOp == SMR_AAO_REWRITE_ATTR )
    {
        IDE_ASSERT( mWriteOffset == sAnchorOffset );
        // [3] 로그앵커 메모리 버퍼에 기록한다.
        IDE_TEST( writeToBuffer( (SChar*)&mDataFileAttr,
                                 ID_SIZEOF( smiDataFileAttr ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( aAttrOp == SMR_AAO_LOAD_ATTR )
    {
        if ( SMI_FILE_STATE_IS_NOT_DROPPED( mDataFileAttr.mState ) )
        {
            // 4.4 해당 tablespace 노드에 datafile 노드를 생성
            IDE_TEST( sddDiskMgr::loadDataFileNode( NULL, /* idvSQL* */
                                                    &mDataFileAttr,
                                                    sAnchorOffset )
                      != IDE_SUCCESS );
        }
        else
        {
            // 삭제된 파일은 초기화하지 않는다.
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

/*
  메모리 체크포인트 PATH 초기화

  [IN]  aWhich       - 유효한 로그앵커 번호
  [IN]  aAttrOp      - Attribute초기화 도중 수행할 작업
  [IN]  aReadOffset  - 로그앵커 파일상에서 현재 속성의 오프셋
  [OUT] aReadOffset  - 로그앵커 파일상에서 다음 속성 시작 오프셋
*/
IDE_RC smrLogAnchorMgr::initChkptPathAttr( UInt                aWhich,
                                           smrAnchorAttrOption aAttrOp,
                                           UInt              * aReadOffset )
{
    UInt  sAnchorOffset;

    sctTableSpaceNode    * sSpaceNode;

    IDE_DASSERT( aReadOffset != NULL );


    // 현재 속성의 오프셋을 백업한다.
    sAnchorOffset = *aReadOffset;

    // [1] 로그앵커로부터 현재 속성을 판독한다.
    IDE_TEST( readChkptPathNodeAttr( &mFile[aWhich],
                                     aReadOffset,
                                     &mChkptPathAttr )
              != IDE_SUCCESS );

    if ( aAttrOp == SMR_AAO_REWRITE_ATTR )
    {
        IDE_ASSERT( mWriteOffset == sAnchorOffset );

        IDE_TEST( writeToBuffer( (SChar*)&mChkptPathAttr,
                                 ID_SIZEOF( smiChkptPathAttr ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( aAttrOp == SMR_AAO_LOAD_ATTR )
    {
        sSpaceNode = sctTableSpaceMgr::findSpaceNodeIncludingDropped( mChkptPathAttr.mSpaceID );

        if ( sSpaceNode != NULL )
        {
            if ( SMI_TBS_IS_DROPPED(sSpaceNode->mState) )
            {
                // Drop 된 Tablespace의 경우
                // Tablespace Node조차 Add되지 않은 상태이다.
                //
                // Checkpoint Path add하지 않는다
            }
            else
            {
                // DROP_PENDING의 상태인 경우에도 여기로 분기된다.
                // Drop Tablespace Pending에서 처리하지 못한 작업 처리를 위해서
                // Checkpoint Path Node가 Tablespace에 매달려야 한다.

                //4.4 해당 tablespace 노드에 Chkpt Path 노드를 생성
                IDE_TEST( smmTBSStartupShutdown::createChkptPathNode(
                              (smiChkptPathAttr*)&mChkptPathAttr,
                              sAnchorOffset ) != IDE_SUCCESS );
            }
        }
        else
        {
            // DROP된 Tablespace의 경우 아예 initTablespaceAttr에서
            // load조차 되지 않았기 때문에 sSpaceNode가 NULL로 나온다.
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

/*
  메모리 데이타파일 메타헤더 초기화

  [IN] aWhich       - 유효한 로그앵커 번호
  [IN] aAttrOp      - Attribute초기화 도중 수행할 작업
  [IN] aReadOffset  - 로그앵커 파일상에서 현재 속성의 오프셋
  [OUT] aReadOffset - 로그앵커 파일상에서 다음 속성 시작 오프셋
*/
IDE_RC smrLogAnchorMgr::initChkptImageAttr( UInt                aWhich,
                                            smrAnchorAttrOption aAttrOp,
                                            UInt              * aReadOffset )
{
    UInt  sAnchorOffset;
    sctTableSpaceNode    * sSpaceNode;

    IDE_DASSERT( aReadOffset != NULL );

    // 현재 속성의 오프셋을 백업한다.
    sAnchorOffset = *aReadOffset;

    IDE_TEST( readChkptImageAttr( &mFile[aWhich],
                                  aReadOffset,
                                  &mChkptImageAttr )
              != IDE_SUCCESS );

    if ( aAttrOp == SMR_AAO_REWRITE_ATTR )
    {
        IDE_ASSERT( mWriteOffset == sAnchorOffset );

        IDE_TEST( writeToBuffer( (SChar*)&mChkptImageAttr,
                                 ID_SIZEOF( smmChkptImageAttr ))
                  != IDE_SUCCESS );
    }

    if ( aAttrOp == SMR_AAO_LOAD_ATTR )
    {
        sSpaceNode = sctTableSpaceMgr::findSpaceNodeIncludingDropped( mChkptImageAttr.mSpaceID );

        if ( sSpaceNode != NULL )
        {
            if ( SMI_TBS_IS_DROPPED(sSpaceNode->mState) )
            {
                // Drop 된 Tablespace의 경우
                // Tablespace Node조차 Add되지 않은 상태이다.
                //
                // Checkpoint Image를 add하지 않는다
            }
            else
            {
                // DROP_PENDING의 상태인 경우에도 여기로 분기된다.
                // Drop Tablespace Pending에서 처리하지 못한 작업 처리를 위해서
                // Checkpoint Image Node의 정보를 토대로
                // Tablespace Node의 데이터파일 객체를 설정하여야 한다.

                // 로그앵커로부터 판독된 메모리 데이타파일 속성을
                // 메모리 데이타파일 런타임헤더에 설정한다.
                IDE_TEST( smmTBSStartupShutdown::initializeChkptImageAttr(
                                                          &mChkptImageAttr,
                                                          &mLogAnchor->mMemEndLSN,
                                                          sAnchorOffset )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // DROP된 Tablespace의 경우 아예 initTablespaceAttr에서
            // load조차 되지 않았기 때문에 sSpaceNode가 NULL로 나온다.
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
  Description :  Secondary  Buffer  파일

   [IN] aWhich        - 유효한 로그앵커 번호
   [IN] aAttrOp       - Attribute초기화 도중 수행할 작업
   [IN] aReadOffset   - 로그앵커 파일상에서 현재 속성의 오프셋
   [OUT] aReadOffset  - 로그앵커 파일상에서 다음 속성 시작 오프셋
 **********************************************************************/
IDE_RC smrLogAnchorMgr::initSBufferFileAttr( UInt                aWhich,
                                             smrAnchorAttrOption aAttrOp,
                                             UInt              * aReadOffset )
{
    UInt  sAnchorOffset;

    IDE_DASSERT( aReadOffset != NULL );

    // 현재 속성의 오프셋을 백업한다.
    sAnchorOffset = *aReadOffset;

    // 로그앵커로부터 현재 속성을 판독한다.
    IDE_TEST( readSBufferFileAttr( &mFile[aWhich],
                                   aReadOffset,
                                   &mSBufferFileAttr )  
               != IDE_SUCCESS );

    if ( aAttrOp == SMR_AAO_REWRITE_ATTR )
    {
        IDE_ASSERT( mWriteOffset == sAnchorOffset );
        // 로그앵커 메모리 버퍼에 기록한다.
        IDE_TEST( writeToBuffer( (SChar*)&mSBufferFileAttr,
                                 ID_SIZEOF( smiSBufferFileAttr ) )
                 != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    if ( aAttrOp == SMR_AAO_LOAD_ATTR )
    {
        IDE_TEST( sdsBufferMgr::loadFileAttr( &mSBufferFileAttr,
                                              sAnchorOffset ) 
                  != IDE_SUCCESS );    
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
 * Description : loganchor에 서버상태 정보 및 로깅 정보 flush
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateSVRStateAndFlush( smrServerStatus   aSvrStatus,
                                                smLSN           * aEndLSN,
                                                UInt            * aLstCrtLog )
{
    UInt   sState = 0;

    IDE_DASSERT( aEndLSN != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    mLogAnchor->mServerStatus =  aSvrStatus;
    mLogAnchor->mMemEndLSN    = *aEndLSN;
    mLogAnchor->mLstCreatedLogFileNo = *aLstCrtLog;

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : loganchor에 서버상태 정보 flush
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateSVRStateAndFlush( smrServerStatus  aSvrStatus )
{
    UInt   sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    mLogAnchor->mServerStatus =  aSvrStatus;

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if (sState != 0)
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : loganchor에 서버가 사용하는 TXSEG Entry 개수를
 *               저장한다.
 *
 *               저장되는 값은 보정되지 않은 프로퍼티에서 읽은 값이다.
 *
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateTXSEGEntryCntAndFlush( UInt aEntryCnt )
{
    IDE_ASSERT( lock() == IDE_SUCCESS );

    mLogAnchor->mTXSEGEntryCnt = aEntryCnt;

    IDE_ASSERT( flushAll() == IDE_SUCCESS );
    IDE_ASSERT( unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}


/***********************************************************************
 * Description : loganchor에 서버상태 정보 flush
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateArchiveAndFlush( smiArchiveMode   aArchiveMode )
{
    UInt sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    mLogAnchor->mArchiveMode = aArchiveMode;

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;

}

/*
   Loganchor에 Disk Redo LSN 과 RedoLSN을 갱신한다 .

   [IN] aDiskRedoLSN   : 디스크 데이타베이스의 Restart Point
   [IN] aMemRedoLSN : 디스크 데이타베이스의 Restart Point
*/
IDE_RC smrLogAnchorMgr::updateRedoLSN( smLSN*  aDiskRedoLSN,
                                       smLSN*  aMemRedoLSN )
{
    UInt   sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    if ( aDiskRedoLSN != NULL )
    {
        mLogAnchor->mDiskRedoLSN = *aDiskRedoLSN;
    }
    else
    {
        /* nothing to do */
    }

    if ( aMemRedoLSN != NULL )
    {
        IDE_ASSERT( ( aMemRedoLSN->mFileNo != ID_UINT_MAX ) &&
                    ( aMemRedoLSN->mOffset != ID_UINT_MAX ) );

        SM_GET_LSN( mLogAnchor->mMemEndLSN,
                    *aMemRedoLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : loganchor에 Disk Redo LSN 정보 flush
 **********************************************************************/
IDE_RC smrLogAnchorMgr::updateResetLSN(smLSN*  aResetLSN)
{

    UInt   sState = 0;

    IDE_DASSERT ( aResetLSN != NULL );

    // Update하는 경우는 다음 2가지 경우이다.
    // 이 함수를 호출하기전에 Error 체크를 한다.

    // (1) In-Complete Media Recovery 수행완료시

    // 불완전복구이기 때문에 미디어 복구를 수행하고,
    // 과거의 Disk Redo LSN 보다 미래의 ResetLog LSN을
    // 설정한다.

    // (2) Meta ResetLogs 수행시
    // LSN MAX로 Update하기 때문에, mDiskRedoLSN보다 무조건 크다.

    IDE_ASSERT( smrCompareLSN::isLT( &mLogAnchor->mDiskRedoLSN,
                                     aResetLSN )
                == ID_TRUE );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    SM_GET_LSN( mLogAnchor->mResetLSN, *aResetLSN );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : loganchor에 MediaRecovery시 redo를 시작할 LSN 정보 flush
 **********************************************************************/
IDE_RC smrLogAnchorMgr::updateMediaRecoveryLSN( smLSN * aMediaRecoveryLSN )
{

    UInt   sState = 0;

    IDE_DASSERT( aMediaRecoveryLSN != NULL );

    /* aMediaRecoveryLSN의 값은 checkpoint LSN(mDiskRedoLSN or mBeginChkptLSN )
     * 보다 작거나 같아야 한다.
     * 단, create database시에는 mDiskRedoLSN와 mBeginChkptLSN은INIT 값으로
     * 들어 온다. */

    if ( (SM_IS_LSN_INIT(mLogAnchor->mDiskRedoLSN) == ID_FALSE ) &&
         (SM_IS_LSN_INIT(mLogAnchor->mBeginChkptLSN) == ID_FALSE) )
    {
        IDE_ASSERT( smrCompareLSN::isLTE( aMediaRecoveryLSN,
                                          &mLogAnchor->mDiskRedoLSN )
                    == ID_TRUE );

        IDE_ASSERT( smrCompareLSN::isLTE( aMediaRecoveryLSN,
                                          &mLogAnchor->mBeginChkptLSN )
                    == ID_TRUE );
    }

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    SM_GET_LSN( mLogAnchor->mMediaRecoveryLSN, *aMediaRecoveryLSN );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : loganchor에 checkpoint 정보 flush
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateChkptAndFlush( smLSN  * aBeginChkptLSN,
                                             smLSN  * aEndChkptLSN,
                                             smLSN  * aDiskRedoLSN,
                                             smLSN  * aEndLSN,
                                             UInt   * aFirstFileNo,
                                             UInt   * aLastFileNo )
{
    UInt sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    mLogAnchor->mBeginChkptLSN    = *aBeginChkptLSN;
    mLogAnchor->mDiskRedoLSN      = *aDiskRedoLSN;
    mLogAnchor->mEndChkptLSN      = *aEndChkptLSN;

    SM_GET_LSN( mLogAnchor->mMemEndLSN, *aEndLSN );

    /* BUG-39289 : 만약 업데이트할 LST(aLastFileNo)가 현재 로그 앵커의
       LST(mLogAnchor->mLstDeleteFileNo)와 같으면 체크포인트에서는 로그를
       지우지 않는다는 의미이다. 따라서 업데이트할 FST, LST값과 로그앵커의
       FST, LST값이 같다는 의미이므로 이 경우에는 로그앵커에 업데이트하지
       않는다. */
    if ( mLogAnchor->mLstDeleteFileNo != *aLastFileNo )
    {
        mLogAnchor->mFstDeleteFileNo = *aFirstFileNo;
        mLogAnchor->mLstDeleteFileNo = *aLastFileNo;
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : Recovery From Replication(proj-1608)을 위한 LSN 정보설정 및 flush
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateReplRecoveryLSN( smLSN aReplRecoveryLSN )
{

    UInt   sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    SM_GET_LSN( mLogAnchor->mReplRecoveryLSN, aReplRecoveryLSN );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_ASSERT(unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*
   PRJ-1548 User Memory Tablespace
   변경된 하나의 TBS Node를 Loganchor에 반영한다.

   sctTableSpaceMgr::lockSpaceNode()를 획득한 상태에서 호출되어야 한다.
*/

IDE_RC smrLogAnchorMgr::updateTBSNodeAndFlush( sctTableSpaceNode  * aSpaceNode )
{
    UInt      sState = 0;
    UInt      sAnchorOffset;

    IDE_DASSERT( aSpaceNode != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    getTBSAttrAndAnchorOffset( aSpaceNode->mID,
                               &mTableSpaceAttr,
                               &sAnchorOffset );

    /* write to loganchor buffer */
    IDE_TEST( updateToBuffer( (SChar*)&mTableSpaceAttr,
                              sAnchorOffset,
                              ID_SIZEOF(smiTableSpaceAttr) ) 
              != IDE_SUCCESS );
    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
   PRJ-1548 User Memory Tablespace
   변경된 하나의 DRDB DBF Node를 Loganchor에 반영한다.

   SpaceNode Mutex()을 획득한 상태에서 호출되어야 한다.
*/

IDE_RC smrLogAnchorMgr::updateDBFNodeAndFlush( sddDataFileNode  * aFileNode )
{
    UInt      sState = 0;

    IDE_DASSERT( aFileNode != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * 1. smrLogAnchor 버퍼 갱신
     * ----------------------------------------------*/
    sddDataFile::getDataFileAttr( aFileNode,
                                  &mDataFileAttr );

    IDE_TEST( updateToBuffer( (SChar *)&mDataFileAttr,
                              aFileNode->mAnchorOffset,
                              ID_SIZEOF(smiDataFileAttr) )
            != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
   PRJ-1548 User Memory Tablespace
   변경된 하나의 Checkpint Path Node를 Loganchor에 반영한다.

   SpaceNode Mutex()을 획득한 상태에서 호출되어야 한다.

   [IN] aChkptPathNode - 변경할 체크포인트 경로 노드
*/

IDE_RC smrLogAnchorMgr::updateChkptPathAttrAndFlush(
                                       smmChkptPathNode * aChkptPathNode )

{
    UInt      sState = 0;

    IDE_DASSERT( aChkptPathNode != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * 1. smrLogAnchor 버퍼 갱신
     * ----------------------------------------------*/
    IDE_TEST( updateToBuffer( (SChar*)& aChkptPathNode->mChkptPathAttr,
                              aChkptPathNode->mAnchorOffset,
                              ID_SIZEOF(aChkptPathNode->mChkptPathAttr) ) 
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
   PRJ-1548 User Memory Tablespace
   변경된 하나의 Checkpoint Path Node를 Loganchor에 반영한다.

   spaceNode Mutex을 획득한 상태에서 호출되어야 한다.
*/

IDE_RC smrLogAnchorMgr::updateChkptImageAttrAndFlush(
                              smmCrtDBFileInfo   * aCrtDBFileInfo,
                              smmChkptImageAttr  * aChkptImageAttr )
{
    UInt      sState = 0;

    IDE_DASSERT( aCrtDBFileInfo  != NULL );
    IDE_DASSERT( aChkptImageAttr != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * 1. smrLogAnchor 버퍼 갱신
     * ----------------------------------------------*/
    IDE_TEST( updateToBuffer( (SChar*)aChkptImageAttr,
                              aCrtDBFileInfo->mAnchorOffset,
                              ID_SIZEOF(smmChkptImageAttr) ) 
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
   PROJ-2102 Fast Secondary Buffer
   변경된 Secondary Buffer Node를 Loganchor에 반영한다.
*/

IDE_RC smrLogAnchorMgr::updateSBufferNodeAndFlush( sdsFileNode   * aFileNode )
{
    UInt      sState = 0;

    IDE_DASSERT( aFileNode  != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * 1. smrLogAnchor 버퍼 갱신
     * ----------------------------------------------*/
    sdsBufferMgr::getFileAttr( aFileNode,
                               &mSBufferFileAttr );

    IDE_TEST( updateToBuffer( (SChar*)&mSBufferFileAttr,
                              aFileNode->mAnchorOffset,
                              ID_SIZEOF(smiSBufferFileAttr) ) 
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/******************************************************************************
 * PRJ-1548 User Memory Tablespace
 * loganchor의 TBS Node 정보를 추가하는 함수
 ******************************************************************************/
IDE_RC smrLogAnchorMgr::addTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode,
                                            UInt              * aAnchorOffset )
{
    UInt    sState  = 0;

    IDE_DASSERT( aSpaceNode     != NULL );
    IDE_DASSERT( aAnchorOffset  != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    // New TableSpace ID 갱신
    mLogAnchor->mNewTableSpaceID = sctTableSpaceMgr::getNewTableSpaceID();

    switch( sctTableSpaceMgr::getTBSLocation( aSpaceNode ) )
    {
        case SMI_TBS_DISK:
            sddTableSpace::getTableSpaceAttr( (sddTableSpaceNode *)aSpaceNode,
                                              &mTableSpaceAttr );
            break;
        case SMI_TBS_MEMORY:
            smmManager::getTableSpaceAttr( (smmTBSNode*)aSpaceNode,
                                           &mTableSpaceAttr );
            break;
        case SMI_TBS_VOLATILE:
            svmManager::getTableSpaceAttr( (svmTBSNode*)aSpaceNode,
                                           &mTableSpaceAttr );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    // 기록하기 전에 메모리 버퍼 오프셋을 반환
    *aAnchorOffset = mWriteOffset;

    /* write to loganchor buffer */
    IDE_TEST( writeToBuffer( (SChar *)&mTableSpaceAttr,
                             ID_SIZEOF(smiTableSpaceAttr) )
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/* PRJ-1548 User Memory Tablespace
 * loganchor의 DBF Node 정보를 추가하는 함수 */
IDE_RC smrLogAnchorMgr::addDBFNodeAndFlush( sddTableSpaceNode * aSpaceNode,
                                            sddDataFileNode   * aFileNode,
                                            UInt              * aAnchorOffset )
{
    UInt      sState = 0;

    IDE_DASSERT( aSpaceNode    != NULL );
    IDE_DASSERT( aFileNode     != NULL );
    IDE_DASSERT( aAnchorOffset != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    // TBS Attr의 NewFileID 갱신
    sddTableSpace::getTableSpaceAttr( (sddTableSpaceNode *)aSpaceNode,
                                      &mTableSpaceAttr );

    IDE_TEST( updateToBuffer( (SChar*)&mTableSpaceAttr,
                              aSpaceNode->mAnchorOffset,
                              ID_SIZEOF(smiTableSpaceAttr) )
              != IDE_SUCCESS );

    // 새로 생성된 DBF Attr 정보 획득
    sddDataFile::getDataFileAttr( (sddDataFileNode *)aFileNode,
                                  &mDataFileAttr );


    // 기록하기 전에 메모리 버퍼 오프셋을 반환
    *aAnchorOffset = mWriteOffset;

    /* write to loganchor buffer */
    IDE_TEST( writeToBuffer( (SChar *)&mDataFileAttr,
                             ID_SIZEOF(smiDataFileAttr) )
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
   PRJ-1548 User Memory Tablespace
    loganchor의 ChkptPath Node 정보를 추가하는 함수
*/
IDE_RC smrLogAnchorMgr::addChkptPathNodeAndFlush( smmChkptPathNode  * aChkptPathNode,
                                                  UInt              * aAnchorOffset )
{
    UInt      sState = 0;

    IDE_DASSERT( aChkptPathNode != NULL );
    IDE_DASSERT( aAnchorOffset != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    // 기록하기 전에 메모리 버퍼 오프셋을 반환
    *aAnchorOffset = mWriteOffset;

    /* write to loganchor buffer */
    IDE_TEST( writeToBuffer( (SChar*)&(aChkptPathNode->mChkptPathAttr),
                             ID_SIZEOF(smiChkptPathAttr) )
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    PRJ-1548 User Memory Tablespace
    메모리 체크포인트 이미지생성시 로그앵커에 체크포인트이미지 속성을
    저장한다.

    [IN[  aChkptImageAttr : 체크포인트이미지의 속성
*/
IDE_RC smrLogAnchorMgr::addChkptImageAttrAndFlush(
                                            smmChkptImageAttr * aChkptImageAttr,
                                            UInt              * aAnchorOffset )
{
    UInt      sState = 0;

    IDE_DASSERT( aChkptImageAttr != NULL );
    IDE_DASSERT( aAnchorOffset   != NULL );

    IDE_ASSERT( smmManager::isCreateDBFileAtLeastOne(
                                           aChkptImageAttr->mCreateDBFileOnDisk )
                 == ID_TRUE );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    // 기록하기 전에 메모리 버퍼 오프셋을 반환
    *aAnchorOffset = mWriteOffset;

    /* write to loganchor buffer */
    IDE_TEST( writeToBuffer( (SChar*)aChkptImageAttr,
                             ID_SIZEOF( smmChkptImageAttr ) )
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/* Secondary Buffrer Node 정보를 LogAnchor에 추가한다. */
IDE_RC smrLogAnchorMgr::addSBufferNodeAndFlush( sdsFileNode  * aFileNode,
                                                UInt         * aAnchorOffset )
{
    UInt      sState = 0;

    IDE_DASSERT( aAnchorOffset != NULL );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    // 새로 생성된 DBF Attr 정보 획득
    sdsBufferMgr::getFileAttr( aFileNode,
                               &mSBufferFileAttr );

    // 기록하기 전에 메모리 버퍼 오프셋을 반환
    *aAnchorOffset = mWriteOffset;

     /* write to loganchor buffer */
    IDE_TEST( writeToBuffer( (SChar*)&mSBufferFileAttr,
                             ID_SIZEOF(smiSBufferFileAttr) )
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

     if ( sState != 0 )
     {
         IDE_ASSERT( unlock() == IDE_SUCCESS );
     }

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : checkpoint시 memory tablespace의 Stable No를 갱신한다.
 * checkpoint시에만 호출된다.
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateStableNoOfAllMemTBSAndFlush()
{
    UInt               sState = 0;
    sctTableSpaceNode* sCurrSpaceNode;
    sctTableSpaceNode* sNextSpaceNode;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * 1. tablespace정보를 mBuffer에 기록
     * 디스크관리자의 모든 tablespace 노드 리스트를 순회하면서
     * 요약정보를 얻어서 기록한다.
     * ----------------------------------------------*/
    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while ( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );

        switch( sctTableSpaceMgr::getTBSLocation( sCurrSpaceNode ) )
        {
            case SMI_TBS_DISK:
            case SMI_TBS_VOLATILE:
                {
                    sCurrSpaceNode = sNextSpaceNode;
                    continue;
                }
                break;
            case SMI_TBS_MEMORY:
                break;
            default:
                IDE_ERROR(0);
                break;
        }

        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( sCurrSpaceNode )
                  != IDE_SUCCESS );
        sState = 2;

        // drop/discarded/executing_drop_pending/offline된
        // tablespace는 skip 한다.
        if ( sctTableSpaceMgr::hasState( sCurrSpaceNode,
                                         SCT_SS_SKIP_CHECKPOINT )
             == ID_TRUE )
        {
            sState = 1;
            IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( sCurrSpaceNode )
                      != IDE_SUCCESS );

            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // memory tablespace에 대해서만 stable no를 갱신한다.
        IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( sCurrSpaceNode->mID )
                    == ID_TRUE );

        // memory tablespace node에 currentdb를 switch 시킨다.
        smmManager::switchCurrentDB((smmTBSNode*)sCurrSpaceNode );

        /* read from memory manager */
        smmManager::getTableSpaceAttr( (smmTBSNode*)sCurrSpaceNode,
                                       &mTableSpaceAttr );

        /* update to loganchor buffer */
        // [주의] Current DB만 변경한다.
        // 왜냐하면 CheckPoint시 TBS 노드에 Lock을 획득하지 않기 때문에
        // 다른 TBS DDL과 쫑이 날수 있으며, 원하지 않았던 변경사항을
        // 반영할 수 있기 때문이다.
        IDE_TEST( updateToBuffer(
                          (SChar*)&(mTableSpaceAttr.mMemAttr.mCurrentDB),
                          ((smmTBSNode*)sCurrSpaceNode)->mAnchorOffset +
                          offsetof(smiTableSpaceAttr, mMemAttr) +
                          offsetof(smiMemTableSpaceAttr, mCurrentDB),
                          ID_SIZEOF(mTableSpaceAttr.mMemAttr.mCurrentDB) )
                  != IDE_SUCCESS );

        sState = 1;
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( sCurrSpaceNode )
                  != IDE_SUCCESS );

        sCurrSpaceNode = sNextSpaceNode;
    }

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch ( sState )
        {
            case 2:
            IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex( sCurrSpaceNode )
                        == IDE_SUCCESS );
            case 1:
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

UInt smrLogAnchorMgr::getTBSAttrStartOffset()
{
    return ID_SIZEOF(smrLogAnchor);
}

/***********************************************************************
 * Description : loganchor에 tablespace 정보 Sorting하여 다시 Flush
 * Startup/Shutdown 시에만 호출된다.
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateAllTBSAndFlush()
{
    UInt               sState = 0;
    UInt               sLoop;
    UInt               sWhichDB;
    smuList*           sChkptPathList;
    smuList*           sChkptPathBaseList;
    sctTableSpaceNode* sCurrSpaceNode;
    sctTableSpaceNode* sNextSpaceNode;
    sddDataFileNode*   sFileNode;
    smmChkptPathNode * sChkptPathNode;
    smmChkptImageHdr   sChkptImageHdr;
    smmChkptImageAttr  sChkptImageAttr;
    smmDatabaseFile  * sDatabaseFile;
    smmTBSNode       * sMemTBSNode;
    UInt               i;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* ------------------------------------------------
     * 1. smrLogAnchor 버퍼 갱신
     * ----------------------------------------------*/
    mLogAnchor->mNewTableSpaceID = sctTableSpaceMgr::getNewTableSpaceID();

    /* ------------------------------------------------
     * TBS 정보를 기록할 위치를 설정함
     * ----------------------------------------------*/
    mWriteOffset = getTBSAttrStartOffset();

    /* ------------------------------------------------
     * 2. tablespace정보를 mBuffer에 기록
     * 디스크관리자의 모든 tablespace 노드 리스트를 순회하면서
     * 요약정보를 얻어서 기록한다.
     * ----------------------------------------------*/
    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while ( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );

        // PRJ-1548 User Memory Tablespace
        // DROPPED 상태의 테이블스페이스노드는 loganchor에
        // 저장하지 않는다. 그러므로, 서버구동시에 loganchor
        // 초기화시 DROPPED인 테이블스페이스 노드는 없다.
        if( SMI_TBS_IS_DROPPED(sCurrSpaceNode->mState) )
        {
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }

        if( sctTableSpaceMgr::isDiskTableSpace( sCurrSpaceNode->mID ) == ID_TRUE )
        {
            /* read from disk manager */
            sddTableSpace::getTableSpaceAttr(
                              (sddTableSpaceNode*)sCurrSpaceNode,
                              &mTableSpaceAttr);

            ((sddTableSpaceNode*)sCurrSpaceNode)->mAnchorOffset
                              = mWriteOffset;

            /* write to loganchor buffer */
            IDE_TEST( writeToBuffer((SChar*)&mTableSpaceAttr,
                                    (UInt)ID_SIZEOF(smiTableSpaceAttr))
                      != IDE_SUCCESS );

            /* ------------------------------------------------
             * loganchor 버퍼에 datafile attributes 정보를 갱신한다.
             * ----------------------------------------------*/

            for ( i = 0 ; i < ((sddTableSpaceNode*)sCurrSpaceNode)->mNewFileID ; i++ )
            {
                // 여기서는 sFileNode가 NULL일경우는 없음
                sFileNode=((sddTableSpaceNode*)sCurrSpaceNode)->mFileNodeArr[i];

                // alter tablespace drop datafile & restart
                if ( sFileNode == NULL )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                // PRJ-1548 User Memory Tablespace
                // DROPPED 상태의 데이타파일노드는 loganchor에
                // 저장하지 않는다. 그러므로, 서버구동시에 loganchor
                // 초기화시 DROPPED인 데이타파일 노드는 없다.
                if ( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                /* read from disk manager */
                sddDataFile::getDataFileAttr( sFileNode, &mDataFileAttr );

                sFileNode->mAnchorOffset = mWriteOffset;

                /* write to loganchor buffer */
                IDE_TEST( writeToBuffer( (SChar*)&mDataFileAttr,
                                         ID_SIZEOF(smiDataFileAttr) )
                          != IDE_SUCCESS );

            }
        }
        else if ( sctTableSpaceMgr::isMemTableSpace( sCurrSpaceNode->mID ) )
        // memory tablespace
        {
            /* read from memory manager */
            smmManager::getTableSpaceAttr( (smmTBSNode*)sCurrSpaceNode,
                                           &mTableSpaceAttr );

            ((smmTBSNode*)sCurrSpaceNode)->mAnchorOffset = mWriteOffset;
            /* write to loganchor buffer */
            IDE_TEST( writeToBuffer( (SChar*)&mTableSpaceAttr,
                                     ID_SIZEOF(smiTableSpaceAttr) )
                      != IDE_SUCCESS );

            /* ------------------------------------------------
             * loganchor 버퍼에 checkpoint path attributes
             * 정보를 갱신한다.
             * ----------------------------------------------*/
            sChkptPathBaseList = &( ((smmTBSNode*)sCurrSpaceNode)->mChkptPathBase );
            for ( sChkptPathList  = SMU_LIST_GET_FIRST(sChkptPathBaseList) ;
                  sChkptPathList != sChkptPathBaseList ;
                  sChkptPathList  = SMU_LIST_GET_NEXT(sChkptPathList) )
            {
                sChkptPathNode = (smmChkptPathNode*)sChkptPathList->mData;

                sChkptPathNode->mAnchorOffset = mWriteOffset;

                /* write to loganchor buffer */
                IDE_TEST( writeToBuffer(
                               (SChar*)&(sChkptPathNode->mChkptPathAttr),
                               ID_SIZEOF(smiChkptPathAttr) )
                          != IDE_SUCCESS );

            }

            // LstCreatedDBFile에 포함되더라도 오픈이 안되어 있는 경우에는
            // 존재하지 않을 수도 있다.

            sWhichDB = smmManager::getCurrentDB( (smmTBSNode*)sCurrSpaceNode );

            for ( sLoop = 0;
                  sLoop <= ((smmTBSNode*)sCurrSpaceNode)->mLstCreatedDBFile;
                  sLoop++ )
            {
                if ( sctTableSpaceMgr::hasState( sCurrSpaceNode->mID,
                                                 SCT_SS_SKIP_AGING_DISK_TBS ) 
                     != ID_TRUE )
                {
                    IDE_ASSERT( smmManager::openAndGetDBFile(
                                                    ((smmTBSNode*)sCurrSpaceNode),
                                                    sWhichDB,
                                                    sLoop,
                                                    &sDatabaseFile ) 
                                == IDE_SUCCESS );

                    IDE_ASSERT( sDatabaseFile != NULL );

                    // 만약 데이타파일이 오픈된 경우가 생성된 경우이므로,
                    // 로그앵커에 저장한다.
                    IDE_ASSERT( sDatabaseFile->isOpen() == ID_TRUE );

                    // 체크포인트이미지의 런타임 메타헤더를 얻는다.
                    sDatabaseFile->getChkptImageHdr( &sChkptImageHdr );

                    // 체크포인트이미지의 메타헤더를 검증한다.
                    IDE_ASSERT( sDatabaseFile->checkValuesOfDBFHdr( &sChkptImageHdr )
                                == IDE_SUCCESS );

                    // 로그앵커에 저장하기 위해 체크포인트이미지의
                    // 속성을 얻는다.
                    sDatabaseFile->getChkptImageAttr( (smmTBSNode*)sCurrSpaceNode,
                                                      &sChkptImageAttr );

                    // 로그앵커에 저장된 Chkpt Image Attribute라면
                    // Attribute의 CreateDBFileOnDisk 중 적어도 하나는 ID_TRUE
                    // 값을 가져야 한다.

                    IDE_ASSERT( smmManager::isCreateDBFileAtLeastOne(
                                             sChkptImageAttr.mCreateDBFileOnDisk )
                                == ID_TRUE );
                }
                else
                {
                    /* BUG-41689 A discarded tablespace is redone in recovery
                     * tablespace가 discard된 이후에 체크포인트이미지에 접근하면 안된다.
                     * 따라서, 로그 앵커 정보인 SpaceNode의 정보를 이용해
                     * sChkptImageAttr 정보를 구성 한다. */
                    sChkptImageAttr.mAttrType = SMI_CHKPTIMG_ATTR;
                    sChkptImageAttr.mSpaceID  = sCurrSpaceNode->mID;
                    sChkptImageAttr.mFileNum  = sLoop;

                    /* loganchor 에서 읽어온 chkpt image정보를 가져 온다. */
                    sMemTBSNode   = (smmTBSNode*)sCurrSpaceNode;
                    sDatabaseFile = (smmDatabaseFile*)(sMemTBSNode->mDBFile[sWhichDB][sLoop]);

                    SM_GET_LSN( sChkptImageAttr.mMemCreateLSN, sDatabaseFile->mChkptImageHdr.mMemCreateLSN );

                    for ( i = 0 ; i < SMM_PINGPONG_COUNT; i++ )
                    {
                        sChkptImageAttr.mCreateDBFileOnDisk[ i ]
                            = smmManager::getCreateDBFileOnDisk( sMemTBSNode,
                                                                 i,
                                                                 sLoop );
                    }

                    sChkptImageAttr.mDataFileDescSlotID.mBlockID = 
                        sDatabaseFile->mChkptImageHdr.mDataFileDescSlotID.mBlockID;
                    sChkptImageAttr.mDataFileDescSlotID.mSlotIdx = 
                        sDatabaseFile->mChkptImageHdr.mDataFileDescSlotID.mSlotIdx;

                }

                /* BUG-23763: Server Restart후 Memory내에 존해하는 TBS, File Node순서와
                 * LogAnchor Buffer의 Node순서를 맞출때 MDB File Node의 Anchor Offset을
                 * 갱신하지 않아서 차우 MDB File Node의 정보가 갱신때 이상한 위치에 Write하여
                 * Loganchor가 깨지는 버그가 있습니다. */
                ((smmTBSNode*)sCurrSpaceNode)->mCrtDBFileInfo[sLoop].mAnchorOffset =
                    mWriteOffset;

                /* write to loganchor buffer */
                IDE_TEST( writeToBuffer( (SChar*)&sChkptImageAttr,
                                         ID_SIZEOF( smmChkptImageAttr ) )
                          != IDE_SUCCESS );
            }
        }
        /* PROJ-1594 Volatile TBS */
        else if ( sctTableSpaceMgr::isVolatileTableSpace( sCurrSpaceNode->mID ) == ID_TRUE )
        {
            /* read from volatile manager */
            svmManager::getTableSpaceAttr( (svmTBSNode*)sCurrSpaceNode,
                                           &mTableSpaceAttr );

            ((svmTBSNode*)sCurrSpaceNode)->mAnchorOffset = mWriteOffset;
            /* write to loganchor buffer */
            IDE_TEST( writeToBuffer( (SChar*)&mTableSpaceAttr,
                                     ID_SIZEOF(smiTableSpaceAttr) )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_ASSERT(0);
        }

        sCurrSpaceNode = sNextSpaceNode;
    }

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;

}


/* PROJ-2102 Fast Secondary Buffer */
IDE_RC smrLogAnchorMgr::updateAllSBAndFlush( )
{
    sdsFileNode     * sFileNode  = NULL;
    UInt              sState     = 0;

    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE,  SKIP );

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    sdsBufferMgr::getFileNode( &sFileNode );

    IDE_ASSERT( sFileNode != NULL);

    sdsBufferMgr::getFileAttr( sFileNode,
                               &mSBufferFileAttr );

    sFileNode->mAnchorOffset = mWriteOffset;

    /* write to loganchor buffer */
    IDE_TEST( writeToBuffer( (SChar*)&mSBufferFileAttr,
                             ID_SIZEOF(smiSBufferFileAttr))
              != IDE_SUCCESS );

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 각 Attribute의 구조체의 크기를 반환한다.
 *
 *  aAttrType - [IN] 크기를 확인할 AttrType
 **********************************************************************/
UInt smrLogAnchorMgr::getAttrSize( smiNodeAttrType aAttrType )
{
    UInt sAttrSize = 0;

    switch ( aAttrType )
    {
        case SMI_TBS_ATTR:
            sAttrSize = ID_SIZEOF( smiTableSpaceAttr );
            break;

        case SMI_DBF_ATTR :
            sAttrSize = ID_SIZEOF( smiDataFileAttr );
            break;

        case SMI_CHKPTPATH_ATTR :
            sAttrSize = ID_SIZEOF( smiChkptPathAttr );
            break;

        case SMI_CHKPTIMG_ATTR:
            sAttrSize = ID_SIZEOF( smmChkptImageAttr );
            break;

        case SMI_SBUFFER_ATTR:
            sAttrSize = ID_SIZEOF( smiSBufferFileAttr );
            break;

        default:
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_INVALID_LOGANCHOR_ATTRTYPE,
                        aAttrType);
            IDE_ASSERT( 0 );
            break;

    }
    return sAttrSize;
}

/***********************************************************************
 * Description : 각 TBS의 Attribute와 Anchor Offset을 반환한다.
 *
 *  aSpaceID      - [IN]  확인할 TableSpace의 SpaceID
 *  aSpaceAttr    - [OUT] SpaceAttr을 반환한다.
 *  aAnchorOffset - [OUT] AnchorOffset을 반환한다.
 **********************************************************************/
IDE_RC smrLogAnchorMgr::getTBSAttrAndAnchorOffset( scSpaceID          aSpaceID,
                                                   smiTableSpaceAttr* aSpaceAttr,
                                                   UInt             * aAnchorOffset )
{
    sctTableSpaceNode* sSpaceNode ;

    IDE_ASSERT( aSpaceAttr    != NULL );
    IDE_ASSERT( aAnchorOffset != NULL );

    sSpaceNode     = sctTableSpaceMgr::findSpaceNodeIncludingDropped( aSpaceID );
    *aAnchorOffset = ID_SINT_MAX;

    IDE_ASSERT( sSpaceNode != NULL );

    IDU_FIT_POINT("BUG-46450@smrLogAnchorMgr::getTBSAttrAndAnchorOffset::getTBSLocation");

    switch ( sctTableSpaceMgr::getTBSLocation( sSpaceNode ) )
    {
        case SMI_TBS_DISK:
            sddTableSpace::getTableSpaceAttr( (sddTableSpaceNode*)sSpaceNode,
                                              aSpaceAttr );

            *aAnchorOffset = ((sddTableSpaceNode*)sSpaceNode)->mAnchorOffset;
            break;

        case SMI_TBS_MEMORY:
            smmManager::getTableSpaceAttr( (smmTBSNode*)sSpaceNode,
                                           aSpaceAttr );

            *aAnchorOffset = ((smmTBSNode*)sSpaceNode)->mAnchorOffset;
            break;

        case SMI_TBS_VOLATILE:
            svmManager::getTableSpaceAttr( (svmTBSNode*)sSpaceNode,
                                           aSpaceAttr );

            *aAnchorOffset = ((svmTBSNode*)sSpaceNode)->mAnchorOffset;
            break;

        default:
            IDE_ASSERT_MSG( 0,
                            "Tablespace Type not found ( ID : %"ID_UINT32_FMT", Name : %s ) \n",
                            sSpaceNode->mID,
                            sSpaceNode->mName );
            break;
    }

    return IDE_SUCCESS;

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

/***********************************************************************
 * Description : smmChkptImageAttr에 저장된 DataFileDescSlotID를 반환한다.
 * PROJ-2133 incremental backup
 * TODO  추가적인 검증 코드 필요?
 *
 *  aReadOffset         - [IN]  ChkptImageAttr이 위치한 loganchor Buffer의
 *                              offset
 *  aDataFileDescSlotID - [OUT] logAnchor에 저장된 DataFileDescSlotID를
 *                              반환한다.
 *
 **********************************************************************/
IDE_RC smrLogAnchorMgr::getDataFileDescSlotIDFromChkptImageAttr( 
                                UInt                       aReadOffset,
                                smiDataFileDescSlotID    * aDataFileDescSlotID )
{
    smmChkptImageAttr * sChkptImageAttr;

    IDE_ASSERT( aDataFileDescSlotID    != NULL );

    sChkptImageAttr = ( smmChkptImageAttr * )( mBuffer + aReadOffset );
    IDE_DASSERT( sChkptImageAttr->mAttrType == SMI_CHKPTIMG_ATTR );

    aDataFileDescSlotID->mBlockID = 
                    sChkptImageAttr->mDataFileDescSlotID.mBlockID;
    aDataFileDescSlotID->mSlotIdx = 
                    sChkptImageAttr->mDataFileDescSlotID.mSlotIdx;


   return IDE_SUCCESS;
}

/***********************************************************************
 * Description : DBFNodeAttr에 저장된 DataFileDescSlotID를 반환한다.
 * PROJ-2133 incremental backup
 * TODO  추가적인 검증 코드 필요?
 *
 *  aReadOffset         - [IN]  ChkptImageAttr이 위치한 loganchor Buffer의
 *                              offset
 *  aDataFileDescSlotID - [OUT] logAnchor에 저장된 DataFileDescSlotID를
 *                              반환한다.
 *
 **********************************************************************/
IDE_RC smrLogAnchorMgr::getDataFileDescSlotIDFromDBFNodeAttr( 
                                UInt                       aReadOffset,
                                smiDataFileDescSlotID    * aDataFileDescSlotID )
{
    smiDataFileAttr * sDBFNodeAttr;

    IDE_ASSERT( aDataFileDescSlotID    != NULL );

    sDBFNodeAttr = ( smiDataFileAttr * )( mBuffer + aReadOffset );
    IDE_DASSERT( sDBFNodeAttr->mAttrType = SMI_DBF_ATTR );

    aDataFileDescSlotID->mBlockID = 
                    sDBFNodeAttr->mDataFileDescSlotID.mBlockID;
    aDataFileDescSlotID->mSlotIdx = 
                    sDBFNodeAttr->mDataFileDescSlotID.mSlotIdx;


   return IDE_SUCCESS;
}

/***********************************************************************
 * Description : log anchor buffer를 각 node와 비교 점검한다.
 *               log anchor buffer값을 기준으로 space node를 확인한다.
 **********************************************************************/
idBool smrLogAnchorMgr::checkLogAnchorBuffer()
{
    UInt               sAnchorOffset;
    smiNodeAttrType    sAttrType;
    idBool             sIsValidAttr;
    idBool             sIsValidAnchor = ID_TRUE;
    UChar*             sAttrPtr;
    smiTableSpaceAttr  sSpaceAttr;
    smiDataFileAttr    sDBFileAttr;
    smiSBufferFileAttr sSBufferFileAttr;    

    sAnchorOffset = getTBSAttrStartOffset();

    while ( sAnchorOffset < mWriteOffset )
    {
        sAttrPtr = mBuffer + sAnchorOffset;
        idlOS::memcpy( &sAttrType, sAttrPtr, ID_SIZEOF(smiNodeAttrType) );

        switch ( sAttrType )
        {
            case SMI_TBS_ATTR :
                idlOS::memcpy( &sSpaceAttr, sAttrPtr,
                               ID_SIZEOF(smiTableSpaceAttr) );

                sIsValidAttr = checkTBSAttr( &sSpaceAttr,
                                             sAnchorOffset ) ;
                break;

            case SMI_DBF_ATTR :
                idlOS::memcpy( &sDBFileAttr, sAttrPtr,
                               ID_SIZEOF(smiDataFileAttr) );

                sIsValidAttr = checkDBFAttr( &sDBFileAttr,
                                             sAnchorOffset ) ;
                break;

            case SMI_CHKPTPATH_ATTR :
                /*   XXX ChkptPath와 ChkptImage확인은
                 *  동시성이 확보되지 않은 문제로 인하여
                 *  동시성이 확보 될 때 까지 일단 제외한다. */
/*
                idlOS::memcpy( &sChkptPathAttr, sAttrPtr,
                               ID_SIZEOF(smiChkptPathAttr) );
                sIsValidAttr = checkChkptPathAttr( &sChkptPathAttr,
                                                   sAnchorOffset ) ;
*/
                sIsValidAttr = ID_TRUE;
                break;

            case SMI_CHKPTIMG_ATTR :
/*
                idlOS::memcpy( &sChkptImgAttr, sAttrPtr,
                               ID_SIZEOF(smmChkptImageAttr) );
                sIsValidAttr = checkChkptImageAttr( &sChkptImgAttr,
                                                  sAnchorOffset ) ;
*/
                sIsValidAttr = ID_TRUE;
                break;

            case SMI_SBUFFER_ATTR :
                idlOS::memcpy( &sSBufferFileAttr, 
                               sAttrPtr,
                               ID_SIZEOF(smiSBufferFileAttr) );

                sIsValidAttr = checkSBufferFileAttr( &sSBufferFileAttr,
                                                     sAnchorOffset ) ;
                break; 

            default :
                ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                             SM_TRC_INVALID_LOGANCHOR_ATTRTYPE,
                             sAttrType );
                IDE_ASSERT( 0 );
                break;
        }

        if ( sIsValidAttr != ID_TRUE )
        {
            sIsValidAnchor = ID_FALSE;
        }
        else
        {
            /* nothing to do */
        }

        sAnchorOffset += getAttrSize( sAttrType );
    }

    if ( sAnchorOffset != mWriteOffset )
    {
        // 마지막 위치는 버퍼의 mWriteOffset 값과 같아야 한다.
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_INVALID_OFFSET,
                     sAnchorOffset,
                     mWriteOffset );

        sIsValidAnchor = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    return sIsValidAnchor;
}

/***********************************************************************
 * Description : TBS의 Space Node와 LogAnchor Buffer를 비교 검사한다.
 *
 *  aSpaceAttrByAnchor - [IN] SpaceNode와 비교할 LogAnchor의 Attribute
 *  aOffsetByAnchor    - [IN] 비교 검사할 Attribute의 Buffer Offset
 **********************************************************************/
idBool smrLogAnchorMgr::checkTBSAttr( smiTableSpaceAttr* aSpaceAttrByAnchor,
                                      UInt               aOffsetByAnchor )
{
    sctTableSpaceNode* sSpaceNode = NULL;
    UInt               sOffsetByNode;
    UInt               sTBSState;
    idBool             sIsValid   = ID_TRUE;

    IDE_ASSERT( aSpaceAttrByAnchor != NULL );
    IDE_ASSERT( aSpaceAttrByAnchor->mAttrType == SMI_TBS_ATTR );

    sTBSState = aSpaceAttrByAnchor->mTBSStateOnLA;

    if ( ( sTBSState & SMI_TBS_DROPPED ) != SMI_TBS_DROPPED )
    {
        // Drop되지 않았다면 Attr을 서로 비교한다.

        getTBSAttrAndAnchorOffset( aSpaceAttrByAnchor->mID,
                                   &mTableSpaceAttr,
                                   &sOffsetByNode );

        // SpaceNode에 AnchorOffset이 제대로 기록 되었는지 확인한다.
        // 다만 space node create시엔 logAnchor File에 flush한 후에
        // SpaceNode에 AnchorOffset을 기록하기 때문에 아직 기록되지
        // 않았을 수도 있다.
        if ( ( sOffsetByNode != aOffsetByAnchor ) &&
             ( sOffsetByNode != SCT_UNSAVED_ATTRIBUTE_OFFSET ) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_VOL_TBS,
                         aSpaceAttrByAnchor->mID,
                         aOffsetByAnchor,
                         sOffsetByNode );

            sIsValid = ID_FALSE;
        }

        // offset이 달라도 값을 확인하기 위해서 tbs attr 내용을
        // 확인한다.
        if ( cmpTableSpaceAttr( &mTableSpaceAttr,
                                aSpaceAttrByAnchor) != ID_TRUE )
        {
            sIsValid = ID_FALSE;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        // 만약 state가 Drop이면 정말로 Drop되었는지 확인한다.

        sSpaceNode = sctTableSpaceMgr::findSpaceNodeIncludingDropped( aSpaceAttrByAnchor->mID );

        // server start 시 null일 수도 있다.
        if ( sSpaceNode != NULL )
        {
            sTBSState = sSpaceNode->mState ;
            IDE_ASSERT( SMI_TBS_IS_DROPPED(sTBSState) );
        }
        else
        {
            /* nothing to do */
        }
    }

    return sIsValid;
}

/***********************************************************************
 * Description : Disk TBS의 File Node를 LogAnchor Buffer와 비교 검사한다.
 *
 *  aFileAttrByAnchor - [IN] File Node와 비교할 LogAnchor의 Attribute
 *  aOffsetByAnchor   - [IN] 비교 검사할 Attribute의 Buffer Offset
 **********************************************************************/
idBool smrLogAnchorMgr::checkDBFAttr( smiDataFileAttr* aFileAttrByAnchor,
                                      UInt             aOffsetByAnchor )
{
    sddTableSpaceNode* sSpaceNode = NULL;
    sddDataFileNode  * sFileNode  = NULL;
    idBool             sIsValid   = ID_TRUE;
    UInt               sFileAttrState;

    IDE_ASSERT( aFileAttrByAnchor != NULL );
    IDE_ASSERT( aFileAttrByAnchor->mAttrType == SMI_DBF_ATTR );

    sFileAttrState = aFileAttrByAnchor->mState ;

    if ( SMI_FILE_STATE_IS_NOT_DROPPED( sFileAttrState ) )
    {
        //-----------------------------------------
        // File node를 가져온다.
        //-----------------------------------------

        IDE_ASSERT( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                                                    aFileAttrByAnchor->mSpaceID,
                                                    (void**)&sSpaceNode )
                     == IDE_SUCCESS );

        IDE_ASSERT( sSpaceNode != NULL );

        IDE_ASSERT( aFileAttrByAnchor->mID < sSpaceNode->mNewFileID );

        sFileNode = sSpaceNode->mFileNodeArr[ aFileAttrByAnchor->mID ];

        IDE_ASSERT( sFileNode != NULL );

        //-----------------------------------------
        // offset 및 attr 검사
        //-----------------------------------------

        if ( ( sFileNode->mAnchorOffset != aOffsetByAnchor ) &&
             ( sFileNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET ) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_DBFILE,
                         aFileAttrByAnchor->mSpaceID,
                         aFileAttrByAnchor->mID,
                         aOffsetByAnchor,
                         sFileNode->mAnchorOffset );

            sIsValid = ID_FALSE;
        }

        sddDataFile::getDataFileAttr( sFileNode,
                                      &mDataFileAttr );

        if ( cmpDataFileAttr( &mDataFileAttr,
                              aFileAttrByAnchor ) != ID_TRUE )
        {
            sIsValid = ID_FALSE;
        }
    }
    else
    {
        //-----------------------------------------
        // table space가 drop인지 확인.
        //-----------------------------------------

        sSpaceNode = (sddTableSpaceNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aFileAttrByAnchor->mSpaceID );

        // server start 시 drop이면 null일 수도 있다.
        if ( sSpaceNode != NULL )
        {
            sFileAttrState = sSpaceNode->mHeader.mState ;

            if ( ( sFileAttrState & SMI_TBS_DROPPED ) != SMI_TBS_DROPPED )
            {
                // TBS가 drop이 아니라면 file node가 drop인지 확인

                IDE_ASSERT( aFileAttrByAnchor->mID < sSpaceNode->mNewFileID );

                sFileNode = sSpaceNode->mFileNodeArr[ aFileAttrByAnchor->mID ];

                // server start 시 drop이면 null일 수도 있다.
                if ( sFileNode != NULL )
                {
                    sFileAttrState = sFileNode->mState ;

                    IDE_ASSERT( SMI_FILE_STATE_IS_DROPPED(  sFileAttrState ) );
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

    return sIsValid;
}

/***********************************************************************
 * Description : LogAnchor buffer의 Checkpoint Path Attr에 해당하는
 *               Checkpoint Path node가 존재하는지 확인한다.
 *
 *  aCPPathAttrByAnchor - [IN] CP Path Node와 비교할 LogAnchor의 Attribute
 *  aOffsetByAnchor     - [IN] 비교 검사할 Attribute의 Buffer Offset
 **********************************************************************/
idBool smrLogAnchorMgr::checkChkptPathAttr( smiChkptPathAttr* aCPPathAttrByAnchor,
                                            UInt              aOffsetByAnchor )
{
    smuList*           sChkptPathList;
    smuList*           sChkptPathBaseList;
    smmChkptPathNode * sChkptPathNode;
    smiChkptPathAttr * sCPPathAttrByNode;
    smmTBSNode*        sSpaceNode  = NULL;
    idBool             sIsValid    = ID_TRUE;
    UInt               sDropState;

    IDE_ASSERT( aCPPathAttrByAnchor->mAttrType == SMI_CHKPTPATH_ATTR );

    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aCPPathAttrByAnchor->mSpaceID );

    // TBS가 Drop되면 restart시 loganchor에는 있으나
    // space node는 null이 될 수도 있다.
    IDE_TEST_CONT( sSpaceNode == NULL , Skip_Attr_Compare );

    // TBS를 확인해서 Drop이 아닐 경우에만 확인
    sDropState = sSpaceNode->mHeader.mState ;

    IDE_TEST_CONT( SMI_TBS_IS_DROPPED(sDropState) ||
                   SMI_TBS_IS_DROPPING(sDropState), 
                   Skip_Attr_Compare );

    sIsValid = ID_FALSE;

    sChkptPathBaseList = &(sSpaceNode->mChkptPathBase);

    for ( sChkptPathList  = SMU_LIST_GET_FIRST(sChkptPathBaseList);
          sChkptPathList != sChkptPathBaseList;
          sChkptPathList  = SMU_LIST_GET_NEXT(sChkptPathList) )
    {
        sChkptPathNode    = (smmChkptPathNode*)sChkptPathList->mData;
        sCPPathAttrByNode = &(sChkptPathNode->mChkptPathAttr);

        // 동일안 값을 가진 Path Node가 있는지 확인한다.
        if ( ( sCPPathAttrByNode->mSpaceID != aCPPathAttrByAnchor->mSpaceID ) ||
             ( idlOS::strcmp( sCPPathAttrByNode->mChkptPath,
                             aCPPathAttrByAnchor->mChkptPath ) != 0 ) )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // chkpt node를 찾았다
        sIsValid    = ID_TRUE;

        // 값 허용범위 조사
        if ( ( sCPPathAttrByNode->mAttrType != SMI_CHKPTPATH_ATTR ) ||
             ( sCPPathAttrByNode->mSpaceID  >= SC_MAX_SPACE_COUNT ) ||
             ( idlOS::strlen(sCPPathAttrByNode->mChkptPath)
               >  SMI_MAX_CHKPT_PATH_NAME_LEN ) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_CHKPT_PATH,
                         sSpaceNode->mHeader.mID,
                         aOffsetByAnchor,
                         sChkptPathNode->mAnchorOffset );

            sIsValid = ID_FALSE;
        }
        else
        {
            /* nothing to do */
        }

        // offset확인
        if ( ( sChkptPathNode->mAnchorOffset != aOffsetByAnchor ) &&
             ( sChkptPathNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET ) )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_CHKPT_PATH,
                         sSpaceNode->mHeader.mID,
                         aOffsetByAnchor,
                         sChkptPathNode->mAnchorOffset );

            sIsValid = ID_FALSE;
        }
        else
        {
            /* nothing to do */
        }

        break;
    }

    if ( sIsValid != ID_TRUE )
    {
        // 값이 틀리다면 Anchor 정보 출력
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_CHKPT_PATH_ATTR_INVALID,
                     aCPPathAttrByAnchor->mSpaceID,
                     aCPPathAttrByAnchor->mAttrType,
                     aCPPathAttrByAnchor->mChkptPath,
                     aOffsetByAnchor,
                     mWriteOffset,
                     ID_SIZEOF( smiChkptPathAttr ) );

        for ( sChkptPathList  = SMU_LIST_GET_FIRST(sChkptPathBaseList);
              sChkptPathList != sChkptPathBaseList;
              sChkptPathList  = SMU_LIST_GET_NEXT(sChkptPathList) )
        {
            sChkptPathNode    = (smmChkptPathNode*)sChkptPathList->mData;
            sCPPathAttrByNode = &(sChkptPathNode->mChkptPathAttr);

            // 동일한 값을 가진 node가 없다면 node 값을 출력
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_ANCHOR_CHECK_CHKPT_PATH_ATTR_NOT_FOUND,
                         sCPPathAttrByNode->mSpaceID,
                         sCPPathAttrByNode->mAttrType,
                         sCPPathAttrByNode->mChkptPath );
        }
        IDE_ASSERT( 0 );
    }

    IDE_EXCEPTION_CONT( Skip_Attr_Compare );

    return sIsValid;
}

/***********************************************************************
 * Description : LogAnchor buffer의 Checkpoint Image Attr에 해당하는
 *               Checkpoint Image node가 존재하는지 확인한다.
 *
 *  aCPImgAttrByAnchor - [IN] CP Image Node와 비교할 LogAnchor의 Attribute
 *  aOffsetByAnchor    - [IN] 비교 검사할 Attribute의 Buffer Offset
 **********************************************************************/
idBool smrLogAnchorMgr::checkChkptImageAttr( smmChkptImageAttr* aCPImgAttrByAnchor,
                                             UInt               aOffsetByAnchor )
{
    UInt               sWhichDB;
    UInt               sOffsetByNode;
    smmDatabaseFile  * sDatabaseFile;
    smmTBSNode*        sSpaceNode = NULL;
    idBool             sIsValid   = ID_TRUE;
    UInt               sDropState;

    IDE_ASSERT( aCPImgAttrByAnchor->mAttrType == SMI_CHKPTIMG_ATTR );

    sSpaceNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeIncludingDropped( aCPImgAttrByAnchor->mSpaceID );

    // TBS가 Drop되면 restart시 loganchor에는 있으나
    // space node는 null이 될 수도 있다.
    IDE_TEST_RAISE( sSpaceNode == NULL , Skip_Attr_Compare );

    // TBS를 확인해서 Drop이 아닐 경우에만 확인

    sDropState = sSpaceNode->mHeader.mState ;

    IDE_TEST_RAISE( SMI_TBS_IS_DROPPED(sDropState) ||
                    SMI_TBS_IS_DROPPING(sDropState) , 
                    Skip_Attr_Compare);

    for ( sWhichDB = 0 ; sWhichDB < SMM_PINGPONG_COUNT ; sWhichDB++ )
    {
        if ( aCPImgAttrByAnchor->mCreateDBFileOnDisk[ sWhichDB ] != ID_TRUE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        if ( smmManager::getDBFile( sSpaceNode,
                                    sWhichDB,
                                    aCPImgAttrByAnchor->mFileNum,
                                    SMM_GETDBFILEOP_NONE,
                                    &sDatabaseFile ) == IDE_SUCCESS )
        {
            IDE_ASSERT( sDatabaseFile != NULL );

            sDatabaseFile->getChkptImageAttr( sSpaceNode,
                                              &mChkptImageAttr );

            sOffsetByNode = smmManager::getAnchorOffsetFromCrtDBFileInfo(
                                                        sSpaceNode,
                                                        mChkptImageAttr.mFileNum );

            if ( ( sOffsetByNode != aOffsetByAnchor ) &&
                 ( sOffsetByNode != SCT_UNSAVED_ATTRIBUTE_OFFSET ) )
            {
                ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                             SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_CHKPT_IMG,
                             sSpaceNode->mHeader.mID,
                             aOffsetByAnchor,
                             sOffsetByNode );

                sIsValid = ID_FALSE;
            }
            else
            {
                /* nothing to do */
            }

            // offset이 달라도 내용 확인을 위해 비교검사한다.
            if ( cmpChkptImageAttr( &mChkptImageAttr,
                                    aCPImgAttrByAnchor ) != ID_TRUE )
            {
                sIsValid  = ID_FALSE;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            sIsValid = ID_FALSE;

            // 동일한 값을 가진 node가 없다면 ChkptImageAttr 값을 출력
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_ANCHOR_CHECK_CHKPT_IMAGE_ATTR_NOT_FOUND,
                         aCPImgAttrByAnchor->mSpaceID,
                         aCPImgAttrByAnchor->mAttrType,
                         sWhichDB,
                         aCPImgAttrByAnchor->mFileNum,
                         aCPImgAttrByAnchor->mCreateDBFileOnDisk[0],
                         aCPImgAttrByAnchor->mCreateDBFileOnDisk[1],
                         aOffsetByAnchor,
                         mWriteOffset,
                         ID_SIZEOF( smmChkptImageAttr ) );


            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         "MemCreateLSN (%"ID_UINT32_FMT"/%"ID_UINT32_FMT")",
                         aCPImgAttrByAnchor->mMemCreateLSN.mFileNo,
                         aCPImgAttrByAnchor->mMemCreateLSN.mOffset );
        }
    }

    IDE_EXCEPTION_CONT( Skip_Attr_Compare );

    /* BUG-40385 sResult 값에 따라 Failure 리턴일 수 있으므로,
     * 위에 IDE_TEST_RAISE -> IDE_TEST_CONT 로 변환하지 않는다. */
    return sIsValid;
}


/***********************************************************************
 * Description :  File Node를 LogAnchor Buffer와 비교 검사한다.
 *
 *  aFileAttrByAnchor - [IN] File Node와 비교할 LogAnchor의 Attribute
 *  aOffsetByAnchor   - [IN] 비교 검사할 Attribute의 Buffer Offset
 **********************************************************************/
idBool smrLogAnchorMgr::checkSBufferFileAttr( smiSBufferFileAttr* aFileAttrByAnchor,
                                              UInt                aOffsetByAnchor )
{
    sdsFileNode     * sFileNode  = NULL;
    idBool            sIsValid   = ID_TRUE;

    IDE_ASSERT( aFileAttrByAnchor != NULL );
    IDE_ASSERT( aFileAttrByAnchor->mAttrType == SMI_SBUFFER_ATTR );

    //-----------------------------------------
    // File node를 가져온다.
    //-----------------------------------------

    sdsBufferMgr::getFileNode ( &sFileNode );

    IDE_ASSERT( sFileNode != NULL );

    //-----------------------------------------
    // offset 및 attr 검사
    //-----------------------------------------

    if ( ( sFileNode->mAnchorOffset != aOffsetByAnchor ) &&
         ( sFileNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET ) )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_INVALID_OFFSET_IN_SECONDARY_BUFFER,
                     aOffsetByAnchor,
                     sFileNode->mAnchorOffset );

        sIsValid = ID_FALSE;
    }

    sdsBufferMgr::getFileAttr( sFileNode,
                               &mSBufferFileAttr );

    if ( cmpSBufferFileAttr( &mSBufferFileAttr,
                             aFileAttrByAnchor ) != ID_TRUE )
    {
        sIsValid = ID_FALSE;
    }
    return sIsValid;
}

/***********************************************************************
 * Description : TBS Node와 LogAnchor Buffer를 비교한다.
 *
 *  aSpaceAttrByNode   - [IN] Space Node에서 추출한 Attribute
 *  aSpaceAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 **********************************************************************/
idBool smrLogAnchorMgr::cmpTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                                           smiTableSpaceAttr* aSpaceAttrByAnchor)

{
    idBool sIsValid = ID_TRUE;
    idBool sIsEqual = ID_TRUE;

    IDE_DASSERT( aSpaceAttrByNode   != NULL );
    IDE_DASSERT( aSpaceAttrByAnchor != NULL );

    // 허용되는 차이, 경고만 출력
    if ( ( aSpaceAttrByAnchor->mAttrFlag     != aSpaceAttrByNode->mAttrFlag ) ||
         ( aSpaceAttrByAnchor->mTBSStateOnLA != aSpaceAttrByNode->mTBSStateOnLA ) )
    {
        sIsEqual = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 허용되지않는 차이, 오류 반환
    if ( ( aSpaceAttrByAnchor->mAttrType != aSpaceAttrByNode->mAttrType ) ||
         ( aSpaceAttrByAnchor->mID       != aSpaceAttrByNode->mID ) ||
         ( aSpaceAttrByAnchor->mType     != aSpaceAttrByNode->mType ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 값의 허용범위 초과, 오류 반환
    if ( ( aSpaceAttrByNode->mAttrType   != SMI_TBS_ATTR ) ||
         ( aSpaceAttrByNode->mNameLength >  SMI_MAX_TABLESPACE_NAME_LEN ) ||
         ( aSpaceAttrByNode->mID         >= SC_MAX_SPACE_COUNT ) ||
         ( aSpaceAttrByNode->mType       >= SMI_TABLESPACE_TYPE_MAX ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    IDU_FIT_POINT("BUG-46450@smrLogAnchorMgr::cmpTableSpaceAttr::getTBSLocation");

    switch ( sctTableSpaceMgr::getTBSLocation( aSpaceAttrByAnchor->mID ) )
    {
        case SMI_TBS_DISK:
            cmpDiskTableSpaceAttr( aSpaceAttrByNode,
                                   aSpaceAttrByAnchor,
                                   sIsEqual,
                                   &sIsValid );
            break;

        case SMI_TBS_MEMORY:
            cmpMemTableSpaceAttr( aSpaceAttrByNode,
                                  aSpaceAttrByAnchor,
                                  sIsEqual,
                                  &sIsValid );
            break;

        case SMI_TBS_VOLATILE:
            cmpVolTableSpaceAttr( aSpaceAttrByNode,
                                  aSpaceAttrByAnchor,
                                  sIsEqual,
                                  &sIsValid  );
            break;

        default:
            IDE_ASSERT_MSG( 0,
                            "Tablespace Type not found ( ID : %"ID_UINT32_FMT", Name : %s ) \n",
                            aSpaceAttrByAnchor->mID,
                            aSpaceAttrByAnchor->mName );
            break;
    }

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;
#endif

    return sIsValid;
}

/***********************************************************************
 * Description : Disk TBS Node와 LogAnchor Buffer를 비교한다.
 *
 *  aSpaceAttrByNode   - [IN] Space Node에서 추출한 Attribute
 *  aSpaceAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 *  aIsEqual           - [IN] 공통 영역의 Space Attr에 허용되는 범위
 *                            안에서 차이가 있다.
 *  aIsValid           - [IN/OUT] 공통 영역의 TableSpace Attr에 치명적인
 *                                문제가 있는지 여부를 나타내고 이후
 *                                개별 영역의 비교 결과를 반환한다.
 **********************************************************************/
void smrLogAnchorMgr::cmpDiskTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                                             smiTableSpaceAttr* aSpaceAttrByAnchor,
                                             idBool             aIsEqual,
                                             idBool           * aIsValid )
{
    smiDiskTableSpaceAttr * sDiskAttrByNode;
    smiDiskTableSpaceAttr * sDiskAttrByAnchor;

    IDE_DASSERT( aSpaceAttrByNode   != NULL );
    IDE_DASSERT( aSpaceAttrByAnchor != NULL );

    sDiskAttrByNode   = &aSpaceAttrByNode->mDiskAttr;
    sDiskAttrByAnchor = &aSpaceAttrByAnchor->mDiskAttr;

    // 허용되는 차이, 경고만 출력
    if ( sDiskAttrByAnchor->mNewFileID != sDiskAttrByNode->mNewFileID )
    {
        aIsEqual = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 허용되지않는 차이, 오류 반환
    if ( ( sDiskAttrByAnchor->mExtMgmtType  != sDiskAttrByNode->mExtMgmtType ) ||
         ( sDiskAttrByAnchor->mExtPageCount != sDiskAttrByNode->mExtPageCount ) ||
         ( sDiskAttrByAnchor->mSegMgmtType  != sDiskAttrByNode->mSegMgmtType ) ||
         ( sDiskAttrByAnchor->mNewFileID    >  SD_MAX_FID_COUNT ) ||
         ( sDiskAttrByAnchor->mExtMgmtType  >= SMI_EXTENT_MGMT_MAX ) ||
         ( sDiskAttrByAnchor->mSegMgmtType  >= SMI_SEGMENT_MGMT_MAX ) )
    {
        *aIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 오류 출력
    if ( ( aIsEqual  != ID_TRUE ) ||
         ( *aIsValid != ID_TRUE ) )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_DISK_TBS_ATTR,

                     "SpaceNode",
                     aSpaceAttrByNode->mAttrType,
                     aSpaceAttrByNode->mID,
                     aSpaceAttrByNode->mName,
                     idlOS::strlen(aSpaceAttrByNode->mName),
                     aSpaceAttrByNode->mType,
                     aSpaceAttrByNode->mTBSStateOnLA,
                     aSpaceAttrByNode->mAttrFlag,
                     sDiskAttrByNode->mNewFileID,
                     sDiskAttrByNode->mExtMgmtType,
                     sDiskAttrByNode->mExtPageCount );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_DISK_TBS_ATTR,

                     "LogAnchor",
                     aSpaceAttrByAnchor->mAttrType,
                     aSpaceAttrByAnchor->mID,
                     aSpaceAttrByAnchor->mName,
                     aSpaceAttrByAnchor->mNameLength,
                     aSpaceAttrByAnchor->mType,
                     aSpaceAttrByAnchor->mTBSStateOnLA,
                     aSpaceAttrByAnchor->mAttrFlag,
                     sDiskAttrByAnchor->mNewFileID,
                     sDiskAttrByAnchor->mExtMgmtType,
                     sDiskAttrByAnchor->mExtPageCount );
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : Memory TBS Node와 LogAnchor Buffer를 비교한다.
 *
 *  aSpaceAttrByNode   - [IN] Space Node에서 추출한 Attribute
 *  aSpaceAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 *  aIsEqual           - [IN] 공통 영역의 Space Attr에 허용되는 범위
 *                            안에서 차이가 있다.
 *  aIsValid           - [IN/OUT] 공통 영역의 TableSpace Attr에 치명적인
 *                                문제가 있는지 여부를 나타내고, 이후
 *                                개별 영역의 비교 결과를 반환한다.
 **********************************************************************/
void smrLogAnchorMgr::cmpMemTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                                            smiTableSpaceAttr* aSpaceAttrByAnchor,
                                            idBool             aIsEqual,
                                            idBool           * aIsValid )
{
    smiMemTableSpaceAttr * sMemAttrByNode;
    smiMemTableSpaceAttr * sMemAttrByAnchor;

    IDE_ASSERT( aSpaceAttrByNode   != NULL );
    IDE_ASSERT( aSpaceAttrByAnchor != NULL );

    sMemAttrByNode   = &aSpaceAttrByNode->mMemAttr;
    sMemAttrByAnchor = &aSpaceAttrByAnchor->mMemAttr;

    // 허용되는 차이, 경고만 출력
    if ( ( sMemAttrByAnchor->mIsAutoExtend   != sMemAttrByNode->mIsAutoExtend )  ||
         ( sMemAttrByAnchor->mMaxPageCount   != sMemAttrByNode->mMaxPageCount )  ||
         ( sMemAttrByAnchor->mNextPageCount  != sMemAttrByNode->mNextPageCount ) ||
         ( sMemAttrByAnchor->mShmKey         != sMemAttrByNode->mShmKey )    ||
         ( sMemAttrByAnchor->mCurrentDB      != sMemAttrByNode->mCurrentDB ) ||
         ( sMemAttrByAnchor->mChkptPathCount != sMemAttrByNode->mChkptPathCount ) ||
         ( sMemAttrByAnchor->mSplitFilePageCount
          != sMemAttrByNode->mSplitFilePageCount ) )
    {
        aIsEqual = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 허용되지않는 차이, 오류 반환
    if ( ( sMemAttrByAnchor->mInitPageCount  != sMemAttrByNode->mInitPageCount ) ||
         ( ( sMemAttrByAnchor->mIsAutoExtend != ID_TRUE ) &&
           ( sMemAttrByAnchor->mIsAutoExtend != ID_FALSE ) ) )
    {
        *aIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 오류 출력
    if ( ( aIsEqual  != ID_TRUE ) ||
         ( *aIsValid != ID_TRUE ) )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_MEM_TBS_ATTR,

                     "SpaceNode",
                     aSpaceAttrByNode->mAttrType,
                     aSpaceAttrByNode->mID,
                     aSpaceAttrByNode->mName,
                     idlOS::strlen(aSpaceAttrByNode->mName),
                     aSpaceAttrByNode->mType,
                     aSpaceAttrByNode->mTBSStateOnLA,
                     aSpaceAttrByNode->mAttrFlag,

                     sMemAttrByNode->mShmKey,
                     sMemAttrByNode->mCurrentDB,
                     sMemAttrByNode->mChkptPathCount,
                     sMemAttrByNode->mSplitFilePageCount,

                     sMemAttrByNode->mIsAutoExtend,
                     sMemAttrByNode->mMaxPageCount,
                     sMemAttrByNode->mNextPageCount,
                     sMemAttrByNode->mInitPageCount );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_MEM_TBS_ATTR,

                     "LogAnchor",
                     aSpaceAttrByAnchor->mAttrType,
                     aSpaceAttrByAnchor->mID,
                     aSpaceAttrByAnchor->mName,
                     aSpaceAttrByAnchor->mNameLength,
                     aSpaceAttrByAnchor->mType,
                     aSpaceAttrByAnchor->mTBSStateOnLA,
                     aSpaceAttrByAnchor->mAttrFlag,

                     sMemAttrByNode->mShmKey,
                     sMemAttrByNode->mCurrentDB,
                     sMemAttrByNode->mChkptPathCount,
                     sMemAttrByNode->mSplitFilePageCount,

                     sMemAttrByAnchor->mIsAutoExtend,
                     sMemAttrByAnchor->mMaxPageCount,
                     sMemAttrByAnchor->mNextPageCount,
                     sMemAttrByAnchor->mInitPageCount );
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : Volatile TBS Node와 LogAnchor Buffer를 비교한다.
 *
 *  aSpaceAttrByNode   - [IN] Space Node에서 추출한 Attribute
 *  aSpaceAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 *  aIsEqual           - [IN] 공통 영역의 Space Attr에 허용되는 범위
 *                            안에서 차이가 있다.
 *  aIsValid           - [IN/OUT] 공통 영역의 TableSpace Attr에 치명적인
 *                                문제가 있는지 여부를 나타내고 이후
 *                                개별 영역의 비교 결과를 반환한다.
 **********************************************************************/
void smrLogAnchorMgr::cmpVolTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                                            smiTableSpaceAttr* aSpaceAttrByAnchor,
                                            idBool             aIsEqual,
                                            idBool           * aIsValid )
{
    smiVolTableSpaceAttr * sVolAttrByAnchor;
    smiVolTableSpaceAttr * sVolAttrByNode;

    IDE_ASSERT( aSpaceAttrByNode   != NULL );
    IDE_ASSERT( aSpaceAttrByAnchor != NULL );

    sVolAttrByNode   = &aSpaceAttrByNode->mVolAttr;
    sVolAttrByAnchor = &aSpaceAttrByAnchor->mVolAttr;

    // 허용되는 차이, 경고만 출력
    if ( ( sVolAttrByAnchor->mIsAutoExtend  != sVolAttrByNode->mIsAutoExtend ) ||
         ( sVolAttrByAnchor->mMaxPageCount  != sVolAttrByNode->mMaxPageCount ) ||
         ( sVolAttrByAnchor->mNextPageCount != sVolAttrByNode->mNextPageCount ) )
    {
        aIsEqual = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 허용되지않는 차이, 오류 반환
    if ( ( sVolAttrByAnchor->mInitPageCount  != sVolAttrByNode->mInitPageCount ) ||
         ( ( sVolAttrByAnchor->mIsAutoExtend != ID_TRUE ) &&
           ( sVolAttrByAnchor->mIsAutoExtend != ID_FALSE ) ) )
    {
        *aIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 오류 출력
    if ( ( aIsEqual  != ID_TRUE ) ||
         ( *aIsValid != ID_TRUE ) )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_VOL_TBS_ATTR,

                     "SpaceNode",
                     aSpaceAttrByNode->mAttrType,
                     aSpaceAttrByNode->mID,
                     aSpaceAttrByNode->mName,
                     idlOS::strlen(aSpaceAttrByNode->mName),
                     aSpaceAttrByNode->mType,
                     aSpaceAttrByNode->mTBSStateOnLA,
                     aSpaceAttrByNode->mAttrFlag,
                     sVolAttrByNode->mIsAutoExtend,
                     sVolAttrByNode->mMaxPageCount,
                     sVolAttrByNode->mNextPageCount,
                     sVolAttrByNode->mInitPageCount );

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_VOL_TBS_ATTR,

                     "LogAnchor",
                     aSpaceAttrByAnchor->mAttrType,
                     aSpaceAttrByAnchor->mID,
                     aSpaceAttrByAnchor->mName,
                     aSpaceAttrByAnchor->mNameLength,
                     aSpaceAttrByAnchor->mType,
                     aSpaceAttrByAnchor->mTBSStateOnLA,
                     aSpaceAttrByAnchor->mAttrFlag,
                     sVolAttrByAnchor->mIsAutoExtend,
                     sVolAttrByAnchor->mMaxPageCount,
                     sVolAttrByAnchor->mNextPageCount,
                     sVolAttrByAnchor->mInitPageCount );
    }
    else
    {
        /* nothing to do */
    }
}

/***********************************************************************
 * Description : Disk TBS의 DBFile Node와 LogAnchor Buffer를 비교한다.
 *
 *  aFileAttrByNode   - [IN] DBFile Node에서 추출한 Attribute
 *  aFileAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 **********************************************************************/
idBool smrLogAnchorMgr::cmpDataFileAttr( smiDataFileAttr*   aFileAttrByNode,
                                         smiDataFileAttr*   aFileAttrByAnchor )
{
    idBool sIsEqual = ID_TRUE;
    idBool sIsValid = ID_TRUE;

    IDE_DASSERT( aFileAttrByNode != NULL );
    IDE_DASSERT( aFileAttrByAnchor != NULL );

    // 변경 가능한것, 일시적으로 차이가 날 수 있다. 로그만 기록한다.
    if ( ( idlOS::strcmp(aFileAttrByAnchor->mName, aFileAttrByNode->mName) != 0 ) ||
         ( aFileAttrByAnchor->mIsAutoExtend != aFileAttrByNode->mIsAutoExtend ) ||
         ( aFileAttrByAnchor->mNameLength   != idlOS::strlen(aFileAttrByNode->mName) ) ||
         ( aFileAttrByAnchor->mState        != aFileAttrByNode->mState ) ||
         ( aFileAttrByAnchor->mMaxSize      != aFileAttrByNode->mMaxSize ) ||
         ( aFileAttrByAnchor->mNextSize     != aFileAttrByNode->mNextSize ) ||
         ( aFileAttrByAnchor->mCurrSize     != aFileAttrByNode->mCurrSize ) )
    {
        sIsEqual = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 변경 불 가능한것, 차이가 날 수 없다.
    if ( ( aFileAttrByAnchor->mAttrType   != aFileAttrByNode->mAttrType ) ||
         ( aFileAttrByAnchor->mSpaceID    != aFileAttrByNode->mSpaceID ) ||
         ( aFileAttrByAnchor->mID         != aFileAttrByNode->mID ) ||
         ( aFileAttrByAnchor->mInitSize   != aFileAttrByNode->mInitSize ) ||
         ( aFileAttrByAnchor->mCreateMode != aFileAttrByNode->mCreateMode ) ||
         ( smrCompareLSN::isEQ( &(aFileAttrByAnchor->mCreateLSN),
                               &(aFileAttrByNode->mCreateLSN) ) != ID_TRUE ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 값 허용범위 검사
    if ( ( aFileAttrByNode->mAttrType   != SMI_DBF_ATTR ) ||
         ( aFileAttrByNode->mSpaceID    >= SC_MAX_SPACE_COUNT ) ||
         ( aFileAttrByNode->mID         >= SD_MAX_FID_COUNT ) ||
         ( aFileAttrByNode->mCreateMode >= SMI_DATAFILE_CREATE_MODE_MAX ) ||
         ( aFileAttrByNode->mNameLength >  SMI_MAX_DATAFILE_NAME_LEN ) ||
         ( aFileAttrByNode->mState      >  SMI_DATAFILE_STATE_MAX ) ||
         ( ( aFileAttrByNode->mIsAutoExtend != ID_TRUE ) &&
           ( aFileAttrByNode->mIsAutoExtend != ID_FALSE ) ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    if ( ( sIsEqual != ID_TRUE ) ||
         ( sIsValid != ID_TRUE ) )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_DBFILE_ATTR_BY_NODE,
                     aFileAttrByNode->mAttrType,
                     aFileAttrByNode->mSpaceID,
                     aFileAttrByNode->mID,
                     aFileAttrByNode->mName,
                     idlOS::strlen(aFileAttrByNode->mName),
                     aFileAttrByNode->mIsAutoExtend,
                     aFileAttrByNode->mState,
                     aFileAttrByNode->mMaxSize,
                     aFileAttrByNode->mNextSize,
                     aFileAttrByNode->mCurrSize,
                     aFileAttrByNode->mInitSize,
                     aFileAttrByNode->mCreateMode,
                     aFileAttrByNode->mCreateLSN.mFileNo ,
                     aFileAttrByNode->mCreateLSN.mOffset );


        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_DBFILE_ATTR_BY_ANCHOR,
                     aFileAttrByAnchor->mAttrType,
                     aFileAttrByAnchor->mSpaceID,
                     aFileAttrByAnchor->mID,
                     aFileAttrByAnchor->mName,
                     aFileAttrByAnchor->mNameLength,
                     aFileAttrByAnchor->mIsAutoExtend,
                     aFileAttrByAnchor->mState,
                     aFileAttrByAnchor->mMaxSize,
                     aFileAttrByAnchor->mNextSize,
                     aFileAttrByAnchor->mCurrSize,
                     aFileAttrByAnchor->mInitSize,
                     aFileAttrByAnchor->mCreateMode,
                     aFileAttrByAnchor->mCreateLSN.mFileNo,
                     aFileAttrByAnchor->mCreateLSN.mOffset );
    }
    else
    {
        /* nothing to do */
    }

    return sIsValid;
}

/***********************************************************************
 * Description : Memory TBS의 Checkpoint Image Node와
 *               LogAnchor Buffer를 비교한다.
 *
 *  aFileAttrByNode   - [IN] Checkpoint Image Node에서 추출한 Attribute
 *  aFileAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 **********************************************************************/
idBool smrLogAnchorMgr::cmpChkptImageAttr( smmChkptImageAttr*   aImageAttrByNode,
                                           smmChkptImageAttr*   aImageAttrByAnchor )
{
    idBool sIsValid = ID_TRUE;
    idBool sIsEqual = ID_TRUE;

    // 변경 가능한것, 일시적으로 차이가 날 수 있다. 로그만 기록
    if ( ( aImageAttrByAnchor->mCreateDBFileOnDisk[0]
           != aImageAttrByNode->mCreateDBFileOnDisk[0] ) ||
         ( aImageAttrByAnchor->mCreateDBFileOnDisk[1]
           != aImageAttrByNode->mCreateDBFileOnDisk[1] ) ||
         ( aImageAttrByAnchor->mFileNum != aImageAttrByNode->mFileNum ) )
    {
        sIsEqual = ID_FALSE;
    }

    // 변경 불 가능한것, 차이가 날 수 없다.
    if ( ( aImageAttrByAnchor->mAttrType != aImageAttrByNode->mAttrType ) ||
         ( aImageAttrByAnchor->mSpaceID  != aImageAttrByNode->mSpaceID ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // 값 허용범위 검사
    if ( ( aImageAttrByNode->mSpaceID  >= SC_MAX_SPACE_COUNT ) ||
         ( aImageAttrByNode->mAttrType != SMI_CHKPTIMG_ATTR )  ||
         ( ( aImageAttrByNode->mCreateDBFileOnDisk[0] != ID_TRUE ) &&
           ( aImageAttrByNode->mCreateDBFileOnDisk[0] != ID_FALSE ) ) ||
         ( ( aImageAttrByNode->mCreateDBFileOnDisk[1] != ID_TRUE ) &&
           ( aImageAttrByNode->mCreateDBFileOnDisk[1] != ID_FALSE ) ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    // SMI_MAX_DATAFILE_NAME_LEN
    if ( ( sIsEqual != ID_TRUE ) ||
         ( sIsValid != ID_TRUE ) )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_ANCHOR_CHECK_CHKPT_IMG_ATTR,
                    aImageAttrByNode->mSpaceID,
                    aImageAttrByNode->mAttrType,
                    aImageAttrByNode->mFileNum,
                    aImageAttrByNode->mCreateDBFileOnDisk[0],
                    aImageAttrByNode->mCreateDBFileOnDisk[1],
                    aImageAttrByAnchor->mSpaceID,
                    aImageAttrByAnchor->mAttrType,
                    aImageAttrByAnchor->mFileNum,
                    aImageAttrByAnchor->mCreateDBFileOnDisk[0],
                    aImageAttrByAnchor->mCreateDBFileOnDisk[1]);
    }
    else
    {
        /* nothing to do */
    }

    if ( smrCompareLSN::isEQ( &(aImageAttrByAnchor->mMemCreateLSN),
                              &(aImageAttrByNode->mMemCreateLSN) )
         != ID_TRUE )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_CRT_LSN_IN_CHKPT_IMG_ATTR,
                     aImageAttrByNode->mSpaceID,
                     aImageAttrByNode->mMemCreateLSN.mFileNo,
                     aImageAttrByNode->mMemCreateLSN.mOffset,
                     aImageAttrByAnchor->mSpaceID,
                     aImageAttrByAnchor->mMemCreateLSN.mFileNo,
                     aImageAttrByAnchor->mMemCreateLSN.mOffset );

        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do ... */
    }

    return sIsValid;
}


/***********************************************************************
 * Description : Secondary Buffer의 File Node와 LogAnchor Buffer를 비교한다.
 *
 *  aFileAttrByNode   - [IN] DBFile Node에서 추출한 Attribute
 *  aFileAttrByAnchor - [IN] Anchor Buffer상의 Attribute
 **********************************************************************/
idBool smrLogAnchorMgr::cmpSBufferFileAttr( smiSBufferFileAttr*   aFileAttrByNode,
                                            smiSBufferFileAttr*   aFileAttrByAnchor )
{
    idBool sIsValid = ID_TRUE;

    IDE_DASSERT( aFileAttrByNode != NULL );
    IDE_DASSERT( aFileAttrByAnchor != NULL );

    if ( ( aFileAttrByAnchor->mAttrType     != aFileAttrByNode->mAttrType )            ||
        ( idlOS::strcmp(aFileAttrByAnchor->mName, aFileAttrByNode->mName) != 0 )      ||
        ( aFileAttrByAnchor->mNameLength   != idlOS::strlen(aFileAttrByNode->mName) ) ||
        ( aFileAttrByAnchor->mPageCnt      != aFileAttrByNode->mPageCnt )             ||
        ( smrCompareLSN::isEQ( &(aFileAttrByAnchor->mCreateLSN),                     
                               &(aFileAttrByNode->mCreateLSN) ) != ID_TRUE ) )
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    if ( ( aFileAttrByNode->mAttrType   != SMI_SBUFFER_ATTR )          ||
         ( aFileAttrByNode->mNameLength >  SMI_MAX_DATAFILE_NAME_LEN ) ||
         ( aFileAttrByNode->mState      >  SMI_DATAFILE_STATE_MAX ) ) 
    {
        sIsValid = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    if ( sIsValid != ID_TRUE )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_SBFILE_ATTR_BY_NODE,
                     aFileAttrByNode->mAttrType,
                     aFileAttrByNode->mName,
                     idlOS::strlen(aFileAttrByNode->mName),
                     aFileAttrByNode->mPageCnt,
                     aFileAttrByNode->mState,
                     aFileAttrByNode->mCreateLSN.mFileNo,
                     aFileAttrByNode->mCreateLSN.mOffset );


        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_ANCHOR_CHECK_SBFILE_ATTR_BY_ANCHOR,
                     aFileAttrByNode->mAttrType,
                     aFileAttrByNode->mName,
                     idlOS::strlen(aFileAttrByNode->mName),
                     aFileAttrByNode->mPageCnt,
                     aFileAttrByNode->mState,
                     aFileAttrByNode->mCreateLSN.mFileNo ,
                     aFileAttrByNode->mCreateLSN.mOffset );
    }
    else
    {
        /* nothing to do */
    }

    return sIsValid;
}

// fix BUG-20241
/***********************************************************************
 * Description : loganchor에 FstDeleteFile 정보 변경 후 flush
 *
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::updateFstDeleteFileAndFlush()
{
    UInt sState = 0;
    UInt sDelLogCnt = 0;
    

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /* BUG-39289 : 로그앵커의 delete logfile range가 N+1 ~ N으로 출력되는 문제.
       기존에 로그앵커의 FST(mFstDeleteFileNo)를 LST(mLstDeleteFileNo)
       로 업데이트 하던 것을 로그앵커의 FST를 LST + 삭제한 로그 수 로
       업데이트 한다. 로그앵커의 delete logfile range 출력시 실제 삭제된
       로그 범위로 출력하기 위함.*/
    /* BUG-40323 로그파일을 지우지 않고, 이 함수를 호출하면 안된다.
     * BUG-39289 적용으로 Fst >= Lst 값을 가져야 하는데,
     * 새로운 로그파일의 삭제, 즉 Lst 값이 증가하지 않고 다시 호출되면
     * Fst <= Lst로 수정될 수 있다. */

    sDelLogCnt = mLogAnchor->mLstDeleteFileNo - mLogAnchor->mFstDeleteFileNo;

    mLogAnchor->mFstDeleteFileNo = mLogAnchor->mLstDeleteFileNo + sDelLogCnt;

    IDE_TEST( flushAll() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/*************************************************************************************
 * Description : 마지막에 생성된 로그 파일 번호를 loganchor에 업데이트한다.(BUG-39764)
 *               
 * [IN] aLstCrtFileNo : 마지막에 생성된 로그 파일 번호
 *************************************************************************************/
IDE_RC smrLogAnchorMgr::updateLastCreatedLogFileNumAndFlush( UInt aLstCrtFileNo )
{
    UInt sState = 0;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    mLogAnchor->mLstCreatedLogFileNo = aLstCrtFileNo;

    IDE_TEST( flushAll() != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : loganchor 버퍼를 loganchor 파일에 flush
 *
 * mBuffer에 기록된 loganchor를 모든 loganchor 파일들에
 * 모두 flush한다.
 *
 * - flush할 loganchor의 내용이 존재하는 mBuffer의
 *   checksum을 계산하여 mBuffer의 해당 부분을 갱신
 *   + MUST : checksum을 제외한 나머지 loganchor 정보들은
 *            모두 설정되어 있어야 한다.
 * - 모든 loganchor파일에 대하여 mBuffer를 flush 함
 ***********************************************************************/
IDE_RC smrLogAnchorMgr::flushAll()
{
    UInt      i;
    ULong     sFileSize = 0;
#ifdef DEBUG
    idBool    sIsValidLogAnchor;
#endif
    UInt      sState = 0;

    /* PROJ-2162 RestartRiskReduction
     * DB가 정상적일때에만 LogAnchor를 갱신한다. */
    if ( ( smrRecoveryMgr::getConsistency() == ID_TRUE ) || 
         ( smuProperty::getCrashTolerance() == 2 ) )
    {
        // BUG-23136 LogAnchorBuffer와 각 node를 비교 검사 하여
        // 잘못된 값이 LogAnchor File에 기록되는 것을 방지한다.
        // XXX Tablespace에 대한 동시성 제어가 보장되지 않는 문제가 있어
        // release에서는 결과 기록만 하고 DASSERT로 처리한다.
        // 이후 동시성 제어가 보장되면 바로 ASSERT하도록 변경해야 한다.
#ifdef DEBUG
        if ( mIsProcessPhase == ID_FALSE )
        {
            sIsValidLogAnchor = checkLogAnchorBuffer() ;
            IDE_DASSERT( sIsValidLogAnchor == ID_TRUE );
        }
#endif
        /* ------------------------------------------------
         * updateAndFlushCount와 checksum 계산하여 mBuffer에 설정한다.
         * ----------------------------------------------*/
        mLogAnchor->mUpdateAndFlushCount++;
        mLogAnchor->mCheckSum = makeCheckSum( mBuffer, mWriteOffset );

        /* ----------------------------------------------------------------
         * In case createdb, SMU_TRANSACTION_DURABILITY_LEVEL is always 3.
         * ---------------------------------------------------------------- */

        /* ------------------------------------------------
         * 실제 모든 loganchor 파일에 mBuffer를 기록하고 sync한다.
         * ----------------------------------------------*/
        for ( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++ )
        {
            /* ------------------------------------------------
             * [BUG-24236] 운영중 로그 앵커 삭제에 대한 고려가 필요합니다.
             * - 로그 앵커 파일이 삭제되었다면 다시 생성한다.
             * ----------------------------------------------*/
            if ( mFile[i].open() != IDE_SUCCESS )
            {
                ideLog::log( IDE_SM_0, SM_TRC_MEMORY_LOGANCHOR_RECREATE );

                IDE_TEST( mFile[i].createUntilSuccess( smLayerCallback::setEmergency )
                          != IDE_SUCCESS );

                /* ------------------------------------------------
                 * 그래도 실패하면 IDE_FAILURE
                 * ----------------------------------------------*/
                IDE_TEST( mFile[i].open() != IDE_SUCCESS );
            }
            sState = 1;

            IDE_TEST( mFile[i].writeUntilSuccess(
                                            NULL, /* idvSQL* */
                                            0,
                                            mBuffer,
                                            mWriteOffset,
                                            smLayerCallback::setEmergency,
                                            smrRecoveryMgr::isFinish())
                      != IDE_SUCCESS );

            IDE_TEST( mFile[i].syncUntilSuccess(
                                            smLayerCallback::setEmergency)
                      != IDE_SUCCESS );

            IDE_TEST( mFile[i].getFileSize( &sFileSize ) != IDE_SUCCESS );

            if ( (SInt)( mWriteOffset - (UInt)sFileSize ) < 0 )
            {
                IDE_TEST( mFile[i].truncate( mWriteOffset )
                          != IDE_SUCCESS );
            }

            sState = 0;
            /* ------------------------------------------------
             * [BUG-24236] 운영중 로그 앵커 삭제에 대한 고려가 필요합니다.
             * ----------------------------------------------*/
            IDE_ASSERT( mFile[i].close() == IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( mFile[i].close() == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;

}

/***********************************************************************
 * Description :  Check LogAnchor Dir Exist
 **********************************************************************/
IDE_RC smrLogAnchorMgr::checkLogAnchorDirExist()
{
    SChar    sLogAnchorDir[SM_MAX_FILE_NAME];
    SInt     i;

    for( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++ )
    {
        idlOS::snprintf( sLogAnchorDir,
                         SM_MAX_FILE_NAME,
                         "%s",
                         smuProperty::getLogAnchorDir(i) );

        IDE_TEST_RAISE( idf::access(sLogAnchorDir, F_OK) != 0,
                        err_loganchordir_not_exist )
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_loganchordir_not_exist);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, sLogAnchorDir));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :  read loganchor header
 *
 * [IN]  aLogAnchorFile  : 로그앵커파일 객체
 * [IN]  aCurOffset      : 로그파일상에서 현재 속성의 오프셋
 * [OUT] aCurOffset      : 로그파일상에서 다음 속성의 오프셋
 * [OUT] aHeader         : 로그앵커 고정영역
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readLogAnchorHeader( iduFile *      aLogAnchorFile,
                                             UInt *         aCurOffset,
                                             smrLogAnchor * aHeader )
{
    IDE_DASSERT(*aCurOffset == 0);

    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    *aCurOffset,
                                    (SChar*)aHeader,
                                    (UInt)ID_SIZEOF(smrLogAnchor) )
              != IDE_SUCCESS );

    *aCurOffset = ID_SIZEOF(smrLogAnchor);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :  read one tablespace attribute
 *
 * [IN]  aLogAnchorFile  : 로그앵커파일 객체
 * [IN]  aCurOffset      : 로그파일상에서 현재 속성의 오프셋
 * [OUT] aCurOffset      : 로그파일상에서 다음 속성의 오프셋
 * [OUT] aTBSAttr        : Disk/Memory Tablespace Attribute
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readTBSNodeAttr(
                                   iduFile *           aLogAnchorFile,
                                   UInt *              aCurOffset,
                                   smiTableSpaceAttr * aTBSAttr )
{
    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aCurOffset != NULL );
    IDE_DASSERT( aTBSAttr != NULL );

    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    *aCurOffset,
                                    (SChar*)aTBSAttr,
                                    ID_SIZEOF(smiTableSpaceAttr) )
             != IDE_SUCCESS );

    *aCurOffset += (UInt)ID_SIZEOF(smiTableSpaceAttr);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  read one datafile attribute
 *
 * [IN]  aLogAnchorFile  : 로그앵커파일 객체
 * [IN]  aCurOffset      : 로그파일상에서 현재 속성의 오프셋
 * [OUT] aCurOffset      : 로그파일상에서 다음 속성의 오프셋
 * [OUT] aFileAttr       : Disk Datafile Attribute
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readDBFNodeAttr(
                               iduFile *           aLogAnchorFile,
                               UInt *              aCurOffset,
                               smiDataFileAttr *   aFileAttr )
{
    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aCurOffset != NULL );
    IDE_DASSERT( aFileAttr != NULL );

    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    *aCurOffset,
                                    (SChar*)aFileAttr,
                                    ID_SIZEOF(smiDataFileAttr) )
             != IDE_SUCCESS );

    *aCurOffset += (UInt)ID_SIZEOF(smiDataFileAttr);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  read one checkpoint path attribute
 *
 * [IN]  aLogAnchorFile  : 로그앵커파일 객체
 * [IN]  aCurOffset      : 로그파일상에서 현재 속성의 오프셋
 * [OUT] aCurOffset      : 로그파일상에서 다음 속성의 오프셋
 * [OUT] aChkptImageAttr : Checkpoint Path Attribute
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readChkptPathNodeAttr(
                                     iduFile *          aLogAnchorFile,
                                     UInt *             aCurOffset,
                                     smiChkptPathAttr * aChkptPathAttr )
{
    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aCurOffset != NULL );
    IDE_DASSERT( aChkptPathAttr != NULL );

    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    *aCurOffset,
                                    (SChar*)aChkptPathAttr,
                                    ID_SIZEOF(smiChkptPathAttr) )
             != IDE_SUCCESS );

    *aCurOffset += (UInt)ID_SIZEOF(smiChkptPathAttr);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  read one checkpoint image attribute
 *
 * [IN]  aLogAnchorFile  : 로그앵커파일 객체
 * [IN]  aCurOffset      : 로그파일상에서 현재 속성의 오프셋
 * [OUT] aCurOffset      : 로그파일상에서 다음 속성의 오프셋
 * [OUT] aChkptImageAttr : Checkpoint Image Attribute
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readChkptImageAttr(
                                   iduFile *           aLogAnchorFile,
                                   UInt *              aCurOffset,
                                   smmChkptImageAttr * aChkptImageAttr )
{
    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aCurOffset != NULL );
    IDE_DASSERT( aChkptImageAttr != NULL );

    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    *aCurOffset,
                                    (SChar*)aChkptImageAttr,
                                    ID_SIZEOF( smmChkptImageAttr ) )
              != IDE_SUCCESS );

    IDE_ASSERT( smmManager::isCreateDBFileAtLeastOne(
                                    aChkptImageAttr->mCreateDBFileOnDisk )
                == ID_TRUE );

    *aCurOffset += (UInt)ID_SIZEOF(smmChkptImageAttr);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :  read one SBufferFile attribute
 *
 * [IN]  aLogAnchorFile  : 로그앵커파일 객체
 * [IN]  aCurOffset      : 로그파일상에서 현재 속성의 오프셋
 * [OUT] aCurOffset      : 로그파일상에서 다음 속성의 오프셋
 * [OUT] aFileAttr       : Disk Datafile Attribute
 **********************************************************************/
IDE_RC smrLogAnchorMgr::readSBufferFileAttr(
                               iduFile              * aLogAnchorFile,
                               UInt                 * aCurOffset,
                               smiSBufferFileAttr   * aFileAttr )
{
    IDE_DASSERT( aLogAnchorFile != NULL );
    IDE_DASSERT( aCurOffset != NULL );
    IDE_DASSERT( aFileAttr != NULL );

    IDE_TEST( aLogAnchorFile->read( NULL, /* idvSQL* */
                                    *aCurOffset,
                                    (SChar*)aFileAttr,
                                    ID_SIZEOF(smiSBufferFileAttr) )
            != IDE_SUCCESS ); 

    *aCurOffset += (UInt)ID_SIZEOF(smiSBufferFileAttr);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : loganchor 버퍼를 resize함
 **********************************************************************/
IDE_RC smrLogAnchorMgr::resizeBuffer( UInt aBufferSize )
{
    UChar * sBuffer;
    UInt    sState  = 0;

    IDE_DASSERT( aBufferSize >= idlOS::align((UInt)ID_SIZEOF(smrLogAnchorMgr)));

    sBuffer = NULL;

    /* TC/FIT/Limit/sm/smr/smrLogAnchorMgr_resizeBuffer_malloc.tc */
    IDU_FIT_POINT_RAISE( "smrLogAnchorMgr::resizeBuffer::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                       aBufferSize,
                                       (void**)&sBuffer ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    IDU_FIT_POINT_RAISE( "1.BUG-30024@smrLogAnchorMgr::resizeBuffer", 
                          insufficient_memory );

    idlOS::memset( sBuffer, 0x00, aBufferSize );

    if ( mWriteOffset != 0 )
    {
        /* ------------------------------------------------
         * loganchor 버퍼가 사용중일때 기록된 내용을
         * 새로운 버퍼에 memcpy 한다.
         * ----------------------------------------------*/
        idlOS::memcpy( sBuffer, mBuffer, mWriteOffset );
    }
    else
    {
        /* nothing to do */
    }

    // To Fix BUG-18268
    //     disk tablespace create/alter/drop 도중 checkpoint하다가 사망
    //
    // mBuffer를 해제하기 전에 mLogAnchor가 새로 할당한 메모리를
    // 가리키도록 수정
    mLogAnchor = (smrLogAnchor*)sBuffer;

    IDE_ASSERT( iduMemMgr::free( mBuffer ) == IDE_SUCCESS );
    mBuffer = sBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sBuffer ) == IDE_SUCCESS );
            sBuffer = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : loganchor 버퍼를 해제함
 **********************************************************************/
IDE_RC smrLogAnchorMgr::freeBuffer()
{
    IDE_TEST( iduMemMgr::free( mBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: loganchor 버퍼에 기록
 **********************************************************************/
IDE_RC smrLogAnchorMgr::writeToBuffer( void*  aBuffer,
                                       UInt   aWriteSize )
{
    UInt sBufferSize;

    IDE_DASSERT( aBuffer != NULL );
    IDE_DASSERT( aWriteSize > 0 );

    /* To fix BUG-30024 : resizeBuffer() 에서 malloc실패시 비정상종료합니다. */
    if ( ( mWriteOffset + aWriteSize ) >= mBufferSize )
    {
        sBufferSize = idlOS::align( (mWriteOffset + aWriteSize), SM_PAGE_SIZE);

        if ( sBufferSize == ( mWriteOffset + aWriteSize) )
        {
            sBufferSize += SM_PAGE_SIZE;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( resizeBuffer( sBufferSize ) != IDE_SUCCESS );
        mBufferSize = sBufferSize;
    }
    else
    {
        /* nothing to do */
    }

    idlOS::memcpy( mBuffer + mWriteOffset, 
                   aBuffer, 
                   aWriteSize );
    mWriteOffset += aWriteSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
   PRJ-1548 User Memory Tablespace
   Loganchor 메모리버퍼의 특정오프셋에서 주어진 길이만큼
   UPDATE 한다.
 */
IDE_RC smrLogAnchorMgr::updateToBuffer( void      * aBuffer,
                                        UInt        aOffset,
                                        UInt        aWriteSize )
{
    IDE_DASSERT( aBuffer != NULL );

    IDE_ASSERT( ( aOffset + aWriteSize ) <= mWriteOffset );

    // Tablespace나 DBF Node를 Update하다가
    // Log Anchor의 시작부분에 기록되는 고정길이 영역의
    // smrLogAnchor구조체를 덮어쓰면
    // 시스템이 영구적으로 복구가 불가능해진다.
    // ASSERT로 이를 확인한다.
    IDE_ASSERT( aOffset >= ID_SIZEOF(smrLogAnchor) );

    idlOS::memcpy( mBuffer + aOffset, 
                   aBuffer, 
                   aWriteSize);

    return IDE_SUCCESS;
}

/*
   PROJ-2133 incremental backup
   Change tracking attr을 갱신한다.
 */
IDE_RC smrLogAnchorMgr::updateCTFileAttr( SChar          * aCTFileName, 
                                          smriCTMgrState * aCTMgrState, 
                                          smLSN          * aChkptLSN )
{
    
    IDE_TEST( lock() != IDE_SUCCESS );

    if ( aCTFileName != NULL )
    {
        idlOS::memcpy( mLogAnchor->mCTFileAttr.mCTFileName, 
                       aCTFileName, 
                       SM_MAX_FILE_NAME );
    }
    else
    {
        /* nothing to do */
    }
    
    if ( aCTMgrState != NULL )
    {
        mLogAnchor->mCTFileAttr.mCTMgrState = *aCTMgrState;
    }
    else
    {
        /* nothing to do */
    }

    if ( aChkptLSN != NULL )
    {
        mLogAnchor->mCTFileAttr.mLastFlushLSN.mFileNo = aChkptLSN->mFileNo;
        mLogAnchor->mCTFileAttr.mLastFlushLSN.mOffset = aChkptLSN->mOffset;
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( flushAll() != IDE_SUCCESS );

    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   PROJ-2133 incremental backup
   Backup Info attr을 갱신한다.
 */
IDE_RC smrLogAnchorMgr::updateBIFileAttr( SChar          * aBIFileName, 
                                          smriBIMgrState * aBIMgrState, 
                                          smLSN          * aBackupLSN,
                                          SChar          * aBackupDir,
                                          UInt           * aDeleteArchLogFileNo )
{

    IDE_TEST( lock() != IDE_SUCCESS );

    if ( aBIFileName != NULL )
    {
        idlOS::memcpy( mLogAnchor->mBIFileAttr.mBIFileName, 
                       aBIFileName, 
                       SM_MAX_FILE_NAME);
    }
    else
    {
        /* nothing to do */
    }
    
    if ( aBIMgrState != NULL )
    {
        mLogAnchor->mBIFileAttr.mBIMgrState = *aBIMgrState;
    }
    else
    {
        /* nothing to do */
    }

    if ( aBackupLSN != NULL )
    {
        mLogAnchor->mBIFileAttr.mBeforeBackupLSN.mFileNo = 
                                mLogAnchor->mBIFileAttr.mLastBackupLSN.mFileNo;
        mLogAnchor->mBIFileAttr.mBeforeBackupLSN.mOffset = 
                                mLogAnchor->mBIFileAttr.mLastBackupLSN.mOffset;

        mLogAnchor->mBIFileAttr.mLastBackupLSN.mFileNo = aBackupLSN->mFileNo;
        mLogAnchor->mBIFileAttr.mLastBackupLSN.mOffset = aBackupLSN->mOffset;
    }
    else
    {
        /* nothing to do */
    }

    if ( aBackupDir != NULL )
    {
        idlOS::memcpy( mLogAnchor->mBIFileAttr.mBackupDir, 
                       aBackupDir, 
                       SM_MAX_FILE_NAME );
    }
    else
    {
        /* nothing to do */
    }

    if ( aDeleteArchLogFileNo != NULL )
    {
        mLogAnchor->mBIFileAttr.mDeleteArchLogFileNo = *aDeleteArchLogFileNo;
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( flushAll() != IDE_SUCCESS );

    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogAnchorMgr::initialize4ProcessPhase()
{
    UInt              i;
    UInt              sWhich;
    UInt              sState    = 0;
    ULong             sFileSize = 0;
    smrLogAnchor      sLogAnchor;
    SChar             sAnchorFileName[SM_MAX_FILE_NAME];
    UInt              sFileState = 0;

    // LOGANCHOR_DIR 확인
    IDE_TEST( checkLogAnchorDirExist() != IDE_SUCCESS );

    for ( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++ )
    {
        idlOS::snprintf( sAnchorFileName, SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT"",
                         smuProperty::getLogAnchorDir(i),
                         IDL_FILE_SEPARATOR,
                         SMR_LOGANCHOR_NAME,
                         i );

        // Loganchor 파일 접근 권한 확인
        IDE_TEST_RAISE( idf::access( sAnchorFileName, F_OK|W_OK|R_OK ) != 0,
                        error_file_not_exist );
    }

    IDE_TEST_RAISE( mMutex.initialize( (SChar*)"LOGANCHOR_MGR_MUTEX",
                                       IDU_MUTEX_KIND_NATIVE,
                                       IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS,
                    error_mutex_init );

    idlOS::memset( &mTableSpaceAttr,
                   0x00,
                   ID_SIZEOF(smiTableSpaceAttr) );

    idlOS::memset( &mDataFileAttr,
                   0x00,
                   ID_SIZEOF(smiDataFileAttr) );

    // 메모리 버퍼(mBuffer)를 SM_PAGE_SIZE 크기로 할당 및 초기화
    IDE_TEST( allocBuffer( SM_PAGE_SIZE ) != IDE_SUCCESS );
    sState = 1;

    // 유효한 loganchor파일을 선정한다.
    IDE_TEST( checkAndGetValidAnchorNo( &sWhich ) != IDE_SUCCESS );

    // 모든 loganchor 파일을 오픈한다.
    for ( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++ )
    {
        idlOS::snprintf( sAnchorFileName, SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT"",
                         smuProperty::getLogAnchorDir(i),
                         IDL_FILE_SEPARATOR,
                         SMR_LOGANCHOR_NAME,
                         i );
        IDE_TEST( mFile[i].initialize( IDU_MEM_SM_SMR,
                                       1, /* Max Open FD Count */
                                       IDU_FIO_STAT_OFF,
                                       IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );
        IDE_TEST( mFile[i].setFileName( sAnchorFileName ) != IDE_SUCCESS );
        IDE_TEST( mFile[i].open() != IDE_SUCCESS );
        sFileState++;
    }

    // 메모리버퍼 오프셋 초기화
    initBuffer();

    /*
       PRJ-1548 고정길이 영역을 메모리 버퍼에 로딩하는 단계
    */

    IDE_TEST( mFile[sWhich].getFileSize( &sFileSize ) != IDE_SUCCESS );

    // CREATE DATABASE 과정중에 가변영역이 존재하지 않을 수는 있지만,
    // 고정영역은 저장되어 있어야 한다.
    IDE_ASSERT ( sFileSize >= ID_SIZEOF(smrLogAnchor) );

    // 파일로 부터 Loganchor 고정영역을 로딩한다.
    IDE_TEST( mFile[sWhich].read( NULL, /* idvSQL* */
                                  0,
                                  (SChar*)&sLogAnchor,
                                  ID_SIZEOF(smrLogAnchor) )
             != IDE_SUCCESS );

    // 메모리 버퍼에 Loganchor 고정역영을 로딩한다.
    IDE_TEST( writeToBuffer( (SChar*)&sLogAnchor,
                             ID_SIZEOF(smrLogAnchor) )
              != IDE_SUCCESS );

    mLogAnchor = (smrLogAnchor*)mBuffer;

    /* ------------------------------------------------
     * 5. 유효한 loganchor 버퍼를 모든 anchor 파일에 flush 한다.
     * ----------------------------------------------*/
    IDE_TEST( readLogAnchorToBuffer( sWhich ) != IDE_SUCCESS );

    for ( i = 0 ; i < SM_LOGANCHOR_FILE_COUNT ; i++)
    {
        sFileState--;
        IDE_ASSERT( mFile[i].close() == IDE_SUCCESS );
    }

    mIsProcessPhase = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_file_not_exist );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidLogAnchorFile));
    }
    IDE_EXCEPTION( error_mutex_init );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;

    while ( sFileState > 0 )
    {
        sFileState--;
        IDE_ASSERT( mFile[sFileState].close() == IDE_SUCCESS ); 
    }
    
    if ( sState != 0 )
    {
        (void)freeBuffer();
    }

    return IDE_FAILURE;
}
