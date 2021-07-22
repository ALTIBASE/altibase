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
 * $Id: smrBackupMgr.h 86110 2019-09-02 04:52:04Z et16 $
 *
 * Description :
 *
 * 본 파일은 백업 관리자에 대한 헤더 파일이다.
 *
 * # 기능
 *   1.
 **********************************************************************/

#ifndef _O_SMR_BACKUP_MGR_H_
#define _O_SMR_BACKUP_MGR_H_ 1

#include <idu.h>
#include <idp.h>
#include <smrDef.h>
#include <sctTableSpaceMgr.h>
#include <smriDef.h>

class smrBackupMgr
{

public:

    static IDE_RC initialize();
    static IDE_RC destroy();

    static inline idBool isBackupingTBS( scSpaceID   aSpaceID );

    // ALTER DATABASE BACKUP LOGANCHOR .. 구문에 의해서
    // Loganchor파일 백업을 수행한다.
    static IDE_RC backupLogAnchor( idvSQL*  aStatistics,
                                   SChar  * aDestFilePath );

    // ALTER TABLESPACE BACKUP .. 구문에 의해서 TABLESPACE LEVEL의
    // 백업을 수행한다.
    static IDE_RC backupTableSpace(idvSQL*   aStatistics,
                                   void *    aTrans,
                                   scSpaceID aTbsID,
                                   SChar*    aBackupDir);

    // ALTER DATABASE BACKUP .. 구문에 의해서 DB LEVEL의
    // 전체백업을 수행한다.
    static IDE_RC backupDatabase(idvSQL* aStatistics,
                                 void  * aTrans,
                                 SChar * aBackupDir);

    // .. BACKUP BEGIN.. 구문에 의해서 테이블스페이스 상태의
    // 백업상태로 설정한다.
    static IDE_RC beginBackupTBS(scSpaceID aSpaceID);

    // .. BACKUP END.. 구문에 의해서 테이블스페이스 상태의
    // 백업상태를 해제한다.
    static IDE_RC endBackupTBS(scSpaceID aSpaceID);

    // 메모리 테이블스페이스의 실제 파일 백업을 진행한다.
    static IDE_RC backupMemoryTBS( idvSQL*      aStatistics,
                                   smmTBSNode * aSpaceNode,
                                   SChar     *  aBackupDir );

    // 디스크 테이블스페이스의 실제 파일 백업을 진행한다.
    static IDE_RC backupDiskTBS( idvSQL            * aStatistics,
                                 sddTableSpaceNode * aSpaceNode,
                                 SChar             * aBackupDir );

    // 백업완료를 위해서 로그파일을 강제로 아카이브시킨다.
    static IDE_RC swithLogFileByForces();

    //해당 path의 모든 memory db 관련 file들을 제거함
    static IDE_RC unlinkChkptImages( SChar* aPathName,
                                     SChar* aTBSName );

    // 해당 path의 disk db 관련 file을 제거함
    static IDE_RC unlinkDataFile( SChar*  aDataFileName );

    // 해당 path의 모든 disk db 관련 log file들을 제거함
    static IDE_RC unlinkAllLogFiles( SChar* aPathName );

    /************************************
    //PROJ-2133 incremental backup begin
    ************************************/
    static IDE_RC unlinkChangeTrackingFile( SChar * aChangeTrackingFileName ); 

    static IDE_RC unlinkBackupInfoFile( SChar * aBackupInfoFileName );

    static smrBackupState getBackupState();

    static IDE_RC incrementalBackupDatabase( idvSQL            * aStatistics,
                                             void              * aTrans,    
                                             SChar             * aBackupDir,
                                             smiBackuplevel      aBackupLevel,
                                             UShort              aBackupType,
                                             SChar             * aBackupTag );

