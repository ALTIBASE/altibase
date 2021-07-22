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
 * $Id: smrBackupMgr.cpp 86343 2019-11-11 01:28:10Z jiwon.kim $
 *
 * Description :
 *
 * 본 파일은 백업 관리자에 대한 구현파일이다.
 *
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smr.h>
#include <smrReq.h>
#include <sdd.h>

//For Read Online Backup Status
iduMutex           smrBackupMgr::mMtxOnlineBackupStatus;
SChar              smrBackupMgr::mOnlineBackupStatus[256];
smrBackupState     smrBackupMgr::mOnlineBackupState;

// PRJ-1548 User Memory Tablespace
UInt               smrBackupMgr::mBeginBackupDiskTBSCount;
UInt               smrBackupMgr::mBeginBackupMemTBSCount;

// PROJ-2133 Incremental Backup
UInt               smrBackupMgr::mBackupBISlotCnt;
SChar              smrBackupMgr::mLastBackupTagName[ SMI_MAX_BACKUP_TAG_NAME_LEN ];


/***********************************************************************
 * Description : 백업관리자 초기화
 **********************************************************************/
IDE_RC smrBackupMgr::initialize()
{
    IDE_TEST( mMtxOnlineBackupStatus.initialize(
                        (SChar*)"BACKUP_MANAGER_MUTEX",
                        IDU_MUTEX_KIND_NATIVE,
                        IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    idlOS::strcpy(mOnlineBackupStatus, "");

    mOnlineBackupState = SMR_BACKUP_NONE;

   // BACKUP 진행중인 디스크 테이블스페이스의 개수
    mBeginBackupDiskTBSCount = 0;
    mBeginBackupMemTBSCount  = 0;

    // incremental backup을 수행하기전 backupinfo파일의 slot 개수를 저장
    mBackupBISlotCnt = SMRI_BI_INVALID_SLOT_CNT;
    idlOS::memset( mLastBackupTagName, 0x00, SMI_MAX_BACKUP_TAG_NAME_LEN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 백업 관리자 해제
 **********************************************************************/
IDE_RC smrBackupMgr::destroy()
{
    IDE_TEST(mMtxOnlineBackupStatus.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   ONLINE BACKUP 플래그에 설정하고자 하는 상태값을 설정한다.

   [IN] aOR : 설정하고자하는 상태값
*/
void smrBackupMgr::setOnlineBackupStatusOR( UInt  aOR )
{
    IDE_ASSERT( gSmrChkptThread.lock( NULL /* idvSQL* */)
                == IDE_SUCCESS );

    mOnlineBackupState |=  (aOR);

    IDE_ASSERT( gSmrChkptThread.unlock() == IDE_SUCCESS );

    return;
}


/*
   ONLINE BACKUP 플래그에 해제하고자 하는 상태값을 해재한다.

   [IN] aNOT : 해제하고자하는 상태값
*/
void smrBackupMgr::setOnlineBackupStatusNOT( UInt aNOT )
{
    IDE_ASSERT( gSmrChkptThread.lock( NULL /* idvSQL* */)
                == IDE_SUCCESS );

    mOnlineBackupState &= ~(aNOT);

    IDE_ASSERT( gSmrChkptThread.unlock() == IDE_SUCCESS );

    return;
}

/***********************************************************************
 * Description : 해당 path에서 memory db 관련 파일 제거
 * destroydb, onlineBackup등에서 호출된다.
 **********************************************************************/
IDE_RC smrBackupMgr::unlinkChkptImages( SChar* aPathName,
                                        SChar* aTBSName )
{
    SInt               sRc;
    DIR               *sDIR     = NULL;
    struct  dirent    *sDirEnt  = NULL;
    struct  dirent    *sResDirEnt;
    SInt               sOffset;
    SChar              sFileName[SM_MAX_FILE_NAME];
    SChar              sFullFileName[SM_MAX_FILE_NAME];

    //To fix BUG-4980
    SChar              sPrefix[SM_MAX_FILE_NAME];
    UInt               sLen;

    idlOS::snprintf(sPrefix, SM_MAX_FILE_NAME, "%s", "ALTIBASE_INDEX_");
    sLen = idlOS::strlen(sPrefix);

    /* smrBackupMgr_unlinkChkptImages_malloc_DirEnt.tc */
    IDU_FIT_POINT_RAISE( "smrBackupMgr::unlinkChkptImages::malloc::DirEnt",
                         insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                       ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                       (void**)&sDirEnt,
                                       IDU_MEM_FORCE ) != IDE_SUCCESS,
                    insufficient_memory );

    /* smrBackupMgr_unlinkChkptImages_opendir.tc */
    IDU_FIT_POINT_RAISE( "smrBackupMgr::unlinkChkptImages::opendir", err_open_dir );
    sDIR = idf::opendir(aPathName);
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);

    /* For each file in Backup Path, unlink */
    sResDirEnt = NULL;
    errno = 0;

    /* smrBackupMgr_unlinkChkptImages_readdir_r.tc */
    IDU_FIT_POINT_RAISE( "smrBackupMgr::unlinkChkptImages::readdir_r", err_read_dir );
    sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
    IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
    errno = 0;

    while(sResDirEnt != NULL)
    {
        idlOS::strcpy(sFileName, sResDirEnt->d_name);

        if (idlOS::strcmp(sFileName, ".") == 0 ||
            idlOS::strcmp(sFileName, "..") == 0)
        {
            sResDirEnt = NULL;
            errno = 0;
            sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
            errno = 0;
            continue;
        }

        /* ----------------------------------------------------------------
           [1] unlink backup_version file, loganchor file, db file, log file
           ---------------------------------------------------------------- */
        if (idlOS::strcmp(sFileName, SMR_BACKUP_VER_NAME) == 0 ||
            idlOS::strncmp(sFileName, aTBSName, idlOS::strlen(aTBSName)) == 0 )
        {
            idlOS::snprintf(sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                           aPathName, IDL_FILE_SEPARATOR, sFileName);
            IDE_TEST_RAISE(idf::unlink(sFullFileName) != 0,
                           err_file_delete);

            // To Fix BUG-13924
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECORD_FILE_UNLINK,
                        sFullFileName);

            sResDirEnt = NULL;
            errno = 0;
            sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
            errno = 0;

            continue;
        }

        // [2] unlink internal database file
        sOffset = (SInt)(idlOS::strlen(sFileName) - 4);
        if(sOffset > 4)
        {
            if(idlOS::strncmp(sFileName + sOffset,
                              SM_TABLE_BACKUP_EXT, 4) == 0)
            {
                idlOS::snprintf(sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                               aPathName, IDL_FILE_SEPARATOR, sFileName);
                IDE_TEST_RAISE(idf::unlink(sFullFileName) != 0,
                               err_file_delete);

                // To Fix BUG-13924
                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                            SM_TRC_MRECORD_FILE_UNLINK,
                            sFullFileName);
            }
        }

        // To fix BUG-4980
        // [3] unlink index file
        if(idlOS::strlen(sFileName) >= sLen)
        {
            if(idlOS::strncmp(sFileName, sPrefix, sLen) == 0)
            {
                idlOS::snprintf(sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                                aPathName, IDL_FILE_SEPARATOR, sFileName);
                IDE_TEST_RAISE(idf::unlink(sFullFileName) != 0,
                               err_file_delete);

                // To Fix BUG-13924
                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                            SM_TRC_MRECORD_FILE_UNLINK,
                            sFullFileName);
            }
        }

        sResDirEnt = NULL;
        errno = 0;
        sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }

    idf::closedir(sDIR);
    sDIR = NULL;

    IDE_TEST( iduMemMgr::free( sDirEnt ) != IDE_SUCCESS );
    sDirEnt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, aPathName ) );
    }
    IDE_EXCEPTION(err_file_delete);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, sFullFileName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sDIR != NULL )
        {
            idf::closedir( sDIR );
            sDIR = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sDirEnt != NULL )
        {
            IDE_ASSERT( iduMemMgr::free( sDirEnt ) == IDE_SUCCESS );
            sDirEnt = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 해당 path에서 disk db 관련 파일 제거
 * destroydb, onlineBackup등에서 호출된다.
 **********************************************************************/
IDE_RC smrBackupMgr::unlinkDataFile( SChar*  aDataFileName )
{
    IDE_TEST_RAISE( idf::unlink(aDataFileName) != 0, err_file_delete );
    //=====================================================================
    // To Fix BUG-13924
    //=====================================================================

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECORD_FILE_UNLINK,
                 aDataFileName );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_delete);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, aDataFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 해당 path에서 memory db 관련 파일 제거
 * destroydb, onlineBackup등에서 호출된다.
 **********************************************************************/
IDE_RC smrBackupMgr::unlinkAllLogFiles( SChar* aPathName )
{

    SInt               sRc;
    DIR               *sDIR     = NULL;
    struct  dirent    *sDirEnt  = NULL;
    struct  dirent    *sResDirEnt;
    SChar              sFileName[SM_MAX_FILE_NAME];
    SChar              sFullFileName[SM_MAX_FILE_NAME];

    IDE_DASSERT( aPathName != NULL );

    /* smrBackupMgr_unlinkAllLogFiles_malloc_DirEnt.tc */
    IDU_FIT_POINT_RAISE( "smrBackupMgr::unlinkAllLogFiles::malloc::DirEnt",
                         insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                       ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                       (void**)&sDirEnt,
                                       IDU_MEM_FORCE ) != IDE_SUCCESS,
                    insufficient_memory );

    /* smrBackupMgr_unlinkAllLogFiles_opendir.tc */
    IDU_FIT_POINT_RAISE( "smrBackupMgr::unlinkAllLogFiles::opendir", err_open_dir );
    sDIR = idf::opendir(aPathName);
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);

    /* For each file in Backup Path, unlink */
    sResDirEnt = NULL;
    errno = 0;

    /* smrBackupMgr_unlinkAllLogFiles_readdir_r.tc */
    IDU_FIT_POINT_RAISE( "smrBackupMgr::unlinkAllLogFiles::readdir_r", err_read_dir );
    sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
    IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
    errno = 0;

    while(sResDirEnt != NULL)
    {
        idlOS::strcpy(sFileName, sResDirEnt->d_name);

        if (idlOS::strcmp(sFileName, ".") == 0 ||
            idlOS::strcmp(sFileName, "..") == 0)
        {
            sResDirEnt = NULL;
            errno = 0;
            sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
            errno = 0;
            continue;
        }

        // [1] unlink backup_version file, loganchor file, db file, log file
        if (idlOS::strncmp(sFileName,
                    SMR_LOGANCHOR_NAME,
                    idlOS::strlen(SMR_LOGANCHOR_NAME)) == 0 ||
            idlOS::strncmp(sFileName,
                    SMR_LOG_FILE_NAME,
                    idlOS::strlen(SMR_LOG_FILE_NAME)) == 0 )
        {
            idlOS::snprintf(sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                           aPathName, IDL_FILE_SEPARATOR, sFileName);

            IDE_TEST_RAISE(idf::unlink(sFullFileName) != 0,
                           err_file_delete);

            // To Fix BUG-13924
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECORD_FILE_UNLINK,
                        sFullFileName);

            sResDirEnt = NULL;
            errno = 0;
            sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
            errno = 0;

            continue;
        }

        sResDirEnt = NULL;
        errno = 0;
        sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }

    idf::closedir( sDIR );
    sDIR = NULL;

    IDE_TEST( iduMemMgr::free( sDirEnt ) != IDE_SUCCESS );
    sDirEnt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, aPathName ) );
    }
    IDE_EXCEPTION(err_file_delete);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, sFullFileName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sDIR != NULL )
        {
            idf::closedir( sDIR );
            sDIR = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sDirEnt != NULL )
        {
            IDE_ASSERT( iduMemMgr::free( sDirEnt ) == IDE_SUCCESS );
            sDirEnt = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : PROJ-2133 incremental backup 
 *               해당 path에서 change tracking파일 제거
 *               destroydb 에서 호출된다.
 **********************************************************************/
IDE_RC smrBackupMgr::unlinkChangeTrackingFile( SChar * aChangeTrackingFileName )
{
    IDE_TEST_RAISE( idf::unlink(aChangeTrackingFileName) != 0, err_file_delete );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECORD_FILE_UNLINK,
                 aChangeTrackingFileName );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_delete);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, 
                                aChangeTrackingFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : PROJ-2133 incremental backup
 *               해당 path에서 backup info파일 제거
 *               destroydb 에서 호출된다.
 **********************************************************************/
