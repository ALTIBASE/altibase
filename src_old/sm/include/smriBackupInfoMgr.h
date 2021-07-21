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
 * $Id$
 *
 * Description :
 *
 * - Backup Info Manager
 *
 **********************************************************************/

#ifndef _O_SMRI_BI_MANAGER_H_
#define _O_SMRI_BI_MANAGER_H_ 1

#include <smiDef.h>
#include <smriDef.h>

class smriBackupInfoMgr
{
private:

    /*BI파일 헤더*/
    static smriBIFileHdr      mBIFileHdr;

    /*BI slot메모리 공간에 대한 포인터*/
    static smriBISlot       * mBISlotArea;

    static idBool             mIsBISlotAreaLoaded;

    /*BI파일*/
    static iduFile            mFile;

    static iduMutex           mMutex;

private:

    /*BI파일 헤더 초기화*/
    static void initBIFileHdr();

    static IDE_RC checkBIFileHdrCheckSum();
    static void setBIFileHdrCheckSum();

    static IDE_RC checkBISlotCheckSum( smriBISlot *aBISlot );
    static void setBISlotCheckSum( smriBISlot * aBISlot );

    static IDE_RC processRestBackupFile( SChar    * aBackupPath,
                                         SChar    * aDescPath,
                                         iduFile  * aFile,
                                         idBool     aIsRemove );

    inline static UInt convertDayToSec( UInt aBackupInfoRetentionPeriod )
    {   return aBackupInfoRetentionPeriod * 60 * 60 * 24; };

    static  IDE_RC moveFile( SChar   * aSrcFilePath,        
                             SChar   * aDestFilePath,       
                             SChar   * aFileName,           
                             iduFile * aFile );
public:
    
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC createBIFile();
    static IDE_RC removeBIFile();

    static IDE_RC begin();
    static IDE_RC end();    

    static IDE_RC loadBISlotArea();
    static IDE_RC unloadBISlotArea();

    /*backup file삭제*/
    static IDE_RC removeBackupFile( SChar * aBackupFileName );

    /*BI파일 flush*/
    static IDE_RC flushBIFile( UInt aValidSlotIdx, smLSN * aLastBackupLSN );
    
    /*time기반 불완전 복구일때 해당 시간에 해당하는 BISlotIdx를 구한다.*/
    static IDE_RC findBISlotIdxUsingTime( UInt    UntilaTime, 
                                          UInt  * aTargetBackupTagSlotIdx );

    /*복원해야할 BISlotIdx를 구한다.*/
    static IDE_RC getRestoreTargetSlotIdx( SChar            * aUntilBackupTag,
                                           UInt               aStartScanBISlotIdx,
                                           idBool             aSearchUntilBackupTag,
                                           smriBIBackupTarget aBackupTarget,
                                           scSpaceID          aSpaceID,
                                           UInt             * aRestoreSlotIdx );

    /*Level1 backup file 복원을 수행할 BISlot의 범위를 구한다.*/
    static IDE_RC calcRestoreBISlotRange4Level1( 
                                         UInt            aScanStartSlotIdx,
                                         smiRestoreType  aRestoreType,
                                         UInt            aUltilTime,
                                         SChar         * aUntilBackupTag,
                                         UInt          * aRestoreStartSlotIdx,
                                         UInt          * aRestoreEndSlotIdx );

    /*BI파일을 백업한다.*/
    static IDE_RC backup( SChar * aBackupPath );

    /*한 BIslot을 BI파일에 추가한다.*/
    static IDE_RC appendBISlot( smriBISlot * aBISlot );

    /*기간이 지난 BIslot을 삭제*/
    static IDE_RC removeObsoleteBISlots();

    /*기간이 지나지않은 첫번째 BISlot의 Idx를 가져온다.*/
    static IDE_RC getValidBISlotIdx( UInt * aValidSlotIdx );

    /* incremental backup 파일을 지정된 위치로 이동시킨다. */
    static IDE_RC moveIncrementalBackupFiles( SChar * aMovePath, 
                                              idBool  aWithFile );

    /*BISlotIdx에 해당하는 BISlot을 가져온다.*/
    static IDE_RC getBISlot( UInt aBISlotIdx, smriBISlot ** aBISlot );

    /*BI를 backup파일 헤더에 저장한다.*/
    static void setBI2BackupFileHdr( smriBISlot * aBackupFileHdrBISlot, 
                                     smriBISlot * aBISlot );

    /*복원된 데이터파일 헤더에 존재하는 BI정보를 삭제한다.*/
    static void clearDataFileHdrBI( smriBISlot * aDataFileHdrBI );

    /*BI파일의 헤더를 가져온다.*/
    static IDE_RC getBIFileHdr( smriBIFileHdr ** aBIFileHdr );

    /* pathName이 유효한 절대경로인지 확인한다. */
    static IDE_RC isValidABSPath( idBool aCheckPerm, SChar  * aPathName );

    /* incremental backup이 실패할경우 backupinfo 파일을 백업이전  상태로
     * rollbackup한다. 
     */ 
    static IDE_RC rollbackBIFile( UInt aBISlotCnt );
    
    /* incremental backup에 실패한 backupDir을 삭제한다. */
    static IDE_RC removeBackupDir( SChar * aIncrementalBackupPath );

    static IDE_RC checkDBName( SChar * aDBName );

    static IDE_RC lock()
    { return mMutex.lock(NULL); }

    static IDE_RC unlock()
    { return mMutex.unlock(); }

};

#endif //_O_SMRI_BI_MANAGER_H_
