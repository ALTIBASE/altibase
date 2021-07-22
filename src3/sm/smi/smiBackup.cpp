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
 * $Id: smiBackup.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

/**********************************************************************
 * FILE DESCRIPTION : smiBackup.cpp
 *--------------------------------------------------------------------- 
 *  backup기능을 담당하는 모듈이며 다음과 같은 기능을 제공한다.
 *
 * -  table space에 대한 hot backup기능.
 *  : disk table space.
 *  : memory table space.
 * -  log anchor파일 backup기능.
 * -  DB전체에 대한  backup에  기능.
 **********************************************************************/

#include <ide.h>
#include <smErrorCode.h>
#include <smr.h>
#include <smiBackup.h>
#include <smiTrans.h>
#include <smiMain.h>
#include <sctTableSpaceMgr.h>
#include <smriChangeTrackingMgr.h>
#include <smriBackupInfoMgr.h>

/*********************************************************
 * function description: archivelog mode 변경
 * archivelog 모드를 활성화 또는 비활성화 시킵니다.
 * alter database 구문으로 startup control 단계에서만 사용이 가능
 * 합니다.
 *
 * sql - alter database [archivelog|noarchivelog]
 * 
 * + archivelog 모드의 활성화
 *  - 데이타베이스를 종료합니다.
 *    : shutdown abort
 *  - startup control 상태로 만듭니다.
 *  - alter database [archivelog|noarchivelog] 수행합니다.
 *  - 데이타베이스를 종료합니다.
 *    : shutdown abort
 *  - 데이타베이스의 전체 닫힌 백업을 수행합니다.
 *    : 비활성화 => 활성화 변경될 경우 모든 데이타파일을 백업해야한다.
 *    데이타베이스가 noarchive 모드에서 운영되는 동안의
 *    모든 백업은 논리적으로 더이상 사용할 수 없다.
 *    archivelog 모드로 바꾼후에 취해진 새로운 백업은 앞으로의
 *    모든 archive redo 로그 파일이 적용될 것에 대비한 백업이다.
 * 
 * + archivelog 모드의 비활성화
 *  - 데이타베이스를 종료합니다.
 *    : shutdown abort
 *  - startup control 상태로 만듭니다.
 *  - alter database [archivelog|noarchivelog] 수행합니다.
 *  - 데이타베이스를 종료합니다.
 *    : shutdown abort
 *********************************************************/
IDE_RC smiBackup::alterArchiveMode( smiArchiveMode  aArchiveMode,
                                    idBool          aCheckPhase )
{

    IDE_DASSERT((aArchiveMode == SMI_LOG_ARCHIVE) ||
                (aArchiveMode == SMI_LOG_NOARCHIVE));

    if (aCheckPhase == ID_TRUE)
    {
        // 다단계 startup단계 중, control단계에서
        // archive mode를 변경할수 있다.
        IDE_TEST_RAISE(smiGetStartupPhase() != SMI_STARTUP_CONTROL,
                       err_cannot_alter_archive_mode);
        
    }
    
    IDE_TEST( smrRecoveryMgr::updateArchiveMode(aArchiveMode) 
            != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_cannot_alter_archive_mode);
    {        
        IDE_SET(ideSetErrorCode(smERR_ABORT_MediaRecoDataFile,
                           "ALTER DATABASE ARCHIVELOG or NOARCHIVELOG"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
 * function description: alter system archive log start 수행
 * startup open meta 상태 이후에서만 archivelog thread를
 * 활성화시킬수 있다.
 *
 * + archivelog thread의 활성화
 *   - 데이타베이스를 startup meta 상태인지 확인한다.
 *   - 데이타베이스가 archivelog 모드로 활성화 되어 있는지
 *     확인한다. (archive log list)
 *   - alter system archivelog start를 수행하여 thread를
 *     수행시킨다.
 *********************************************************/
IDE_RC smiBackup::startArchThread()
{

    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_archivelog_mode);
    
    if ( smrLogMgr::getArchiveThread().isStarted() == ID_FALSE )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_INTERFACE,
                    SM_TRC_INTERFACE_ARCHIVE_THREAD_START);
        IDE_TEST_RAISE( smrLogMgr::startupLogArchiveThread() != IDE_SUCCESS,
                       err_cannot_start_thread);
        ideLog::log(SM_TRC_LOG_LEVEL_INTERFACE,
                    SM_TRC_INTERFACE_ARCHIVE_THREAD_SUCCESS);
    }
    else
    {
        /* nothing to do ... */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_cannot_start_thread );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CanStartARCH));
    }
    IDE_EXCEPTION_END;
    
    ideLog::log(SM_TRC_LOG_LEVEL_INTERFACE,
                SM_TRC_INTERFACE_ARCHIVE_THREAD_FAILURE);

    return IDE_FAILURE;
}