IDE_RC smrBackupMgr::unlinkBackupInfoFile( SChar * aBackupInfoFileName )
{
    IDE_TEST_RAISE( idf::unlink(aBackupInfoFileName) != 0, err_file_delete );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECORD_FILE_UNLINK,
                 aBackupInfoFileName );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_delete);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, 
                                aBackupInfoFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * Description:
 * LogAnchor파일을 destnation directroy에 copy한다.
 * A4, PRJ-1149관련 log anchor,  tablespace, database backup API
 *********************************************************/
IDE_RC smrBackupMgr::backupLogAnchor( idvSQL* aStatistics,
                                      SChar * aDestFilePath )
{
    smrLogAnchorMgr*    sAnchorMgr;
    SInt                sSystemErrno;
    UInt                sDestFilePathLen;

    IDE_DASSERT( aDestFilePath != NULL );

    sDestFilePathLen = idlOS::strlen(aDestFilePath);
    IDE_TEST_RAISE( sDestFilePathLen == 0, error_backupdir_path_null);
    // 백업 경로에 loganchor full name 을 더한 길이
    IDE_TEST_RAISE( (sDestFilePathLen+idlOS::strlen(SMR_LOGANCHOR_NAME)+1+1) > SM_MAX_FILE_NAME , error_backupdir_path_too_long );

    //log anchor 관리자 획득
    sAnchorMgr = smrRecoveryMgr::getLogAnchorMgr();

    errno = 0;
    if ( sAnchorMgr->backup( aStatistics,
                             aDestFilePath ) != IDE_SUCCESS )
    {
        sSystemErrno = ideGetSystemErrno();
        
        if ( isRemainSpace(sSystemErrno) == ID_TRUE )
        {
            IDE_RAISE( err_logAnchor_backup_write );

        } 
        else
        {
            IDE_RAISE( err_online_backup_write );
        }
    }

    //BUG-47538: 로그앵커 백업 실패 메시지 테스트
    IDU_FIT_POINT_RAISE( "BUG-47538@smrBackupMgr::backupLogAnchor", err_online_backup_write );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_backupdir_path_null );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_PathIsNullString) );
    }
    IDE_EXCEPTION( error_backupdir_path_too_long );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_TooLongFilePath, aDestFilePath,SMR_LOGANCHOR_NAME) );
    }
    IDE_EXCEPTION( err_online_backup_write );
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT );
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupWrite));
    }
    IDE_EXCEPTION( err_logAnchor_backup_write );
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT );
        //log anchor에서 로그 파일연산관련 error코드를 settin하기 때문에 skip.
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * Description:
 * - 테이블 스페이스 media 종류에 따른 backupTableSpace를
 *  분기시킨다.
 * -> memory table space,
 * -> disk tables space.
 *********************************************************/
IDE_RC smrBackupMgr::backupTableSpace( idvSQL*   aStatistics,
                                       void*     aTrans,
                                       scSpaceID aSpaceID,
                                       SChar*    aBackupDir )
{
    UInt     sState = 0;
    UInt     sRestoreState = 0;
    idBool   sLocked;
    sctTableSpaceNode *sSpaceNode = NULL;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aBackupDir != NULL );

    IDE_TEST( mMtxOnlineBackupStatus.trylock(sLocked) != IDE_SUCCESS );

    IDE_TEST_RAISE( sLocked == ID_FALSE, error_backup_going);
    sState = 1;

    // alter database backup tablespace or database뿐만아니라,
    // alter tablespace begin backup 경로로 올수 있기 때문에,
    // 다음과 같이 검사한다.
    IDE_TEST_RAISE( mOnlineBackupState != SMR_BACKUP_NONE,
                    error_backup_going );

    // Aging 수행과정에서 현 트랜잭션을 고려하지 않고 버전수집이
    // 가능하게 한다. (참고) smxTransMgr::getMemoryMinSCN()
    smLayerCallback::updateSkipCheckSCN( aTrans, ID_TRUE );
    sState = 2;

    // 체크포인트를 발생시킨다.
    IDE_TEST( gSmrChkptThread.resumeAndWait( aStatistics )
              != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_START);

    sSpaceNode = sctTableSpaceMgr::getSpaceNodeBySpaceID( aSpaceID );

    IDE_TEST_RAISE( sSpaceNode == NULL ,
                    error_not_found_tablespace_node );

    IDE_TEST_RAISE( ( (sSpaceNode->mState & SMI_TBS_DROPPED)   == SMI_TBS_DROPPED ) ||
                    ( (sSpaceNode->mState & SMI_TBS_DISCARDED) == SMI_TBS_DISCARDED ),
                    error_not_found_tablespace_node );

    IDE_TEST( sctTableSpaceMgr::startTableSpaceBackup( aStatistics,
                                                       sSpaceNode )
              != IDE_SUCCESS );
    sState = 3;

    // Backup Flag 세팅 후 한번 더 확인함.
    // Discard는 서비스 중에 되지 않으니 Drop만 다시 확인하면 됨
    IDE_TEST_RAISE(( sSpaceNode->mState & SMI_TBS_DROPPED ) == SMI_TBS_DROPPED,
                    error_not_found_tablespace_node );

    switch( sctTableSpaceMgr::getTBSLocation( sSpaceNode ) )
    {
        case SMI_TBS_MEMORY:
            {
                setOnlineBackupStatusOR( SMR_BACKUP_MEMTBS );
                sRestoreState = 2;

                IDE_TEST( backupMemoryTBS( aStatistics,
                                           (smmTBSNode*)sSpaceNode,
                                           aBackupDir )
                          != IDE_SUCCESS );

                setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
                sRestoreState = 0;
            }
            break;
        case SMI_TBS_DISK:
            {
                setOnlineBackupStatusOR( SMR_BACKUP_DISKTBS );
                sRestoreState = 1;

                IDE_TEST( backupDiskTBS( aStatistics,
                                         (sddTableSpaceNode*)sSpaceNode,
                                         aBackupDir )
                          != IDE_SUCCESS );

                setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
                sRestoreState = 0;
            }
            break;
        case SMI_TBS_VOLATILE:
            {
                // Nothing to do...
                // volatile tablespace에 대해서는 백업을 지원하지 않는다.
            }
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    sState = 2;
    IDE_TEST( sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                                     sSpaceNode ) != IDE_SUCCESS );

    // 강제로 해당 로그파일을 switch 하여 archive 시킨다.
    IDE_TEST( smrLogMgr::switchLogFileByForce() != IDE_SUCCESS );
    sState = 1;

    smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_END );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                  aSpaceID) );
    }
    IDE_EXCEPTION(error_backup_going);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BACKUP_GOING));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sRestoreState )
        {
            case 2:
                setOnlineBackupStatusNOT ( SMR_BACKUP_MEMTBS );
                break;
            case 1:
                setOnlineBackupStatusNOT ( SMR_BACKUP_DISKTBS );
                break;
            default:
                break;
        }

        switch( sState )
        {
            case 3:
                sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                                       sSpaceNode ) ;
            case 2:
                smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );
            case 1:
                IDE_ASSERT( unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }

        ideLog::log(SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_FAIL);
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************
 * Description: smrBackup::backupMemoryTBS
 * memory table space의 데이타 파일을 backup시킨다.
 *  1. check point를 disable 시킨다.
 *  2. memory database file들을 copy한다.
 *  3. alter table등으로 발생한  Internal backup database File copy.
 *  4. check point를 재개한다.
 **********************************************************/