    static IDE_RC incrementalBackupTableSpace( idvSQL            * aStatistics,
                                               void              * aTrans,    
                                               scSpaceID           aSpaceID,
                                               SChar             * aBackupDir,
                                               smiBackuplevel      aBackupLevel,
                                               UShort              aBackupType,
                                               SChar             * aBackupTag,
                                               idBool              aCheckTagName );

    static IDE_RC incrementalBackupMemoryTBS( idvSQL     * aStatistics,
                                              smmTBSNode * aSpaceNode,
                                              SChar      * aBackupDir,
                                              smriBISlot * aCommonBackupInfo );

    static IDE_RC incrementalBackupDiskTBS( idvSQL            * aStatistics,
                                            sddTableSpaceNode * aSpaceNode,
                                            SChar             * aBackupDir,
                                            smriBISlot        * aCommonBackupInfo );

    static IDE_RC setBackupInfoAndPath(
                                    SChar            * aBackupDir, 
                                    smiBackupLevel     aBackupLevel,
                                    UShort             aBackupType,
                                    smriBIBackupTarget aBackupTarget,
                                    SChar            * aBackupTag,
                                    smriBISlot       * aCommonBackupInfo,
                                    SChar            * aIncrementalBackupPath,
                                    idBool             aCheckTagName );
    /************************************
    //PROJ-2133 incremental backup end
    ************************************/
                                        

private:


    // ONLINE BACKUP 플래그에 설정하고자 하는 상태값을 설정한다.
    static void setOnlineBackupStatusOR( UInt  aOR );

    // ONLINE BACKUP 플래그에 해제하고자 하는 상태값을 해재한다.
    static void setOnlineBackupStatusNOT( UInt aNOT );

    /* 온라인 백업 진행상태 설정 */
    static inline idBool isRemainSpace(SInt  sSystemErrno);

    static IDE_RC beginBackupDiskTBS( idvSQL            * aStatistics,
                                      sddTableSpaceNode * aSpaceNode );

    static inline IDE_RC lock( idvSQL* aStatistics )
    { return mMtxOnlineBackupStatus.lock( aStatistics ); }
    static inline IDE_RC unlock() { return mMtxOnlineBackupStatus.unlock(); }

    //For Read Online Backup Status
    static iduMutex         mMtxOnlineBackupStatus;
    static SChar            mOnlineBackupStatus[256];
    static smrBackupState   mOnlineBackupState;

    // PRJ-1548 User Memory Tablespace
    static UInt             mBeginBackupDiskTBSCount;
    static UInt             mBeginBackupMemTBSCount;

    // PROJ-2133 Incremental Backup
    static UInt             mBackupBISlotCnt;
    static SChar            mLastBackupTagName[ SMI_MAX_BACKUP_TAG_NAME_LEN ];
};


/***********************************************************************
 * Description : 시스템 에러번호 분석 (Is enough space?)
 **********************************************************************/
inline idBool smrBackupMgr::isRemainSpace(SInt sSystemErrno)
{

    if ( sSystemErrno == ENOSPC)
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }

}

/***********************************************************************
 *
 * Description : 서버의 백업상태를 반환한다.
 *
 **********************************************************************/
inline smrBackupState smrBackupMgr::getBackupState()
{
    return mOnlineBackupState;
}

/***********************************************************************
 *
 * Description : 테이블스페이스가 Backup 상태인지 체크한다.
 *
 * aSpaceID  - [IN] 테이블스페이스 ID
 *
 **********************************************************************/
inline idBool smrBackupMgr::isBackupingTBS( scSpaceID   aSpaceID )
{
    // 백업관리자의 상태를 먼저 체크한다.
    if ( (getBackupState() & SMR_BACKUP_DISKTBS)
         != SMR_BACKUP_DISKTBS )
    {
        return ID_FALSE;
    }

    // 테이블스페이스의 상태를 체크한다.
    return sctTableSpaceMgr::isBackupingTBS( aSpaceID );
}

#endif /* _O_SMR_BACKUP_MGR_H_ */