/*********************************************************
 * function description: alter system archive log stop 수행
 * startup open meta 상태 이후에서만 archivelog thread를
 * 활성화시킬수 있다.
 *
 * + archivelog thread의 활성화
 *   - 데이타베이스를 startup meta 상태인지 확인한다.
 *   - 데이타베이스가 archivelog 모드로 활성화 되어 있는지
 *     확인한다. (archive log list)
 *   - alter system archivelog stop을 수행하여 thread를
 *     중지시킨다.
 *********************************************************/
IDE_RC smiBackup::stopArchThread()
{

    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode()
                   != SMI_LOG_ARCHIVE,
                   err_archivelog_mode);
    
    if ( smrLogMgr::getArchiveThread().isStarted() == ID_TRUE )
    {
        
        ideLog::log(SM_TRC_LOG_LEVEL_INTERFACE,
                    SM_TRC_INTERFACE_ARCHIVE_THREAD_SHUTDOWN);
        IDE_TEST( smrLogMgr::shutdownLogArchiveThread() != IDE_SUCCESS );
        ideLog::log(SM_TRC_LOG_LEVEL_INTERFACE,
                    SM_TRC_INTERFACE_ARCHIVE_THREAD_SUCCESS);
    }
    else
    {
        /* nothing to do ... */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION_END;
    
    ideLog::log(SM_TRC_LOG_LEVEL_INTERFACE,
                SM_TRC_INTERFACE_ARCHIVE_THREAD_FAILURE);
    
    return IDE_FAILURE;
}


/*********************************************************
  function description: smiBackup::backupLogAnchor
  - logAnchor파일을 destnation directroy에 copy한다.
  *********************************************************/
