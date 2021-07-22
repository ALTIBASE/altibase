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
* $Id: smmTBSMediaRecovery.cpp 19201 2006-11-30 00:54:40Z kmkim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smu.h>
#include <smm.h>
#include <smmReq.h>
#include <sctTableSpaceMgr.h>

/*
  생성자 (아무것도 안함)
*/
smmTBSMediaRecovery::smmTBSMediaRecovery()
{

}


/*
   PRJ-1548 User Memory Tablespace

   메모리 테이블스페이스를 모두 백업한다.

  [IN] aTrans : 트랜잭션
  [IN] aBackupDir : 백업 Dest. 디렉토리
*/
IDE_RC smmTBSMediaRecovery::backupAllMemoryTBS( idvSQL * aStatistics,
                                                void   * aTrans,
                                                SChar  * aBackupDir )
{
    sctActBackupArgs sActBackupArgs;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aBackupDir != NULL );

    sActBackupArgs.mTrans               = aTrans;
    sActBackupArgs.mBackupDir           = aBackupDir;
    sActBackupArgs.mCommonBackupInfo    = NULL;
    sActBackupArgs.mIsIncrementalBackup = ID_FALSE;

    // BUG-27204 Database 백업 시 session event check되지 않음
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                        aStatistics,
                        smmTBSMediaRecovery::doActOnlineBackup,
                        (void*)&sActBackupArgs, /* Action Argument*/
                        SCT_ACT_MODE_NONE) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  PRJ-1548 User Memory Tablespace

  테이블스페이스의 상태를 설정하고 백업을 수행한다.
  CREATING 중이거나 DROPPING 중인 경우 TBS Mgr Latch를 풀고
  잠시 대기하다가 Latch를 다시 시도한 다음 다시 시도한다.

  본 함수에 호출되기전에 TBS Mgr Latch는 획득된 상태이다.

  [IN] aSpaceNode : 백업할 TBS Node
  [IN] aActionArg : 백업에 필요한 인자
*/
IDE_RC smmTBSMediaRecovery::doActOnlineBackup(
                              idvSQL            * aStatistics,
                              sctTableSpaceNode * aSpaceNode,
                              void              * aActionArg )
{
    sctActBackupArgs * sActBackupArgs;
    UInt               sState = 0;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aActionArg != NULL );

    sActBackupArgs = (sctActBackupArgs*)aActionArg;

    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        if ( ((aSpaceNode->mState & SMI_TBS_DROPPED)   != SMI_TBS_DROPPED) &&
             ((aSpaceNode->mState & SMI_TBS_DISCARDED) != SMI_TBS_DISCARDED) )
        {
            /* ------------------------------------------------
             * [1] disk table space backup 상태 변경및
             * 첫번째 데이타파일의 상태를 backup으로 변경한다.
             * ----------------------------------------------*/
            IDE_TEST( sctTableSpaceMgr::startTableSpaceBackup(
                          aStatistics,
                          aSpaceNode ) != IDE_SUCCESS );
            sState = 1;

            if ( (aSpaceNode->mState & SMI_TBS_DROPPED) != SMI_TBS_DROPPED )
            {
                IDE_DASSERT( SMI_TBS_IS_ONLINE(aSpaceNode->mState));

                if ( sActBackupArgs->mIsIncrementalBackup == ID_TRUE )
                {
                    IDE_TEST( smLayerCallback::incrementalBackupMemoryTBS(
                                  aStatistics,
                                  (smmTBSNode*)aSpaceNode,
                                  sActBackupArgs->mBackupDir,
                                  sActBackupArgs->mCommonBackupInfo )
                              != IDE_SUCCESS );

                }
                else
                {
                    IDE_TEST( smLayerCallback::backupMemoryTBS(
                                  aStatistics,
                                  (smmTBSNode*)aSpaceNode,
                                  sActBackupArgs->mBackupDir )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // 테이블스페이스가 DROPPED, DISCARDED 상태인 경우 백업을 하지 않는다.
                // NOTHING TO DO ...
            }

            sState = 0;
            IDE_TEST( sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                                             aSpaceNode ) != IDE_SUCCESS );
        }
    }
    else
    {
        // 디스크 테이블스페이스의 백업은 본 함수에서 처리하지 않는다.
        // NOTHING TO DO..
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                               aSpaceNode ) ;
    }

    return IDE_FAILURE;

}

/*
   PROJ-2133 incremental backup

   메모리 테이블스페이스를 모두 incremental 백업한다.

  [IN] aTrans : 트랜잭션
  [IN] aBackupDir : 백업 Dest. 디렉토리
*/
IDE_RC smmTBSMediaRecovery::incrementalBackupAllMemoryTBS( 
                                                idvSQL     * aStatistics,
                                                void       * aTrans,
                                                smriBISlot * aCommonBackupInfo,
                                                SChar      * aBackupDir )
{
    sctActBackupArgs sActBackupArgs;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aBackupDir != NULL );

    sActBackupArgs.mTrans               = aTrans;
    sActBackupArgs.mBackupDir           = aBackupDir;
    sActBackupArgs.mCommonBackupInfo    = aCommonBackupInfo;
    sActBackupArgs.mIsIncrementalBackup = ID_TRUE;

    // BUG-27204 Database 백업 시 session event check되지 않음
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                        aStatistics,
                        smmTBSMediaRecovery::doActOnlineBackup,
                        (void*)&sActBackupArgs, /* Action Argument*/
                        SCT_ACT_MODE_NONE) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    하나의 Tablespace에 속한 모든 DB file의 Header에 Redo LSN을 기록

    aSpaceNode [IN] Redo LSN이 기록될 Tablespace의 Node

    [주의] Tablespace의 Sync Latch가 잡힌채로 이 함수가 호출된다.
 */
