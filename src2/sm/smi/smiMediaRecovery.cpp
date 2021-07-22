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
 * $Id: smiMediaRecovery.cpp 86110 2019-09-02 04:52:04Z et16 $
 **********************************************************************/
/**********************************************************************
 * FILE DESCRIPTION : smiMediaRecovery.cpp                            *
 * -------------------------------------------------------------------*
 * media recovey를 위한 inteface 모듈이며, 다음과 같은 기능을 제공한다.
 *
 * (1) backup이 없는 데이타 파일이 손상, 유실이 된 경우에 새로운 empty
 *     데이타파일 생성한다.
 * (2) disk 가 깨져서, 다른 위치에 데이타 파일을 옮기는 경우를 위한
 *     데이타파일 이름 변경.
 * (3) incompleted recovery 때문에 기존 redo log파일들에 대한 redo all/undo
 *     all을 skip하기 위하여, 로그 파일들을 초기화 작업.
 * (4) database/tablespace/datafile단위 media recovery
 *     단, datafile단위는 completed media recovery만 가능하다.
 **********************************************************************/

#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smiMain.h>
#include <smiMediaRecovery.h>
#include <smiBackup.h>
#include <sdd.h>
#include <smr.h>
#include <sct.h>

/*
  BACKUP이 없는 디스크 데이타 파일이 손상, 유실이
  된 경우에 새로운 빈 데이타 파일 생성한다.

  SQL구문 : ALTER DATABASE CREATE DATAFILE 'OLD-FILESPEC' [ AS 'NEW-FILESPEC' ];

  - aNewFileSpec이 null인 경우에는  이전 데이타파일과
  동일한 이름과 속성을 가지는 빈 데이타 파일을생성한다.

  - aNewFileSpec이 null이 아닌 경우에는 이전 데이타 파일과
  동일한속성이지만 , 생성 위치가 다른 경우이다.

  [ 알고리즘 ]

  기존 파일이름으로 hash에서 데이타 파일노드를 검색;
  if(검색이 fail )  then
      return failure
  fi

  if( 새로운 위치가 지정되었으면 ) // aNewFileSpec !=NULL
  then
      데이타 파일 노드의 file이름을 갱신한다.
      새로운 데이타  파일위치로 생성
      // BUGBUG- logAnchor  sync는 restart recovery
      // 후에 하는것으로 결정.
  else
      // 기존 위치에 같은 파일으로 생성한다.
      if( 기존 파일이 존재하는지 확인) // aSrcFileName.
      then
          return failure
      fi
      새로운 데이타 파일 생성;
  fi //else

  [인자]

  [IN] aOldFileSpec - 로그앵커상의 파일경로 [ FILESPEC ]
  [IN] aNewFileSpec - 다른곳에 파일을 생성하는 경우 파일경로 [ FILESPEC ]
*/
IDE_RC smiMediaRecovery::createDatafile( SChar* aOldFileSpec,
                                         SChar* aNewFileSpec )
{
    UInt             sStrLen;
    scSpaceID        sSpaceID;
    sddDataFileHdr   sDBFileHdr;
    sddDataFileNode* sOldFileNode;
    sddDataFileNode* sNewFileNode;
    SChar            sOldFileSpec[ SMI_MAX_DATAFILE_NAME_LEN + 1 ];
    SChar            sNewFileSpec[ SMI_MAX_DATAFILE_NAME_LEN + 1 ];
    smLSN            sDiskRedoLSN;

    IDE_ASSERT( aOldFileSpec != NULL );

    idlOS::memset( sOldFileSpec, 0x00, SMI_MAX_DATAFILE_NAME_LEN + 1 );
    idlOS::memset( sNewFileSpec, 0x00, SMI_MAX_DATAFILE_NAME_LEN + 1 );

    // 다단계startup단계중 control 단계에서만 불릴수 있다.
    IDE_TEST_RAISE( smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                    err_startup_phase );

    // [1] 기존 FILESPEC을 복사한다.
    idlOS::strncpy( sOldFileSpec,
                    aOldFileSpec,
                    SMI_MAX_DATAFILE_NAME_LEN );
    sOldFileSpec[ SMI_MAX_DATAFILE_NAME_LEN ] = '\0';

    sStrLen = idlOS::strlen( sOldFileSpec );

    // [2] 파일 퍼미션 확인 및 절대경로로 반환한다.
    IDE_TEST( sctTableSpaceMgr::makeValidABSPath( ID_TRUE,
                                                  sOldFileSpec,
                                                  &sStrLen,
                                                  SMI_TBS_DISK )
              != IDE_SUCCESS );

    // [3] 기존 파일이름으로 hash에서 데이타 파일노드를 검색한다.
    sctTableSpaceMgr::getDataFileNodeByName( sOldFileSpec,
                                             &sOldFileNode,
                                             &sSpaceID );

    IDE_TEST_RAISE( sOldFileNode == NULL, err_not_found_filenode );

    // [4] Empty 데이타파일의 헤더를 설정하기 위한 파일헤더를 설정한다.
    idlOS::memset(&sDBFileHdr, 0x00, ID_SIZEOF(sddDataFileHdr));
    sDBFileHdr.mSmVersion = smVersionID;
    SM_GET_LSN( sDBFileHdr.mCreateLSN,
                sOldFileNode->mDBFileHdr.mCreateLSN );

    /* BUG-40371
     * 생성된 Empty File의 CreateLSN 값이 LogAnchor의 RedoLSN 값보다 큰 경우
     * Empty File의 RedoLSN 을 CreateLSN과 같게 설정하여
     * Restart Recovery가 수행될수 있도록 한다. */
    smrRecoveryMgr::getDiskRedoLSNFromLogAnchor( &sDiskRedoLSN );
    if ( smrCompareLSN::isGT( &sDBFileHdr.mCreateLSN , &sDiskRedoLSN ) == ID_TRUE )
    {
         SM_GET_LSN( sDBFileHdr.mRedoLSN,
                     sDBFileHdr.mCreateLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    // [5] Empty 데이타파일을 생성한다.
    if( aNewFileSpec != NULL)
    {
        // 새로운 경로에 데이타파일을 생성한다.
        // [5-1] 새로운 FILESPEC을 복사한다.
        idlOS::strncpy( sNewFileSpec,
                        aNewFileSpec,
                        SMI_MAX_DATAFILE_NAME_LEN );
        sNewFileSpec[ SMI_MAX_DATAFILE_NAME_LEN ] = '\0';

        sStrLen = idlOS::strlen(sNewFileSpec);

        // [5-2] 파일 퍼미션 확인 및 절대경로로 반환한다.
        IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                    ID_TRUE,
                                    sNewFileSpec,
                                    &sStrLen,
                                    SMI_TBS_DISK )
                 != IDE_SUCCESS );

        // [5-3] 새로운 파일이름으로 hash에서 데이타 파일노드를 검색한다.
        sctTableSpaceMgr::getDataFileNodeByName( sNewFileSpec,
                                                 &sNewFileNode,
                                                 &sSpaceID );

        IDE_TEST_RAISE(sNewFileNode != NULL, err_inuse_filename );

        // [5-4] 새로운 파일이름으로 갱신.
        IDE_TEST(sddDataFile::setDataFileName( sOldFileNode,
                                               sNewFileSpec,
                                               ID_FALSE )
                 != IDE_SUCCESS);
    }
    else
    {
        // 기존 경로에 데이타파일을 생성한다.

        // 기존 경로에 데이타파일이 이미 존재하는지 검사
        IDE_TEST_RAISE( idf::access(sOldFileSpec, F_OK) == 0,
                        err_already_exist_datafile );
    }

    // [6] 이전 파일로 동일한 정보를가지고 ,
    // 새로운 데이타파일을 생성한다.
    IDE_TEST( sddDataFile::create( NULL, sOldFileNode, &sDBFileHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_MediaRecoDataFile,
                                "ALTER DATABASE CRAEATE DATAFILE"));
    }
    IDE_EXCEPTION( err_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundDataFileNode,
                                aOldFileSpec));
    }
    IDE_EXCEPTION( err_already_exist_datafile );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AlreadyExistFile, sOldFileSpec));
    }
    IDE_EXCEPTION( err_inuse_filename );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_UseFileInOtherTBS,
                 sNewFileSpec) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
  BACKUP이 없는 디스크 데이타 파일이 손상, 유실이
  된 경우에 새로운 빈 데이타 파일 생성한다.

  SQL구문 : ALTER DATABASE CREATE CHECKPOINT IMAGE 'FILESPEC';

  [IN] aOldFileSpec - 로그앵커상의 파일경로 [ FILESPEC ]
  [IN] aNewFileSpec - 다른곳에 파일을 생성하는 경우 파일경로 [ FILESPEC ]

  [ 고려사항 ]
  STABLE한 체크포인트 이미지를 입력받아서 2개의 체크포인트 이미지를
  생성한다.