IDE_RC smrBackupMgr::backupMemoryTBS( idvSQL*      aStatistics,
                                      smmTBSNode * aSpaceNode,
                                      SChar*       aBackupDir )
{

    UInt                 i;
    UInt                 sWhichDB;
    UInt                 sDataFileNameLen;
    UInt                 sBackupPathLen;
    SInt                 sSystemErrno;
    smmDatabaseFile    * sDatabaseFile;
    SChar                sStrFullFileName[ SMI_MAX_CHKPT_PATH_NAME_LEN ];

    IDE_DASSERT( aBackupDir != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE );

    // [2] memory database file들을 copy한다.
    sWhichDB = smmManager::getCurrentDB( (smmTBSNode*)aSpaceNode );
    sBackupPathLen = idlOS::strlen( aBackupDir );

    /* BUG-34357
     * [sBackupPathLen - 1] causes segmentation fault
     * */
    IDE_TEST_RAISE( sBackupPathLen == 0, error_backupdir_path_null );
    //                                                   SYS_TBS_MEM_DATA -0-0
    IDE_TEST_RAISE( (sBackupPathLen+idlOS::strlen(aSpaceNode->mHeader.mName)+4+1) > SMI_MAX_CHKPT_PATH_NAME_LEN, error_backupdir_path_too_long );
    

    for ( i = 0; i <= aSpaceNode->mLstCreatedDBFile; i++ )
    {
        idlOS::memset(sStrFullFileName, 0x00, SMI_MAX_CHKPT_PATH_NAME_LEN);

        /* ------------------------------------------------
         * BUG-11206와 같이 source와 destination이 같아서
         * 원본 데이타 파일이 유실되는 경우를 막기위하여
         * 다음과 같이 정확히 path를 구성함.
         * ----------------------------------------------*/
        if( aBackupDir[sBackupPathLen - 1] == IDL_FILE_SEPARATOR )
        {
            idlOS::snprintf(sStrFullFileName, SMI_MAX_CHKPT_PATH_NAME_LEN,
                            "%s%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT,
                            aBackupDir,
                            aSpaceNode->mHeader.mName,
                            sWhichDB, 
                            i);
        }
        else
        {
            idlOS::snprintf(sStrFullFileName, SMI_MAX_CHKPT_PATH_NAME_LEN,
                            "%s%c%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT,
                            aBackupDir,
                            IDL_FILE_SEPARATOR,
                            aSpaceNode->mHeader.mName,
                            sWhichDB,
                            i);
        }

        sDataFileNameLen = idlOS::strlen(sStrFullFileName);

        IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                              ID_TRUE,
                                              sStrFullFileName,
                                              &sDataFileNameLen,
                                              SMI_TBS_DISK)
                  != IDE_SUCCESS );

        IDE_TEST( smmManager::openAndGetDBFile( aSpaceNode,
                                                sWhichDB,
                                                i,
                                                &sDatabaseFile )
                  != IDE_SUCCESS );

        errno = 0;
        IDE_TEST_RAISE( idlOS::strcmp(sStrFullFileName,
                                      sDatabaseFile->getFileName())
                        == 0, error_self_copy);

        /* BUG-18678: Memory/Disk DB File을 /tmp에 Online Backup할때 Disk쪽
           Backup이 실패합니다.*/
        if ( sDatabaseFile->copy( aStatistics,
                                  sStrFullFileName )
             != IDE_SUCCESS )
        {
            sSystemErrno = ideGetSystemErrno();

            IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                           error_backup_datafile);

            IDE_RAISE(err_online_backup_write);
        }

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_BACKUP_INFO1,
                    sDatabaseFile->getFileName(),
                    sStrFullFileName);

        IDE_TEST_RAISE(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS,
                       error_abort_backup);
    }

    /* ------------------------------------------------
     * [2] alter table등으로 발생한 Backup Internal Database File
     * ----------------------------------------------*/
    errno = 0;
    if ( smLayerCallback::copyAllTableBackup( smLayerCallback::getBackupDir(),
                                              aBackupDir )
         != IDE_SUCCESS )
    {
        sSystemErrno = ideGetSystemErrno();
        IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                       err_backup_internalbackup_file);

        IDE_RAISE(err_online_backup_write);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_self_copy);
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_SelfCopy) );
    }
    IDE_EXCEPTION(err_online_backup_write);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_ABORT1 );

        IDE_SET( ideSetErrorCode(smERR_ABORT_BackupWrite) );
    }
    IDE_EXCEPTION(error_backup_datafile);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT2,
                     sStrFullFileName );

        IDE_SET( ideSetErrorCode( smERR_ABORT_BackupDatafile,
                                  "MEMORY TABLESPACE",
                                  sStrFullFileName ) );
    }
    IDE_EXCEPTION(err_backup_internalbackup_file);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT3,
                     sStrFullFileName );
        IDE_SET( ideSetErrorCode( smERR_ABORT_BackupDatafile,
                                 "BACKUPED INTERNAL FILE",
                                 "" ) );
    }
    IDE_EXCEPTION(error_abort_backup);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_ABORT4);
    }
    IDE_EXCEPTION(error_backupdir_path_null);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_PathIsNullString));
    }
    IDE_EXCEPTION( error_backupdir_path_too_long );
    {
        idlOS::snprintf(sStrFullFileName, SMI_MAX_CHKPT_PATH_NAME_LEN,
                        "%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT,
                        aSpaceNode->mHeader.mName,
                        sWhichDB, 
                        0);
        IDE_SET( ideSetErrorCode(smERR_ABORT_TooLongFilePath, aBackupDir, sStrFullFileName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * Description:
 * disk table space의 데이타 파일을 backup시킨다.
 **********************************************************/
IDE_RC smrBackupMgr::backupDiskTBS( idvSQL            * aStatistics,
                                    sddTableSpaceNode * aSpaceNode,
                                    SChar             * aBackupDir )
{
    SInt                  sSystemErrno;
    UInt                  sState = 0;
    UInt                  sDataFileNameLen;
    UInt                  sBackupPathLen;
    sddDataFileNode     * sDataFileNode = NULL;
    SChar               * sDataFileName;
    SChar                 sStrDestFileName[ SM_MAX_FILE_NAME ];
    UInt                  i;

    IDE_DASSERT( aBackupDir != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );

    if ( ( (aSpaceNode->mHeader.mState & SMI_TBS_DROPPED)   != SMI_TBS_DROPPED ) &&
         ( (aSpaceNode->mHeader.mState & SMI_TBS_DISCARDED) != SMI_TBS_DISCARDED ) )
    {
        /* ------------------------------------------------
         * 테이블 스페이스  데이타 파일들  copy.
         * ----------------------------------------------*/
        for (i=0; i < aSpaceNode->mNewFileID ; i++ )
        {
            sDataFileNode = aSpaceNode->mFileNodeArr[i] ;

            if( sDataFileNode == NULL)
            {
                continue;
            }

            if( SMI_FILE_STATE_IS_DROPPED( sDataFileNode->mState ) )
            {
                continue;
            }

            // XXX backup 중에 file의 creating, resize, dropping 이 가능한가?
            // 그런데 다음 파일로 넘겨버려도 되나?
            // 다음 파일로 넘기면 sleep은 왜하지
            /* Drop File을 확인해보니 TableSpace 로는 File이 unlink되어도
             * Drop File로는 File이 실제로 지워지지 않는다.
             * 여기를 통과 한다고 해도 상관은 없는 듯 하다.
             * 다만 rollback 시에 문제가 될 수 있다.
             * */
            if( SMI_FILE_STATE_IS_CREATING( sDataFileNode->mState ) ||
                SMI_FILE_STATE_IS_RESIZING( sDataFileNode->mState ) ||
                SMI_FILE_STATE_IS_DROPPING( sDataFileNode->mState ) )
            {
                idlOS::sleep(1);
                continue;
            }

            idlOS::memset(sStrDestFileName,0x00,SM_MAX_FILE_NAME);
            sDataFileName = idlOS::strrchr( sDataFileNode->mName,
                                            IDL_FILE_SEPARATOR );
            IDE_TEST_RAISE(sDataFileName == NULL,error_backupdir_file_path);

            /* ------------------------------------------------
             * BUG-11206와 같이 source와 destination이 같아서
             * 원본 데이타 파일이 유실되는 경우를 막기위하여
             * 다음과 같이 정확히 path를 구성함.
             * ----------------------------------------------*/
            sDataFileName = sDataFileName+1;
            sBackupPathLen = idlOS::strlen(aBackupDir);

            /* BUG-34357
             * [sBackupPathLen - 1] causes segmentation fault
             * */
            IDE_TEST_RAISE( sBackupPathLen == 0, error_backupdir_path_null );

            if( aBackupDir[sBackupPathLen -1 ] == IDL_FILE_SEPARATOR)
            {
                idlOS::snprintf(sStrDestFileName, SM_MAX_FILE_NAME, "%s%s",
                                aBackupDir,
                                sDataFileName);
            }
            else
            {
                idlOS::snprintf(sStrDestFileName, SM_MAX_FILE_NAME, "%s%c%s",
                                aBackupDir,
                                IDL_FILE_SEPARATOR,
                                sDataFileName);
            }

            sDataFileNameLen = idlOS::strlen(sStrDestFileName);
        IDE_TEST_RAISE( (sBackupPathLen+idlOS::strlen(sDataFileName)+1) > SM_MAX_FILE_NAME, error_backupdir_path_too_long );
        

            IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                          ID_TRUE,
                          sStrDestFileName,
                          &sDataFileNameLen,
                          SMI_TBS_DISK)
                      != IDE_SUCCESS );

            IDE_TEST_RAISE(idlOS::strcmp(sDataFileNode->mFile.getFileName(),
                                         sStrDestFileName) == 0,
                           error_self_copy);

            /* ------------------------------------------------
             * [3] 데이타 파일의 상태를 backup begin으로 변경한다.
             * ----------------------------------------------*/
            IDE_TEST(sddDiskMgr::prepareFileBackup( aStatistics,
                                                    aSpaceNode,
                                                    sDataFileNode )
                     != IDE_SUCCESS);
            sState = 1;

            /* FIT/ART/sm/Bugs/BUG-32218/BUG-32218.sql */
            IDU_FIT_POINT( "1.BUG-32218@smrBackupMgr::backupDiskTBS" );

            if ( sddDiskMgr::copyDataFile( aStatistics,
                                           aSpaceNode,
                                           sDataFileNode,
                                           sStrDestFileName )
                 != IDE_SUCCESS )
            {
                sSystemErrno = ideGetSystemErrno();
                IDE_TEST_RAISE( isRemainSpace(sSystemErrno) == ID_TRUE,
                                error_backup_datafile );
                IDE_RAISE(err_online_backup_write);
            }

            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_BACKUP_INFO2,
                        aSpaceNode->mHeader.mName,
                        sDataFileNode->mFile.getFileName(),
                        sStrDestFileName);

            IDE_TEST_RAISE(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS,
                           error_abort_backup);

            /* ------------------------------------------------
             * [4] 데이타 파일의 백업이 끝났기 때문에, PIL hash의
             * logg내용을 데이타 파일에 적용한다.
             * ----------------------------------------------*/

            sState = 0;
            sddDiskMgr::completeFileBackup( aStatistics,
                                            aSpaceNode,
                                            sDataFileNode );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_self_copy);
    {
       IDE_SET( ideSetErrorCode(smERR_ABORT_SelfCopy));
    }
    IDE_EXCEPTION(err_online_backup_write);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_BACKUP_ABORT1);
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupWrite));
    }
    IDE_EXCEPTION(error_backup_datafile);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_BACKUP_ABORT5,
                    sStrDestFileName);
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupDatafile,"DISK TABLESPACE",
                                sStrDestFileName ));
    }
    IDE_EXCEPTION(error_backupdir_file_path);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathABS));
    }
    IDE_EXCEPTION(error_backupdir_path_null);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_PathIsNullString));
    }
    IDE_EXCEPTION( error_backupdir_path_too_long );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_TooLongFilePath,aBackupDir,sDataFileName) );
    }
    IDE_EXCEPTION(error_abort_backup);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_BACKUP_ABORT4);

    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sddDiskMgr::completeFileBackup( aStatistics,
                                        aSpaceNode,
                                        sDataFileNode );
    }

    return IDE_FAILURE;
}

/*********************************************************
 * Description : database 단위 백업
 *
 * - Database에 있는 모든 테이블 스페이스에 대하여
 *  hot backup을 수행하여, 각 테이블 스페이스의
 *  데이타 파일을 복사한다.
 * - log anchor파일들을 복사한다.
 **********************************************************/
IDE_RC smrBackupMgr::backupDatabase( idvSQL *  aStatistics,
                                     void   *  aTrans,
                                     SChar  *  aBackupDir )
{
    idBool sLocked;
    UInt   sState = 0;
    UInt   sRestoreState = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aBackupDir != NULL );

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_START2);

    // BUG-29861 [WIN] Zero Length File Name을 제대로 검사하지 못합니다.
    IDE_TEST_RAISE( idlOS::strlen(aBackupDir) == 0, error_backupdir_path_null );

    IDE_TEST( mMtxOnlineBackupStatus.trylock( sLocked )
                    != IDE_SUCCESS );

    IDE_TEST_RAISE( sLocked == ID_FALSE, error_backup_going );
    sState = 1;

    // ALTER DATABASE BACKUP TABLESPACE OR DATABASE 구문과
    // ALTER TABLESPACE .. BEGIN BACKUP 구문 수행시 다음을 체크한다.
    IDE_TEST_RAISE( mOnlineBackupState != SMR_BACKUP_NONE,
                    error_backup_going );

    // Aging 수행과정에서 현 트랜잭션을 고려하지 않고 버전수집이
    // 가능하게 한다. (참고) smxTransMgr::getMemoryMinSCN()
    smLayerCallback::updateSkipCheckSCN( aTrans, ID_TRUE );
    sState = 2;

    // [1] 체크포인트를 발생시킨다.
    IDE_TEST( gSmrChkptThread.resumeAndWait( aStatistics )
              != IDE_SUCCESS );

    // [2] BACKUP 관리자의 MEMORY TABLESPACE의 백업상태를 설정한다.
    setOnlineBackupStatusOR( SMR_BACKUP_MEMTBS );
    sRestoreState = 2;

    IDE_TEST( gSmrChkptThread.lock( aStatistics ) != IDE_SUCCESS );
    sState = 3;

    // [3] 각각의 MEMORY TABLESPACE의 상태설정과 백업을 진행한다.
    // BUG-27204 Database 백업 시 session event check되지 않음
    IDE_TEST( smmTBSMediaRecovery::backupAllMemoryTBS(
                             aStatistics,
                             aTrans,
                             aBackupDir ) != IDE_SUCCESS );

    // [4] Loganchor 파일을 백업한다.
    IDE_TEST( smrBackupMgr::backupLogAnchor( aStatistics,
                                             aBackupDir )
              != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( gSmrChkptThread.unlock() != IDE_SUCCESS );

    // [5] BACKUP 관리자의 MEMORY TABLESPACE의 백업상태를 해제한다.
    setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
    sRestoreState = 0;

    // [6] BACKUP 관리자의 DISK TABLESPACE의 백업상태를 설정한다.
    setOnlineBackupStatusOR( SMR_BACKUP_DISKTBS );
    sRestoreState = 1;

    // [7] DISK TABLESPACE들을 백업상태를 설정하고 백업한다.
    IDE_TEST( sddDiskMgr::backupAllDiskTBS( aStatistics,
                                            aBackupDir )
              != IDE_SUCCESS );

    setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
    sRestoreState = 0;

    // [8] 강제로 해당 로그파일을 switch 하여 archive 시킨다.
    IDE_TEST( smrLogMgr::switchLogFileByForce() != IDE_SUCCESS);
    sState = 1;

    smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_END2 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_backup_going );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BACKUP_GOING));
    }
    // BUG-29861 [WIN] Zero Length File Name을 제대로 검사하지 못합니다.
    IDE_EXCEPTION( error_backupdir_path_null );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_PathIsNullString));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch ( sRestoreState )
        {
            case 2:
                if ( sState == 3 )
                {
                    IDE_ASSERT( gSmrChkptThread.unlock()
                                == IDE_SUCCESS );
                    sState = 2;
                }
                setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
                break;
            case 1:
                setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
                break;
            default:
                break;
        }

        switch(sState)
        {
            case 3:
                IDE_ASSERT( gSmrChkptThread.unlock()  == IDE_SUCCESS );
            case 2:
                smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );
            case 1:
                IDE_ASSERT( unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }

        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_FAIL2 );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************
  function description: smrBackup::switchLogFile
  - 백업이 종료한 후에 현재 온라인 로그파일을 archive log로
   백업받도록 한다.
  - 현재 alter database backup구문으로 백업을 받고 있거나,
    begin backup 중이면 에러를 올린다.
    -> 이구문은 백업이 완료된상태에서 수행하여야 한다.
  *********************************************************/
