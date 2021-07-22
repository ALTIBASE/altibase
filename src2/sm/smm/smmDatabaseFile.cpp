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
 * $Id: smmDatabaseFile.cpp 85837 2019-07-14 23:44:48Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#ifndef VC_WIN32
#include <iduFileAIO.h>
#endif
#include <idu.h>
#include <smErrorCode.h>
#include <smm.h>
#include <sct.h>
#include <smmReq.h>
#include <smmDatabaseFile.h>
#include <smmTBSChkptPath.h>
#include <smriChangeTrackingMgr.h>
#include <smriBackupInfoMgr.h>

smmDatabaseFile::smmDatabaseFile()
{
}

/*
  데이타베이스파일(CHECKPOINT IMAGE) 객체를 초기화한다.

  [IN] aSpaceID    - 데이타베이스파일의 TBS ID
  [IN] aPingpongNo - PINGPONG 번호
  [IN] aDBFileNo   - 데이타베이스파일 번호
  [IN] aFstPageID   - 데이타베이스파일의 첫번째 PID
  [IN] aLstPageID   - 데이타베이스파일의 마지막 PID
 */
IDE_RC smmDatabaseFile::initialize( scSpaceID   aSpaceID,
                                    UInt        aPingPongNum,
                                    UInt        aFileNum,
                                    scPageID  * aFstPageID,
                                    scPageID  * aLstPageID )
{
    smLSN   sMaxLSN;
    SChar   sMutexName[128];

    // BUG-27456 Klocwork SM (4)
    mDir = NULL;

    IDE_TEST( mFile.initialize( IDU_MEM_SM_SMM,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );


    // To Fix BUG-18434
    //        alter tablespace online/offline 동시수행중 서버사망
    //
    // Page Buffer에 접근에 대한 동시성 제어를 위한 Mutex 초기화
    idlOS::snprintf( sMutexName,
                     128,
                     "MEM_DBFILE_PAGEBUFFER_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"_MUTEX",
                     (UInt) aSpaceID,
                     (UInt) aPingPongNum,
                     (UInt) aFileNum );

    IDE_TEST(mPageBufferMutex.initialize( sMutexName,
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );

    mPageBufferMemPtr = NULL;
    mAlignedPageBuffer = NULL;

    // BUG-27456 Klocwork SM (4)
    /* smmDatabaseFile_initialize_malloc_Dir.tc */
    IDU_FIT_POINT("smmDatabaseFile::initialize::malloc::Dir");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMM,
                                 SM_MAX_FILE_NAME,
                                 (void **)&mDir,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );

    IDE_TEST( mFile.allocBuff4DirectIO( IDU_MEM_SM_SMM,
                                        SM_PAGE_SIZE,
                                        (void**)&mPageBufferMemPtr,
                                        (void**)&mAlignedPageBuffer )
              != IDE_SUCCESS );

    // To Fix BUG-18130
    //    Meta Page중 일부만 File Header구조체로 memcpy후
    //    Disk에 내릴때는 Meta Page전체를 내리는 문제 해결
    idlOS::memset( mAlignedPageBuffer,
                   0,
                   SM_DBFILE_METAHDR_PAGE_SIZE );

    idlOS::memset( &mChkptImageHdr,
                   0,
                   ID_SIZEOF(mChkptImageHdr) );

    // PROJ-2133 incremental backup
    mChkptImageHdr.mDataFileDescSlotID.mBlockID = SMRI_CT_INVALID_BLOCK_ID;
    mChkptImageHdr.mDataFileDescSlotID.mSlotIdx = 
                                    SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;

    mSpaceID     = aSpaceID;
    mPingPongNum = aPingPongNum;
    mFileNum     = aFileNum;

    SM_LSN_MAX( sMaxLSN );


    // RedoLSN은 로그앵커로부터 초기화되기전까지는 또는
    // 체크포인트가 발생하기 전 전까지는 ID_UINT_MAX 값을 가진다.
    SM_GET_LSN( mChkptImageHdr.mMemRedoLSN,
                sMaxLSN )

    // CreateLSN은 로그앵커로부터 초기화되기전까지는 또는
    // 데이타파일이 생성되기 전까지는 ID_UINT_MAX 값을 가진다.
    SM_GET_LSN( mChkptImageHdr.mMemCreateLSN,
                sMaxLSN )

    ////////////////////////////////////////////////////////////
    // PRJ-1548 User Memory TableSpace 개념도입

    // 메모리테이블스페이스의 미디어 복구연산에서 사용

    if ( aFstPageID != NULL )
    {
        mFstPageID      = *aFstPageID;  // 데이타파일의 마지막 PID
    }

    if ( aLstPageID != NULL )
    {
        mLstPageID      = *aLstPageID;  // 데이타파일의 첫번째 PID
    }
    mIsMediaFailure = ID_FALSE;    // 미디어복구진행여부

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-27456 Klocwork SM (4)
    if ( mDir != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mDir ) == IDE_SUCCESS );

        mDir = NULL;
    }

    return IDE_FAILURE;

}

IDE_RC smmDatabaseFile::destroy()
{

    IDE_TEST( iduMemMgr::free( mPageBufferMemPtr ) != IDE_SUCCESS );

    // BUG-27456 Klocwork SM (4)
    IDE_TEST( iduMemMgr::free( mDir ) != IDE_SUCCESS );

    mDir = NULL;

    IDE_TEST( mPageBufferMutex.destroy() != IDE_SUCCESS );

    IDE_TEST( mFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    // BUG-27456 Klocwork SM (4)
    if ( mDir != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mDir ) == IDE_SUCCESS );

        mDir = NULL;
    }

    return IDE_FAILURE;
}

smmDatabaseFile::~smmDatabaseFile()
{
}

IDE_RC
smmDatabaseFile::syncUntilSuccess()
{

    return mFile.syncUntilSuccess( smLayerCallback::setEmergency );

}

const SChar*
smmDatabaseFile::getDir()
{

    return (SChar*)mDir;

}


void
smmDatabaseFile::setDir( SChar *aDir )
{
    // BUG-27456 Klocwork SM (4)
    IDE_ASSERT( aDir != NULL );

    idlOS::strncpy( mDir, aDir, SM_MAX_FILE_NAME - 1);

    mDir[ SM_MAX_FILE_NAME - 1 ] = '\0';
}

const SChar*
smmDatabaseFile::getFileName()
{

    return mFile.getFileName();

}


IDE_RC
smmDatabaseFile::setFileName(SChar *aFilename)
{

    return mFile.setFileName(aFilename);

}

IDE_RC
smmDatabaseFile::setFileName( SChar *aDir,
                              SChar *aTBSName,
                              UInt   aPingPongNum,
                              UInt   aFileNum )
{

    SChar sDBFileName[SM_MAX_FILE_NAME];

    IDE_DASSERT( aDir != NULL );
    IDE_DASSERT( aTBSName != NULL );

    idlOS::snprintf( sDBFileName,
                     ID_SIZEOF(sDBFileName),
                     SMM_CHKPT_IMAGE_NAME_WITH_PATH,
                     aDir,
                     aTBSName,
                     aPingPongNum,
                     aFileNum );

    return mFile.setFileName(sDBFileName);

}


/***********************************************************************
 * Description : <aCurrentDB, aDBFileNo>에 해당하는 Database파일을 생성하고
 *               크기는 aSize로 한다.
 *
 * aTBSNode   : Db File을 만들려고 하는  Tablespace의 Node
 * aCurrentDB : MMDB는 Ping-Pong이기 때문에 두개의 Database Image가 존재. 따라서
 *              어느 Image에 DB를 생성할지 결정히기위해 넘김
 *              0 or 1이어야 함.
 * aDBFileNo  : Datbase File No
 * aSize      : 생성할 파일의 크기.(Byte), 만약 aSize == 0라면, DB Header만
 *              기록한다.
 * aChkptImageHdr : 메모리 체크포인트이미지(데이타파일)의 메타헤더
 **********************************************************************/