*/
IDE_RC smiMediaRecovery::createChkptImage( SChar * aFileName )
{
    SInt                 sPingPongNum;
    UInt                 sLoop;
    UInt                 sFileNum;
    UInt                 sStrLength;
    smmChkptImageHdr     sChkptImageHdr;
    sctTableSpaceNode  * sSpaceNode;
    smmDatabaseFile    * sDatabaseFile[SMM_PINGPONG_COUNT];
    SChar              * sChrPtr1;
    SChar              * sChrPtr2;
    SChar                sSpaceName[ SMI_MAX_TABLESPACE_NAME_LEN + 1 ];
    SChar                sNumberStr[ SM_MAX_FILE_NAME + 1 ];
    SChar                sCreateDir[ SM_MAX_FILE_NAME + 1 ];
    SChar                sFileName [ SM_MAX_FILE_NAME + 1 ];
    SChar                sFullPath [ SM_MAX_FILE_NAME + 1 ];
    smLSN                sInitLSN;
    smmTBSNode         * sTBSNode    = NULL;
    smmTBSNode         * sDicTBSNode = NULL;
    smmMemBase           sDicMemBase;
    void               * sPageBuffer = NULL;
    void               * sDummyPage  = NULL;
    UChar              * sBasePage   = NULL;
    UInt                 sStage      = 0;

    IDE_DASSERT( aFileName != NULL );

    SM_LSN_INIT( sInitLSN );

    // 다단계startup단계중 control 단계에서만 불릴수 있다.
    IDE_TEST_RAISE( smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                    err_startup_phase );

    idlOS::memset( sSpaceName,  // 테이블스페이스 명
                   0x00,
                   SMI_MAX_TABLESPACE_NAME_LEN + 1 );

    idlOS::memset( sFileName, // 체크포인트 이미지 명
                   0x00,
                   SM_MAX_FILE_NAME + 1 );

    idlOS::memset( sCreateDir, // 디렉토리 명
                   0x00,
                   SM_MAX_FILE_NAME + 1 );

    idlOS::memset( sFullPath, // 체크포인트 이미지 경로
                   0x00,
                   SM_MAX_FILE_NAME + 1 );

    // [1] 입력된 FileSpec을 저장한다.
    idlOS::strncpy( sFileName,
                    aFileName,
                    SM_MAX_FILE_NAME );
    sFileName[ SM_MAX_FILE_NAME ] = '\0';

    sStrLength = idlOS::strlen( sFileName );

    // [2] 테이블스페이스 이름을 복사한다.
    sChrPtr1 = idlOS::strchr( sFileName, '-' );
    IDE_TEST_RAISE( sChrPtr1 == NULL, err_invalie_filespec_format );

    idlOS::strncpy( sSpaceName, sFileName, (sChrPtr1 - sFileName) );
    sSpaceName[ SMI_MAX_TABLESPACE_NAME_LEN ] = '\0';

    sChrPtr1++;

    // [3] FILESPEC으로부터 핑퐁번호를 얻는다.
    sChrPtr2 = idlOS::strchr( sChrPtr1, '-' );
    IDE_TEST_RAISE( sChrPtr2 == NULL, err_invalie_filespec_format );

    idlOS::memset( sNumberStr, 0x00, SM_MAX_FILE_NAME + 1 );
    idlOS::strncpy( sNumberStr,
                    sChrPtr1,
                    (sChrPtr2 - sChrPtr1) );

    sNumberStr[ SM_MAX_FILE_NAME ] = '\0';
    sPingPongNum = (UInt)idlOS::atoi( sNumberStr );

    IDE_TEST_RAISE( sPingPongNum >= SMM_PINGPONG_COUNT,
                    err_not_exist_datafile );

    sChrPtr2++;

    // [4] FILESPEC으로부터 파일번호를 얻는다.
    idlOS::memset( sNumberStr, 0x00, SM_MAX_FILE_NAME + 1 );
    idlOS::strncpy( sNumberStr,
                    sChrPtr2, (sStrLength - (UInt)(sChrPtr2 - sFileName)) );

    sNumberStr[ SM_MAX_FILE_NAME ] = '\0';

    IDE_TEST_RAISE( smuUtility::isDigitForString(
                           &sNumberStr[0],
                           sStrLength - (UInt)(sChrPtr2 - sFileName) ) == ID_FALSE,
                    err_not_found_tablespace_by_name );

    sFileNum = (UInt)idlOS::atoi( sNumberStr );

    // [5] 테이블스페이스 검색
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeByName( sSpaceName,
                                                     (void**)&sSpaceNode,
                                                     ID_FALSE ) // lock
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSpaceNode == NULL, err_not_found_tablespace_by_name );

    // 메모리테이블스페이스인지 확인 => 에러 : 파일없음
    IDE_TEST_RAISE( sctTableSpaceMgr::isMemTableSpace( sSpaceNode ) != ID_TRUE,
                    err_not_found_tablespace_by_name );

    // 핑퐁번호 확인 => 에러 : 파일 없음
    IDE_TEST_RAISE( sPingPongNum >= SMM_PINGPONG_COUNT,
                    err_not_exist_datafile );

    for ( sLoop = 0 ; sLoop < SMM_PINGPONG_COUNT ; sLoop ++ )
    {
        // CREATE LSN 확인 => 에러 : 파일 없음
        IDE_TEST( smmManager::getDBFile( (smmTBSNode*)sSpaceNode,
                                         sLoop,
                                         sFileNum,
                                         SMM_GETDBFILEOP_NONE,
                                         &sDatabaseFile[sLoop] )
                  != IDE_SUCCESS );

        sDatabaseFile[sLoop]->getChkptImageHdr( &sChkptImageHdr );

        IDE_TEST_RAISE( sDatabaseFile[sLoop]->checkValuesOfDBFHdr( &sChkptImageHdr ),
                        err_not_exist_datafile );

        // [6] 데이타파일이 존재하는지 검사한다.
        IDE_TEST( smmDatabaseFile::makeDBDirForCreate(
                                   (smmTBSNode*)sSpaceNode,
                                   sFileNum,
                                   sCreateDir ) != IDE_SUCCESS );

        idlOS::snprintf( sFullPath, SM_MAX_FILE_NAME,
                                    "%s%c%s-%"ID_INT32_FMT"-%"ID_INT32_FMT,
                                    sCreateDir,
                                    IDL_FILE_SEPARATOR,
                                    sSpaceName,
                                    sLoop,
                                    sFileNum );

        // [7] 기존 경로에 데이타파일이 이미 존재하는지 검사
        IDE_TEST_RAISE( idf::access( sFullPath, F_OK ) == 0,
                        err_already_exist_datafile );
    }

    // STABLE DB 확인 => 에러 : STABLE한 파일명 입력 요구
    IDE_TEST_RAISE( sPingPongNum != smmManager::getCurrentDB( (smmTBSNode*)sSpaceNode ),
                    err_input_unstable_chkpt_image );

    // [8] 이전 파일로 동일한 정보를가지고, Empty 데이타파일을
    // 생성한다. 데이타파일의 헤더는 RedoLSN만 모두 INIT으로
    // 설정하고, 나머지는 로그앵커에 저장된 내용으로 설정한다.
    /* BUG-39354 
     * Empty Memory Checkpoint Image 파일의 RedoLSN을 INIT으로 설정하면
     * 데이터파일 유효성 검사를 통과하지 못합니다.
     * Empty 파일의 RedoLSN을 CreateLSN과 같은 값으로 설정하여
     * RedoLSN과 CreateLSN이 같으면 Empty 파일로 알수 있게 합니다. */
    SM_GET_LSN( sChkptImageHdr.mMemRedoLSN, sChkptImageHdr.mMemCreateLSN );

    for ( sLoop = 0 ; sLoop < SMM_PINGPONG_COUNT ; sLoop ++ )
    {
        /* BUG-39354
         * 로그앵커의 내용을 그대로 사용할 경우
         * 데이타파일이 존재한다고 기록되어 있기 때문에
         * 실제 데이타파일은 존재하지 않아 문제가 발생합니다.
         * 새로 생성할 파일이 존재하지 않다고 정보를 변경해 주어야합니다. */
        smmManager::setCreateDBFileOnDisk( (smmTBSNode*)sSpaceNode,
                                           sLoop,
                                           sFileNum,
                                           ID_FALSE );

        IDE_TEST( sDatabaseFile[sLoop]->createDbFile(
                                            (smmTBSNode*)sSpaceNode,
                                            sLoop,
                                            sFileNum,
                                            0 /* 파일헤더페이지만 기록 */,
                                            &sChkptImageHdr )
                  != IDE_SUCCESS );
    }

    /* BUG-39354
     * Memory Checkpoint Image 파일은 0번 페이지에 Membase 정보를 가지고 있습니다.
     * Membase에 정상적인 값이 있어야만 Media Recovery를 수행할 수 있기 때문에
     * Membase를 생성해 주어야합니다.
     * DicMemBase를 읽어와서 필요한 값을 복사하고 초기값이 필요한 값만 초기화 시켜줍니다. */
    sDicTBSNode = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    IDE_TEST( smmManager::readMemBaseFromFile( sDicTBSNode,
                                               &sDicMemBase )
              != IDE_SUCCESS);

    IDE_TEST( iduFile::allocBuff4DirectIO( IDU_MEM_SM_SMI,
                                           SM_PAGE_SIZE,
                                           (void**)&sPageBuffer,
                                           (void**)&sDummyPage )
              != IDE_SUCCESS );
    sStage = 1;
    IDU_FIT_POINT( "BUG-45283@smiMediaRecovery::createChkptImage" );

    idlOS::memset( sDummyPage, 0, SM_PAGE_SIZE );
    sBasePage = (UChar*) sDummyPage;

    sTBSNode           = (smmTBSNode*) sSpaceNode;
    sTBSNode->mMemBase = (smmMemBase *)(sBasePage + SMM_MEMBASE_OFFSET);

    /* BUG-39354
     * DataFile Signature의 정보는 각 TBS 별로 고유한 값이나
     * Valid Check시에 사용되는 값은 항상 DicMemBase값이기 때문에
     * 그냥 복사해주었습니다. */
    idlOS::memcpy( sTBSNode->mMemBase,
                   &sDicMemBase,
                   ID_SIZEOF(smmMemBase) );

    sTBSNode->mMemBase->mAllocPersPageCount    = 0;
    sTBSNode->mMemBase->mCurrentExpandChunkCnt = 0;
    SM_INIT_SCN( &(sTBSNode->mMemBase->mSystemSCN) );

    for ( sLoop = 0 ; sLoop < SMM_PINGPONG_COUNT ; sLoop++ )
    {
        (void)sDatabaseFile[sLoop]->writePage( sTBSNode, SMM_MEMBASE_PAGEID, (UChar*) sDummyPage );
        (void)sDatabaseFile[sLoop]->syncUntilSuccess();
    }

    sStage = 0;
    IDE_TEST( iduMemMgr::free( sPageBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_MediaRecoDataFile,
                                 "ALTER DATABASE CRAEATE CHECKPOINT IMAGE") );
    }
    IDE_EXCEPTION( err_invalie_filespec_format );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_CIMAGE_FILESPEC_FORMAT,
                                  sFileName ) );
    }
    IDE_EXCEPTION( err_not_found_tablespace_by_name );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNodeByName,
                                  sSpaceName ) );
    }
    IDE_EXCEPTION( err_already_exist_datafile );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistFile, sFullPath ) );
    }
    IDE_EXCEPTION( err_not_exist_datafile );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistFile ) );
    }
    IDE_EXCEPTION( err_input_unstable_chkpt_image );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INPUT_UNSTABLE_CIMAGE,
                                  sFileName ));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sPageBuffer ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;

}