IDE_RC smrBackupMgr::swithLogFileByForces()
{

    UInt      sState = 0;
    idBool    sLocked;

    IDE_TEST( mMtxOnlineBackupStatus.trylock( sLocked )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sLocked == ID_FALSE, error_backup_going );

    sState =1;

    IDE_TEST_RAISE( mOnlineBackupState != SMR_BACKUP_NONE, error_backup_going );

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_BACKUP_SWITCH_START);

    IDE_TEST( smrLogMgr::switchLogFileByForce() != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_SWITCH_END );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_backup_going );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BACKUP_GOING));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }

        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_SWITCH_FAIL );
    }
    IDE_POP();

    return IDE_FAILURE;

}

/*
   ALTER TABLESPACE ... BEGIN BACKUP 기능 수행한다.

  기능 :
  베리타스와 같이 BACKUP 연동을 위하여 추가된 함수이며, 주어진
  테이블스페이스의 상태를 백업 시작(BACKUP_BEGIN)으로 변경한다.

  PRJ-1548 User Memroy Tablespace 개념도입

  테이블스페이스의 종류와 백업상태에 따라 체크포인트 타입이 달라지거나
  활성화/비활성화가 된다.

  A. 백업진행중이 메모리테이블스페이스가 없을 경우
  1. 1개이상의 디스크 테이블스페이스가 백업진행중인 경우
    => MRDB 체크포인트만 진행된다.
  2. 백업중인 디스크 테이블스페이스가 없는 경우
    => BOTH 체크포인트가 진행된다.

  B. 백업진행중이 메모리테이블스페이스가 있는 경우
  1. 1개이상의 디스크 테이블스페이스가 백업진행중인 경우
    => 체크포인트 쓰레드는 다음 주기까지 대기한다.
  2. 백업중인 디스크 테이블스페이스가 없는 경우
    =>: DRDB 체크포인트가 진행된다.

  인자 :
  [IN] aSpaceID : BACKUP 진행할 테이블스페이스 ID

*/
IDE_RC smrBackupMgr::beginBackupTBS( scSpaceID aSpaceID )
{
    UInt                 sRestoreState = 0;
    idBool               sLockedBCKMgr = ID_FALSE;
    sctTableSpaceNode  * sSpaceNode;

    IDE_TEST( lock( NULL /* idvSQL*3 */) != IDE_SUCCESS );
    sLockedBCKMgr = ID_TRUE;

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_BACKUP_TABLESPACE_BEGIN,
                aSpaceID);

    sSpaceNode = sctTableSpaceMgr::getSpaceNodeBySpaceID( aSpaceID );

    IDE_TEST_RAISE( sSpaceNode == NULL, error_not_found_tablespace_node );

    if( sctTableSpaceMgr::isDiskTableSpace( sSpaceNode ) == ID_TRUE )
    {
        setOnlineBackupStatusOR( SMR_BACKUP_DISKTBS );
        sRestoreState = 1;

        IDE_TEST( beginBackupDiskTBS( NULL,
                                      (sddTableSpaceNode*)sSpaceNode ) != IDE_SUCCESS );

        // PRJ-1548 User Memory Tablespace 개념도입
        // BACKUP 시작한 Disk Tablespace 개수를 카운트한다.
        mBeginBackupDiskTBSCount++;
    }
    else
    {
        IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( sSpaceNode ) == ID_TRUE );

        setOnlineBackupStatusOR( SMR_BACKUP_MEMTBS );
        sRestoreState = 2;

        // [1] disk table space backup 상태 변경및
        // 첫번째 데이타파일의 상태를 backup으로 변경한다.
        IDE_TEST( sctTableSpaceMgr::startTableSpaceBackup(
                      NULL,
                      sSpaceNode ) != IDE_SUCCESS );

        // PRJ-1548 User Memory Tablespace 개념도입
        // BACKUP 시작한 Memory Tablespace 개수를 카운트한다.
        mBeginBackupMemTBSCount++;
    }

    sLockedBCKMgr = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                  aSpaceID ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch ( sRestoreState )
        {
            case 2: /* Memory TBS */
            {
                // To fix BUG-21671
                if ( mBeginBackupMemTBSCount == 0 )
                {
                    setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
                }
                else
                {
                    // BUG-21671
                    // 동일 begin 쿼리문을 재차 날렸을 경우
                    // mOnlineBackupState를 NOT으로 설정하지 않음
                }
                break;
            }
            case 1: /* Disk TBS */
            {
                // To fix BUG-21671
                if ( mBeginBackupDiskTBSCount == 0 )
                {
                    setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
                }
                else
                {
                    // BUG-21671
                    // 동일 begin 쿼리문을 재차 날렸을 경우
                    // mOnlineBackupState를 NOT으로 설정하지 않음
                }
                break;
            }
            default :
                break;
        }

        if ( sLockedBCKMgr == ID_TRUE )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}


/*
  ALTER TABLESPACE ... BEGIN BACKUP 수행시 디스크테이블스페이스의
  데이타파일의 상태를 BEGIN_BACKUP 상태로 변경한다.

  인자 :
  [IN] aSpaceID : BACKUP 진행할 테이블스페이스 ID
*/
IDE_RC smrBackupMgr::beginBackupDiskTBS( idvSQL            * aStatistics,
                                         sddTableSpaceNode * aSpaceNode )
{
    UInt                  sState = 0;
    sddDataFileNode*      sDataFileNode;
    UInt                  i;

    // [1] disk table space backup 상태 변경및
    // 첫번째 데이타파일의 상태를 backup으로 변경한다.
    IDE_TEST( sctTableSpaceMgr::startTableSpaceBackup( aStatistics,
                                                       (sctTableSpaceNode*)aSpaceNode ) != IDE_SUCCESS );
    sState = 1;

    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sDataFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sDataFileNode == NULL)
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sDataFileNode->mState ) )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_CREATING( sDataFileNode->mState ) ||
            SMI_FILE_STATE_IS_RESIZING( sDataFileNode->mState ) ||
            SMI_FILE_STATE_IS_DROPPING( sDataFileNode->mState ) )
        {
            idlOS::sleep(1);
            continue;
        }

        IDE_DASSERT( SMI_FILE_STATE_IS_ONLINE( sDataFileNode->mState ) );

        // [2] 데이타 파일의 상태를 BACKUP 상태로 변경
        IDE_TEST( sddDiskMgr::prepareFileBackup(
                      aStatistics,  /* idvSQL* */
                      aSpaceNode,
                      sDataFileNode ) != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 ) // XXXX 왜 주석 처리 했었지?
    {
        sddDiskMgr::abortBackupTableSpace( NULL,
                                           aSpaceNode );
    }

    return IDE_FAILURE;
}