IDE_RC smiBackup::backupLogAnchor( idvSQL*   aStatistics,
                                   SChar*    aDestFilePath )
{

    IDE_DASSERT( aDestFilePath != NULL );
    
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
        
    // logAnchor파일을 destnation directroy에 copy한다.
    IDE_TEST( smrBackupMgr::backupLogAnchor( aStatistics,
                                             aDestFilePath )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,"ALTER DATABASE BACKUP LOGANCHOR"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*********************************************************
 * function description: smiBackup::backupTableSpace
 * 인자로 주어진 테이블 스페이스에 대하여 hot backup
 * 을 수행하며, 주어진  테이블 스페이스의 데이타 파일들을 복사한다.       
 **********************************************************/
IDE_RC smiBackup::backupTableSpace(idvSQL*      aStatistics,
                                   smiTrans*    aTrans,
                                   scSpaceID    aTbsID,
                                   SChar*       aBackupDir)
{

    IDE_DASSERT(aTrans != NULL);
    IDE_DASSERT(aBackupDir != NULL);
    
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
    
    // BUG-17213 Volatile TBS에 대해서는 backup 기능을 막는다.
    IDE_TEST_RAISE(sctTableSpaceMgr::isVolatileTableSpace(aTbsID)
                   == ID_TRUE, err_volatile_backup);

    IDE_TEST(smrBackupMgr::backupTableSpace(aStatistics,
                                            aTrans->getTrans(),
                                            aTbsID,
                                            aBackupDir)
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,"ALTER DATABASE BACKUP TABLESPACE"));
    }
    IDE_EXCEPTION( err_volatile_backup );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UNABLE_TO_BACKUP_FOR_VOLATILE_TABLESPACE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*********************************************************
  function description: smiBackup::backupDatabase
  - Database에 있는 모든 테이블 스페이스에 대하여
    hot backup을 수행하여, 각 테이블 스페이스의
    데이타 파일을 복사한다.
    
  - log anchor파일들을 복사한다.
  
  - DB backup을 받는 동안  생긴 active log 파일들을
    archive log로 만들어  복사한다.
  *********************************************************/
IDE_RC smiBackup::backupDatabase( idvSQL* aStatistics,
                                  smiTrans* aTrans,
                                  SChar*    aBackupDir )
{

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aBackupDir != NULL );
    
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);

    IDE_TEST(smrBackupMgr::backupDatabase(aStatistics,
                                          aTrans->getTrans(),
                                          aBackupDir)
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,"ALTER DATABASE BACKUP DATABASE"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
  function description: smiBackup::beginBackupTBS
  - veritas와 같이 backup 연동을 위하여 추가된 함수이며,
   주어진 테이블 스페이스의 상태를 백업 시작(BACKUP_BEGIN)
   으로 변경한다.
  메모리 tablespace의 경우에는 checkpoint를 disable 시킨다. 
  *********************************************************/
IDE_RC smiBackup::beginBackupTBS( scSpaceID aSpaceID )
{

    
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
    
    IDE_TEST(smrBackupMgr::beginBackupTBS(aSpaceID)
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,
                              "ALTER TABLESPACE BEGIN BACKUP"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
  function description: smiBackup::endBackupTBS
  - veritas와 같이 backup 연동을 위하여 추가된 함수이며,
   주어진 테이블 스페이스의 상태를 백업 종료인 online상태로
   변경시킨다.
   
  메모리 tablespace의 경우에는 checkpoint를 enable 시킨다. 
  *********************************************************/
IDE_RC smiBackup::endBackupTBS( scSpaceID aSpaceID )
{

    
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
    
    IDE_TEST(smrBackupMgr::endBackupTBS(aSpaceID)
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,
                              "ALTER TABLESPACE END BACKUP"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
  function description: smiBackup::switchLogFile
  - 백업이 종료한 후에 현재 온라인 로그파일을 archive log로
   백업받도록 한다.
  *********************************************************/
IDE_RC smiBackup::switchLogFile()
{
    
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);
    
    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
    
    
    IDE_TEST( smrBackupMgr::swithLogFileByForces()
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,
                                "ALTER SYSTEM SWITCH LOGFILE"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
  function description: smiBackup::incrementalBackupDatabase
    PROJ-2133 incremental backup
  - Database에 있는 모든 테이블 스페이스에 대하여
    incrementalb backup을 수행한다.
  *********************************************************/
IDE_RC smiBackup::incrementalBackupDatabase( idvSQL*           aStatistics,
                                             smiTrans*         aTrans,
                                             smiBackupLevel    aBackupLevel,
                                             UShort            aBackupType,
                                             SChar*            aBackupDir,
                                             SChar*            aBackupTag )
{
    
    IDE_DASSERT( aTrans != NULL );

    IDE_DASSERT( ( aBackupLevel == SMI_BACKUP_LEVEL_0 ) ||
                 ( aBackupLevel == SMI_BACKUP_LEVEL_1 ) );

    IDE_DASSERT( (( aBackupType & SMI_BACKUP_TYPE_FULL ) != 0) ||
                 (( aBackupType & SMI_BACKUP_TYPE_DIFFERENTIAL ) != 0) ||
                 (( aBackupType & SMI_BACKUP_TYPE_CUMULATIVE ) != 0) );

    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);
    
    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
    
    IDE_TEST_RAISE(smrRecoveryMgr::isCTMgrEnabled() != ID_TRUE,
                   err_change_traking_state);
    
    IDE_TEST( smrBackupMgr::incrementalBackupDatabase( aStatistics,
                                                       aTrans->getTrans(),
                                                       aBackupDir,
                                                       aBackupLevel,
                                                       aBackupType,
                                                       aBackupTag )
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,
                                "ALTER DATABASE INCREMENTAL BACKUP DATABASE"));
    }
    IDE_EXCEPTION( err_change_traking_state );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ChangeTrackingState));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
  function description: smiBackup::incrementalBackupDatabase
    PROJ-2133 incremental backup
  - Database에 있는 모든 테이블 스페이스에 대하여
    incrementalb backup을 수행한다.
  *********************************************************/
IDE_RC smiBackup::incrementalBackupTableSpace( idvSQL*           aStatistics,
                                               smiTrans*         aTrans,
                                               scSpaceID         aSpaceID,
                                               smiBackupLevel    aBackupLevel,
                                               UShort            aBackupType,
                                               SChar*            aBackupDir,
                                               SChar*            aBackupTag,
                                               idBool            aCheckTagName )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( ( aBackupLevel == SMI_BACKUP_LEVEL_0 ) ||
                 ( aBackupLevel == SMI_BACKUP_LEVEL_1 ) );

    IDE_DASSERT( (( aBackupType & SMI_BACKUP_TYPE_FULL ) != 0) ||
                 (( aBackupType & SMI_BACKUP_TYPE_DIFFERENTIAL ) != 0) ||
                 (( aBackupType & SMI_BACKUP_TYPE_CUMULATIVE ) != 0) );

    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_META,
                   err_startup_phase);
    
    IDE_TEST_RAISE(smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                   err_backup_mode);
    
    IDE_TEST_RAISE(smrRecoveryMgr::isCTMgrEnabled() != ID_TRUE,
                   err_change_traking_state);
    
    IDE_TEST( smrBackupMgr::incrementalBackupTableSpace( aStatistics,
                                                         aTrans->getTrans(),
                                                         aSpaceID,
                                                         aBackupDir,
                                                         aBackupLevel,
                                                         aBackupType,
                                                         aBackupTag,
                                                         aCheckTagName )
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupLogMode,
                                "ALTER DATABASE INCREMENTAL BACKUP TABLESPACE"));
    }
    IDE_EXCEPTION( err_change_traking_state );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ChangeTrackingState));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*********************************************************
  function description: smiBackup::enableChangeTracking
    PROJ-2133 incremental backup
  - incremental backup을 수행하기 위해 데이터베이스에 
    속한 모든 데이터파일들의 변경 정보를 추적하는 기능을
    enable 시킨다.
  *********************************************************/