/*********************************************************
 * function description:
 * - disk 가 깨져서, 다른 위치에 데이타 파일을 옮기는
 *   경우를 위한 데이타 파일 이름 변경.
 *
 *   alter database rename file 'old-file-name' to 'new_file_name'
 *
 * - pseudo code
 *   기존 파일이름으로 hash에서 데이타 파일노드를 검색;
 *   if(검색이 성공)
 *   then
 *       데이타 파일 노드의 file이름을 갱신한다.
 *      // BUGBUG- logAnchor  sync는 restart recovery
 *     // 후에 하는것으로 결정.
 *   else
 *    return failture
 *   fi
 *********************************************************/
IDE_RC smiMediaRecovery::renameDataFile(SChar* aOldFileSpec,
                                        SChar* aNewFileSpec)
{

    UInt             sStrLength;
    scSpaceID        sSpaceID;
    sddDataFileNode* sOldFileNode;
    sddDataFileNode* sNewFileNode;
    SChar            sOldFileSpec[SMI_MAX_DATAFILE_NAME_LEN + 1];
    SChar            sNewFileSpec[SMI_MAX_DATAFILE_NAME_LEN + 1];

    IDE_ASSERT( aOldFileSpec != NULL);
    IDE_ASSERT( aNewFileSpec != NULL);

    // 다단계startup단계중 control 단계에서만 불릴수 있다.
    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);

    idlOS::memset(sOldFileSpec, 0x00,SMI_MAX_DATAFILE_NAME_LEN);
    idlOS::memset(sNewFileSpec, 0x00,SMI_MAX_DATAFILE_NAME_LEN);

    // [1] 기존 FILESPEC을 복사한다.
    idlOS::strncpy( sOldFileSpec,
                    aOldFileSpec,
                    SMI_MAX_DATAFILE_NAME_LEN );
    sOldFileSpec[ SMI_MAX_DATAFILE_NAME_LEN ] = '\0';

    sStrLength = idlOS::strlen(sOldFileSpec);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_FALSE,
                                sOldFileSpec,
                                &sStrLength,
                                SMI_TBS_DISK )
              != IDE_SUCCESS );

    // [2] 새 FILESPEC을 복사한다.
    idlOS::strncpy( sNewFileSpec,
                    aNewFileSpec,
                    SMI_MAX_DATAFILE_NAME_LEN );
    sNewFileSpec[ SMI_MAX_DATAFILE_NAME_LEN ] = '\0';

    sStrLength = idlOS::strlen(sNewFileSpec);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_TRUE,
                                sNewFileSpec,
                                &sStrLength,
                                SMI_TBS_DISK )
              != IDE_SUCCESS );

    //기존 파일이름으로 hash에서 데이타 파일노드를 검색.
    sctTableSpaceMgr::getDataFileNodeByName( sOldFileSpec,
                                             &sOldFileNode,
                                             &sSpaceID );

    IDE_TEST_RAISE(sOldFileNode == NULL, err_not_found_filenode);

    sctTableSpaceMgr::getDataFileNodeByName( sNewFileSpec,
                                             &sNewFileNode,
                                             &sSpaceID );

    IDE_TEST_RAISE(sNewFileNode != NULL, err_inuse_filename );

    IDE_TEST( sddDiskMgr::alterDataFileName( NULL, /* idvSQL* */
                                             sSpaceID,
                                             sOldFileSpec,
                                             sNewFileSpec )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_MediaRecoDataFile,
                                  "ALTER DATABASE RENAME DATAFILE") );
    }
    IDE_EXCEPTION( err_not_found_filenode );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_NotFoundDataFileNode,
                                 sOldFileSpec ) );
    }
    IDE_EXCEPTION( err_inuse_filename );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UseFileInOtherTBS, sNewFileSpec));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
 * function description: smiMediaRecovery::recoverDatabase
 * - 현재 시점까지 media recovery 하려면 aUntilTime을
 *   ULong max로.
 *
 * - 과거의 특정시점으로 DB으로 돌릴때는
 *   date를 idlOS::time(0)으로 변환한 값을 넘긴다.
 *   recover database recover database until time '2005-01-29:17:55:00'
 *
 *********************************************************/