/*
  ALTER TABLESPACE ... END BACKUP 수행시 디스크테이블스페이스의
  데이타파일의 상태를 END_BACKUP 상태로 변경한다.

  PRJ-1548 User Memroy Tablespace 개념도입

  테이블스페이스의 종류와 백업상태에 따라 체크포인트 타입이 달라지거나
  활성화/비활성화가 된다.

  A. 백업진행중이 메모리테이블스페이스가 없을 경우
  1. 1개이상의 디스크 테이블스페이스가 백업진행중인 경우
    => MRDB 체크포인트만 진행된다.
  2. 백업중인 디스크 테이블스페이스가 없는 경우
    => BOTH 체크포인트가 진행된다.

  B. 백업진행중이 메모리테이블스페이스가 있는 경우
  1. 1개이상의 디스크 테이블스페이스가 백업진행중인 경우
    => 체크포인트 쓰레드는 다음 주기까지 대기한다.
  2. 백업중인 디스크 테이블스페이스가 없는 경우
    =>: DRDB 체크포인트가 진행된다.

  인자 :
  [IN] aSpaceID : BACKUP 종료할 테이블스페이스 ID
*/
IDE_RC smrBackupMgr::endBackupTBS( scSpaceID aSpaceID )
{
    idBool              sLockedBCKMgr = ID_FALSE;
    sctTableSpaceNode * sSpaceNode;

    IDE_TEST( lock( NULL /* idvSQL* */) != IDE_SUCCESS );
    sLockedBCKMgr = ID_TRUE;

    IDE_TEST_RAISE( mOnlineBackupState == SMR_BACKUP_NONE,
                    error_no_begin_backup );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                 SM_TRC_MRECOVERY_BACKUP_TABLESPACE_END,
                 aSpaceID );
    sSpaceNode = sctTableSpaceMgr::getSpaceNodeBySpaceID( aSpaceID );

    IDE_TEST_RAISE( sSpaceNode == NULL, error_not_found_tablespace_node );

    if ( sctTableSpaceMgr::isDiskTableSpace( sSpaceNode ) == ID_TRUE )
    {
        // 백업중인 디스크 테이블스페이스가 없는경우 Exception 처리
        IDE_TEST_RAISE( (mOnlineBackupState & SMR_BACKUP_DISKTBS)
                        != SMR_BACKUP_DISKTBS,
                        error_not_begin_backup);

        // 디스크 테이블스페이스의 백업완료과정을 수행한다.
        IDE_TEST( sddDiskMgr::endBackupDiskTBS(
                      NULL,  /* idvSQL* */
                      (sddTableSpaceNode*)sSpaceNode ) != IDE_SUCCESS );

        // 테이블스페이스의 백업상태를 해제하고, MIN PI 노드를 제거한다.
        IDE_TEST( sctTableSpaceMgr::endTableSpaceBackup( NULL,
                                                         sSpaceNode )
                  != IDE_SUCCESS );

        mBeginBackupDiskTBSCount--;

        if ( mBeginBackupDiskTBSCount == 0 )
        {
            setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
        }
    }
    else
    {
        IDE_ERROR (sctTableSpaceMgr::isMemTableSpace( sSpaceNode ) == ID_TRUE );

        // 백업중인 메모리 테이블스페이스가 없는경우 Exception 처리
        IDE_TEST_RAISE((mOnlineBackupState & SMR_BACKUP_MEMTBS)
                       != SMR_BACKUP_MEMTBS,
                       error_not_begin_backup);

        // 테이블스페이스의 백업상태를 해제한다.
        IDE_TEST( sctTableSpaceMgr::endTableSpaceBackup( NULL,
                                                         sSpaceNode )
                  != IDE_SUCCESS );

        mBeginBackupMemTBSCount--;

        if ( mBeginBackupMemTBSCount == 0 )
        {
            setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
        }
    }

    sLockedBCKMgr = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundTableSpaceNode,
                                aSpaceID));
    }
    IDE_EXCEPTION(error_no_begin_backup);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoActiveBeginBackup));
    }
    IDE_EXCEPTION(error_not_begin_backup);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotBeginBackup, aSpaceID));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sLockedBCKMgr == ID_TRUE )
        {
            IDE_ASSERT( unlock() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;

}

/*********************************************************************** 
 * Database 전체를 Incremental Backup한다.
 * PROJ-2133 Incremental backup
 *
 * aTrans       - [IN]  
 * aBackupDir   - [IN] incremental backup 위치 -- <현재 사용되지않는 인자>
 * aBackupLevel - [IN] 수행되는 incremental backup Level
 * aBackupType  - [IN] 수행되는 incremental backup type
 * aBackupTag   - [IN] 수행되는 incremental backup tag 이름
 *
 **********************************************************************/
IDE_RC smrBackupMgr::incrementalBackupDatabase(
                                     idvSQL            * aStatistics,
                                     void              * aTrans,
                                     SChar             * aBackupDir,
                                     smiBackuplevel      aBackupLevel,
                                     UShort              aBackupType,
                                     SChar             * aBackupTag )
{
    idBool              sLocked;
    idBool              sIsDirCreate = ID_FALSE;
    smriBISlot          sCommonBackupInfo;
    SChar               sIncrementalBackupPath[ SM_MAX_FILE_NAME ];
    smrLogAnchorMgr   * sAnchorMgr;
    smriBIFileHdr     * sBIFileHdr;
    smLSN               sMemRecvLSNOfDisk;    
    smLSN               sDiskRedoLSN;
    smLSN               sMemEndLSN;
    SInt                sSystemErrno;
    UInt                sDeleteArchLogFileNo = 0;
    UInt                sState = 0;
    UInt                sRestoreState = 0;
    smLSN               sLastBackupLSN;

    IDE_DASSERT( aTrans != NULL );

    mBackupBISlotCnt = SMRI_BI_INVALID_SLOT_CNT;

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_START2);

    IDE_TEST( mMtxOnlineBackupStatus.trylock( sLocked )
                    != IDE_SUCCESS );

    IDE_TEST_RAISE( sLocked == ID_FALSE, error_backup_going );
    sState = 1;

    // ALTER DATABASE BACKUP TABLESPACE OR DATABASE 구문과
    // ALTER TABLESPACE .. BEGIN BACKUP 구문 수행시 다음을 체크한다.
    IDE_TEST_RAISE( mOnlineBackupState != SMR_BACKUP_NONE,
                    error_backup_going );

    /* aBackupDir이 존재하는지 확인한다. 존재하지 않다면 디렉토리를 생성한다. */
    IDE_TEST( setBackupInfoAndPath( aBackupDir, 
                                    aBackupLevel, 
                                    aBackupType, 
                                    SMRI_BI_BACKUP_TARGET_DATABASE, 
                                    aBackupTag,
                                    &sCommonBackupInfo,
                                    sIncrementalBackupPath,
                                    ID_TRUE )
              != IDE_SUCCESS );
    sIsDirCreate = ID_TRUE;

    // Aging 수행과정에서 현 트랜잭션을 고려하지 않고 버전수집이
    // 가능하게 한다. (참고) smxTransMgr::getMemoryMinSCN()
    smLayerCallback::updateSkipCheckSCN( aTrans, ID_TRUE );
    sState = 2;

    // [1] 체크포인트를 발생시킨다.
    IDE_TEST( gSmrChkptThread.resumeAndWait( aStatistics )
              != IDE_SUCCESS );

    // [2] BACKUP 관리자의 MEMORY TABLESPACE의 백업상태를 설정한다.
    setOnlineBackupStatusOR( SMR_BACKUP_MEMTBS );
    sRestoreState = 2;

    IDE_TEST( smriBackupInfoMgr::lock() != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( smriBackupInfoMgr::getBIFileHdr( &sBIFileHdr ) != IDE_SUCCESS );
    mBackupBISlotCnt = sBIFileHdr->mBISlotCnt;

    /* BUG-37371
     * level 0 백업이 존재하지 않는경우 level 1 백업을 실패하도록 한다. */
    IDE_TEST_RAISE( (mBackupBISlotCnt == 0) && 
                    (aBackupLevel == SMI_BACKUP_LEVEL_1),
                    there_is_no_level0_backup );

    IDE_TEST( gSmrChkptThread.lock( aStatistics ) != IDE_SUCCESS );
    sState = 4;

    /* 
     * incremental backup 이후 archiving된 log file들중 삭제해도 무방한
     * archive log file번호를 구한다. logAnchor에 저장된 DiskRedoLSN과
     * MemEndLSN중 작은 LSN을 구하고, 구한 LSN이 속한 log file 번호
     * 보다 1 작은 log파일까지 삭제 할수있는 archivelogfile로 설정한다.
     */
    sAnchorMgr = smrRecoveryMgr::getLogAnchorMgr();

    sAnchorMgr->getEndLSN( &sMemEndLSN );

    sMemRecvLSNOfDisk = sMemEndLSN;
    sDiskRedoLSN      = sAnchorMgr->getDiskRedoLSN();

    if ( smrCompareLSN::isGTE(&sMemRecvLSNOfDisk, &sDiskRedoLSN)
         == ID_TRUE )
    {
        sMemEndLSN = sDiskRedoLSN;
    }

    if ( sMemEndLSN.mFileNo != 0 )
    {
        /* 
         * recovery시 필요한 log파일 번호보다 1 작은 log파일을 삭제할수
         * 있는 archivelog파일로 설정한다.
         */
        sDeleteArchLogFileNo = sMemEndLSN.mFileNo - 1;
    }
    else
    {
        /* 
         * log파일 번호가 0이면 삭제 가능한 log file이 없음으로 
         * ID_UINT_MAX로 설정한다.
         */
        sDeleteArchLogFileNo = ID_UINT_MAX;
    }

    // [3] 각각의 MEMORY TABLESPACE의 상태설정과 백업을 진행한다.
    // BUG-27204 Database 백업 시 session event check되지 않음
    IDE_TEST( smmTBSMediaRecovery::incrementalBackupAllMemoryTBS(
                             aStatistics,
                             aTrans,
                             &sCommonBackupInfo,
                             sIncrementalBackupPath) != IDE_SUCCESS );

    smrLogMgr::getLstLSN( &sLastBackupLSN );

    IDE_TEST( smrRecoveryMgr::updateBIFileAttrToLogAnchor( 
                                                    NULL,
                                                    NULL,
                                                    &sLastBackupLSN,
                                                    NULL,
                                                    &sDeleteArchLogFileNo )
              != IDE_SUCCESS );

    // [4] Loganchor 파일을 백업한다.
    IDE_TEST( smrBackupMgr::backupLogAnchor( aStatistics,
                                             sIncrementalBackupPath )
              != IDE_SUCCESS );

    sState = 3;
    IDE_TEST( gSmrChkptThread.unlock() != IDE_SUCCESS );

    // [5] BACKUP 관리자의 MEMORY TABLESPACE의 백업상태를 해제한다.
    setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
    sRestoreState = 0;

    // [6] BACKUP 관리자의 DISK TABLESPACE의 백업상태를 설정한다.
    setOnlineBackupStatusOR( SMR_BACKUP_DISKTBS );
    sRestoreState = 1;

    // [7] DISK TABLESPACE들을 백업상태를 설정하고 백업한다.
    IDE_TEST( sddDiskMgr::incrementalBackupAllDiskTBS( aStatistics,
                                                       &sCommonBackupInfo,
                                                       sIncrementalBackupPath )
              != IDE_SUCCESS );
    
    IDE_TEST( smriBackupInfoMgr::flushBIFile( SMRI_BI_INVALID_SLOT_IDX, 
                                              &sLastBackupLSN ) 
              != IDE_SUCCESS );

    /* backup info 파일을 백업한다. */
    if( smriBackupInfoMgr::backup( sIncrementalBackupPath ) 
        != IDE_SUCCESS )
    {
        sSystemErrno = ideGetSystemErrno();
 
        IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                       error_backup_datafile);
 
        IDE_RAISE(err_online_backup_write);
    }

    sState = 2;
    IDE_TEST( smriBackupInfoMgr::unlock() != IDE_SUCCESS );

    setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
    sRestoreState = 0;

    // [8] 강제로 해당 로그파일을 switch 하여 archive 시킨다.
    IDE_TEST( smrLogMgr::switchLogFileByForce() != IDE_SUCCESS );

    sState = 1;
    smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_END2 );

    return IDE_SUCCESS;

    IDE_EXCEPTION(there_is_no_level0_backup);
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_Cannot_Perform_Level1_Backup) );
    }
    IDE_EXCEPTION(err_online_backup_write);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_ABORT1 );

        IDE_SET( ideSetErrorCode(smERR_ABORT_BackupWrite) );
    }
    IDE_EXCEPTION(error_backup_datafile);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT2,
                     smrRecoveryMgr::getBIFileName() );

        IDE_SET( ideSetErrorCode( smERR_ABORT_BackupDatafile,
                                  "BACKUP INFOMATON FILE",
                                  smrRecoveryMgr::getBIFileName() ) );
    }
    IDE_EXCEPTION( error_backup_going );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BACKUP_GOING));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch ( sRestoreState )
        {
            case 2:
                if ( sState == 4 )
                {
                    IDE_ASSERT( gSmrChkptThread.unlock()
                                == IDE_SUCCESS );
                    sState = 3;
                }
                setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
                break;
            case 1:
                setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
                break;
            default:
                break;
        }

        switch(sState)
        {
            case 4:
                IDE_ASSERT( gSmrChkptThread.unlock()  == IDE_SUCCESS );
            case 3:
                IDE_ASSERT( smriBackupInfoMgr::unlock() == IDE_SUCCESS );
            case 2:
                smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );
            case 1:
                IDE_ASSERT( unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }

        if( mBackupBISlotCnt != SMRI_BI_INVALID_SLOT_CNT )
        {
            IDE_ASSERT( smriBackupInfoMgr::rollbackBIFile( mBackupBISlotCnt ) 
                        == IDE_SUCCESS );
        }

        if( sIsDirCreate == ID_TRUE )
        {
           IDE_ASSERT( smriBackupInfoMgr::removeBackupDir( 
                                        sIncrementalBackupPath ) 
                       == IDE_SUCCESS );
        }

        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_FAIL2 );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************************** 
 * 한테이블 스페이스를 Incremental Backup한다.
 * PROJ-2133 Incremental backup
 *
 * aTrans       - [IN]  
 * aSpaceID     - [IN] 백업하려는 테이블 스페이스 ID
 * aBackupDir   - [IN] incremental backup 위치 --<현재 사용되지 않는 인자>
 * aBackupLevel - [IN] 수행되는 incremental backup Level
 * aBackupType  - [IN] 수행되는 incremental backup type
 * aBackupTag   - [IN] 수행되는 incremental backup tag
 *
 **********************************************************************/