IDE_RC smiBackup::enableChangeTracking()
{
    UInt    sState = 0;

    IDE_TEST_RAISE( smiGetStartupPhase() < SMI_STARTUP_META,
                    err_startup_phase );

    IDE_TEST_RAISE( smrRecoveryMgr::getArchiveMode() != SMI_LOG_ARCHIVE,
                    err_backup_mode );
    
    IDE_TEST_RAISE( smrRecoveryMgr::isCTMgrEnabled() != ID_FALSE,
                    err_change_traking_state );

    IDE_TEST( smriChangeTrackingMgr::createCTFile() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smriBackupInfoMgr::createBIFile() !=IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( smriBackupInfoMgr::begin() != IDE_SUCCESS );
    sState = 3;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_WrongStartupPhase ) );
    }
    IDE_EXCEPTION( err_backup_mode );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_BackupLogMode,
                                 "ALTER DATABASE ENABLE INCREMENTAL CHUNK CHANGE TRACKING" ) );
    }
    IDE_EXCEPTION( err_change_traking_state );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_ChangeTrackingState ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3: 
            IDE_ASSERT( smriBackupInfoMgr::end() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( smriBackupInfoMgr::removeBIFile() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smriChangeTrackingMgr::removeCTFile() == IDE_SUCCESS );
        default:
            break;
    }
    return IDE_FAILURE;
}

/*********************************************************
  function description: smiBackup::disableChangeTracking
    PROJ-2133 incremental backup
  - incremental backup을 수행하기 위해 데이터베이스에 
    속한 모든 데이터파일들의 변경 정보를 추적하는 기능을 
    disable 시킨다.
  *********************************************************/
