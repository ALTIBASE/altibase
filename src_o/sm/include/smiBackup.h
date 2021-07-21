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
 * $Id: smiBackup.h 82075 2018-01-17 06:39:52Z jina.kim $
 * Description :
 * 
 * 본 파일은 backup 관련 처리를 담당하는 class 헤더 파일입니다.
 *
 **********************************************************************/
#ifndef _O_SMI_BACKUP_H_
#define _O_SMI_BACKUP_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smriDef.h>

class smiTrans;

class smiBackup
{
public:
    
    // alter database [archivelog|noarchivelog];
    static IDE_RC alterArchiveMode( smiArchiveMode   aArchiveMode,
                                    idBool           aCheckPhase = ID_TRUE );

    // alter system archive log [start|stop] 수행
    static IDE_RC startArchThread();
    static IDE_RC stopArchThread();
    
    static IDE_RC backupLogAnchor( idvSQL   * aStatistics,
                                   SChar    * aDestFilePath );
    
    static IDE_RC backupTableSpace( idvSQL    * aStatistics,
                                    smiTrans  * aTrans,
                                    scSpaceID   aTbsID,
                                    SChar     * aBackupDir );
    
    static IDE_RC backupDatabase( idvSQL   * aStatistics,
                                  smiTrans * aTrans,
                                  SChar    * aBackupDir );
    
    static IDE_RC beginBackupTBS( scSpaceID aSpaceID );
    
    static IDE_RC endBackupTBS( scSpaceID aSpaceID );
    
    static IDE_RC switchLogFile();

    //PROJ-2133 incremental backup
    static IDE_RC incrementalBackupDatabase( idvSQL          * aStatistics,
                                             smiTrans        * aTrans,
                                             smiBackupLevel    aBackupLevel,
                                             UShort            aBackupType,
                                             SChar           * aBackupDir,
                                             SChar           * aBackupTag );
    
    //PROJ-2133 incremental backup
    static IDE_RC incrementalBackupTableSpace( idvSQL         * aStatistics,
                                               smiTrans       * aTrans,
                                               scSpaceID        aSpaceID,
                                               smiBackupLevel   aBackupLevel,
                                               UShort           aBackupType,
                                               SChar*           aBackupDir,
                                               SChar*           aBackupTag,
                                               idBool           aCheckTagName );

    static IDE_RC enableChangeTracking(); 
    static IDE_RC disableChangeTracking(); 
    static IDE_RC removeObsoleteBackupFile(); 
    static IDE_RC changeIncrementalBackupDir( SChar * aNewBackupDir );
    static IDE_RC moveBackupFile( SChar       * aMoveDir, 
                                  idBool        aWithFile );
    static IDE_RC removeBackupInfoFile(); 
};

#endif