IDE_RC smmTBSMediaRecovery::flushRedoLSN4AllDBF( smmTBSNode * aSpaceNode )
{
    IDE_ASSERT( aSpaceNode != NULL );


    UInt                sLoop;
    SInt                sWhichDB;
    smmDatabaseFile*    sDatabaseFile;

    // Stable과 UnStable 모두 Memory Redo LSN을 설정한다.
    for ( sWhichDB = 0; sWhichDB < SMM_PINGPONG_COUNT; sWhichDB ++ )
    {
        for ( sLoop = 0; sLoop <= aSpaceNode->mLstCreatedDBFile;
              sLoop ++ )
        {
            IDE_TEST( smmManager::getDBFile( aSpaceNode,
                                             sWhichDB,
                                             sLoop,
                                             SMM_GETDBFILEOP_NONE,
                                             &sDatabaseFile )
                      != IDE_SUCCESS );

            IDE_ASSERT( sDatabaseFile != NULL );

            if ( sDatabaseFile->isOpen() == ID_TRUE )
            {
                // sync할 dbf 노드에 checkpoint 정보 설정
                sDatabaseFile->setChkptImageHdr(
                                    sctTableSpaceMgr::getMemRedoLSN(),
                                    NULL,     // aMemCreateLSN
                                    NULL,     // aSpaceID
                                    NULL,     // aSmVersion
                                    NULL );   // aDataFileDescSlotID

                IDE_ASSERT( sDatabaseFile->flushDBFileHdr()
                            == IDE_SUCCESS );
            }
            else
            {
                // 아직 생성되지 않은 데이타파일이다.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/*
  테이블스페이스의 데이타파일 메타헤더에 체크포인트정보를 갱신한다.
  본 함수에 호출되기전에 TBS Mgr Latch는 획득된 상태이다.

  [IN] aSpaceNode : Sync할 TBS Node
  [IN] aActionArg : NULL
*/
IDE_RC smmTBSMediaRecovery::doActUpdateAllDBFileHdr(
                                       idvSQL             * /* aStatistics*/,
                                       sctTableSpaceNode * aSpaceNode,
                                       void              * aActionArg )
{
    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aActionArg == NULL );
    
    ACP_UNUSED( aActionArg );

    UInt sStage = 0;


    if ( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE )
    {
        IDE_TEST( sctTableSpaceMgr::latchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
        sStage = 1;


        if ( sctTableSpaceMgr::hasState( aSpaceNode,
                                         SCT_SS_SKIP_UPDATE_DBFHDR )
             == ID_TRUE )
        {
            // 메모리테이블스페이스가 DROPPED/DISCARDED 인 경우
            // 갱신하지 않음.
        }
        else
        {
            // Tablespace에 속한 모든 DB File의 Header에
            // Redo LSN을 기록한다.
            IDE_TEST( flushRedoLSN4AllDBF( (smmTBSNode*) aSpaceNode )
                      != IDE_SUCCESS );

        }

        sStage = 0;
        IDE_TEST( sctTableSpaceMgr::unlatchSyncMutex( aSpaceNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // 메모리테이블스페이스가 아닌경우 갱신하지 않음.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            IDE_ASSERT( sctTableSpaceMgr::unlatchSyncMutex( aSpaceNode )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
  메모리 테이블스페이스에 대한 체크포인트 연산중에 디스크
  Stable/Unstable 체크포인트 이미지들의 메타헤더를 갱신한다.
  정보를 갱신한다.
*/
IDE_RC smmTBSMediaRecovery::updateDBFileHdr4AllTBS()
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                  NULL, /* idvSQL* */
                  smmTBSMediaRecovery::doActUpdateAllDBFileHdr,
                  NULL, /* Action Argument*/
                  SCT_ACT_MODE_NONE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  데이터베이스 확장에 따라 새로운 Expand Chunk가 할당됨에 따라
  새로 생겨나게 되는 DB파일마다 런타임 헤더에 보정되지 않은 i
  CreateLSN을 설정한다.

  Chunk가 확장되더라도 한번에 하나의 DB파일만 생성될 수 있다.

  [IN] aSpaceNode      - 메모리 테이블스페이스 노드
  [IN] aNewDBFileChunk - 확장되면서 새로 늘어날 데이타파일 개수
  [IN] aCreateLSN      - 데이타파일(들)의 CreateLSN
 */
IDE_RC smmTBSMediaRecovery::setCreateLSN4NewDBFiles(
                                           smmTBSNode * aSpaceNode,
                                           smLSN      * aCreateLSN )
{
    UInt              sWhichDB;
    UInt              sCurrentDBFileNum;
    smmDatabaseFile * sDatabaseFile;

    IDE_DASSERT( aSpaceNode     != NULL );
    IDE_DASSERT( aCreateLSN  != NULL );

    // 데이타파일의 런타임 헤더에 CreateLSN 설정
    for ( sWhichDB = 0; sWhichDB < SMM_PINGPONG_COUNT; sWhichDB ++ )
    {
        sCurrentDBFileNum = aSpaceNode->mMemBase->mDBFileCount[ sWhichDB ];

        IDE_TEST( smmManager::getDBFile( aSpaceNode,
                                         sWhichDB,
                                         sCurrentDBFileNum,
                                         SMM_GETDBFILEOP_NONE,
                                         &sDatabaseFile )
                  != IDE_SUCCESS );

        // 새로 생성된 데이타파일 헤더를 설정한다.
        // Memory Redo LSN을 설정하지 않는 것은
        // 실제 파일 생성할때 설정하면 되기 때문이다.
        sDatabaseFile->setChkptImageHdr(
                                NULL,         // Memory Redo LSN 설정하지 않음.
                                aCreateLSN,
                                &aSpaceNode->mHeader.mID,
                                (UInt*)&smVersionID,
                                NULL ); // aDataFileDescSlotID
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// 서버구동시에 모든 메모리 테이블스페이스의 모든 메모리 DBFile들의
// 미디어 복구 필요 여부를 체크한다.
IDE_RC smmTBSMediaRecovery::identifyDBFilesOfAllTBS( idBool aIsOnCheckPoint )
{
    sctActIdentifyDBArgs sIdentifyDBArgs;

    sIdentifyDBArgs.mIsValidDBF  = ID_TRUE;
    sIdentifyDBArgs.mIsFileExist = ID_TRUE;

    if ( aIsOnCheckPoint == ID_FALSE )
    {
        // 서버구동시
        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                      NULL, /* idvSQL* */
                      smmTBSMediaRecovery::doActIdentifyAllDBFiles,
                      &sIdentifyDBArgs, /* Action Argument*/
                      SCT_ACT_MODE_NONE ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIdentifyDBArgs.mIsFileExist != ID_TRUE,
                        err_file_not_found );

        IDE_TEST_RAISE( sIdentifyDBArgs.mIsValidDBF != ID_TRUE,
                        err_need_media_recovery );
    }
    else
    {
        // 체크포인트과정에서 런타임헤더가 파일에
        // 제대로 기록이 되었는지 데이타파일을 다시 읽어서
        // 검증한다.
        IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                      NULL, /* idvSQL* */
                      doActIdentifyAllDBFiles,
                      &sIdentifyDBArgs, /* Action Argument*/
                      SCT_ACT_MODE_LATCH ) != IDE_SUCCESS );

        IDE_ASSERT( sIdentifyDBArgs.mIsValidDBF  == ID_TRUE );
        IDE_ASSERT( sIdentifyDBArgs.mIsFileExist == ID_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistFile ) );
    }
    IDE_EXCEPTION( err_need_media_recovery );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NeedMediaRecovery ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  모든 데이타파일의 메타헤더를 판독하여 미디어 오류여부를 확인한다.

  [ 중요 ]
  Stable 데이타파일만 검사하는 것이 아니고, UnStable 데이타파일의
  존재여부와 Stable 데이타파일과 동일한 버전인지는 확인할 필요가 있다.
  검증하지 않고 서버구동을 실시한다면 다른 Unstable 데이타파일로 인해
  데이타베이스 일관성이 깨질수 있다.

  [ 알고리즘 ***** ]

  loganchor에 저장되어 있는 데이타파일에 대해서 검사한다.
  1. 모든 Tablespace에 대해서 loganchor상에 저장된 Stable Version은
     반드시 존재해야한다.
  2. 모든 Tablespace에 대해서 loganchor상에 저장된 Unstable Version은
     없을수 도 있지만, 있으면  반드시 유효한  Stable Version과의
     유효한 버전이어야 한다.
  3. 만약 아직 생성하지 않은 Checkpoint Image File이 해당 폴더에 존재하면
     유효성 검사를 하지 않는다. (BUG-29607)
     create cp image에서 지우고 다시 생성한다.

  [IN]  aTBSNode   - 검증할 TBS Node
  [OUT] aActionArg - 데이타파일의 존재여부 및 유효버전 여부
*/
IDE_RC smmTBSMediaRecovery::doActIdentifyAllDBFiles(
                           idvSQL             * /* aStatistics*/,
                           sctTableSpaceNode  * aTBSNode,
                           void               * aActionArg )
{
    UInt                     sWhichDB;
    UInt                     sFileNum;
    idBool                   sIsMediaFailure;
    smmTBSNode             * sTBSNode;
    smmDatabaseFile        * sDatabaseFile;
    smmChkptImageHdr         sChkptImageHdr;
    smmChkptImageAttr        sChkptImageAttr;
    sctActIdentifyDBArgs   * sIdentifyDBArgs;
    SChar                    sMsgBuf[ SM_MAX_FILE_NAME ];

    IDE_DASSERT( aTBSNode != NULL );
    IDE_DASSERT( aActionArg != NULL );

    // 메모리 테이블스페이스가 아닌 경우 체크하지 않는다.
    IDE_TEST_CONT( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) != ID_TRUE ,
                   CONT_SKIP_IDENTIFY );

    // 테이블스페이스가 DISCARD된 경우 체크하지 않는다.
    // 테이블스페이스가 삭제된 경우 체크하지 않는다.
    IDE_TEST_CONT( sctTableSpaceMgr::hasState( aTBSNode,
                                                SCT_SS_SKIP_IDENTIFY_DB )
                    == ID_TRUE , CONT_SKIP_IDENTIFY );

    sIdentifyDBArgs = (sctActIdentifyDBArgs*)aActionArg;
    sTBSNode        = (smmTBSNode*)aTBSNode;

    // LstCreatedDBFile은 RESTART가 완료된이후에 재계산되며,
    // 로그앵커가 초기화시에도 계산이 된다.
    for ( sFileNum = 0;
          sFileNum <= sTBSNode->mLstCreatedDBFile;
          sFileNum ++ )
    {
        // Stable과 Unstable 데이타파일 모두 검사한다.
        for ( sWhichDB = 0; sWhichDB < SMM_PINGPONG_COUNT; sWhichDB ++ )
        {
            // PRJ-1548 User Memory Tablespace
            // Loganchor에 저장된 Checkpoint Image는 반드시 존재해야한다.

            // 파일 생성여부를 확인한다.
            if ( smmManager::getCreateDBFileOnDisk( sTBSNode,
                                                    sWhichDB,
                                                    sFileNum )
                 == ID_FALSE )
            {
                /* BUG-23700: [SM] Stable DB의 DB File들이 Last Create File
                 * 이전에 모두 존재하지는 않습니다.
                 *
                 * Table이 Drop되면 할당된 Page가 Free가 되고 Checkpoint시에는
                 * Free된 페이지를 내리지 않습니다. 해서 TBS의 mLstCreatedDBFile
                 * 이전 파일중에서 Table들이 Drop되어서 중간 File들은 한번도
                 * 페이지가 내려가지 않아서 File생성이 되지 않고 뒤 파일들은
                 * 내려가서 생기는 경우가 존재하여 Stable DB또한 mLstCreatedDBFile
                 * 이전에 모든 DBFile이 존재하지는 않습니다. */

                // BUG-29607 Checkpoint를 완료 하고 유효성 검사를 할 때
                // 체크포인트에서 사용하지 않은, 자신이 생성하지도 않은 CP Image File의
                // 유효성을 검사하다, 오류를 발생하고 FATAL후 Server Start가 되지 않는
                // 문제가 있었습니다. 자신이 생성한 CP Image File의 유효성만 검사합니다.
                continue;
            }

            // 파일을 생성 하였다면 OPEN이 가능해야 한다.
            if ( smmManager::openAndGetDBFile( sTBSNode,
                                               sWhichDB,
                                               sFileNum,
                                               &sDatabaseFile )
                 != IDE_SUCCESS )
            {
                // fix BUG-17343 으로 인해서
                // Unstable은 아직 생성되었는지 정학하게 확인
                // 가능하다.
                sIdentifyDBArgs->mIsFileExist = ID_FALSE;

                idlOS::snprintf(sMsgBuf, SM_MAX_FILE_NAME,
                                "  [SM-WARNING] CANNOT IDENTIFY DATAFILE\n\
                                [TBS:%s, PPID-%"ID_UINT32_FMT"-FID-%"ID_UINT32_FMT"] Datafile Not Found\n",
                                sTBSNode->mHeader.mName,
                                sWhichDB,
                                sFileNum );

                IDE_CALLBACK_SEND_MSG(sMsgBuf);

                continue;
            }

            // version, oldest lsn, create lsn 일치하지 않으면
            // media recovery가 필요하다.
            IDE_TEST_RAISE( sDatabaseFile->checkValidationDBFHdr(
                                                    &sChkptImageHdr,
                                                    &sIsMediaFailure ) != IDE_SUCCESS,
                            err_invalid_data_file_header );

            if ( sIsMediaFailure == ID_TRUE )
            {
                // 미디어 오류가 있는 데이타파일
                sDatabaseFile->getChkptImageAttr( sTBSNode,
                                                  &sChkptImageAttr );

                idlOS::snprintf( sMsgBuf, 
                                 SM_MAX_FILE_NAME,
                                 "  [SM-WARNING] CANNOT IDENTIFY DATAFILE\n\
                            [TBS:%s, PPID-%"ID_UINT32_FMT"-FID:%"ID_UINT32_FMT"] Mismatch Datafile Version\n",
                                 sTBSNode->mHeader.mName,
                                 sFileNum,
                                 sChkptImageAttr.mFileNum );

                IDE_CALLBACK_SEND_MSG( sMsgBuf );

                sIdentifyDBArgs->mIsValidDBF = ID_FALSE;
            }
            else
            {
                // 미디어 오류가 없는 데이타파일
            }
        }
    }

    IDE_EXCEPTION_CONT( CONT_SKIP_IDENTIFY );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_data_file_header );
    {
        /* BUG-33353 */
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDatafileHeader,
                                  sDatabaseFile->getFileName(),
                                  sDatabaseFile->mSpaceID,
                                  sDatabaseFile->mFileNum,
                                  (sDatabaseFile->mChkptImageHdr).mMemRedoLSN.mFileNo,
                                  (sDatabaseFile->mChkptImageHdr).mMemRedoLSN.mOffset,
                                  sChkptImageHdr.mMemRedoLSN.mFileNo,
                                  sChkptImageHdr.mMemRedoLSN.mOffset,
                                  (sDatabaseFile->mChkptImageHdr).mMemCreateLSN.mFileNo,
                                  (sDatabaseFile->mChkptImageHdr).mMemCreateLSN.mOffset,
                                  sChkptImageHdr.mMemCreateLSN.mFileNo,
                                  sChkptImageHdr.mMemCreateLSN.mOffset,
                                  smVersionID & SM_CHECK_VERSION_MASK,
                                  (sDatabaseFile->mChkptImageHdr).mSmVersion & SM_CHECK_VERSION_MASK,
                                  sChkptImageHdr.mSmVersion & SM_CHECK_VERSION_MASK) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  메모리 테이블스페이스의 미디어오류가 있는 데이타파일 목록을 만든다.

  [ 중요 ]
  미디어 복구에서는 Stable 데이타파일들에 대해서만 미디어복구를
  진행한다. Unstable 데이타파일의 Indentify는 서버 구동과정에서도
  이루어진다.

  [IN]  aTBSNode              - 테이블스페이스 노드
  [IN]  aRecoveryType         - 미디어복구 타입
  [OUT] aMediaFailureDBFCount - 미디어오류가 발생한 데이타파일노드 개수
  [OUT] aFromRedoLSN          - 복구시작 Redo LSN
  [OUT] aToRedoLSN            - 복구완료 Redo LSN
*/
IDE_RC smmTBSMediaRecovery::makeMediaRecoveryDBFList( sctTableSpaceNode * aTBSNode,
                                                      smiRecoverType      aRecoveryType,
                                                      UInt              * aFailureChkptImgCount,
                                                      smLSN             * aFromRedoLSN,
                                                      smLSN             * aToRedoLSN )
{
    UInt                sWhichDB;
    UInt                sFileNum;
    smmTBSNode       *  sTBSNode;
    smmChkptImageHdr    sChkptImageHdr;
    smmDatabaseFile   * sDatabaseFile;
    idBool              sIsMediaFailure;
    SChar               sMsgBuf[ SM_MAX_FILE_NAME ];
    smLSN               sFromRedoLSN;
    smLSN               sToRedoLSN;

    IDE_DASSERT( aTBSNode              != NULL );
    IDE_DASSERT( aFailureChkptImgCount != NULL );
    IDE_DASSERT( aFromRedoLSN       != NULL );
    IDE_DASSERT( aToRedoLSN         != NULL );

    idlOS::memset( &sChkptImageHdr, 0x00, ID_SIZEOF(smmChkptImageHdr) );

    sTBSNode = (smmTBSNode*)aTBSNode;

    // StableDB 번호를 얻는다.
    sWhichDB = smmManager::getCurrentDB( sTBSNode );

    // 로그앵커에 저장된 Stable한 데이타파일들을 복구대상을 선정한다.
    // LstCreatedDBFile은 RESTART가 완료된이후에 재계산되며,
    // 로그앵커가 초기화시에도 계산이 된다.

    for( sFileNum = 0 ;
         sFileNum <= sTBSNode->mLstCreatedDBFile ;
         sFileNum ++ )
    {
        /* ------------------------------------------------
         * [1] 데이타파일이 존재하는지 검사
         * ----------------------------------------------*/
        // Loganchor에 저장된 Checkpoint Image는 반드시 존재해야한다.
        // 파일이 존재한다면 OPEN이 가능하다
        if ( smmManager::openAndGetDBFile( sTBSNode,
                                           sWhichDB,
                                           sFileNum,
                                           &sDatabaseFile )
             != IDE_SUCCESS )
        {
            idlOS::snprintf( sMsgBuf, SM_MAX_FILE_NAME,
                             "  [SM-WARNING] CANNOT IDENTIFY DATAFILE\n\
               [%s-<DBF ID:%"ID_UINT32_FMT">] DATAFILE NOT FOUND\n",
                             sTBSNode->mHeader.mName,
                             sFileNum );
            IDE_CALLBACK_SEND_MSG( sMsgBuf );

            IDE_RAISE( err_file_not_found );
        }

        /* ------------------------------------------------
         * [2] 데이타파일과 파일노드와 바이너리버전을 검사
         * ----------------------------------------------*/
        IDE_TEST_RAISE( sDatabaseFile->checkValidationDBFHdr(
                                        &sChkptImageHdr,
                                        &sIsMediaFailure ) != IDE_SUCCESS,
                        err_data_file_header_invalid );

        if ( sIsMediaFailure == ID_TRUE )
        {
            /*
               미디어 오류가 존재하는 경우

               일치하지 않는 경우는 완전(COMPLETE) 복구를
               수행해야 한다.
            */
            IDE_TEST_RAISE( ( aRecoveryType == SMI_RECOVER_UNTILTIME ) ||
                            ( aRecoveryType == SMI_RECOVER_UNTILCANCEL ),
                            err_incomplete_media_recovery );
        }
        else
        {
            /*
              미디어오류가 없는 데이타파일

              불완전복구(INCOMPLETE) 미디어 복구시에는
              백업본을 가지고 재수행을 시작하므로 REDO LSN이
              로그앵커와 일치하지 않는 경우는 존재하지 않는다.

              그렇다고 완전복구시에 모든 데이타파일이
              오류가 있어야 한다는 말은 아니다.
            */
        }

        if ( ( aRecoveryType == SMI_RECOVER_COMPLETE ) &&
             ( sIsMediaFailure != ID_TRUE ) )
        {
            // 완전복구시 오류가 없는 데이타파인 경우
            // 미디어 복구가 필요없다.
            continue;
        }
        else
        {
            // 오류를 복구하고자하는 완전복구 또는
            // 오류가 없는 불완전복구도
            // 모두 미디어 복구를 진행한다.

            // 불완전 복구 :
            // 데이타파일 헤더의 oldest lsn을 검사한 다음,
            // 불일치하는 경우 복구대상파일로 선정한다.

            // 완전복구 :
            // 모든 데이타파일이 복구대상이 된다.

            // 미디어오류 설정과 재수행 구간을 반환한다.
            IDE_TEST( sDatabaseFile->prepareMediaRecovery(
                                            aRecoveryType,
                                            &sChkptImageHdr,
                                            &sFromRedoLSN,
                                            &sToRedoLSN )
                      != IDE_SUCCESS );

            // 모든 복구대상 데이타파일들의 복구구간을 포함할 수 있는
            // 최소 From Redo LSN, 최대 To Redo LSN을 구합니다.
            if ( smLayerCallback::isLSNGT( aFromRedoLSN,
                                           &sFromRedoLSN )
                 == ID_TRUE )
            {
                SM_GET_LSN( *aFromRedoLSN,
                            sFromRedoLSN );
            }
            else
            {
                /* nothing to do ... */
            }
           
            if ( smLayerCallback::isLSNGT( &sToRedoLSN,
                                           aToRedoLSN )
                 == ID_TRUE)
            {
                SM_GET_LSN( *aToRedoLSN,
                            sToRedoLSN );
            }
            else
            {
                /* nothing to do ... */
            }

            // 메모리 데이타파일의 복구대상 파일 개수를 계산
            *aFailureChkptImgCount = *aFailureChkptImgCount + 1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistFile ) );
    }
    IDE_EXCEPTION( err_data_file_header_invalid );
    {
        /* BUG-33353 */
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDatafileHeader,
                                  sDatabaseFile->getFileName(),
                                  sDatabaseFile->mSpaceID,
                                  sDatabaseFile->mFileNum,
                                  (sDatabaseFile->mChkptImageHdr).mMemRedoLSN.mFileNo,
                                  (sDatabaseFile->mChkptImageHdr).mMemRedoLSN.mOffset,
                                  sChkptImageHdr.mMemRedoLSN.mFileNo,
                                  sChkptImageHdr.mMemRedoLSN.mOffset,
                                  (sDatabaseFile->mChkptImageHdr).mMemCreateLSN.mFileNo,
                                  (sDatabaseFile->mChkptImageHdr).mMemCreateLSN.mOffset,
                                  sChkptImageHdr.mMemCreateLSN.mFileNo,
                                  sChkptImageHdr.mMemCreateLSN.mOffset,
                                  smVersionID & SM_CHECK_VERSION_MASK,
                                  (sDatabaseFile->mChkptImageHdr).mSmVersion & SM_CHECK_VERSION_MASK,
                                  sChkptImageHdr.mSmVersion & SM_CHECK_VERSION_MASK) );
    }
    IDE_EXCEPTION( err_incomplete_media_recovery );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NeedMediaRecovery ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

///*
//   테이블스페이스의 N번째 데이타파일의 PageID 구간 반환
//
//   [IN] aTBSNode - 테이블스페이스 노드
//   [IN] aFileNum - 데이타파일 번호
//   [OUT] aFstPageID - 첫번째 Page ID
//   [OUT] aLstPageID - 마지막 Page ID
//
//*/
//void smmTBSMediaRecovery::getPageRangeOfNthFile( smmTBSNode * aTBSNode,
//                                                 UInt         aFileNum,
//                                                 scPageID   * aFstPageID,
//                                                 scPageID   * aLstPageID )
//{
//    scPageID  sFstPageID;
//    scPageID  sLstPageID;
//    UInt      sPageCountPerFile;
//    UInt      sFileNum;
//
//    IDE_DASSERT( aTBSNode   != NULL );
//    IDE_DASSERT( aFstPageID != NULL );
//    IDE_DASSERT( aLstPageID != NULL );
//
//    sLstPageID        = 0;
//    sFstPageID        = 0;
//    sPageCountPerFile = 0;
//
//    for ( sFileNum = 0; sFileNum <= aFileNum; sFileNum ++ )
//    {
//        sFstPageID += sPageCountPerFile;
//
//        // 이 파일에 기록할 수 있는 Page의 수
//        sPageCountPerFile = smmManager::getPageCountPerFile( aTBSNode,
//                                                             sFileNum );
//
//        sLstPageID = sFstPageID + sPageCountPerFile - 1;
//    }
//
//    *aFstPageID = sFstPageID;
//    *aLstPageID = sLstPageID;
//
//    return;
//}

/*
  미디어오류로 인해 미디어복구를 진행한 메모리 데이타파일들을
  찾아서 파일헤더를 복구한다.

  [IN]  aTBSNode   - 검색할 TBS Node
  [OUT] aActionArg - Repair 정보
*/
IDE_RC smmTBSMediaRecovery::doActRepairDBFHdr(
                              idvSQL             * /* aStatistics*/,
                              sctTableSpaceNode  * aSpaceNode,
                              void               * aActionArg )
{
    UInt                     sWhichDB;
    UInt                     sFileNum;
    UInt                     sNxtStableDB;
    idBool                   isOpened = ID_FALSE;
    smmTBSNode             * sTBSNode;
    smmDatabaseFile        * sDatabaseFile;
    smmDatabaseFile        * sNxtStableDatabaseFile;
    sctActRepairArgs       * sRepairArgs;
    SChar                    sNxtStableFileName[ SM_MAX_FILE_NAME ];
    SChar                  * sFileName;
    SChar                    sBuffer[SMR_MESSAGE_BUFFER_SIZE];
    idBool                   sIsCreated;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aActionArg != NULL );

    sTBSNode = (smmTBSNode*)aSpaceNode;
    sRepairArgs = (sctActRepairArgs*)aActionArg;

    while ( 1 )
    {
        if ( sctTableSpaceMgr::isMemTableSpace( sTBSNode ) != ID_TRUE )
        {
            // 메모리 테이블스페이스가 아닌 경우 체크하지 않는다.
            break;
        }

        if ( sctTableSpaceMgr::hasState( aSpaceNode,
                                         SCT_SS_UNABLE_MEDIA_RECOVERY ) 
             == ID_TRUE )
        {
            // 테이블스페이스가 DROPPED 이거나 DISCARD 상태인 경우
            // 미디어복구를 하지 않고 제외한다.
            break;
        }

        // StableDB 번호를 얻는다.
        sWhichDB = smmManager::getCurrentDB( sTBSNode );

        // LstCreatedDBFile은 RESTART가 완료된이후에 재계산되며,
        // 로그앵커가 초기화시에도 계산이 된다.
        for ( sFileNum = 0;
              sFileNum <= sTBSNode->mLstCreatedDBFile;
              sFileNum ++ )
        {
            // PRJ-1548 User Memory Tablespace
            // Loganchor에 저장된 Checkpoint Image는 반드시 존재해야한다.

            // 파일이 존재한다면 OPEN이 가능하다
            IDE_TEST( smmManager::getDBFile( sTBSNode,
                                             sWhichDB,
                                             sFileNum,
                                             SMM_GETDBFILEOP_NONE,
                                             &sDatabaseFile )
                      != IDE_SUCCESS );

            if ( sDatabaseFile->getIsMediaFailure() == ID_TRUE )
            {
                if ( sRepairArgs->mResetLogsLSN != NULL )
                {
                    IDE_ASSERT( ( sRepairArgs->mResetLogsLSN->mFileNo
                                  != ID_UINT_MAX ) &&
                                ( sRepairArgs->mResetLogsLSN->mOffset
                                  != ID_UINT_MAX ) );

                    // 불완전 복구시에는 인자로 받은 ResetLogsLSN
                    // 설정한다.
                    sDatabaseFile->setChkptImageHdr(
                                    sRepairArgs->mResetLogsLSN,
                                    NULL,   // aMemCreateLSN
                                    NULL,   // aSpaceID
                                    NULL,   // aSmVersion
                                    NULL ); // aDataFileDescSlotID
                }
                else
                {
                    // 완전복구시에는 Loganchor에 있는 정보를
                    // 그대로 설정함.
                }

                // 파일헤더 복구
                IDE_TEST( sDatabaseFile->flushDBFileHdr()
                          != IDE_SUCCESS );

            }
            else
            {
                // 미디어오류가 없는 파일
                // Nothing to do ...
            }

            //PROJ-2133 incremental backup
            //Media 복구가 끝난 체크포인트 이미지의 pingpong 체크포인트
            //이미지를 만든다.
            sNxtStableDB = smmManager::getNxtStableDB( sTBSNode );
            sIsCreated   = smmManager::getCreateDBFileOnDisk( sTBSNode,
                                                              sNxtStableDB,
                                                              sFileNum );

            /* CHKPT 이미지가 이미 생성되어있는 상태일때만 생성한다. */
            if ( sIsCreated == ID_TRUE )
            {
                IDE_TEST( smmManager::getDBFile( sTBSNode,
                                                 sNxtStableDB,                     
                                                 sFileNum,              
                                                 SMM_GETDBFILEOP_NONE,          
                                                 &sNxtStableDatabaseFile )               
                          != IDE_SUCCESS );

                if ( idlOS::strncmp( sNxtStableDatabaseFile->getFileName(),  
                                     "\0",                          
                                     SMI_MAX_DATAFILE_NAME_LEN ) == 0 )
                {
                    IDE_TEST( sNxtStableDatabaseFile->setFileName( 
                                                    sDatabaseFile->getFileDir(),
                                                    sTBSNode->mHeader.mName,    
                                                    sNxtStableDB,
                                                    sDatabaseFile->getFileNum() )
                            != IDE_SUCCESS );
                }
                else 
                {
                    /* nothing to do */
                }

                idlOS::strncpy( sNxtStableFileName, 
                                sNxtStableDatabaseFile->getFileName(),
                                SM_MAX_FILE_NAME );
    
                if ( sNxtStableDatabaseFile->isOpen() == ID_TRUE ) 
                {
                    isOpened = ID_TRUE;
                    // copy target 파일이 open된 상태에서 copy하면 안됨
                    IDE_TEST( sNxtStableDatabaseFile->close() 
                              != IDE_SUCCESS );
                }
                else 
                {
                    /* nothing to do */
                }

                if ( idf::access( sNxtStableFileName, F_OK )  == 0 )
                {
                    idf::unlink( sNxtStableFileName );
                }
                else 
                {
                    /* nothing to do */
                }

                sFileName = idlOS::strrchr( sNxtStableFileName, 
                                            IDL_FILE_SEPARATOR );
                sFileName = sFileName + 1;

                idlOS::snprintf( sBuffer,
                                 SMR_MESSAGE_BUFFER_SIZE,     
                                 "                Restoring unstable checkpoint image [%s]",
                                 sFileName );

                IDE_TEST( sDatabaseFile->copy( NULL, sNxtStableFileName )
                          != IDE_SUCCESS );

                IDE_CALLBACK_SEND_MSG(sBuffer);  

                if ( isOpened == ID_TRUE )
                {
                    IDE_TEST( sNxtStableDatabaseFile->open() != IDE_SUCCESS );
    
                    isOpened = ID_FALSE;
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
        }// for

        break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( isOpened == ID_TRUE )
    {
        /* isOpened이 TRUE 면  FDCnt는 0 이어야 한다.  
           아니더라도 크게 문제가 없으니 디버그에서만 확인 하도록 한다. */
        IDE_DASSERT( sNxtStableDatabaseFile->isOpen() != ID_TRUE )
        
        /* target 파일을 복사를 위해 닫았으니 다시 열어준다.*/
        (void)sNxtStableDatabaseFile->open();
        isOpened = ID_FALSE;

    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/*
   모든 테이블스페이스의 데이타파일에서 입력된 페이지 ID를 가지는
   Failure 데이타파일의 존재여부를 반환한다.

   [IN]  aTBSID        - 테이블스페이스 ID
   [IN]  aPageID       - 페이지 ID
   [OUT] aExistTBS     - TBSID에 해당하는 TableSpace 존재여부
   [OUT] aIsFailureDBF - 페이지 ID를 포함하는 Failure DBF 존재여부

*/
IDE_RC smmTBSMediaRecovery::findMatchFailureDBF( scSpaceID   aTBSID,
                                                 scPageID    aPageID,
                                                 idBool    * aIsExistTBS,
                                                 idBool    * aIsFailureDBF )
{
    idBool              sIsFDBF;
    idBool              sIsETBS;
    scPageID            sFstPageID;
    scPageID            sLstPageID;
    UInt                sWhichDB;
    UInt                sFileNum;
    smmTBSNode        * sTBSNode;
    smmDatabaseFile   * sDatabaseFile;

    IDE_DASSERT( aIsExistTBS   != NULL );
    IDE_DASSERT( aIsFailureDBF != NULL );

    // 테이블스페이스 노드 검색
    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::findSpaceNodeWithoutException( aTBSID );

    if ( sTBSNode != NULL )
    {
        // 페이지를 포함하는 데이타파일 번호를 반환한다.
        sFileNum = smmManager::getDbFileNo( sTBSNode, aPageID );

        // StableDB 번호를 얻는다.
        sWhichDB = smmManager::getCurrentDB( sTBSNode );

        // Failure DBF라는 것은 Create된 데이타파일 중에 존재한다.
        if ( sFileNum <= sTBSNode->mLstCreatedDBFile )
        {
            IDE_TEST( smmManager::getDBFile( sTBSNode,
                                             sWhichDB,
                                             sFileNum,
                                             SMM_GETDBFILEOP_NONE,
                                             &sDatabaseFile )
                      != IDE_SUCCESS );

            if ( sDatabaseFile->getIsMediaFailure() == ID_TRUE )
            {
                // 데이타파일의 페이지범위를 구한다.
                sDatabaseFile->getPageRangeInFile( &sFstPageID,
                                                   &sLstPageID );

                // 페이지가 파일에 포함되는지 확인
                IDE_ASSERT( (sFstPageID <= aPageID) &&
                            (sLstPageID >= aPageID) );

                // 페이지를 포함한 Failure 데이타파일을 찾은 경우
                sIsFDBF = ID_TRUE;
            }
            else
            {
                sIsFDBF = ID_FALSE;
            }
        }
        else
        {
             // 페이지를 포함한 Failure 데이타파일을 못 찾은 경우
             sIsFDBF = ID_FALSE;
        }

        sIsETBS = ID_TRUE;
    }
    else
    {
         // 페이지를 포함한 Failure 데이타파일을 못 찾은 경우
         sIsETBS = ID_FALSE;
         sIsFDBF = ID_FALSE;
    }

    *aIsExistTBS   = sIsETBS;
    *aIsFailureDBF = sIsFDBF;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  미디어복구시 할당되었던 객체들을 파괴하고
  메모리 해제한다.

*/
IDE_RC smmTBSMediaRecovery::resetTBSNode( smmTBSNode * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    if ( sctTableSpaceMgr::hasState( & aTBSNode->mHeader,
                                     SCT_SS_NEED_PAGE_PHASE )
         == ID_TRUE )
    {
        // Load된 모든 Page Memory반납, Page System해제
        IDE_TEST( smmTBSMultiPhase::finiPagePhase( aTBSNode )
                  != IDE_SUCCESS );

        // Page System초기화 ( Prepare/Restore안된 상태 )
        IDE_TEST( smmTBSMultiPhase::initPagePhase( aTBSNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   미디어복구시 할당했던 테이블스페이스의 자원을
   리셋한다.
*/
IDE_RC smmTBSMediaRecovery::doActResetMediaFailureTBSNode(
                                        idvSQL            * /* aStatistics*/,
                                        sctTableSpaceNode * aTBSNode,
                                        void              * aActionArg )
{
    IDE_DASSERT( aTBSNode   != NULL );
    IDE_DASSERT( aActionArg == NULL );

    ACP_UNUSED( aActionArg );

    if ( sctTableSpaceMgr::isMemTableSpace( aTBSNode ) == ID_TRUE )
    {
        // 미디어복구가 진행된 TBS의 경우는 Restore가 되어 있어야 한다.
        if ( ( ( smmTBSNode*)aTBSNode)->mRestoreType
               != SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET )
        {
            // 미디어복구가 진행된 TBS만 ResetTBS를 수행한다.
            // 그래서 다음 Assert를 만족해야만 한다.
            IDE_ASSERT( sctTableSpaceMgr::hasState( aTBSNode->mID,
                                                    SCT_SS_UNABLE_MEDIA_RECOVERY )
                        == ID_FALSE );

            IDE_TEST( resetTBSNode( (smmTBSNode *) aTBSNode )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   미디어복구시 할당했던 테이블스페이스들의 자원을
   리셋한다.
*/
IDE_RC smmTBSMediaRecovery::resetMediaFailureMemTBSNodes()
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  doActResetMediaFailureTBSNode,
                                                  NULL, /* Action Argument*/
                                                  SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