IDE_RC smiMediaRecovery::recoverDB(idvSQL*        aStatistics,
                                   smiRecoverType aRecvType,
                                   UInt           aUntilTime,
                                   SChar*         aUntilTag)
{
    SChar * sLastRestoredTagName;

    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_archivelog_mode);

    if( aRecvType == SMI_RECOVER_UNTILTAG )
    {
        sLastRestoredTagName = smrRecoveryMgr::getLastRestoredTagName();
        IDE_TEST_RAISE( idlOS::strcmp( sLastRestoredTagName, aUntilTag ) != 0,
                        err_until_tag );

        aRecvType  = SMI_RECOVER_UNTILTIME;
        aUntilTime = smrRecoveryMgr::getCurrMediaTime();
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( smrRecoveryMgr::recoverDB( aStatistics,
                                         aRecvType,
                                         aUntilTime ) != IDE_SUCCESS );

    smrRecoveryMgr::initLastRestoredTagName();

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_MediaRecoDataFile,
                           "ALTER RECOVER DATABASE"));
    }
    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION(err_until_tag);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrUntilTag, 
                                sLastRestoredTagName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
 * PROJ-2133 incremental backup
 * function description: smiMediaRecovery::restoreDatabase
 * - 현재 시점까지 media restore 하려면 aUntilTime을
 *   ULong max로, aUntilTag는 NULL로 한다.
 *
 * - 과거의 특정시점으로 DB으로 돌릴때는
 *   date를 idlOS::time(0)으로 변환한 값을 넘긴다.
 *   recover database restore database until time '2005-01-29:17:55:00'
 *
 *  or
 *
 *   tag를 구문을 사용한다.
 *   recover database restore database from tag 'tag_name'
 *
 *********************************************************/
IDE_RC smiMediaRecovery::restoreDB( smiRestoreType    aRestoreType,
                                    UInt              aUntilTime,
                                    SChar           * aUntilTag )
{
    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_archivelog_mode);

    IDE_TEST( smrRecoveryMgr::restoreDB( aRestoreType,
                                         aUntilTime,  
                                         aUntilTag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_MediaRecoDataFile,
                           "ALTER RESTORE DATABASE"));
    }
    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * PROJ-2133 incremental backup
 * function description: smiMediaRecovery::restoreDatabase
 * TBS의 복원및 복구는 완전복구만 가능하기 때문에
 * 불완전복구와 관련된 정보가 필요 없다.
 *
 *
 *********************************************************/
IDE_RC smiMediaRecovery::restoreTBS( scSpaceID    aSpaceID )
{

    IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_archivelog_mode);

    IDE_TEST( smrRecoveryMgr::restoreTBS( aSpaceID ) != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_startup_phase);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_MediaRecoDataFile,
                           "ALTER RECOVER DATABASE"));
    }
    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