IDE_RC smmDatabaseFile::createDbFile(smmTBSNode       * aTBSNode,
                                     SInt               aCurrentDB,
                                     SInt               aDBFileNo,
                                     UInt               aSize,
                                     smmChkptImageHdr * aChkptImageHdr )
{
    SChar     sDummyPage[ SM_DBFILE_METAHDR_PAGE_SIZE ];
    SChar     sDBFileName[SM_MAX_FILE_NAME];
    SChar     sCreateDir[SM_MAX_FILE_NAME];
    idBool    sIsDirectIO;

    IDE_ASSERT( aTBSNode != NULL );
    IDE_ASSERT( mFile.getCurFDCnt() == 0 );

    /* aSize는 DB File Header보다 커야 한다. */
    IDE_ASSERT( ( aSize + 1 > SD_PAGE_SIZE ) || ( aSize == 0 ) );

    if( smuProperty::getIOType() == 0 )
    {
        sIsDirectIO = ID_FALSE;
    }
    else
    {
        sIsDirectIO = ID_TRUE;
    }

    IDE_TEST( makeDBDirForCreate( aTBSNode,
                                  aDBFileNo,
                                  sCreateDir )
             != IDE_SUCCESS );

    setDir( sCreateDir ) ;

    /* DBFile 이름을 만든다. */
    idlOS::snprintf( sDBFileName,
                     ID_SIZEOF(sDBFileName),
                     SMM_CHKPT_IMAGE_NAME_WITH_PATH,
                     sCreateDir,
                     aTBSNode->mTBSAttr.mName,
                     aCurrentDB,
                     aDBFileNo );

    IDE_TEST( setFileName( sDBFileName ) != IDE_SUCCESS );

    if ( mFile.exist() == ID_TRUE )
    {
        // BUG-29607 create checkpoint image 도중 동일 파일이 있으면
        //           재사용 하지 말고 지우고 다시 생성한다.
        // checkpoint image 생성은 checkpoint시 자동으로 발생하므로
        // 파일이 있다고 오류를 반환하면, 사용자가 trc.log를 확인하지 않을 시 장시간
        // checkpoint를 못할 수 있으므로, 오류를 반환하지 않고 삭제 후 재생성한다.
        // 대신, create tablespace시 앞으로 생성 할 것을 감안해서
        // File의 존재 유무를 미리 검사하여 사용자의 실수를 미리 방지한다.
        //
        // BUGBUG 오류 알람 기능이 추가되면 예외로 전환한다.

        IDE_TEST( idf::unlink( sDBFileName ) != IDE_SUCCESS );
    }

    // PRJ-1149 media recovery를 위하여 memory 데이타파일도
    // 파일 헤더가 추가됨.
    // 데이타베이스파일 메타헤더 설정
    // MemCreateLSN은 AllocNewPageChunk 처리시 설정이
    // 이미 되어 있어야 한다.
    setChkptImageHdr( sctTableSpaceMgr::getMemRedoLSN(),
                      NULL,     // aMemCreateLSN
                      NULL,     // spaceID
                      NULL,     // smVersion
                      NULL );   // aDataFileDescSlotID

    /* DBFile Header만을 가진 DBFile을 생성한다. */
    IDE_TEST( createDBFileOnDiskAndOpen( sIsDirectIO )
              != IDE_SUCCESS );

    /* FileHeader에 Checkpont Image Header를 기록한다 */
    IDE_TEST( setDBFileHeader( aChkptImageHdr ) != IDE_SUCCESS );

    IDU_FIT_POINT("BUG-46574@smmDatabaseFile::createDbFile::writeUntilSuccess");

    /* File의 크기를 정한다. */
    if( aSize > (SM_DBFILE_METAHDR_PAGE_SIZE + 1) )
    {
        idlOS::memset( sDummyPage, 0, SM_DBFILE_METAHDR_PAGE_SIZE );

        // Set File Size
        IDE_TEST( writeUntilSuccessDIO( aSize - SM_DBFILE_METAHDR_PAGE_SIZE,
                                        (void*) sDummyPage,
                                        SM_DBFILE_METAHDR_PAGE_SIZE,
                                        smLayerCallback::setEmergency )
                  != IDE_SUCCESS );
    }

    IDE_TEST( mFile.syncUntilSuccess( smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    //==============================================================
    // To Fix BUG-13924
    //==============================================================
    ideLog::log( SM_TRC_LOG_LEVEL_MEMORY,
                 SM_TRC_MEMORY_FILE_CREATE,
                 getFileName() );

    // 로그앵커에 저장되어 있지않으면 저장한다.
    // 로그앵커에 저장되어 있다면 CreateDBFileOnDisk Flag를 갱신하여
    // 저장한다.

    IDE_TEST( addAttrToLogAnchorIfCrtFlagIsFalse( aTBSNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Memory DBFile을 Disk에 생성하고 Open한다.
 *
 * aUsedDirectIO - [IN] Direct IO를 사용하면 ID_TRUE, else ID_FALSE
 *
 */
IDE_RC smmDatabaseFile::createDBFileOnDiskAndOpen( idBool aUseDirectIO )
{
    IDE_TEST( mFile.createUntilSuccess( smLayerCallback::setEmergency,
                                        smLayerCallback::isLogFinished() )
              != IDE_SUCCESS );

    IDE_TEST( mFile.open( aUseDirectIO ) != IDE_SUCCESS );

    // 여기에서 DB File Header Page를 미리 만들어두면 안된다.
    //
    // Create Tablespace의 Undo시에 삭제할 DB File의 Header에 기록된
    // Tablespace ID를 보고, Tablespace ID가 일치할 때만파일을 삭제한다.
    //
    // 그러므로, DB File의 Header에 TablespaceID가 제대로 설정된 후에
    // DB파일의 Header Page를 기록하여야 한다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Memory DB File Header에 메타헤더를 기록한다.
 *
 * aChkptImageHdr - [IN] 메모리 체크포인트이미지(데이타파일)의 메타헤더
 *
 */
IDE_RC smmDatabaseFile::setDBFileHeader( smmChkptImageHdr * aChkptImageHdr )
{
    smmChkptImageHdr * sChkptImageHdr2Write;

    if ( aChkptImageHdr != NULL )
    {
        IDE_DASSERT( checkValuesOfDBFHdr( aChkptImageHdr )
                     == IDE_SUCCESS );

        sChkptImageHdr2Write = aChkptImageHdr;
    }
    else
    {
        // CreateDB시에는 Redo LSN이 Hdr에 설정되어 있지 않는
        // 경우가 있다.

        sChkptImageHdr2Write = & mChkptImageHdr;
    }

    // write dbf file header.
    IDE_TEST( writeUntilSuccessDIO( SM_DBFILE_METAHDR_PAGE_OFFSET,
                                    sChkptImageHdr2Write,
                                    ID_SIZEOF(smmChkptImageHdr),
                                    smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * 지정된 경로의 Memory DB File Header에 메타헤더를 기록한다.
 * dbs폴더에 존재하지 않는(현재 DB구동에 사용되지않는) Memory DB파일의 헤더를
 * 갱신할때 사용되는 함수이다. incremental backup된 파일헤더에 backup정보를
 * 남기기위해 사용된다.
 *
 * aChkptImagePath  - [IN] 메모리 체크포인트이미지(데이타파일)의 경로
 * aChkptImageHdr   - [IN] 메모리 체크포인트이미지(데이타파일)의 메타헤더
 *
 */
IDE_RC smmDatabaseFile::setDBFileHeaderByPath( 
                                    SChar            * aChkptImagePath,
                                    smmChkptImageHdr * aChkptImageHdr )
{
    iduFile sFile;
    UInt    sState = 0;

    IDE_DASSERT( aChkptImagePath != NULL );
    IDE_DASSERT( aChkptImageHdr  != NULL );

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMM, 
                                1, /* Max Open FD Count */     
                                IDU_FIO_STAT_OFF,              
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sFile.setFileName( aChkptImagePath ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sFile.open() != IDE_SUCCESS , err_file_not_found);
    sState = 2;

    // write dbf file header.
    IDE_TEST_RAISE( sFile.write( NULL, /*idvSQL*/               
                                 SM_DBFILE_METAHDR_PAGE_OFFSET, 
                                 aChkptImageHdr,
                                 SM_DBFILE_METAHDR_PAGE_SIZE )
                    != IDE_SUCCESS, err_chkptimagehdr_write_failure );

    sState = 1;
    IDE_TEST( sFile.close()   != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );                      
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath,
                                  aChkptImagePath ) );
    }
    IDE_EXCEPTION( err_chkptimagehdr_write_failure );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_Datafile_Header_Write_Failure, 
                                  aChkptImagePath ) );
    }

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sFile.close()   == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
  체크포인트 이미지 파일에 Checkpoint Image Header가 기록된 경우
  이를 읽어서 Tablespace ID가 일치하는지 비교하고 File을 Close한다.

  [IN] aSpaceID - Checkpoint Image Header의 Tablespace ID와 비교할 ID
  [OUT] aIsHeaderWritten - Checkpoint Image Header가 기록되어 있는지 여부
  [OUT] aSpaceIdMatches - Checkpoint Image Header의 Tablespace ID와
                          aSpaceID가 일치하는지 여부
*/
IDE_RC smmDatabaseFile::readSpaceIdAndClose( scSpaceID   aSpaceID,
                                             idBool    * aIsHeaderWritten,
                                             idBool    * aSpaceIdMatches)
{
    IDE_DASSERT( aIsHeaderWritten != NULL );
    IDE_DASSERT( aSpaceIdMatches != NULL );

    smmChkptImageHdr sChkptImageHdr;
    ULong sChkptFileSize = 0;

    *aIsHeaderWritten  = ID_FALSE;
    *aSpaceIdMatches   = ID_FALSE;

    // Checkpoint Image Header를 읽어오기 위해 File을 Open
    if ( isOpen() == ID_FALSE )
    {
        if ( open() != IDE_SUCCESS )
        {
            errno = 0;
            IDE_CLEAR();
        }
    }

    if ( isOpen() == ID_TRUE )
    {
        IDE_TEST ( mFile.getFileSize( & sChkptFileSize )
                   != IDE_SUCCESS );

        // To Fix BUG-18272   TC/Server/sm4/PRJ-1548/dynmem/../suites/
        //                    restart/rt_ctdt_aa.sql 수행도중 죽습니다.
        //
        // Checkpoint Image File이 생성되다 만 경우
        // Checkpoint Image Header가 기록조차 되어 있지 않을 수도 있다
        //
        // File의 크기가 Checkpoint Image Header보다 클때만
        // Checkpoint Image Header를 읽는다.
        if ( sChkptFileSize >= ID_SIZEOF(sChkptImageHdr) )
        {
            IDE_ASSERT( isOpen() == ID_TRUE );

            IDE_TEST ( readChkptImageHdr(&sChkptImageHdr) != IDE_SUCCESS );
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[TBSID:%d] dbf id : %d\n",
                        aSpaceID,
                        sChkptImageHdr.mSpaceID );

            if ( sChkptImageHdr.mSpaceID == aSpaceID )
            {
                *aSpaceIdMatches = ID_TRUE;
            }
            else
            {
                ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[TBSID:%d] DIFFID!!! \n", aSpaceID );

                *aSpaceIdMatches = ID_FALSE;
            }
            *aIsHeaderWritten = ID_TRUE;
        }

        // Checkpoint Image File을 Close
        IDE_TEST( mFile.close() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
    파일이 존재할 경우 삭제한다.
    존재하지 않을 경우 아무일도 하지 않는다.

    [IN] aSpaceID  - 삭제할 Tablespace의 ID
    [IN] aFileName - 삭제할 파일명
 */
IDE_RC smmDatabaseFile::removeFileIfExist( scSpaceID aSpaceID,
                                           const SChar * aFileName )
{
    if ( idf::access( aFileName, F_OK)
         == 0 ) // 파일존재 ?
    {
        // Checkpoint Image File을 Disk에서 제거
        if ( idf::unlink( aFileName ) != 0 )
        {

            ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                        SM_TRC_MEMORY_CHECKPOINT_IMAGE_NOT_REMOVED,
                        aFileName );
            errno = 0;
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[TBSID:%d] file remove failed \n", aSpaceID );
        }
        else
        {
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[TBSID:%d] REMOVED \n", aSpaceID );

        }

    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[TBSID:%d] NO FILE!!! \n", aSpaceID );
        // 파일이 존재하 않을 경우 무시.
        errno = 0;
    }

    return IDE_SUCCESS;
}


/*
    Checkpoint Image File을 Close하고 Disk에서 지워준다.

    만약 파일이 존재하지 않을 경우 무시하고 넘어간다.

    [IN] aSpaceID - 삭제하려는 Checkpoint Image가 속한 Tablespace의 ID
    [IN] aRemoveImageFiles  - Checkpoint Image File을 지워야 할 지 여부
*/
IDE_RC smmDatabaseFile::closeAndRemoveDbFile(scSpaceID      aSpaceID,
                                             idBool         aRemoveImageFiles,
                                             smmTBSNode   * aTBSNode )
{
    idBool                      sIsHeaderWritten ;
    idBool                      sSpaceIdMatches ;
    idBool                      sSaved;
    UInt                        sFileNum;
    smiDataFileDescSlotID       sDataFileDescSlotID;
    smmChkptImageAttr           sChkptImageAttr;

    if ( smLayerCallback::isRestartRecoveryPhase() == ID_TRUE )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[In Recovery]"  );
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,"[In NormalProcessing]"  );
    }

    // Checkpoint Image Header를 읽어서 Space ID를 체크하고 File을 Close
    // (  Close가 되어야 삭제가능 )
    IDE_TEST( readSpaceIdAndClose( aSpaceID,
                                   & sIsHeaderWritten,
                                   & sSpaceIdMatches ) != IDE_SUCCESS );

    if ( aRemoveImageFiles == ID_TRUE )
    {
        // TO Fix BUG-17143  create/drop tablespace 도중 kill, restart시
        //                   The data file does not exist.에러발생
        // Checkpoint Image상에 기록된 Tablespace ID와
        // DROP하려는 Tablespace의 ID가 일치하는 경우에만 파일을 삭제한다.

        // Checkpoint Image Header안의 Tablespace ID가 일치하거나
        if ( ( sSpaceIdMatches == ID_TRUE ) ||
             // Checkpoint Image Header조차 기록되지 않은 경우
             ( sIsHeaderWritten == ID_FALSE ) )
        {
            IDE_TEST( removeFileIfExist( aSpaceID,
                                         getFileName() )
                      != IDE_SUCCESS );

        }
    }

    //PROJ-2133 incremental backup
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )    
    {
        IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( aSpaceID ) == ID_TRUE );

        // 이미 로그앵커에 저장되었는지 확인한다.
        sSaved = smmManager::isCreateDBFileAtLeastOne(
                (aTBSNode->mCrtDBFileInfo[ mFileNum ]).mCreateDBFileOnDisk ) ;

        // 로그앵커에 DBFile이 저장되어있지 않으므로 해재할 DataFileDescSlot이
        // 없다.
        IDE_TEST_CONT( sSaved != ID_TRUE, 
                        already_dealloc_datafiledesc_slot );

        sFileNum = getFileNum();

        IDE_TEST( smLayerCallback::getDataFileDescSlotIDFromLogAncho4ChkptImage(
                        aTBSNode->mCrtDBFileInfo[sFileNum].mAnchorOffset,
                        &sDataFileDescSlotID )
                  != IDE_SUCCESS );

        // DataFileDescSlot이 이미 할당해제 된경우 
        IDE_TEST_CONT( smriChangeTrackingMgr::isAllocDataFileDescSlotID(
                        &sDataFileDescSlotID ) != ID_TRUE,
                        already_dealloc_datafiledesc_slot );

        // DataFileDescSlot이 이미 할당해제 되었으나 loganchor에는
        // DataFileDescSlotID가 남아있는경우 확인
        IDE_TEST_CONT( smriChangeTrackingMgr::isValidDataFileDescSlot4Mem( 
                        this, &sDataFileDescSlotID ) != ID_TRUE,
                        already_reuse_datafiledesc_slot);

        IDE_TEST( smriChangeTrackingMgr::deleteDataFileFromCTFile(
                    &sDataFileDescSlotID ) != IDE_SUCCESS );
        
        IDE_EXCEPTION_CONT( already_reuse_datafiledesc_slot );

        getChkptImageAttr( aTBSNode, &sChkptImageAttr );

        sChkptImageAttr.mDataFileDescSlotID.mBlockID = 
                                    SMRI_CT_INVALID_BLOCK_ID;
        sChkptImageAttr.mDataFileDescSlotID.mSlotIdx = 
                                    SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;

        IDE_TEST( smLayerCallback::updateChkptImageAttrAndFlush(
                  &(aTBSNode->mCrtDBFileInfo[sFileNum]),
                  &sChkptImageAttr )
                  != IDE_SUCCESS );

        IDE_EXCEPTION_CONT( already_dealloc_datafiledesc_slot );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
   Direct I/O를 위해 Disk Sector Size로 Align된 Buffer를 통해 File Read수행

   [IN] aWhere  - read할 File의 Offset
   [IN] aBuffer - read된 데이터가 복사될 Buffer
   [IN] aSize   - read할 크기
 */
IDE_RC
smmDatabaseFile::readDIO( PDL_OFF_T  aWhere,
                          void*      aBuffer,
                          size_t     aSize )
{
    IDE_ASSERT( aBuffer != NULL );
    IDE_ASSERT( aSize <= SM_PAGE_SIZE );
    IDE_ASSERT( mAlignedPageBuffer != NULL );

    UInt sSizeToRead;
    UInt sStage = 0;
    idBool  sCanDirectIO;
    UChar  *sReadBuffer;

    sCanDirectIO = mFile.canDirectIO( aWhere,
                                      aBuffer,
                                      aSize );

    if( sCanDirectIO == ID_TRUE )
    {
        sSizeToRead = aSize;
        sReadBuffer = (UChar*)aBuffer;
    }
    else
    {
        sReadBuffer = mAlignedPageBuffer;
        sSizeToRead = idlOS::align( aSize,
                                    iduProperty::getDirectIOPageSize() );
    }

    IDE_TEST( mPageBufferMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS );
    sStage = 1;

    IDE_ASSERT( sSizeToRead <= SM_PAGE_SIZE );

    IDE_TEST( mFile.read( NULL, /* idvSQL* */
                          aWhere, 
                          sReadBuffer, 
                          sSizeToRead )
              != IDE_SUCCESS );

    if( sCanDirectIO == ID_FALSE )
    {
        idlOS::memcpy( aBuffer, sReadBuffer, aSize );
    }

    sStage = 0;
    IDE_TEST( mPageBufferMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1:
            IDE_ASSERT( mPageBufferMutex.unlock() == IDE_SUCCESS );
            break;
        default :
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/**
    Direct I/O를 이용하여 write하기 위해 Align된 Buffer에 데이터 복사

    [IN] aAlignedDst - 데이터가 복사될 Align된 메모리 공간
    [IN] aSrc        - 원본 데이터 주소
    [IN] aSrcSize    - 원본 데이터 크기
    [OUT] aSizeToWrite - aAlignedDst를 Direct I/O로 write시에 기록할 크기
 */
IDE_RC smmDatabaseFile::copyDataForDirectWrite( void * aAlignedDst,
                                                void * aSrc,
                                                UInt   aSrcSize,
                                                UInt * aSizeToWrite )
{
    // Direct IO Page크기가 DB File Header Page크기보다 클 경우,
    // DB File Header를 Direct I/O로 기록하기 위해
    // Direct IO Page크기만큼 Align 하여 기록할 경우
    // 0번 Page영역을 침범할 수 있다.
    //
    // 그러므로, Direct IO Page크기는 항상 DB File의 Header Page보다
    // 작거나 같아야 한다.
    IDE_ASSERT( iduProperty::getDirectIOPageSize() <=
                SM_DBFILE_METAHDR_PAGE_SIZE );

    UInt sSizeToWrite = idlOS::align( aSrcSize,
                                      iduProperty::getDirectIOPageSize() );

    // Page 보다 큰 크기를 기록할 수 없다.
    IDE_ASSERT( sSizeToWrite <= SM_PAGE_SIZE );

    if ( sSizeToWrite > aSrcSize )
    {
        // Align된 크기가 더 클 경우, Buffer를 우선 0으로 초기화
        // 초기화되지 않은 영역을 Write하지 않도록 미연에 방지.
        idlOS::memsetOnMemCheck( aAlignedDst, 0, sSizeToWrite );
    }

    // Data 크기만큼 Align된 메모리 공간에 복사
    idlOS::memcpy( aAlignedDst, aSrc, aSrcSize );

    *aSizeToWrite = sSizeToWrite;

    return IDE_SUCCESS;
}

///**
//   Direct I/O를 위해 Disk Sector Size로 Align된 Buffer를 통해 File Write수행
//
//   [IN] aWhere  - write할 File의 Offset
//   [IN] aBuffer - write된 데이터가 복사될 Buffer
//   [IN] aSize   - write할 크기
// */
//IDE_RC
//smmDatabaseFile::writeDIO( PDL_OFF_T aWhere,
//                           void*     aBuffer,
//                           size_t    aSize )
//{
//    IDE_ASSERT( aBuffer != NULL );
//    IDE_ASSERT( aSize <= SM_PAGE_SIZE );
//    IDE_ASSERT( mAlignedPageBuffer != NULL );
//
//    UInt   sSizeToWrite;
//    UInt   sStage = 0;
//    idBool sCanDirectIO;
//    UChar *sWriteBuffer;
//
//    sCanDirectIO = mFile.canDirectIO( aWhere,
//                                      aBuffer,
//                                      aSize );
//
//    IDE_TEST( mPageBufferMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS );
//    sStage = 1;
//
//    if( sCanDirectIO == ID_FALSE)
//    {
//        sWriteBuffer = mAlignedPageBuffer;
//        IDE_TEST( copyDataForDirectWrite( mAlignedPageBuffer,
//                                          aBuffer,
//                                          aSize,
//                                          & sSizeToWrite )
//                  != IDE_SUCCESS );
//    }
//    else
//    {
//        sWriteBuffer = (UChar*)aBuffer;
//        sSizeToWrite = aSize;
//    }
//
//    IDE_TEST( mFile.write( NULL, aWhere, sWriteBuffer, sSizeToWrite )
//              != IDE_SUCCESS );
//
//    sStage = 0;
//    IDE_TEST( mPageBufferMutex.unlock() != IDE_SUCCESS );
//
//    return IDE_SUCCESS;
//
//    IDE_EXCEPTION_END;
//
//    IDE_PUSH();
//
//    switch( sStage )
//    {
//        case 1:
//            IDE_ASSERT( mPageBufferMutex.unlock() == IDE_SUCCESS );
//            break;
//        default :
//            break;
//    }
//
//    IDE_POP();
//
//    return IDE_FAILURE;
//}

/**
   Direct I/O를 위해 Disk Sector Size로 Align된 Buffer를 통해 File Write수행
   ( writeUntilSuccess버젼 )

   [IN] aWhere  - write할 File의 Offset
   [IN] aBuffer - write된 데이터가 복사될 Buffer
   [IN] aSize   - write할 크기
 */
IDE_RC
smmDatabaseFile::writeUntilSuccessDIO( PDL_OFF_T            aWhere,
                                       void*                aBuffer,
                                       size_t               aSize,
                                       iduEmergencyFuncType aSetEmergencyFunc,
                                       idBool               aKillServer )
{
    IDE_ASSERT( aBuffer != NULL );
    IDE_ASSERT( aSize <= SM_PAGE_SIZE );
    IDE_ASSERT( mAlignedPageBuffer != NULL );

    UInt    sSizeToWrite;
    UInt    sStage = 0;
    idBool  sCanDirectIO;
    UChar  *sWriteBuffer;

    sCanDirectIO = mFile.canDirectIO( aWhere,
                                      aBuffer,
                                      aSize );

    IDE_TEST( mPageBufferMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS );
    sStage = 1;

    if( sCanDirectIO == ID_FALSE)
    {
        sWriteBuffer = mAlignedPageBuffer;
        IDE_TEST( copyDataForDirectWrite( mAlignedPageBuffer,
                                          aBuffer,
                                          aSize,
                                          & sSizeToWrite )
                  != IDE_SUCCESS );
    }
    else
    {
        sWriteBuffer = (UChar*)aBuffer;
        sSizeToWrite = aSize;
    }

    IDE_TEST( mFile.writeUntilSuccess( NULL, /* idvSQL* */
                                       aWhere,
                                       sWriteBuffer,
                                       sSizeToWrite,
                                       aSetEmergencyFunc,
                                       aKillServer )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( mPageBufferMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1:
            IDE_ASSERT( mPageBufferMutex.unlock() == IDE_SUCCESS );
            break;
        default :
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}




IDE_RC
smmDatabaseFile::readPage( smmTBSNode * aTBSNode,
                           scPageID     aPageID,
                           UChar      * aPage )
{
    PDL_OFF_T         sOffset;

    //PRJ-1149, backup & media recovery.
    sOffset = ( SM_PAGE_SIZE * smmManager::getPageNoInFile( aTBSNode, aPageID ) ) 
              + SM_DBFILE_METAHDR_PAGE_SIZE;

    /* Direct IO로 IO가 수행될 수 있기 때문에 mAlignedPageBuffer를 이용해야 한다.*/
    IDE_TEST( mFile.read( NULL, /* idvSQL* */
                          sOffset,
                          aPage,
                          SM_PAGE_SIZE ) 
              != IDE_SUCCESS );

    if ( aPageID != smLayerCallback::getPersPageID( (void*)aPage ) )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                    SM_TRC_MEMORY_INVALID_PID_IN_PAGEHEAD1 );

        ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                    SM_TRC_MEMORY_INVALID_PID_IN_PAGEHEAD2,
                    (ULong) aPageID);

        ideLog::log( SM_TRC_LOG_LEVEL_MEMORY,
                     SM_TRC_MEMORY_INVALID_PID_IN_PAGEHEAD3,
                     (ULong)smLayerCallback::getPersPageID( (void*)aPage ) );

        IDE_ASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// For dumplf only !!!
IDE_RC
smmDatabaseFile::readPageWithoutCheck( smmTBSNode * aTBSNode,
                                       scPageID     aPageID,
                                       UChar      * aPage)
{
    PDL_OFF_T         sOffset;

    //PRJ-1149, backup & media recovery.
    sOffset = ( SM_PAGE_SIZE * smmManager::getPageNoInFile( aTBSNode, aPageID ) ) 
              + SM_DBFILE_METAHDR_PAGE_SIZE;

    /* Direct IO로 IO가 수행될 수 있기 때문에 mAlignedPageBuffer를 이용해야 한다.*/
    IDE_TEST( mFile.read( NULL, /* idvSQL* */
                          sOffset,
                          aPage,
                          SM_PAGE_SIZE ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


//IDE_RC
//smmDatabaseFile::readPage(smmTBSNode * aTBSNode, scPageID aPageID)
//{
//
//    void*            sPage;
//    PDL_OFF_T        sOffset;
//
//    IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
//                                            aPageID,
//                                            &sPage )
//                == IDE_SUCCESS );
//
//    //PRJ-1149, backup & media recovery.
//    sOffset = ( SM_PAGE_SIZE * smmManager::getPageNoInFile(
//                    aTBSNode, aPageID ) ) + SM_DBFILE_METAHDR_PAGE_SIZE;
//
//    /* Direct IO로 IO가 수행될 수 있기 때문에 mAlignedPageBuffer를 이용해야 한다.*/
//    IDE_TEST( mFile.read( NULL, /* idvSQL* */
//                             sOffset,
//                             sPage,
//                             SM_PAGE_SIZE )
//              != IDE_SUCCESS );
//
//    IDE_ASSERT( aPageID == smLayerCallback::getPersPageID( sPage ) );
//
//    return IDE_SUCCESS;
//
//    IDE_EXCEPTION_END;
//
//    return IDE_FAILURE;
//}

// PRJ-1149, memory DBF도 데이타파일 헤더가 추가되어서.
// smmManager::loadSerial에서 iduFile::read(PDL_OFF_T, void*, size_t,size_t*)
// 를 바로 호출하지 않기 위하여 추가된 함수.
IDE_RC smmDatabaseFile::readPages(PDL_OFF_T   aWhere,
                                  void*       aBuffer,
                                  size_t      aSize,
                                  size_t *    aReadSize)

{

    IDE_TEST ( mFile.read( NULL, /* idvSQL* */
                           aWhere + SM_DBFILE_METAHDR_PAGE_SIZE,
                           aBuffer,
                           aSize,
                           aReadSize )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#ifndef VC_WIN32
/* **************************************************************************
 * Description : AIO를 이용해 DBFile의 Page들을 읽어들인다.
 *
 *   aWhere          [IN] - DBFile로부터 읽어들일 위치
 *   aBuffer         [IN] - Page들을 저장할 Buffer
 *   aSize           [IN] - 읽어들일 사이즈
 *   aReadSize      [OUT] - 실제 읽어들인 양
 *   aAIOCount       [IN] - AIO 갯수
 *
 *   읽어들일 부분을 AIOCount만큼 분할하여 비동기 Read를 수행한다.
 *   각 AIO에 읽을 위치를 지정해 준 뒤 Read를 맞긴다.
 *   AIO가 Read 완료될 때까지 대기하게 된다.
 *
 * **************************************************************************/
IDE_RC smmDatabaseFile::readPagesAIO( PDL_OFF_T   aWhere,
                                      void*       aBuffer,
                                      size_t      aSize,
                                      size_t     *aReadSize,
                                      UInt        aAIOCount )

{

    UInt        i;
    ULong       sReadUnitSize;
    ULong       sOffset;
    ULong       sStartPos;
    iduFileAIO *sAIOUnit = NULL;
    ULong       sReadSum   = 0;
    SInt        sErrorCode = 0;
    UInt        sFreeCount = 0;
    UInt        sReadCount = 0;
    PDL_HANDLE  sFD;
    SInt        sState = 0;

    IDE_DASSERT( aSize > SMM_MIN_AIO_FILE_SIZE );
    IDE_DASSERT( (aSize % SM_PAGE_SIZE) == 0); // 언제나 페이지의 배수!

    sOffset   = 0;
    sStartPos = aWhere + SM_DBFILE_METAHDR_PAGE_SIZE;
    aAIOCount = IDL_MIN( aAIOCount, SMM_MAX_AIO_COUNT_PER_FILE );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMM,
                                 (ULong)ID_SIZEOF(iduFileAIO) * aAIOCount,
                                 (void **)&sAIOUnit,
                                 IDU_MEM_FORCE)
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * [1] Setup AIO Object
     * ----------------------------------------------*/
    IDE_TEST( mFile.allocFD( NULL /* idvSQL */, &sFD ) != IDE_SUCCESS );
    sState = 1;

    for (i = 0; i < aAIOCount; i++)
    {
        IDE_TEST( sAIOUnit[i].initialize( sFD ) != IDE_SUCCESS );
    }

    // 각 AIO가 읽어들일 Size를 계산한다.
    if( aAIOCount > 1 )
    {
        sReadUnitSize = idlOS::align(
                (aSize + aAIOCount) / aAIOCount, SM_PAGE_SIZE);

        aAIOCount = aSize / sReadUnitSize;

        if( (aSize % sReadUnitSize) == 0 )
        {
            // nothing
        }
        else
        {
            aAIOCount++;
        }
    }
    else
    {
        sReadUnitSize = aSize;
    }
    /* ------------------------------------------------
     * Request Asynchronous Read Operation
     * ----------------------------------------------*/

    while(1)
    {
        ULong sCalcSize;

        if( (aSize - sOffset) <= sReadUnitSize )
        {
            // calc remains which is smaller than sReadUnitSize
            sCalcSize = aSize - sOffset;

            // anyway..should be pagesize align.
            IDE_ASSERT( (sCalcSize % SM_PAGE_SIZE) == 0 );
        }
        else
        {
            sCalcSize = sReadUnitSize;
        }

        // 각 AIO마다 작업 분배
        if( sReadCount < aAIOCount )
        {
            if( sAIOUnit[sReadCount].read( sStartPos + sOffset,
                                           (SChar *)aBuffer + sOffset,
                                           sCalcSize) != IDE_SUCCESS )
            {
                IDE_ASSERT( sAIOUnit[sReadCount].getErrno() == EAGAIN );
                PDL_Time_Value sPDL_Time_Value;
                sPDL_Time_Value.initialize(0, IDU_AIO_FILE_AGAIN_SLEEP_TIME);
                idlOS::sleep( sPDL_Time_Value );
            }
            else
            {
                // on success
                sOffset  += sCalcSize;
                sReadSum += sCalcSize;
                sReadCount++;
            }
        }

        // Read 완료되기까지 대기.
        if( sFreeCount < sReadCount )
        {
            if( sAIOUnit[sFreeCount].isFinish(&sErrorCode) == ID_TRUE )
            {
                if( sErrorCode != 0 )
                {
                    ideLog::log( SM_TRC_LOG_LEVEL_FATAL,
                                 SM_TRC_MEMORY_AIO_FATAL,
                                 sErrorCode );
                    IDE_ASSERT( sErrorCode == 0 );
                }

                IDE_ASSERT( sAIOUnit[sFreeCount].join() == IDE_SUCCESS );
                sFreeCount++;
            }
        }
        else
        {
            if( sFreeCount == aAIOCount )
            {
                break;
            }

        }
    }
    IDE_ASSERT( sReadSum == aSize );

    IDE_ASSERT( iduMemMgr::free(sAIOUnit) == IDE_SUCCESS );
    sAIOUnit = NULL;

    *aReadSize = sReadSum;

    sState = 0;
    IDE_TEST( mFile.freeFD( sFD ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_PUSH();

        if( sAIOUnit != NULL )
        {
            IDE_ASSERT( iduMemMgr::free(sAIOUnit) == IDE_SUCCESS );
        }

        if( sState != 0 )
        {
            IDE_ASSERT( mFile.freeFD( sFD ) == IDE_SUCCESS );
        }

        IDE_POP();
    }

    return IDE_FAILURE;
}
#endif /* VC_WIN32 */


IDE_RC
smmDatabaseFile::writePage(smmTBSNode * aTBSNode, scPageID aPageID)
{

    void*            sPage;
    PDL_OFF_T        sOffset;
    UChar            sTornPageBuffer[ SM_PAGE_SIZE ];
    UInt             sMemDPFlushWaitOffset;
    UInt             sIBChunkID;
    smmChkptImageHdr sChkptImageHdr;

    //PRJ-1149, backup & media recovery.
    sOffset = ( SM_PAGE_SIZE * smmManager::getPageNoInFile( aTBSNode, aPageID ) ) 
              + SM_DBFILE_METAHDR_PAGE_SIZE;

    //PROJ-2133 incremental backup
    // change tracking을 수행한다. 
    if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
    {
        getChkptImageHdr( &sChkptImageHdr );

        if( smriChangeTrackingMgr::isAllocDataFileDescSlotID( 
                    &sChkptImageHdr.mDataFileDescSlotID ) == ID_TRUE )
        {
            sIBChunkID = smriChangeTrackingMgr::calcIBChunkID4MemPage(
                                                smmManager::getPageNoInFile( aTBSNode, aPageID));

            IDE_TEST( smriChangeTrackingMgr::changeTracking( NULL, 
                                                             this, 
                                                             sIBChunkID )
                      != IDE_SUCCESS );
        }
        else
        {
            // DatabaseFile이 생성될때 pageID 0번을 기록하게된다.
            // 이때 DataFileDescSlot이 할당되기 전에 pageID 0번이
            // 기록될 경우가 있는데 이경우에는 change tracking을 하지 않는다.
            // DatabaseFile 생성 후반 작업에서 DataFileDescSlot이 할당된다.
            IDE_ASSERT( aPageID == 0 );
        }
        
    }

    IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                            aPageID,
                                            &sPage )
                == IDE_SUCCESS );

    IDE_ASSERT( aPageID == smLayerCallback::getPersPageID( sPage ) );


    /* BUG-32386       [sm_recovery] If the ager remove the MMDB slot and the
     * checkpoint thread flush the page containing the slot at same time, the
     * server can misunderstand that the freed slot is the allocated slot. */
    if( smuProperty::getMemDPFlushWaitTime() > 0 )
    {
        if( ( smuProperty::getMemDPFlushWaitSID() == aTBSNode->mTBSAttr.mID ) &&
            ( smuProperty::getMemDPFlushWaitPID() == aPageID ) )
        {
            sMemDPFlushWaitOffset = smuProperty::getMemDPFlushWaitOffset();
            idlOS::memcpy( sTornPageBuffer,
                           sPage,
                           sMemDPFlushWaitOffset );
            idlOS::sleep( smuProperty::getMemDPFlushWaitTime() );
            idlOS::memcpy( sTornPageBuffer + sMemDPFlushWaitOffset,
                           ((UChar*)sPage) + sMemDPFlushWaitOffset,
                           SM_PAGE_SIZE - sMemDPFlushWaitOffset );
            sPage = (void*)sTornPageBuffer;
        }
    }

    /* Direct IO로 IO가 수행될 수 있기 때문에 mAlignedPageBuffer를 이용해야 한다.*/
    IDE_TEST( writeUntilSuccessDIO( sOffset,
                                    sPage,
                                    SM_PAGE_SIZE,
                                    smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
smmDatabaseFile::writePage( smmTBSNode * aTBSNode,
                            scPageID     aPageID,
                            UChar *      aPage)
{

    PDL_OFF_T         sOffset;

    //PRJ-1149, backup & media recovery.
    sOffset = (SM_PAGE_SIZE * smmManager::getPageNoInFile(
                   aTBSNode, aPageID ) ) + SM_DBFILE_METAHDR_PAGE_SIZE;

    /* Direct IO로 IO가 수행될 수 있기 때문에 mAlignedPageBuffer를 이용해야 한다.*/
    IDE_TEST( writeUntilSuccessDIO( sOffset,
                                    aPage,
                                    SM_PAGE_SIZE,
                                    smLayerCallback::setEmergency )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smmDatabaseFile::open()
{
    return mFile.open(((smuProperty::getIOType() == 0) ?
                          ID_FALSE : ID_TRUE));
}

/*
    새로운 Checkpoint Image파일을 생성할 Checkpoint Path를 리턴한다.

    [IN]  aTBSNode  - Checkpoint Path를 찾을 Tablespace Node
    [IN]  aDBFileNo - DB File(Checkpoint Image) 번호
    [OUT] aDBDir    - File을 생성할 DB Dir (Checkpoint Path)
 */
IDE_RC smmDatabaseFile::makeDBDirForCreate(smmTBSNode * aTBSNode,
                                           UInt         aDBFileNo,
                                           SChar      * aDBDir )
{
    UInt                sCPathCount ;
    smmChkptPathNode  * sCPathNode;

    IDE_DASSERT ( aTBSNode != NULL );
    IDE_DASSERT ( aDBDir != NULL );


    // BUGBUG-1548 Refactoring
    // Checkpoint Path관련 함수들을 별도의 class로 빼고
    // smmDatabaseFile보다 낮은 Layer에 둔다.

    // 운영중에는 Alter Tablespace Checkpoint Path가 불가능하므로
    // 동시성 제어를 신경 쓸 필요가 없다.

    IDE_TEST( smmTBSChkptPath::getChkptPathNodeCount( aTBSNode,
                                                      & sCPathCount )
              != IDE_SUCCESS );

    // fix BUG-26925 [CodeSonar::Division By Zero]
    IDE_ASSERT( sCPathCount != 0 );

    IDE_TEST( smmTBSChkptPath::getChkptPathNodeByIndex( aTBSNode,
                                                        aDBFileNo % sCPathCount,
                                                        & sCPathNode )
              != IDE_SUCCESS );

    IDE_ASSERT( sCPathNode != NULL );

#if defined(VC_WIN32)
    sctTableSpaceMgr::adjustFileSeparator( sCPathNode->mChkptPathAttr.mChkptPath );
#endif

    idlOS::snprintf(aDBDir, SM_MAX_FILE_NAME, "%s",
                    sCPathNode->mChkptPathAttr.mChkptPath );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE ;
}

/* ------------------------------------------------
 *
 * 모든 dir에서 file을 찾고,
 * 만약 있다면 file의 이름과 true를 리턴한다.
 * false라면 리턴된 file의 이름은 의미없다.
 * ----------------------------------------------*/
idBool smmDatabaseFile::findDBFile(smmTBSNode * aTBSNode,
                                   UInt         aPingPongNum,
                                   UInt         aFileNum,
                                   SChar  *     aFileName,
                                   SChar **     aFileDir)
{
    idBool              sFound = ID_FALSE;
    smuList           * sListNode ;
    smmChkptPathNode  * sCPathNode;

    IDE_DASSERT( aFileName != NULL );

    for (sListNode  = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );
         sListNode != & aTBSNode->mChkptPathBase ;
         sListNode  = SMU_LIST_GET_NEXT( sListNode ))
    {
        IDE_ASSERT( sListNode != NULL );

        sCPathNode = (smmChkptPathNode*) sListNode->mData;

        IDE_ASSERT( sCPathNode != NULL );

#if defined(VC_WIN32)
        sctTableSpaceMgr::adjustFileSeparator( sCPathNode->mChkptPathAttr.mChkptPath );
#endif

        // BUGBUG-1548 Tablespace Name이 NULL문자로 끝나는
        // 문자열이 아닌데도 NULL문자로 끝나는 문자열처럼 취급하고 있음
        idlOS::snprintf(aFileName,
                        SM_MAX_FILE_NAME,
                        SMM_CHKPT_IMAGE_NAME_WITH_PATH,
                        sCPathNode->mChkptPathAttr.mChkptPath,
                        aTBSNode->mTBSAttr.mName,
                        aPingPongNum,
                        aFileNum);

        if( idf::access(aFileName, F_OK) == 0 )
        {
            sFound = ID_TRUE;
            *aFileDir = (SChar*)sCPathNode->mChkptPathAttr.mChkptPath;
            break;
        }
    }

    return sFound;
}

/*
 * 특정 DB파일이 Disk에 있는지 확인한다.
 *
 * aPingPongNum [IN] - Ping/Ping DB번호 ( 0 or 1 )
 * aDBFileNo    [IN] - DB File의 번호 ( 0부터 시작하는 숫자 )
 */
idBool smmDatabaseFile::isDBFileOnDisk( smmTBSNode * aTBSNode,
                                        UInt         aPingPongNum,
                                        UInt         aDBFileNo )
{

    IDE_DASSERT( ( aPingPongNum == 0 ) || ( aPingPongNum == 1 ) );

    SChar   sDBFileName[SM_MAX_FILE_NAME];
    SChar  *sDBFileDir;
    idBool  sFound;

    sFound = findDBFile( aTBSNode,
                         aPingPongNum,
                         aDBFileNo,
                         (SChar*)sDBFileName,
                         &sDBFileDir);
    return sFound ;
}




/*
   데이타 파일 해더 갱신.
   PRJ-1149,  주어진 데이타 파일 노드의 헤더를 이용하여
   데이타 파일 갱신을 한다.
*/
IDE_RC smmDatabaseFile::flushDBFileHdr()
{
    PDL_OFF_T         sOffset;

    IDE_DASSERT( assertValuesOfDBFHdr( &mChkptImageHdr )
                 == IDE_SUCCESS );

    /* BUG-17825 Data File Header기록중 Process Failure시
     *           Startup안될 수 있음
     *
     * 위의 버그와 관련하여 Memory Checkpoint Image Header에
     * 대한 Atomic Write가 비정상 종료로 인해 깨진다하여도
     * Checkpoint 가 실패한 것이기 때문에 이전 Stable을 참고 하게 되므로
     * 문제가 발생하지 않는다. */

    sOffset = SM_DBFILE_METAHDR_PAGE_OFFSET;

    /* PROJ-2162 RestartRiskReduction
     * Consistency 할지 않으면, 강제로 MRDB Flush를 막는다. */
    if( ( smrRecoveryMgr::getConsistency() == ID_TRUE ) || 
        ( smuProperty::getCrashTolerance() == 2 ) )
    {
        IDE_TEST( writeUntilSuccessDIO( sOffset,
                                        &mChkptImageHdr,
                                        ID_SIZEOF(smmChkptImageHdr),
                                        smLayerCallback::setEmergency )
                  != IDE_SUCCESS );

        IDE_TEST( syncUntilSuccess() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   데이타베이스 파일 해더 읽음.
   aChkptImageHdr 가 NULL이 아니면 인자에 읽어들이고,
   NULL이면 mChkptImageHdr에 읽어들인다.

   [OUT] aChkptImageHdr : 데이타파일메타헤더 포인터
*/
IDE_RC smmDatabaseFile::readChkptImageHdr(
                            smmChkptImageHdr * aChkptImageHdr )
{
    smmChkptImageHdr * sChkptImageHdrBuffer ;

    if ( aChkptImageHdr != NULL )
    {
        sChkptImageHdrBuffer = aChkptImageHdr;
    }
    else
    {
        sChkptImageHdrBuffer = &mChkptImageHdr;

    }

    //PRJ-1149, backup & media recovery.
    IDE_TEST( readDIO( SM_DBFILE_METAHDR_PAGE_OFFSET,
                       sChkptImageHdrBuffer,
                       ID_SIZEOF( smmChkptImageHdr ) )
                   != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   데이타파일 HEADER에 체크포인트 정보를 기록한다.

   [IN] aMemRedoLSN         : 미디어복구에 필요한 Memory Redo LSN
   [IN] aMemCreateLSN       : 미디어복구에 필요한 Memory Create LSN
   [IN] aSpaceID            : Checkpoint Image가 속한 Tablespace의 ID
   [IN] aSmVersion          : 미디어복구에 필요한 바이너리 Version
   [IN] aDataFileDescSlotID : change tracking에 할당된 DataFileDescSlotID 
                              PROJ-2133
*/
void smmDatabaseFile::setChkptImageHdr( 
                            smLSN                    * aMemRedoLSN,
                            smLSN                    * aMemCreateLSN,
                            scSpaceID                * aSpaceID,
                            UInt                     * aSmVersion,
                            smiDataFileDescSlotID    * aDataFileDescSlotID )
{
    if ( aMemRedoLSN != NULL )
    {
        SM_GET_LSN( mChkptImageHdr.mMemRedoLSN, *aMemRedoLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    if ( aMemCreateLSN != NULL )
    {
        SM_GET_LSN( mChkptImageHdr.mMemCreateLSN, *aMemCreateLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    if ( aSpaceID != NULL )
    {
        mChkptImageHdr.mSpaceID = *aSpaceID;
    }

    if ( aSmVersion != NULL )
    {
        mChkptImageHdr.mSmVersion = *aSmVersion;
    }

    if ( aDataFileDescSlotID != NULL )
    {
        mChkptImageHdr.mDataFileDescSlotID.mBlockID = 
                                        aDataFileDescSlotID->mBlockID;
        mChkptImageHdr.mDataFileDescSlotID.mSlotIdx = 
                                        aDataFileDescSlotID->mSlotIdx;
    }

    return;
}

/*
  데이타파일 메터헤더 반환

  서버구동시 최초로 Loganchor로부터 초기화된다.

  [OUT] aChkptImageHdr - 데이타파일의 메타헤더를 반환한다.
*/
void smmDatabaseFile::getChkptImageHdr( smmChkptImageHdr * aChkptImageHdr )
{
    IDE_DASSERT( aChkptImageHdr != NULL );

    idlOS::memcpy( aChkptImageHdr,
                   &mChkptImageHdr,
                   ID_SIZEOF( smmChkptImageHdr ) );

    return;
}

/*
  데이타파일 속성 반환

  아래 함수에서는 smmManager::mCreateDBFileOnDisk를 참조하기 때문에
  만약 smmManager::mCreateDBFileOnDisk 변경이 필요하다면
  smmManager::setCreateDBFileOnDisk 호출후에
  getChkptImageAttr를 호출하여 순서를 지켜야 한다.

  [OUT] aChkptImageAttr - 데이타파일의 속성을 반환한다.

*/
void smmDatabaseFile::getChkptImageAttr(
                              smmTBSNode        * aTBSNode,
                              smmChkptImageAttr * aChkptImageAttr )
{
    UInt i;
    
    IDE_DASSERT( aTBSNode        != NULL );
    IDE_DASSERT( aChkptImageAttr != NULL );

    aChkptImageAttr->mAttrType    = SMI_CHKPTIMG_ATTR;

    aChkptImageAttr->mSpaceID     = mSpaceID;
    aChkptImageAttr->mFileNum     = mFileNum;

    //PROJ-2133 incremental backup
    if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) ||
         ( smLayerCallback::isCreatingCTFile() == ID_TRUE ) )
    {

        aChkptImageAttr->mDataFileDescSlotID.mBlockID = 
                                    mChkptImageHdr.mDataFileDescSlotID.mBlockID;
        aChkptImageAttr->mDataFileDescSlotID.mSlotIdx = 
                                    mChkptImageHdr.mDataFileDescSlotID.mSlotIdx;
    }
    else
    {
        aChkptImageAttr->mDataFileDescSlotID.mBlockID = 
                                    SMRI_CT_INVALID_BLOCK_ID;
        aChkptImageAttr->mDataFileDescSlotID.mSlotIdx = 
                                    SMRI_CT_DATAFILE_DESC_INVALID_SLOT_IDX;
    }

    SM_GET_LSN( aChkptImageAttr->mMemCreateLSN, mChkptImageHdr.mMemCreateLSN )

    // fix BUG-17343
    // Loganchor에 이미 저장되어 있다면 ID_TRUE를 설정한다.
    for ( i = 0 ; i < SMM_PINGPONG_COUNT; i++ )
    {
        aChkptImageAttr->mCreateDBFileOnDisk[ i ]
              = smmManager::getCreateDBFileOnDisk( aTBSNode,
                                                   i,
                                                   mFileNum );
    }

    return;
}


/*
  데이타파일의 메타헤더를 판독하여 미디어복구가
  필요한지 판단한다.

  데이타파일객체에 설정된 ChkptImageHdr(== Loganchor로부터 초기화됨)
  와 실제 데이타파일로부터 판독된 ChkptImageHdr를 비교하여
  미디어복구여부를 판단한다.

  [IN ] aChkptImageHdr - 데이타파일로부터 판독한 메타헤더
  [OUT] aNeedRecovery  - 미디어복구의 필요여부 반환
*/
IDE_RC smmDatabaseFile::checkValidationDBFHdr(
                             smmChkptImageHdr  * aChkptImageHdr,
                             idBool            * aIsMediaFailure )
{
    UInt               sDBVer;
    UInt               sCtlVer;
    UInt               sFileVer;
    idBool             sIsMediaFailure;

    IDE_ASSERT( aChkptImageHdr  != NULL );
    IDE_ASSERT( aIsMediaFailure != NULL );

    sIsMediaFailure = ID_FALSE;

    // Loganchor로부터 초기화된 ChkptImageHdr

    IDE_TEST ( readChkptImageHdr(aChkptImageHdr) != IDE_SUCCESS );

// XXX BUG-27058 replicationGiveup.sql에서 서버 비정상 종료로 인하여
// 임시로 다음 디버깅 Msg를 Release에서도 출력 되게 합니다.
// #ifdef DEBUG
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                SM_TRC_MRECOVERY_CHECK_DB_SID_PPID_FID,
                mSpaceID,
                mPingPongNum,
                mFileNum);

    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                "LogAnchor SpaceID=%"ID_UINT32_FMT", SmVersion=%"ID_UINT32_FMT", "
                "DBFileHdr SpaceID=%"ID_UINT32_FMT", SmVersion=%"ID_UINT32_FMT", ",
                mChkptImageHdr.mSpaceID,
                mChkptImageHdr.mSmVersion,
                aChkptImageHdr->mSpaceID,
                aChkptImageHdr->mSmVersion );

    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                SM_TRC_MRECOVERY_CHECK_MEMORY_RLSN,
                mChkptImageHdr.mMemRedoLSN.mFileNo,
                mChkptImageHdr.mMemRedoLSN.mOffset,
                aChkptImageHdr->mMemRedoLSN.mFileNo,
                aChkptImageHdr->mMemRedoLSN.mOffset);

    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                SM_TRC_MRECOVERY_CHECK_MEMORY_CLSN,
                mChkptImageHdr.mMemCreateLSN.mFileNo,
                mChkptImageHdr.mMemCreateLSN.mOffset,
                aChkptImageHdr->mMemCreateLSN.mFileNo,
                aChkptImageHdr->mMemCreateLSN.mOffset);
// #endif

    sDBVer   = smVersionID & SM_CHECK_VERSION_MASK;
    sCtlVer  = mChkptImageHdr.mSmVersion & SM_CHECK_VERSION_MASK;
    sFileVer = aChkptImageHdr->mSmVersion & SM_CHECK_VERSION_MASK;

    IDE_TEST_RAISE( checkValuesOfDBFHdr( &mChkptImageHdr )
                    != IDE_SUCCESS,
                    err_invalid_hdr );

    // [1] SM VERSION 확인
    // 데이타파일과 서버 바이너리의 호환성을 검사한다.
    IDE_TEST_RAISE(sDBVer != sCtlVer, err_invalid_hdr);
    IDE_TEST_RAISE(sDBVer != sFileVer, err_invalid_hdr);

    // [3] CREATE SN 확인
    // 데이타파일의 Create SN은 Loganchor의 Create SN과 동일해야한다.
    // 다르면 파일명만 동일한 완전히 다른 데이타파일이다.
    // CREATE LSN
    IDE_TEST_RAISE( smLayerCallback::isLSNEQ(
                        &mChkptImageHdr.mMemCreateLSN,
                        &aChkptImageHdr->mMemCreateLSN)
                    != ID_TRUE, err_invalid_hdr );

    // [4] Redo LSN 확인
    if ( smLayerCallback::isLSNLTE(
             &mChkptImageHdr.mMemRedoLSN,
             &aChkptImageHdr->mMemRedoLSN ) != ID_TRUE )
    {
        // 데이타 파일의 REDO LSN보다 Loganchor의
        // Redo LSN이 더 크면 미디어 복구가 필요하다.

        sIsMediaFailure = ID_TRUE; // 미디어 복구 필요
    }
    else
    {
        // [ 정상상태 ]
        // 데이타 파일의 REDO LSN보다 Loganchor의 Redo LSN이
        // 더 작거나 같다.
    }

#ifdef DEBUG
    if ( sIsMediaFailure == ID_FALSE )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                    SM_TRC_MRECOVERY_CHECK_DB_SUCCESS);
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                    SM_TRC_MRECOVERY_CHECK_NEED_MEDIARECOV);
    }
#endif

    *aIsMediaFailure = sIsMediaFailure;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_hdr );
    {
        /* BUG-33353 */
        ideLog::log( IDE_SM_0, 
                     "DBVer = %"ID_UINT32_FMT", "
                     "CtrVer = %"ID_UINT32_FMT", "
                     "FileVer = %"ID_UINT32_FMT"",
                     sDBVer,     
                     sCtlVer,    
                     sFileVer );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_CIMAGE_HEADER,
                                  (UInt)mSpaceID,
                                  (UInt)mPingPongNum,
                                  (UInt)mFileNum ) );
    }
    IDE_EXCEPTION_END;

#ifdef DEBUG
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY,
                SM_TRC_MRECOVERY_CHECK_DB_FAILURE);

#endif

    return IDE_FAILURE;
}

/*
   데이타파일 HEADER의 유효성을 검사한다.

   [IN] aChkptImageHdr : smmChkptImageHdr 타입의 포인터
*/
IDE_RC smmDatabaseFile::checkValuesOfDBFHdr(
                                   smmChkptImageHdr*  aChkptImageHdr )
{
    smLSN sCmpLSN;

    IDE_DASSERT( aChkptImageHdr != NULL );

    // REDO LSN과 CREATE LSN은 0이거나 ULONG_MAX 값을 가질 수 없다.
    SM_LSN_INIT(sCmpLSN);

    IDE_TEST( smLayerCallback::isLSNEQ( &aChkptImageHdr->mMemRedoLSN,
                                        &sCmpLSN ) == ID_TRUE );

    SM_LSN_MAX(sCmpLSN);

    IDE_TEST( smLayerCallback::isLSNEQ( &aChkptImageHdr->mMemRedoLSN,
                                        &sCmpLSN ) == ID_TRUE );

    IDE_TEST( smLayerCallback::isLSNEQ( &aChkptImageHdr->mMemCreateLSN,
                                        &sCmpLSN ) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
   데이타파일 HEADER의 유효성을 검사한다. (ASSERT버젼)

   [IN] aChkptImageHdr : smmChkptImageHdr 타입의 포인터
*/
IDE_RC smmDatabaseFile::assertValuesOfDBFHdr(
                          smmChkptImageHdr*  aChkptImageHdr )
{
    smLSN sCmpLSN;

    IDE_DASSERT( aChkptImageHdr != NULL );

    // REDO LSN과 CREATE LSN은 0이거나 ULONG_MAX 값을 가질 수 없다.
    SM_LSN_INIT(sCmpLSN);

    IDE_ASSERT( smLayerCallback::isLSNEQ(
                    &aChkptImageHdr->mMemRedoLSN,
                    &sCmpLSN ) == ID_FALSE );

// BUGBUG-1548 Memory LFG에만 log를 기록한 상황에서는 Disk LFG(0번)의 LSN이 0일수 있다.
/*
        IDE_TEST( smLayerCallback::isLSNEQ(
                         &aChkptImageHdr->mArrMemCreateLSN[ sLoop ],
                         &sCmpLSN ) == ID_TRUE );
*/

    SM_LSN_MAX(sCmpLSN);

    IDE_ASSERT( smLayerCallback::isLSNEQ(
                    &aChkptImageHdr->mMemRedoLSN,
                    &sCmpLSN ) == ID_FALSE );

    IDE_ASSERT( smLayerCallback::isLSNEQ(
                    &aChkptImageHdr->mMemCreateLSN,
                    &sCmpLSN ) == ID_FALSE );

    return IDE_SUCCESS;
}

/*
  PRJ-1548 User Memroy Tablespace 개념도입

  생성된 데이타파일에 대한 속성을 로그앵커에 추가한다.
*/
IDE_RC smmDatabaseFile::addAttrToLogAnchorIfCrtFlagIsFalse(
                                 smmTBSNode* aTBSNode )
{
    idBool                      sSaved;
    smmChkptImageAttr           sChkptImageAttr;
    smiDataFileDescSlotID     * sDataFileDescSlotID;
    smiDataFileDescSlotID       sDataFileDescSlotIDFromLogAnchor;

    IDE_DASSERT( aTBSNode != NULL );

    // smmManager::mCreateDBFileOnDisk[] 플래그는 TRUE로 설정하기 전에는
    // 반드시 FALSE 여야 한다.
    IDE_ASSERT( smmManager::getCreateDBFileOnDisk( aTBSNode,
                                                   mPingPongNum,
                                                   mFileNum )
                == ID_FALSE );

    // 이미 로그앵커에 저장되었는지 확인한다.
    sSaved = smmManager::isCreateDBFileAtLeastOne(
                (aTBSNode->mCrtDBFileInfo[ mFileNum ]).mCreateDBFileOnDisk ) ;

    // 파일이 생성되었으므로 생성여부를 설정한다.
    smmManager::setCreateDBFileOnDisk( aTBSNode,
            mPingPongNum,
            mFileNum,
            ID_TRUE );

    // 데이타파일 헤더로 부터 속성을 얻는다.
    // 아래 함수에서는 smmManager::mCreateDBFileOnDisk를 참조하기 때문에
    // 만약 변경이 필요하다면 smmManager::setCreateDBFileOnDisk 호출후에
    // getChkptImageAttr를 호출하여 순서를 지켜야 한다.
    getChkptImageAttr( aTBSNode, &sChkptImageAttr );

    if ( sSaved == ID_TRUE )
    {
        //PROJ-2133 incremental backup
        //Disk datafile생성과는 다르게 memory DatabaseFile의 생성은 checkpoint를
        //막고 수행되기때문에 별도로 checkpoint에대한 lock을 잡지않고
        //DataFileDescSlot을 할당한다.
        if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) ||
             ( smLayerCallback::isCreatingCTFile() == ID_TRUE ) )
        {
            IDE_ASSERT( smLayerCallback::getDataFileDescSlotIDFromLogAncho4ChkptImage( 
                            aTBSNode->mCrtDBFileInfo[ mFileNum ].mAnchorOffset,
                            &sDataFileDescSlotIDFromLogAnchor )
                        == IDE_SUCCESS );
                                                    
            sChkptImageAttr.mDataFileDescSlotID.mBlockID = 
                                    sDataFileDescSlotIDFromLogAnchor.mBlockID;
            sChkptImageAttr.mDataFileDescSlotID.mSlotIdx = 
                                    sDataFileDescSlotIDFromLogAnchor.mSlotIdx;

            mChkptImageHdr.mDataFileDescSlotID.mBlockID = 
                                    sDataFileDescSlotIDFromLogAnchor.mBlockID;
            mChkptImageHdr.mDataFileDescSlotID.mSlotIdx = 
                                    sDataFileDescSlotIDFromLogAnchor.mSlotIdx;

            //DataFileDescSlotID를 ChkptImageHdr에 기록한다.
            IDE_ASSERT( setDBFileHeader( NULL ) == IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }


        // fix BUG-17343
        // loganchor에 Stable/Unstable Chkpt Image에 대한 생성 정보를 저장
        IDE_ASSERT( smLayerCallback::updateChkptImageAttrAndFlush(
                           &(aTBSNode->mCrtDBFileInfo[ mFileNum ]),
                           &sChkptImageAttr )
                    == IDE_SUCCESS );
    }
    else
    {
        //PROJ-2133 incremental backup
        //Disk datafile생성과는 다르게 memory DatabaseFile의 생성은 checkpoint를
        //막고 수행되기때문에 별도로 checkpoint에대한 lock을 잡지않고
        //DataFileDescSlot을 할당한다.
        if ( ( smLayerCallback::isCTMgrEnabled() == ID_TRUE ) ||
             ( smLayerCallback::isCreatingCTFile() == ID_TRUE ) )
        {
            IDE_ASSERT( smriChangeTrackingMgr::addDataFile2CTFile( 
                                                       aTBSNode->mHeader.mID,
                                                       mFileNum,                             
                                                       SMRI_CT_MEM_TBS,
                                                       &sDataFileDescSlotID ) 
                        == IDE_SUCCESS ); 

            sChkptImageAttr.mDataFileDescSlotID.mBlockID = 
                                        sDataFileDescSlotID->mBlockID;
            sChkptImageAttr.mDataFileDescSlotID.mSlotIdx = 
                                        sDataFileDescSlotID->mSlotIdx;

            mChkptImageHdr.mDataFileDescSlotID.mBlockID = 
                                        sDataFileDescSlotID->mBlockID;
            mChkptImageHdr.mDataFileDescSlotID.mSlotIdx = 
                                        sDataFileDescSlotID->mSlotIdx;
            
            //DataFileDescSlotID를 ChkptImageHdr에 기록한다.
            IDE_ASSERT( setDBFileHeader( NULL ) == IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }


        // 최초 생성이기 때문에 로그앵커에 추가한다.
        IDE_ASSERT( smLayerCallback::addChkptImageAttrAndFlush(
                           &(aTBSNode->mCrtDBFileInfo[ mFileNum ]),
                           &(sChkptImageAttr) )
                    == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}


/*
  PRJ-1548 User Memroy Tablespace 개념도입

  미디어복구를 진행하기 위해 데이타파일에 플래그를 설정한다.
*/
IDE_RC smmDatabaseFile::prepareMediaRecovery(
                               smiRecoverType        aRecoveryType,
                               smmChkptImageHdr    * aChkptImageHdr,
                               smLSN               * aFromRedoLSN,
                               smLSN               * aToRedoLSN )
{
    smLSN  sInitLSN;

    IDE_DASSERT( aChkptImageHdr  != NULL );
    IDE_DASSERT( aFromRedoLSN != NULL );
    IDE_DASSERT( aToRedoLSN   != NULL );

    // 미디어오류 플래그를 ID_TRUE 설정한다.
    mIsMediaFailure = ID_TRUE;

    SM_LSN_INIT( sInitLSN );

    // [1] TO REDO LSN 결정하기
    if ( aRecoveryType == SMI_RECOVER_COMPLETE )
    {
        // 완전복구일 경우 파일헤더의 REDO LSN까지
        // 미디어 복구를 진행한다.
        SM_GET_LSN( *aToRedoLSN, mChkptImageHdr.mMemRedoLSN );
    }
    else
    {
        // 불완전 복구일경우 할수 있는데까지
        // 미디어 복구를 진행한다.
        IDE_ASSERT( ( aRecoveryType == SMI_RECOVER_UNTILCANCEL ) ||
                    ( aRecoveryType == SMI_RECOVER_UNTILTIME) );

        SM_LSN_MAX( *aToRedoLSN );
    }

    /*
      [2] FROM REDOLSN 결정하기
      만약 파일헤더의 REDOLSN이 INITLSN이라면 EMPTY
      데이타파일이다.
     */
    if ( smLayerCallback::isLSNEQ( &aChkptImageHdr->mMemRedoLSN,
                                   &sInitLSN) == ID_TRUE )
    {
        // 불완전복구 요구시 EMPTY 파일이 존재한다면
        // 완전복구를 수행하여야하기 때문에 에러처리한다.
        IDE_TEST_RAISE(
            (aRecoveryType == SMI_RECOVER_UNTILTIME) ||
            (aRecoveryType == SMI_RECOVER_UNTILCANCEL),
            err_incomplete_media_recovery);

        // EMPTY 데이타파일일 경우에는 CREATELSN
        // 부터 미디어복구를 진행한다.
        SM_GET_LSN( *aFromRedoLSN, mChkptImageHdr.mMemCreateLSN );
    }
    else
    {
        /* BUG-39354
         * 파일 헤더의 REDOLSN이 CREATELSN과 동일하면
         * EMPTY Memory Checkpoint Image파일이다.
         */
        if ( smLayerCallback::isLSNEQ( &aChkptImageHdr->mMemRedoLSN,
                                       &mChkptImageHdr.mMemCreateLSN ) == ID_TRUE )
        {
            /* 불완전복구 요구시 EMPTY 파일이 존재한다면
             완전복구를 수행하여야하기 때문에 에러처리한다. */
            IDE_TEST_RAISE(
                (aRecoveryType == SMI_RECOVER_UNTILTIME) ||
                (aRecoveryType == SMI_RECOVER_UNTILCANCEL),
                err_incomplete_media_recovery);
            
            /* EMPTY 데이타파일일 경우에는 CREATELSN
             * 부터 미디어복구를 진행한다. */
            SM_GET_LSN( *aFromRedoLSN, mChkptImageHdr.mMemCreateLSN );
        }
        else
        {
            // 완전복구 또는 불완전 복구에서의
            // FROM REDOLSN은 파일헤더의 REDO LSN부터
            // 진행한다.
            SM_GET_LSN( *aFromRedoLSN, aChkptImageHdr->mMemRedoLSN );
        }
    }

    // 미디어복구에서는
    // FROM REDOLSN < TO REDOLSN의 조건을 만족해야한다.
    IDE_ASSERT( smLayerCallback::isLSNGT( aToRedoLSN,
                                          aFromRedoLSN)
                == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_incomplete_media_recovery );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NeedMediaRecovery ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-28523 [SM] 체크 포인트 경로 삭제시 경로상에
 *               체크포인트 이미지가 존재하는지 검사해야 합니다.
 *               Checkpoint Path를 Drop하기 전에 해당 Path내의
 *               Checkpoint Image를 다른 유효한 경로로 옮겼는지를 확인한다.
 *
 *   aTBSNode          [IN] - Tablespace Node
 *   aCheckpointReason [IN] - Drop하려는 Checkpoint Path Node
 **********************************************************************/
IDE_RC smmDatabaseFile::checkChkptImgInDropCPath( smmTBSNode       * aTBSNode,
                                                  smmChkptPathNode * aDropPathNode )
{
    UInt               sStableDB;
    UInt               sDBFileNo;
    idBool             sFound;
    SChar              sDBFileName[SM_MAX_FILE_NAME];
    smuList          * sListNode ;
    smmChkptPathNode * sCPathNode;
    PDL_stat           sFileInfo;

    IDE_ASSERT( aTBSNode      != NULL );
    IDE_ASSERT( aDropPathNode != NULL );

    sStableDB = smmManager::getCurrentDB(aTBSNode);

    for ( sDBFileNo = 0;
          sDBFileNo <= aTBSNode->mLstCreatedDBFile;
          sDBFileNo++ )
    {
        // Drop하려는 경로를 제외한 다른 경로에서
        // Checkpoint Image File을 찾는다.

        sFound = ID_FALSE;

        for( sListNode  = SMU_LIST_GET_FIRST( & aTBSNode->mChkptPathBase );
             sListNode != & aTBSNode->mChkptPathBase ;
             sListNode  = SMU_LIST_GET_NEXT( sListNode ) )
        {
            IDE_ASSERT( sListNode != NULL );

            sCPathNode = (smmChkptPathNode*) sListNode->mData;

            IDE_ASSERT( sCPathNode != NULL );

#if defined(VC_WIN32)
            sctTableSpaceMgr::adjustFileSeparator( sCPathNode->mChkptPathAttr.mChkptPath );
            sctTableSpaceMgr::adjustFileSeparator( aDropPathNode->mChkptPathAttr.mChkptPath );
#endif

            // Drop 할 Path이외의 경로에 DB Img File이 존재 하여야 한다.
            if( idlOS::strcmp( sCPathNode->mChkptPathAttr.mChkptPath,
                               aDropPathNode->mChkptPathAttr.mChkptPath ) == 0 )
            {
                continue;
            }

            idlOS::snprintf(sDBFileName,
                            ID_SIZEOF( sDBFileName ),
                            SMM_CHKPT_IMAGE_NAME_WITH_PATH,
                            sCPathNode->mChkptPathAttr.mChkptPath,
                            aTBSNode->mTBSAttr.mName,
                            sStableDB,
                            sDBFileNo);

            if( idlOS::stat( sDBFileName, &sFileInfo ) == 0 )
            {
                sFound = ID_TRUE;
                break;
            }
        }

        if( sFound == ID_FALSE )
        {
            // Chkpt Img를 찾지 못했다면 이유는 둘 중 하나이다.
            // 1. Drop하려는 경로에 Chkpt Image가 있는 경우
            // 2. 파일이 아무곳에도 없는 경우

#if defined(VC_WIN32)
            sctTableSpaceMgr::adjustFileSeparator( aDropPathNode->mChkptPathAttr.mChkptPath );
#endif

            idlOS::snprintf(sDBFileName,
                            ID_SIZEOF( sDBFileName ),
                            SMM_CHKPT_IMAGE_NAME_WITH_PATH,
                            aDropPathNode->mChkptPathAttr.mChkptPath,
                            aTBSNode->mTBSAttr.mName,
                            sStableDB,
                            sDBFileNo);

            IDE_TEST_RAISE( idlOS::stat( sDBFileName, &sFileInfo ) == 0 ,
                            err_chkpt_img_in_drop_path );

            IDE_RAISE( err_file_not_exist );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_exist );
    {
        idlOS::snprintf(sDBFileName,
                        ID_SIZEOF( sDBFileName ),
                        SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG_FILENUM,
                        aTBSNode->mTBSAttr.mName,
                        sStableDB,
                        sDBFileNo);

        IDE_SET( ideSetErrorCode( smERR_ABORT_NoExistFile,
                                  (SChar*)sDBFileName ) );
    }
    IDE_EXCEPTION( err_chkpt_img_in_drop_path );
    {
        idlOS::snprintf(sDBFileName,
                        ID_SIZEOF( sDBFileName ),
                        SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG_FILENUM,
                        aTBSNode->mTBSAttr.mName,
                        sStableDB,
                        sDBFileNo);

#if defined(VC_WIN32)
        sctTableSpaceMgr::adjustFileSeparator( aDropPathNode->mChkptPathAttr.mChkptPath );
#endif

        IDE_SET( ideSetErrorCode( smERR_ABORT_DROP_CPATH_NOT_YET_MOVED_CIMG_IN_CPATH,
                                  (SChar*)sDBFileName,
                                  aDropPathNode->mChkptPathAttr.mChkptPath ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-29607 Create Memory Tablespace 시 동일한 이름의 파일이
 *               체크포인트 경로안에 존재하는지를 앞으로 생성될 파일명도
 *               감안해서 확인합니다.
 *
 *   aTBSNode          [IN] - Tablespace Node
 **********************************************************************/
IDE_RC smmDatabaseFile::chkExistDBFileByNode( smmTBSNode * aTBSNode )
{
    UInt              sCPathIdx;
    UInt              sCPathCnt;
    smmChkptPathNode *sCPathNode;

    IDE_TEST( smmTBSChkptPath::getChkptPathNodeCount( aTBSNode,
                                                      &sCPathCnt )
              != IDE_SUCCESS );

    IDE_ASSERT( sCPathCnt != 0 );

    for( sCPathIdx = 0 ; sCPathIdx < sCPathCnt ; sCPathIdx++ )
    {
        IDE_TEST( smmTBSChkptPath::getChkptPathNodeByIndex( aTBSNode,
                                                            sCPathIdx,
                                                            &sCPathNode )
                  != IDE_SUCCESS );

        IDE_ASSERT( sCPathNode != NULL );

#if defined(VC_WIN32)
        sctTableSpaceMgr::adjustFileSeparator( sCPathNode->mChkptPathAttr.mChkptPath );
#endif

        IDE_TEST( chkExistDBFile( aTBSNode->mHeader.mName,
                                  sCPathNode->mChkptPathAttr.mChkptPath )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-29607 Create DB 시 동일한 이름의 파일이
 *               체크포인트 경로안에 존재하는지를 앞으로 생성될 파일명도
 *               감안해서 확인합니다.
 *
 *   aTBSNode          [IN] - Tablespace Name
 **********************************************************************/
IDE_RC smmDatabaseFile::chkExistDBFileByProp( const SChar * aTBSName )
{
    UInt   sCPathIdx;

    for( sCPathIdx = 0 ; sCPathIdx < smuProperty::getDBDirCount() ; sCPathIdx++ )
    {
        IDE_TEST( chkExistDBFile( aTBSName,
                                  smuProperty::getDBDir( sCPathIdx ) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : BUG-29607 Create Memory Tablespace 시 비슷한 이름의 파일이
 *               체크포인트 경로안에 존재하는지 확인합니다.
 *
 *   aTBSName       [IN] - Tablespace Name
 *   aChkptPath     [IN] - Checkpoint Path
 **********************************************************************/
IDE_RC smmDatabaseFile::chkExistDBFile( const SChar * aTBSName,
                                        const SChar * aChkptPath )
{
    DIR           *sDir;
    SInt           sRc;
    SInt           sState = 0;
    UInt           sPingPongNum;
    UInt           sTokenLen;
    SChar*         sCurFileName = NULL;
    SChar          sChkFileName[2][SM_MAX_FILE_NAME];
    SChar          sInvalidFileName[SM_MAX_FILE_NAME];
    struct dirent *sDirEnt;
    struct dirent *sResDirEnt;

    IDE_ASSERT( aTBSName != NULL );
    IDE_ASSERT( aChkptPath  != NULL );

    /* smmDatabaseFile_chkExistDBFile_malloc_DirEnt.tc */
    IDU_FIT_POINT("smmDatabaseFile::chkExistDBFile::malloc::DirEnt");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMM,
                                 ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                 (void**)&sDirEnt,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );
    sState = 1;

    for( sPingPongNum = 0 ; sPingPongNum < SMM_PINGPONG_COUNT ; sPingPongNum++ )
    {
        idlOS::snprintf( (SChar*)&sChkFileName[ sPingPongNum ],
                         ID_SIZEOF( sChkFileName[ sPingPongNum ] ),
                         SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG,
                         aTBSName,
                         sPingPongNum );
    }

    sTokenLen = idlOS::strlen( (SChar*)sChkFileName[0] );
    IDE_ASSERT( idlOS::strlen( (SChar*)sChkFileName[1] ) == sTokenLen );

    sDir   = idf::opendir( aChkptPath );
    sState = 2;

    IDE_TEST_RAISE( sDir == NULL, invalid_path_error );

    while (1)
    {
	/*
         * BUG-31424 In AIX, createdb can't be executed because  
         *           the readdir_r function isn't handled well.
         */
	errno=0;

        // 파일명을 하나씩 가져와서 비교,
        sRc = idf::readdir_r( sDir,
                              sDirEnt,
                              &sResDirEnt );

        IDE_TEST_RAISE( (sRc != 0) && (errno != 0),
                        invalid_path_error );

        if (sResDirEnt == NULL)
        {
            break;
        }

        sCurFileName = sResDirEnt->d_name;

        for( sPingPongNum = 0 ;
             sPingPongNum < SMM_PINGPONG_COUNT ;
             sPingPongNum++ )
        {
            if ( idlOS::strncmp( sCurFileName,
                                 (SChar*)&sChkFileName[ sPingPongNum ],
                                 sTokenLen ) == 0 )
            {
                break;
            }
        }

        if( sPingPongNum == SMM_PINGPONG_COUNT )
        {
            // 파일명이 아예 비슷하지 않은경우 제외
            continue;
        }

        sCurFileName += sTokenLen;

        if( smuUtility::isDigitForString( sCurFileName,
                                          idlOS::strlen( sCurFileName ) )
            == ID_FALSE )
        {
            // 파일명 이후부분이 숫자인 경우가 아니면 제외.
            // Checkpoint Image File Number 확인.
            continue;
        }

        idlOS::snprintf( sInvalidFileName,
                         SM_MAX_FILE_NAME,
                         SMM_CHKPT_IMAGE_NAMES,
                         aTBSName );

        IDE_RAISE( exist_file_error );
    }

    sState = 1;
    idf::closedir(sDir);

    sState = 0;
    IDE_TEST( iduMemMgr::free( sDirEnt ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_path_error );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CPATH_NOT_EXIST,
                                  aChkptPath ));
    }
    IDE_EXCEPTION( exist_file_error );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyExistFile,
                                  sInvalidFileName ));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            idf::closedir(sDir);
        case 1:
            IDE_ASSERT( iduMemMgr::free( sDirEnt ) == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smmDataFile을 incremetalBackup한다.
 * PROJ-2133
 *
 **********************************************************************/
IDE_RC smmDatabaseFile::incrementalBackup( smriCTDataFileDescSlot * aDataFileDescSlot,
                                           smmDatabaseFile        * aDatabaseFile,
                                           smriBISlot             * aBackupInfo )
{
    iduFile               * sSrcFile; 
    iduFile                 sDestFile; 
    UInt                    sState = 0;
    ULong                   sBackupFileSize  = 0;
    ULong                   sBackupSize;
    ULong                   sIBChunkSizeInByte;            
    ULong                   sRestIBChunkSizeInByte;   
    idBool                  sCreateFileState = ID_FALSE;

    IDE_ASSERT( aDataFileDescSlot   != NULL );
    IDE_ASSERT( aDatabaseFile       != NULL );
    IDE_ASSERT( aBackupInfo         != NULL );

    sSrcFile = &aDatabaseFile->mFile;     

    IDE_TEST( sDestFile.initialize( IDU_MEM_SM_SMM,
                                    1,                             
                                    IDU_FIO_STAT_OFF,              
                                    IDV_WAIT_INDEX_NULL )          
              != IDE_SUCCESS );   
    sState = 1;

    IDE_TEST( sDestFile.setFileName( aBackupInfo->mBackupFileName ) 
              != IDE_SUCCESS );

    IDE_TEST( sDestFile.create()    != IDE_SUCCESS );
    sCreateFileState = ID_TRUE;

    IDE_TEST( sDestFile.open()      != IDE_SUCCESS );
    sState = 2;

    /* Incremental backup을 수행한다.*/
    IDE_TEST( smriChangeTrackingMgr::performIncrementalBackup( 
                                                   aDataFileDescSlot,
                                                   sSrcFile,
                                                   &sDestFile,
                                                   SMRI_CT_MEM_TBS,
                                                   aDatabaseFile->mSpaceID,
                                                   aDatabaseFile->mFileNum,
                                                   aBackupInfo )
              != IDE_SUCCESS );

    /* 
     * 생성된 파일의 크기와 복사된 IBChunk의 개수를 비교해 backup된 크기가
     * 동일한지 확인한다
     */
    IDE_TEST( sDestFile.getFileSize( &sBackupFileSize ) != IDE_SUCCESS );

    sIBChunkSizeInByte = (ULong)SM_PAGE_SIZE * aBackupInfo->mIBChunkSize;

    sBackupSize = (ULong)aBackupInfo->mIBChunkCNT * sIBChunkSizeInByte;

    /* 
     * 파일의 확장이나 축소가 IBChunk의 크기만큼 이루지지 않을수 있기때문에
     * 실제 백업된 파일의 크기는 mIBChunkCNT의 배수가 아닐수 있다.
     * 파일의 크기가 mIBChunkCNT의 배수가 되도록 조정해준다.
     */
    
    if( sBackupFileSize > SM_DBFILE_METAHDR_PAGE_SIZE )
    {
        sBackupFileSize -= SM_DBFILE_METAHDR_PAGE_SIZE;

        sRestIBChunkSizeInByte = (ULong)sBackupFileSize % sIBChunkSizeInByte;

        IDE_ASSERT( sRestIBChunkSizeInByte % SM_PAGE_SIZE == 0 );

        if( sRestIBChunkSizeInByte != 0 )
        {
            sBackupFileSize += sIBChunkSizeInByte - sRestIBChunkSizeInByte;
        }
    }

    IDE_ASSERT( sBackupFileSize == sBackupSize );

    sState = 1;
    IDE_TEST( sDestFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sDestFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sDestFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sDestFile.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    if( sCreateFileState == ID_TRUE )
    {
        IDE_ASSERT( idf::unlink( aBackupInfo->mBackupFileName ) 
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 복원된 chkptImage파일의 pingpong파일을 만들어야하는지 판단한다.
 * PROJ-2133
 *
 **********************************************************************/
IDE_RC smmDatabaseFile::isNeedCreatePingPongFile( smriBISlot * aBISlot,
                                                  idBool     * aResult )
{
    UInt                sDBVer;
    UInt                sCtlVer;
    UInt                sFileVer;
    idBool              sResult;
    smmChkptImageHdr    sChkptImageHdr;

    IDE_DASSERT( aBISlot != NULL );
    IDE_DASSERT( aResult != NULL );

    sResult = ID_TRUE;

    IDE_TEST ( readChkptImageHdrByPath( aBISlot->mBackupFileName,
                                        &sChkptImageHdr) != IDE_SUCCESS );

    IDE_TEST_RAISE( checkValuesOfDBFHdr( &mChkptImageHdr )
                    != IDE_SUCCESS,
                    err_invalid_hdr );

    sDBVer   = smVersionID & SM_CHECK_VERSION_MASK;
    sCtlVer  = mChkptImageHdr.mSmVersion & SM_CHECK_VERSION_MASK;
    sFileVer = sChkptImageHdr.mSmVersion & SM_CHECK_VERSION_MASK;

    // [1] SM VERSION 확인
    // 데이타파일과 서버 바이너리의 호환성을 검사한다.
    IDE_TEST_RAISE(sDBVer != sCtlVer, err_invalid_hdr);
    IDE_TEST_RAISE(sDBVer != sFileVer, err_invalid_hdr);

    // [3] CREATE SN 확인
    // 데이타파일의 Create SN은 Loganchor의 Create SN과 동일해야한다.
    // 다르면 파일명만 동일한 완전히 다른 데이타파일이다.
    // CREATE LSN
    IDE_TEST_RAISE( smLayerCallback::isLSNEQ(
                        &mChkptImageHdr.mMemCreateLSN,
                        &sChkptImageHdr.mMemCreateLSN)
                    != ID_TRUE, err_invalid_hdr );

    // [4] Redo LSN 확인
    if ( smLayerCallback::isLSNLTE(
             &mChkptImageHdr.mMemRedoLSN,
             &sChkptImageHdr.mMemRedoLSN ) != ID_TRUE )
    {
        // 데이타 파일의 REDO LSN보다 Loganchor의
        // Redo LSN이 더 크면 미디어 복구가 필요하다.

        sResult = ID_FALSE;
    }
    else
    {
        // [ 정상상태 ]
        // 데이타 파일의 REDO LSN보다 Loganchor의 Redo LSN이
        // 더 작거나 같다.
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_hdr );
    {
        IDE_SET( ideSetErrorCode(
                    smERR_ABORT_INVALID_CIMAGE_HEADER,
                    (UInt)mSpaceID,
                    (UInt)mPingPongNum,
                    (UInt)mFileNum ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
} 

/***********************************************************************
 * Description : 지정된 파일 path에 존재하는 chkptImage의 헤더를 읽어온다.
 * PROJ-2133
 *
 **********************************************************************/
IDE_RC smmDatabaseFile::readChkptImageHdrByPath( SChar            * aChkptImagePath,
                                                 smmChkptImageHdr * aChkptImageHdr )
{
    iduFile sFile;
    UInt    sState = 0;

    IDE_DASSERT( aChkptImagePath != NULL );
    IDE_DASSERT( aChkptImageHdr  != NULL );

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMM, 
                                1, /* Max Open FD Count */     
                                IDU_FIO_STAT_OFF,              
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sFile.setFileName( aChkptImagePath ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sFile.open() != IDE_SUCCESS , err_file_not_found);
    sState = 2;

    IDE_TEST( sFile.read( NULL, /*idvSQL*/               
                          SM_DBFILE_METAHDR_PAGE_OFFSET, 
                          (void*)aChkptImageHdr,
                          ID_SIZEOF( smmChkptImageHdr ) )
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sFile.close()   != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_not_found );                      
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileByPath,
                                  aChkptImagePath ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sFile.close()   == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}