IDE_RC smrBackupMgr::incrementalBackupTableSpace( idvSQL            * aStatistics,
                                                  void              * aTrans,    
                                                  scSpaceID           aSpaceID,
                                                  SChar             * aBackupDir,
                                                  smiBackuplevel      aBackupLevel,
                                                  UShort              aBackupType,
                                                  SChar             * aBackupTag,
                                                  idBool              aCheckTagName )
{
    smriBISlot          sCommonBackupInfo;
    sctTableSpaceNode * sSpaceNode = NULL;
    smriBIFileHdr     * sBIFileHdr;
    SChar               sIncrementalBackupPath[ SM_MAX_FILE_NAME ];
    smLSN               sLastBackupLSN;
    SInt                sSystemErrno;
    UInt                sState = 0;
    UInt                sRestoreState = 0;
    idBool              sLocked;
    idBool              sIsDirCreate = ID_FALSE;

    IDE_DASSERT( aTrans != NULL );

    if( aCheckTagName == ID_TRUE )
    {
        mBackupBISlotCnt = SMRI_BI_INVALID_SLOT_CNT;
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( mMtxOnlineBackupStatus.trylock(sLocked) != IDE_SUCCESS );

    IDE_TEST_RAISE( sLocked == ID_FALSE, error_backup_going);
    sState = 1;

    // alter database backup tablespace or database뿐만아니라,
    // alter tablespace begin backup 경로로 올수 있기 때문에,
    // 다음과 같이 검사한다.
    IDE_TEST_RAISE( mOnlineBackupState != SMR_BACKUP_NONE,
                    error_backup_going );

    /* aBackupDir이 존재하는지 확인한다. 존재하지 않다면 디렉토리를 생성한다. */
    IDE_TEST( setBackupInfoAndPath( aBackupDir, 
                                    aBackupLevel, 
                                    aBackupType, 
                                    SMRI_BI_BACKUP_TARGET_TABLESPACE, 
                                    aBackupTag,
                                    &sCommonBackupInfo,
                                    sIncrementalBackupPath,
                                    aCheckTagName )
              != IDE_SUCCESS );
    sIsDirCreate = ID_TRUE;

    // Aging 수행과정에서 현 트랜잭션을 고려하지 않고 버전수집이
    // 가능하게 한다. (참고) smxTransMgr::getMemoryMinSCN()
    smLayerCallback::updateSkipCheckSCN( aTrans, ID_TRUE );
    sState = 2;

    // 체크포인트를 발생시킨다.
    IDE_TEST( gSmrChkptThread.resumeAndWait( aStatistics )
              != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_START);

    IDE_TEST( smriBackupInfoMgr::lock() != IDE_SUCCESS );
    sState = 3;
    
    if( aCheckTagName == ID_TRUE )
    {
        IDE_TEST( smriBackupInfoMgr::getBIFileHdr( &sBIFileHdr ) != IDE_SUCCESS );
        mBackupBISlotCnt = sBIFileHdr->mBISlotCnt;
    }
    else
    {
        /* nothing to do */
    }

    sSpaceNode = sctTableSpaceMgr::getSpaceNodeBySpaceID( aSpaceID );

    IDE_TEST_RAISE( sSpaceNode == NULL ,
                    error_not_found_tablespace_node );

    IDE_TEST_RAISE( ( (sSpaceNode->mState & SMI_TBS_DROPPED)   == SMI_TBS_DROPPED ) ||
                    ( (sSpaceNode->mState & SMI_TBS_DISCARDED) == SMI_TBS_DISCARDED ),
                    error_not_found_tablespace_node );

    IDE_TEST( sctTableSpaceMgr::startTableSpaceBackup( aStatistics,
                                                       sSpaceNode )
              != IDE_SUCCESS );
    sState = 4;

    // Backup Flag 세팅 후 한번 더 확인함.
    IDE_TEST_RAISE( (sSpaceNode->mState & SMI_TBS_DROPPED) == SMI_TBS_DROPPED,
                    error_not_found_tablespace_node );

    switch( sctTableSpaceMgr::getTBSLocation( sSpaceNode ) )
    {
        case SMI_TBS_MEMORY:
            {
                setOnlineBackupStatusOR( SMR_BACKUP_MEMTBS );
                sRestoreState = 2;

                IDE_TEST( incrementalBackupMemoryTBS( aStatistics,
                                                      (smmTBSNode*)sSpaceNode,
                                                      sIncrementalBackupPath,
                                                      &sCommonBackupInfo )
                          != IDE_SUCCESS );

                setOnlineBackupStatusNOT( SMR_BACKUP_MEMTBS );
                sRestoreState = 0;
            }
            break;
        case SMI_TBS_DISK:
            {
                setOnlineBackupStatusOR( SMR_BACKUP_DISKTBS );
                sRestoreState = 1;

                IDE_TEST( incrementalBackupDiskTBS( aStatistics,
                                                    (sddTableSpaceNode*)sSpaceNode,
                                                    sIncrementalBackupPath,
                                                    &sCommonBackupInfo )
                          != IDE_SUCCESS );

                setOnlineBackupStatusNOT( SMR_BACKUP_DISKTBS );
                sRestoreState = 0;
            }
            break;
        case SMI_TBS_VOLATILE:
            {
                // Nothing to do...
                // volatile tablespace에 대해서는 백업을 지원하지 않는다.
            }
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    sState = 3;
    IDE_TEST( sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                                     sSpaceNode ) != IDE_SUCCESS );
    /* backup대상이 tablespace인경우 sDeleteArchLogFileNo 정보를 갱신하지 않는다.
     * sDeleteArchLogFileNo 정보를 갱신하고 archivelog를 삭제할경우 이전에 수행된 
     * database backup의 복구가 불가능해 진다.
     */
    smrLogMgr::getLstLSN( &sLastBackupLSN );
    IDE_TEST( smrRecoveryMgr::updateBIFileAttrToLogAnchor( 
                                                    NULL,
                                                    NULL,
                                                    &sLastBackupLSN,
                                                    NULL,
                                                    NULL )
              != IDE_SUCCESS );

    IDE_TEST( smriBackupInfoMgr::flushBIFile( SMRI_BI_INVALID_SLOT_IDX, 
                                              &sLastBackupLSN ) 
              != IDE_SUCCESS );

    /* backup info 파일을 백업한다. */
    if( smriBackupInfoMgr::backup( sIncrementalBackupPath ) 
        != IDE_SUCCESS )
    {
        sSystemErrno = ideGetSystemErrno();
 
        IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                       error_backup_datafile);
 
        IDE_RAISE(err_online_backup_write);
    }

    sState = 2;
    IDE_TEST( smriBackupInfoMgr::unlock() != IDE_SUCCESS );

    // 강제로 해당 로그파일을 switch 하여 archive 시킨다.
    IDE_TEST( smrLogMgr::switchLogFileByForce() != IDE_SUCCESS );
    sState = 1;

    smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_BACKUP_END );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                 aSpaceID) );
    }
    IDE_EXCEPTION(err_online_backup_write);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_ABORT1 );

        IDE_SET( ideSetErrorCode(smERR_ABORT_BackupWrite) );
    }
    IDE_EXCEPTION(error_backup_datafile);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT2,
                     smrRecoveryMgr::getBIFileName() );

        IDE_SET( ideSetErrorCode( smERR_ABORT_BackupDatafile,
                                  "BACKUP INFOMATON FILE",
                                  smrRecoveryMgr::getBIFileName() ) );
    }
    IDE_EXCEPTION(error_backup_going);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BACKUP_GOING));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sRestoreState )
        {
            case 2:
                setOnlineBackupStatusNOT ( SMR_BACKUP_MEMTBS );
                break;
            case 1:
                setOnlineBackupStatusNOT ( SMR_BACKUP_DISKTBS );
                break;
            default:
                break;
        }

        switch( sState )
        {
            case 4:
                (void)sctTableSpaceMgr::endTableSpaceBackup( aStatistics,
                                                             sSpaceNode ) ;
            case 3:
                IDE_ASSERT( smriBackupInfoMgr::unlock() == IDE_SUCCESS );
            case 2:
                smLayerCallback::updateSkipCheckSCN( aTrans, ID_FALSE );
            case 1:
                IDE_ASSERT( unlock() == IDE_SUCCESS );
                break;
            default:
                break;
        }

        if( mBackupBISlotCnt != SMRI_BI_INVALID_SLOT_CNT )
        {
            IDE_ASSERT( smriBackupInfoMgr::rollbackBIFile( mBackupBISlotCnt ) 
                        == IDE_SUCCESS );
        }

        if( sIsDirCreate == ID_TRUE )
        {
           IDE_ASSERT( smriBackupInfoMgr::removeBackupDir( 
                                        sIncrementalBackupPath ) 
                       == IDE_SUCCESS );
        }

        ideLog::log(SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_FAIL);
    }
    IDE_POP();

    return IDE_FAILURE;

}


/*********************************************************************** 
 * 메모리 테이블스페이스를 Incremental Backup한다.
 * PROJ-2133 Incremental backup
 *
 * aSpaceID           - [IN] 백업할 테이블스페이스 ID
 * aBackupDir         - [IN] incremental backup 위치
 * aCommonBackupInfo  - [IN] backupinfo 공통 정보
 *
 **********************************************************************/