IDE_RC smiBackup::disableChangeTracking()
{
    IDE_TEST( smriChangeTrackingMgr::removeCTFile() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: smiBackup::removeObsoleteBackupFile
    PROJ-2133 incremental backup
  - 유지기간이 지난 incremental backupfile들을 삭제한다.
  *********************************************************/
IDE_RC smiBackup::removeObsoleteBackupFile()
{
    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_CONTROL,
                   err_startup_phase);

    IDE_TEST( smriBackupInfoMgr::removeObsoleteBISlots() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*********************************************************
  function description: smiBackup::changeBackupDir
    PROJ-2133 incremental backup
  - incremental backupfile들이 생성될 directory를 변경한다.
  *********************************************************/
IDE_RC smiBackup::changeIncrementalBackupDir( SChar * aNewBackupDir )
{
    SChar sPath[SM_MAX_FILE_NAME];

    IDE_DASSERT( aNewBackupDir != NULL );

    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_CONTROL,
                   err_startup_phase);

    IDE_TEST_RAISE(smrRecoveryMgr::isCTMgrEnabled() != ID_TRUE,
                   err_change_traking_state);

    if( smuProperty::getIncrementalBackupPathMakeABSPath() == 1 )
    {
        /* aNewBackupDir이 절대경로가 아닌지 확인한다. */
        IDE_TEST( smriBackupInfoMgr::isValidABSPath( ID_TRUE, aNewBackupDir ) 
                  != IDE_FAILURE );

        idlOS::snprintf(sPath, SM_MAX_FILE_NAME,
                        "%s%c%s",                      
                        smuProperty::getDefaultDiskDBDir(),
                        IDL_FILE_SEPARATOR,            
                        aNewBackupDir);
    }
    else
    {
        idlOS::snprintf(sPath, SM_MAX_FILE_NAME,
                        "%s",
                        aNewBackupDir);
    }

    /* aNewBackupDir이 valid한 절대경로인지 확인한다. */
    IDE_TEST( smriBackupInfoMgr::isValidABSPath( ID_TRUE, sPath ) 
              != IDE_SUCCESS );

    IDE_TEST( smrRecoveryMgr::changeIncrementalBackupDir( sPath ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION( err_change_traking_state );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ChangeTrackingState));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*********************************************************
  function description: smiBackup::moveBackupFile
    PROJ-2133 incremental backup
  - incremental backupfile들의 위치를 이동시킨다.
  *********************************************************/
IDE_RC smiBackup::moveBackupFile( SChar  * aMoveDir, 
                                  idBool   aWithFile )
{
    SChar sPath[SM_MAX_FILE_NAME];

    IDE_DASSERT( aMoveDir != NULL );

    IDE_TEST_RAISE(smiGetStartupPhase() < SMI_STARTUP_CONTROL,
                   err_startup_phase);

    if( smuProperty::getIncrementalBackupPathMakeABSPath() == 1 )
    {
        /* aNewBackupDir이 절대경로가 아닌지 확인한다. */
        IDE_TEST( smriBackupInfoMgr::isValidABSPath( ID_TRUE, aMoveDir ) 
                  != IDE_FAILURE );

        idlOS::snprintf(sPath, SM_MAX_FILE_NAME,
                        "%s%c%s",                      
                        smuProperty::getDefaultDiskDBDir(),
                        IDL_FILE_SEPARATOR,            
                        aMoveDir);
    }
    else
    {
        idlOS::snprintf(sPath, SM_MAX_FILE_NAME,
                        "%s",
                        aMoveDir);
    }

    /* aMoveDir가 valid한 절대경로인지 확인한다. */
    IDE_TEST( smriBackupInfoMgr::isValidABSPath( ID_TRUE, sPath ) 
              != IDE_SUCCESS );

    IDE_TEST( smriBackupInfoMgr::moveIncrementalBackupFiles( 
                                                     sPath, 
                                                     aWithFile )
              != IDE_SUCCESS )

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*********************************************************
  function description: smiBackup::removeBackupInfoFile
    PROJ-2133 incremental backup
  - backupinfo파일을 삭제하고 backupinfo manager를 disable시킨다.
    본 함수를 호출하면 기존에 백업된 모든 incremental backup파일들을 
    사용할수 없게된다.
    backup info파일의 오류로 DB startup이 되지 못할때
    에만 사용해야한다.
  *********************************************************/
IDE_RC smiBackup::removeBackupInfoFile()
{
    IDE_TEST_RAISE(smiGetStartupPhase() > SMI_STARTUP_PROCESS,
                   err_startup_phase);

    IDE_TEST( smriBackupInfoMgr::removeBIFile() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_startup_phase );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongStartupPhase));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