IDE_RC smrBackupMgr::incrementalBackupMemoryTBS( idvSQL     * aStatistics,
                                                 smmTBSNode * aSpaceNode,
                                                 SChar      * aBackupDir,
                                                 smriBISlot * aCommonBackupInfo )
{
    UInt                     i;
    UInt                     sWhichDB;
    UInt                     sDataFileNameLen;
    UInt                     sBmpExtListIdx;
    SInt                     sSystemErrno;
    smmDatabaseFile        * sDatabaseFile;
    smriBISlot               sBackupInfo;
    smriCTDataFileDescSlot * sDataFileDescSlot; 
    smmChkptImageHdr         sChkptImageHdr;
    smriCTState              sTrackingState;

    IDE_DASSERT( aCommonBackupInfo  != NULL );
    IDE_DASSERT( aBackupDir         != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isMemTableSpace( aSpaceNode ) == ID_TRUE );

    // [2] memory database file들을 copy한다.
    sWhichDB = smmManager::getCurrentDB( aSpaceNode );

    idlOS::memcpy( &sBackupInfo, aCommonBackupInfo, ID_SIZEOF(smriBISlot) );

    sBackupInfo.mDBFilePageCnt = 
        smmDatabase::getDBFilePageCount( aSpaceNode->mMemBase);

    for ( i = 0; i <= aSpaceNode->mLstCreatedDBFile; i++ )
    {
        idlOS::snprintf(sBackupInfo.mBackupFileName, SMRI_BI_MAX_BACKUP_FILE_NAME_LEN,
                        "%s%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"_%s%s%s",
                        aBackupDir,
                        aSpaceNode->mHeader.mName,
                        0, //incremental backup파일의 pinpong번호는 0으로 통일한다.
                        i,
                        SMRI_BI_BACKUP_TAG_PREFIX,
                        sBackupInfo.mBackupTag,
                        SMRI_INCREMENTAL_BACKUP_FILE_EXT);

        sDataFileNameLen = idlOS::strlen(sBackupInfo.mBackupFileName);

        IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                              ID_TRUE,
                                              sBackupInfo.mBackupFileName,
                                              &sDataFileNameLen,
                                              SMI_TBS_MEMORY) // MEMORY
                  != IDE_SUCCESS );

        IDE_TEST( smmManager::openAndGetDBFile( aSpaceNode,
                                                sWhichDB,
                                                i,
                                                &sDatabaseFile )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( idlOS::strcmp(sBackupInfo.mBackupFileName,
                                      sDatabaseFile->getFileName())  
                        == 0, error_self_copy);   

        IDE_TEST( sDatabaseFile->readChkptImageHdr( &sChkptImageHdr ) 
                  != IDE_SUCCESS );

        smriChangeTrackingMgr::getDataFileDescSlot( 
                            &sChkptImageHdr.mDataFileDescSlotID,
                            &sDataFileDescSlot );

        sBackupInfo.mBeginBackupTime = smLayerCallback::getCurrTime();
        sBackupInfo.mSpaceID         = aSpaceNode->mHeader.mID;
        sBackupInfo.mFileNo          = i;
        

        /* changeTracking manager가 활성화 된것과 관계없이 데이터파일에 대한
         * 추적시작은 별도의 조건으로 시작된다. 데이터파일이 추가되고 최소한
         * 한번의 level 0백업이 수행 되어야지만 데이터파일에 대한추적이
         * 수행된다.
         */
        sTrackingState = (smriCTState)idCore::acpAtomicGet32( 
                                    &sDataFileDescSlot->mTrackingState );

        /* incremental backup을 수행할수 있는 조건이 되는지 확인한다.
         * incremental backup을 수행조건을 만족하지 못하면 full backup으로
         * backup한다.
         */
        if( ( sTrackingState == SMRI_CT_TRACKING_ACTIVE ) &&
            ( sBackupInfo.mBackupLevel == SMI_BACKUP_LEVEL_1 ) )
        {
            if ( sDatabaseFile->incrementalBackup( sDataFileDescSlot,
                                                   sDatabaseFile,
                                                   &sBackupInfo )
                 != IDE_SUCCESS )
            {
                sSystemErrno = ideGetSystemErrno();
 
                IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                               error_backup_datafile);
 
                IDE_RAISE(err_online_backup_write);
            }
        }
        else
        {
            sBackupInfo.mBackupType          |= SMI_BACKUP_TYPE_FULL;

            /* full backup이 수행됨으로 모든 BmpExtList의 Bitmap을 0으로
             * 초기화한다. 
             */
            for( sBmpExtListIdx = 0; 
                 sBmpExtListIdx < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT; 
                 sBmpExtListIdx++ )
            {
                IDE_TEST( smriChangeTrackingMgr::initBmpExtListBlocks(
                                                            sDataFileDescSlot, 
                                                            sBmpExtListIdx )
                          != IDE_SUCCESS );
            }

            (void)idCore::acpAtomicSet32( &sDataFileDescSlot->mTrackingState, 
                                          (acp_sint32_t)SMRI_CT_TRACKING_ACTIVE);

            /* BUG-18678: Memory/Disk DB File을 /tmp에 Online Backup할때 Disk쪽
               Backup이 실패합니다.*/
            if ( sDatabaseFile->copy( aStatistics,
                                      sBackupInfo.mBackupFileName )
                 != IDE_SUCCESS )
            {
                sSystemErrno = ideGetSystemErrno();
 
                IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                               error_backup_datafile);
 
                IDE_RAISE(err_online_backup_write);
            }
        }

        sBackupInfo.mEndBackupTime = smLayerCallback::getCurrTime();

        smriBackupInfoMgr::setBI2BackupFileHdr( 
                                        &sChkptImageHdr.mBackupInfo,
                                        &sBackupInfo );

        IDE_TEST( sDatabaseFile->setDBFileHeaderByPath( 
                                        sBackupInfo.mBackupFileName, 
                                        &sChkptImageHdr ) 
                  != IDE_SUCCESS );

        IDE_TEST( sDatabaseFile->getFileSize( &sBackupInfo.mOriginalFileSize )
                  != IDE_SUCCESS );

        if( smriBackupInfoMgr::appendBISlot( &sBackupInfo ) 
            != IDE_SUCCESS )
        {
            sSystemErrno = ideGetSystemErrno();
 
            IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                           error_backup_datafile);
 
            IDE_RAISE(err_online_backup_write);
        }

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_BACKUP_INFO1,
                    sDatabaseFile->getFileName(),
                    sBackupInfo.mBackupFileName);

        IDE_TEST_RAISE(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS,
                       error_abort_backup);
    }

    /* ------------------------------------------------
     * [2] alter table등으로 발생한 Backup Internal Database File
     * ----------------------------------------------*/

    
    errno = 0;
    if ( smLayerCallback::copyAllTableBackup( smLayerCallback::getBackupDir(),
                                              aBackupDir )
         != IDE_SUCCESS )
    {
        sSystemErrno = ideGetSystemErrno();
        IDE_TEST_RAISE(isRemainSpace(sSystemErrno) == ID_TRUE,
                       err_backup_internalbackup_file);

        IDE_RAISE(err_online_backup_write);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_self_copy);
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_SelfCopy) );
    }
    IDE_EXCEPTION(err_online_backup_write);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_ABORT1 );

        IDE_SET( ideSetErrorCode(smERR_ABORT_BackupWrite) );
    }
    IDE_EXCEPTION(error_backup_datafile);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT2,
                     sBackupInfo.mBackupFileName );

        IDE_SET( ideSetErrorCode( smERR_ABORT_BackupDatafile,
                                  "MEMORY TABLESPACE",
                                  sBackupInfo.mBackupFileName ) );
    }
    IDE_EXCEPTION(err_backup_internalbackup_file);
    {
        ideLog::log( SM_TRC_LOG_LEVEL_ABORT,
                     SM_TRC_MRECOVERY_BACKUP_ABORT3,
                     sBackupInfo.mBackupFileName );
        IDE_SET( ideSetErrorCode( smERR_ABORT_BackupDatafile,
                                 "BACKUPED INTERNAL FILE",
                                  "" ) );
    }
    IDE_EXCEPTION(error_abort_backup);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT, SM_TRC_MRECOVERY_BACKUP_ABORT4);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************** 
 * 디스크 테이블스페이스를 Incremental Backup한다.
 * PROJ-2133 Incremental backup
 *
 * aSpaceNode         - [IN] 백업할 테이블스페이스 Node
 * aBackupDir         - [IN] incremental backup 위치
 * aCommonBackupInfo  - [IN] backupinfo 공통 정보
 *
 **********************************************************************/
IDE_RC smrBackupMgr::incrementalBackupDiskTBS( idvSQL            * aStatistics,
                                               sddTableSpaceNode * aSpaceNode,
                                               SChar             * aBackupDir, 
                                               smriBISlot        * aCommonBackupInfo )
{
    SInt                        sSystemErrno;
    UInt                        sState = 0;
    UInt                        sDataFileNameLen;
    UInt                        sBmpExtListIdx;
    sddDataFileNode           * sDataFileNode = NULL;
    sddDataFileHdr              sDataFileHdr;
    smriBISlot                  sBackupInfo;
    SChar                     * sDataFileName;
    smriCTDataFileDescSlot    * sDataFileDescSlot; 
    smriCTState                 sTrackingState;
    UInt                        i;
    UShort                      sLockListID;

    IDE_DASSERT( aBackupDir != NULL );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode ) == ID_TRUE );

    idlOS::memcpy( &sBackupInfo, aCommonBackupInfo, ID_SIZEOF(smriBISlot) );

    /* ------------------------------------------------
     * 테이블 스페이스  데이타 파일들  copy.
     * ----------------------------------------------*/
    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sDataFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sDataFileNode == NULL)
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_DROPPED( sDataFileNode->mState ) )
        {
            continue;
        }

        if( SMI_FILE_STATE_IS_CREATING( sDataFileNode->mState ) ||
            SMI_FILE_STATE_IS_RESIZING( sDataFileNode->mState ) ||
            SMI_FILE_STATE_IS_DROPPING( sDataFileNode->mState ) )
        {
            idlOS::sleep(1);
            continue;
        }

        smriChangeTrackingMgr::getDataFileDescSlot( 
                            &sDataFileNode->mDBFileHdr.mDataFileDescSlotID,
                            &sDataFileDescSlot );

        idlOS::memset(sBackupInfo.mBackupFileName,
                      0x00,
                      SMRI_BI_MAX_BACKUP_FILE_NAME_LEN);
        sDataFileName = idlOS::strrchr( sDataFileNode->mName,
                                        IDL_FILE_SEPARATOR );
        IDE_TEST_RAISE(sDataFileName == NULL,error_backupdir_file_path);

        /* ------------------------------------------------
         * BUG-11206와 같이 source와 destination이 같아서
         * 원본 데이타 파일이 유실되는 경우를 막기위하여
         * 다음과 같이 정확히 path를 구성함.
         * ----------------------------------------------*/
        sDataFileName = sDataFileName+1;

        idlOS::snprintf(sBackupInfo.mBackupFileName, SMRI_BI_MAX_BACKUP_FILE_NAME_LEN,
                        "%s%s_%s%s%s",
                        aBackupDir,
                        sDataFileName,
                        SMRI_BI_BACKUP_TAG_PREFIX,
                        sBackupInfo.mBackupTag,
                        SMRI_INCREMENTAL_BACKUP_FILE_EXT);

        sDataFileNameLen = idlOS::strlen(sBackupInfo.mBackupFileName);

        IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                      ID_TRUE,
                      sBackupInfo.mBackupFileName,
                      &sDataFileNameLen,
                      SMI_TBS_DISK)
                  != IDE_SUCCESS );

        IDE_TEST_RAISE(idlOS::strcmp(sDataFileNode->mFile.getFileName(),
                                     sBackupInfo.mBackupFileName) == 0,
                       error_self_copy);

        /* ------------------------------------------------
         * [3] 데이타 파일의 상태를 backup begin으로 변경한다.
         * ----------------------------------------------*/

        IDE_TEST(sddDiskMgr::prepareFileBackup( aStatistics,
                                                aSpaceNode,
                                                sDataFileNode )
                 != IDE_SUCCESS);
        sState = 1;
                        
        IDE_TEST( sddDiskMgr::readDBFHdr( NULL, 
                                          sDataFileNode, 
                                          &sDataFileHdr ) != IDE_SUCCESS );


        sBackupInfo.mBeginBackupTime = smLayerCallback::getCurrTime();
        sBackupInfo.mSpaceID         = aSpaceNode->mHeader.mID;
        sBackupInfo.mFileID          = sDataFileNode->mID;

        /* changeTracking manager가 활성화 된것과 관계없이 데이터파일에 대한
         * 추적시작은 별도의 조건으로 시작된다. 데이터파일이 추가되고 최소한
         * 한번의 level 0백업이 수행 되어야지만 데이터파일에 대한추적이
         * 수행된다.
         */
        sTrackingState = (smriCTState)idCore::acpAtomicGet32( 
                                    &sDataFileDescSlot->mTrackingState );

        /* incremental backup을 수행할수 있는 조건이 되는지 확인한다.
         * incremental backup을 수행조건을 만족하지 못하면 full backup으로
         * backup한다.
         */
        if( ( sTrackingState == SMRI_CT_TRACKING_ACTIVE ) &&
            ( sBackupInfo.mBackupLevel == SMI_BACKUP_LEVEL_1 ) )
        {
            /* incremental backup 수행 */
            if( sddDiskMgr::incrementalBackup( aStatistics,
                                               aSpaceNode,
                                               sDataFileDescSlot,
                                               sDataFileNode,
                                               &sBackupInfo )
                != IDE_SUCCESS )
            {
                sSystemErrno = ideGetSystemErrno();
                IDE_TEST_RAISE( isRemainSpace(sSystemErrno) == ID_TRUE,
                                error_backup_datafile );
                IDE_RAISE(err_online_backup_write);
            }
        }
        else
        {
            sBackupInfo.mBackupType          |= SMI_BACKUP_TYPE_FULL;

            IDU_FIT_POINT("smrBackupMgr::incrementalBackupDiskTBS::wait");

            /* 
             * BUG-40604 The bit count in a changeTracking`s bitmap is not same 
             * with that in a changeTracking`s datafile descriptor 
             * SetBit 함수와의 동시성 문제를 해결하기 위해 Bitmap extent에X latch를 잡는다.
             * 동시성 문제가 발생할 수 있는, 현재 추적중인 differrential bitmap list의 
             * latch만 획득하면 된다.
             * cumulative bitmap 도 현재 추적중인 differrential bitmap list의 
             * latch에 의해 동시성 보장이 가능하다.
             */
            sLockListID = sDataFileDescSlot->mCurTrackingListID;
            IDE_TEST( smriChangeTrackingMgr::lockBmpExtHdrLatchX( sDataFileDescSlot, 
                        sLockListID ) 
                    != IDE_SUCCESS );
            sState = 2;
            
            /* full backup이 수행됨으로 모든 BmpExtList의 Bitmap을 0으로
             * 초기화한다. 
             */
            for( sBmpExtListIdx = 0; 
                 sBmpExtListIdx < SMRI_CT_DATAFILE_DESC_BMP_EXT_LIST_CNT; 
                 sBmpExtListIdx++ )
            {
                IDE_TEST( smriChangeTrackingMgr::initBmpExtListBlocks(
                                                            sDataFileDescSlot, 
                                                            sBmpExtListIdx )
                          != IDE_SUCCESS );
            }

            sState = 1;
            IDE_TEST( smriChangeTrackingMgr::unlockBmpExtHdrLatchX( sDataFileDescSlot, 
                        sLockListID ) 
                    != IDE_SUCCESS );
            
            (void) idCore::acpAtomicSet32( &sDataFileDescSlot->mTrackingState, 
                                           (acp_sint32_t)SMRI_CT_TRACKING_ACTIVE );
            
            /* Full backup 수행 */
            if ( sddDiskMgr::copyDataFile( aStatistics,
                                           aSpaceNode,
                                           sDataFileNode,
                                           sBackupInfo.mBackupFileName )
                 != IDE_SUCCESS )
            {
                sSystemErrno = ideGetSystemErrno();
                IDE_TEST_RAISE( isRemainSpace(sSystemErrno) == ID_TRUE,
                                error_backup_datafile );
                IDE_RAISE(err_online_backup_write);
            }
        }

        sBackupInfo.mEndBackupTime = smLayerCallback::getCurrTime();

        smriBackupInfoMgr::setBI2BackupFileHdr( 
                                        &sDataFileHdr.mBackupInfo,
                                        &sBackupInfo );

        IDE_TEST( sddDataFile::writeDBFileHdrByPath( sBackupInfo.mBackupFileName,
                                                     &sDataFileHdr )
                  != IDE_SUCCESS );

        sBackupInfo.mOriginalFileSize = sDataFileNode->mCurrSize;

        if( smriBackupInfoMgr::appendBISlot( &sBackupInfo ) 
            != IDE_SUCCESS )
        {
            sSystemErrno = ideGetSystemErrno();
            IDE_TEST_RAISE( isRemainSpace(sSystemErrno) == ID_TRUE,
                            error_backup_datafile );
            IDE_RAISE(err_online_backup_write);
        }

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_BACKUP_INFO2,
                    aSpaceNode->mHeader.mName,
                    sDataFileNode->mFile.getFileName(),
                    sBackupInfo.mBackupFileName);

        IDE_TEST_RAISE(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS,
                       error_abort_backup);

        /* ------------------------------------------------
         * [4] 데이타 파일의 백업이 끝났기 때문에, PIL hash의
         * logg내용을 데이타 파일에 적용한다.
         * ----------------------------------------------*/
        sState = 0;
        sddDiskMgr::completeFileBackup( aStatistics,
                                        aSpaceNode,
                                        sDataFileNode );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION(error_self_copy);
    {
       IDE_SET( ideSetErrorCode(smERR_ABORT_SelfCopy));
    }
    IDE_EXCEPTION(err_online_backup_write);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_BACKUP_ABORT1);
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupWrite));
    }
    IDE_EXCEPTION(error_backup_datafile);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_BACKUP_ABORT5,
                    sBackupInfo.mBackupFileName);
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupDatafile,"DISK TABLESPACE",
                                sBackupInfo.mBackupFileName ));
    }
    IDE_EXCEPTION(error_backupdir_file_path);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathABS));
    }
    IDE_EXCEPTION(error_abort_backup);
    {
        ideLog::log(SM_TRC_LOG_LEVEL_ABORT,
                    SM_TRC_MRECOVERY_BACKUP_ABORT4);

    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( smriChangeTrackingMgr::unlockBmpExtHdrLatchX( sDataFileDescSlot, 
                        sLockListID ) 
                    == IDE_SUCCESS );
        case 1:
            sddDiskMgr::completeFileBackup( aStatistics,
                                            aSpaceNode,
                                            sDataFileNode );
            break;
        default:
            break;
    }

    return IDE_FAILURE;


}

/*********************************************************************** 
 * 공통된 backupinfo를 설정하고 incremental backup파일을 생성할 디렉토리
 * 를 설정한다.
 * PROJ-2133 Incremental backup
 *
 * aBackupDir               - [IN] backup할 위치
 * aBackupLevel             - [IN] 수행되는 backup Level
 * aBackupType              - [IN] 수행되는 backup Type
 * aBackupTarget            - [IN] backup을 수행할 대상
 * aBackupTag               - [IN] backup을 수행할 tag이름
 * aCommonBackupInfo        - [IN/OUT] 공통된 백업 정보
 * aIncrementalBackupPath   - [OUT] backup할 위치 

 **********************************************************************/
IDE_RC smrBackupMgr::setBackupInfoAndPath( 
                                    SChar            * aBackupDir,
                                    smiBackupLevel     aBackupLevel,
                                    UShort             aBackupType,
                                    smriBIBackupTarget aBackupTarget,
                                    SChar            * aBackupTag,
                                    smriBISlot       * aCommonBackupInfo,
                                    SChar            * aIncrementalBackupPath,
                                    idBool             aCheckTagName )
{
    UInt        sBackupPathLen;
    time_t      sBackupBeginTime;
    struct tm   sBackupBegin;
    SChar     * sBackupDir;

    idlOS::memset( aCommonBackupInfo, 0x0, ID_SIZEOF( smriBISlot ) );
    idlOS::memset( &sBackupBegin, 0, ID_SIZEOF( struct tm ) );

    sBackupBeginTime = (time_t)smLayerCallback::getCurrTime();
    idlOS::localtime_r( (time_t *)&sBackupBeginTime, &sBackupBegin );
    
    if( aBackupTag == NULL )
    {
        if( aCheckTagName == ID_TRUE )
        {
            idlOS::snprintf( aCommonBackupInfo->mBackupTag, 
                             SMI_MAX_BACKUP_TAG_NAME_LEN,
                             "%04"ID_UINT32_FMT
                             "%02"ID_UINT32_FMT
                             "%02"ID_UINT32_FMT
                             "_%02"ID_UINT32_FMT
                             "%02"ID_UINT32_FMT
                             "%02"ID_UINT32_FMT,
                             sBackupBegin.tm_year + 1900,
                             sBackupBegin.tm_mon + 1,
                             sBackupBegin.tm_mday,
                             sBackupBegin.tm_hour,
                             sBackupBegin.tm_min,
                             sBackupBegin.tm_sec );

            idlOS::strncpy( mLastBackupTagName, 
                            aCommonBackupInfo->mBackupTag, 
                            SMI_MAX_BACKUP_TAG_NAME_LEN );
        }
        else
        {
            idlOS::strncpy( aCommonBackupInfo->mBackupTag, 
                            mLastBackupTagName, 
                            SMI_MAX_BACKUP_TAG_NAME_LEN );
        }
    }
    else
    {
        idlOS::snprintf( aCommonBackupInfo->mBackupTag, 
                         SMI_MAX_BACKUP_TAG_NAME_LEN,
                         "%s",
                         aBackupTag );

    }
    aCommonBackupInfo->mBackupLevel  = aBackupLevel;
    aCommonBackupInfo->mBackupType  |= aBackupType;
    aCommonBackupInfo->mBackupTarget = aBackupTarget;

    if( aBackupDir == NULL )
    {

        sBackupDir = smrRecoveryMgr::getIncrementalBackupDirFromLogAnchor(); 
        
        IDE_TEST_RAISE( idlOS::strcmp( sBackupDir, 
                                       SMRI_BI_INVALID_BACKUP_PATH ) == 0,
                         error_not_defined_backup_path );        

        idlOS::snprintf(aIncrementalBackupPath, 
                        SM_MAX_FILE_NAME,
                        "%s%s%s%c",
                        sBackupDir,
                        SMRI_BI_BACKUP_TAG_PREFIX,
                        aCommonBackupInfo->mBackupTag,
                        IDL_FILE_SEPARATOR );
        
    }
    else
    {
        sBackupPathLen = idlOS::strlen( aBackupDir );

        if( aBackupDir[sBackupPathLen -1 ] == IDL_FILE_SEPARATOR)
        {

            idlOS::snprintf(aIncrementalBackupPath, 
                            SM_MAX_FILE_NAME,
                            "%s%s%s%c",
                            aBackupDir,
                            SMRI_BI_BACKUP_TAG_PREFIX,
                            aCommonBackupInfo->mBackupTag,
                            IDL_FILE_SEPARATOR );
        }
        else
        {
            idlOS::snprintf(aIncrementalBackupPath, 
                            SM_MAX_FILE_NAME,
                            "%s%c%s%s%c",
                            aBackupDir,
                            IDL_FILE_SEPARATOR,
                            SMRI_BI_BACKUP_TAG_PREFIX,
                            aCommonBackupInfo->mBackupTag,
                            IDL_FILE_SEPARATOR );
        }

    }

    if( aCheckTagName == ID_TRUE )
    {
        IDE_TEST_RAISE( idf::access(aIncrementalBackupPath, F_OK) == 0,
                        error_incremental_backup_dir_already_exist );

        IDE_TEST_RAISE( idlOS::mkdir( aIncrementalBackupPath, 
                        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0, // 755권한
                        error_create_path );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST_RAISE( idf::access(aIncrementalBackupPath, F_OK) != 0,
                    error_not_exist_path );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_exist_path );
    {  
        IDE_SET(ideSetErrorCode( smERR_ABORT_NoExistPath, 
                                 aIncrementalBackupPath));
    }
    IDE_EXCEPTION( error_create_path );
    {  
        IDE_SET(ideSetErrorCode( smERR_ABORT_FailToCreateDirectory, 
                                 aIncrementalBackupPath));
    }
    IDE_EXCEPTION( error_not_defined_backup_path );
    {  
        IDE_SET(ideSetErrorCode( smERR_ABORT_NotDefinedIncrementalBackupPath ));
    }  
    IDE_EXCEPTION( error_incremental_backup_dir_already_exist );
    {  
        IDE_SET(ideSetErrorCode( smERR_ABORT_AlreadyExistIncrementalBackupPath, 
                                 aIncrementalBackupPath ));
    }  

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
