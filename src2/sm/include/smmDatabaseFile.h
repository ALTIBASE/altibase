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
 * $Id: smmDatabaseFile.h 85837 2019-07-14 23:44:48Z emlee $
 **********************************************************************/

#ifndef _O_SMM_DATABASE_FILE_H_
#define _O_SMM_DATABASE_FILE_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smriDef.h>
#include <smmDef.h>

class smmDatabaseFile
{
public:
    // To Fix BUG-18434
    //        alter tablespace online/offline 동시수행중 서버사망
    //
    // Page Buffer에 접근에 대한 동시성 제어를 위한 Mutex
    iduMutex mPageBufferMutex;
    
    UChar *mAlignedPageBuffer;
    UChar *mPageBufferMemPtr;

    // 화일이 저장된 경로
    SChar *mDir;

    // 데이타파일 속성
    scSpaceID           mSpaceID;       // 테이블스페이스 ID
    UInt                mPingPongNum;   // 핑퐁 번호
    UInt                mFileNum;       // 파일 번호

    // Media Recovery 자료구조
    smmChkptImageHdr    mChkptImageHdr;  // 데이타파일 메타헤더
    idBool              mIsMediaFailure; // 미디어오류여부 
    scPageID            mFstPageID;      // 데이타파일의 첫번째 PID
    scPageID            mLstPageID;      // 데이타파일의 마지막 PID

    // 미디어복구시작 Redo LSN
    smLSN               mFromRedoLSN;
public:
    //PROJ-2133 incremental backup
    iduFile     mFile;

private:
    // Disk에 DBFile을 생성하고 File을 open한다.
    IDE_RC createDBFileOnDiskAndOpen( idBool aUseDirectIO );
    // DBFile Header에 Checkpoint Image Header를 기록한다.
    IDE_RC setDBFileHeader( smmChkptImageHdr * aChkptImageHdr );

    // 체크포인트 이미지 파일에 Checkpoint Image Header가 기록된 경우 
    // 이를 읽어서 Tablespace ID가 일치하는지 비교하고 File을 Close한다.
    IDE_RC readSpaceIdAndClose( scSpaceID   aSpaceID,
                                idBool    * aIsHeaderWritten,
                                idBool    * aSpaceIdMatches);
    
    // 파일이 존재할 경우 삭제한다.
    // 존재하지 않을 경우 아무일도 하지 않는다.
    static IDE_RC removeFileIfExist( scSpaceID aSpaceID,
                                     const SChar * aFileName );


    // Direct I/O를 위해 Disk Sector Size로 Align된 Buffer를 통해
    // File Read수행
    IDE_RC readDIO(
               PDL_OFF_T  aWhere,
               void*      aBuffer,
               size_t     aSize);

//    // Direct I/O를 위해 Disk Sector Size로 Align된 Buffer를 통해
//    // File Write수행
//    IDE_RC writeDIO(
//               PDL_OFF_T aWhere,
//               void*     aBuffer,
//               size_t    aSize);

    // Direct I/O를 이용하여 write하기 위해 Align된 Buffer에 데이터 복사
    IDE_RC copyDataForDirectWrite(
               void * aAlignedDst,
               void * aSrc,
               UInt   aSrcSize,
               UInt * aSizeToWrite );

    // Direct I/O를 위해 Disk Sector Size로 Align된 Buffer를 통해
    // File Write수행  ( writeUntilSuccess버젼 )
    IDE_RC writeUntilSuccessDIO(
                PDL_OFF_T aWhere,
                void*     aBuffer,
                size_t    aSize,
                iduEmergencyFuncType aSetEmergencyFunc,
                idBool aKillServer = ID_FALSE);
    
public:

    smmDatabaseFile();

    IDE_RC initialize( scSpaceID    aSpaceID, 
                       UInt         aPingPongNum,
                       UInt         aFileNum,
                       scPageID   * aFstPageID = NULL,
                       scPageID   * aLstPageID = NULL );

    IDE_RC setFileName(SChar *aFilename);
    IDE_RC setFileName(SChar *aDir,
                       SChar *aDBName,
                       UInt   aPingPongNum,
                       UInt   aFileNum);

    const SChar* getFileName();

    IDE_RC syncUntilSuccess();

    ~smmDatabaseFile();
    IDE_RC destroy();

    /* Database파일을 생성한다.*/
    IDE_RC createDbFile (smmTBSNode       * aTBSNode,
                         SInt               aCurrentDB,
                         SInt               aDBFileNo,
                         UInt               aSize,
                         smmChkptImageHdr * aChkptImageHdr = NULL );

    // Checkpoint Image File을 Close하고 Disk에서 지워준다.
    IDE_RC closeAndRemoveDbFile(scSpaceID       aSpaceID, 
                                idBool          aRemoveImageFiles, 
                                smmTBSNode    * aTBSNode );
    
//    IDE_RC readPage     (smmTBSNode * aTBSNode, 
//                         scPageID aPageID );

    IDE_RC readPage     (smmTBSNode * aTBSNode,
                         scPageID     aPageID,
                         UChar *      aPage);

    IDE_RC readPageWithoutCheck(smmTBSNode * aTBSNode,
                                scPageID     aPageID,
                                UChar *      aPage);

    IDE_RC readPages(PDL_OFF_T   aWhere,
                     void*   aBuffer,
                     size_t  aSize,
                     size_t *aReadSize);

    IDE_RC readPagesAIO(PDL_OFF_T   aWhere,
                        void*   aBuffer,
                        size_t  aSize,
                        size_t *aReadSize,
                        UInt    aUnitCount);

    IDE_RC writePage    (smmTBSNode * aTBSNode, scPageID aNum);
    IDE_RC writePage    (smmTBSNode * aTBSNode, 
                         scPageID aNum, 
                         UChar *aPage);
    /************************************
    * PROJ-2133 incremental backup begin
    ************************************/
    IDE_RC createUntilSuccess(iduEmergencyFuncType aSetEmergencyFunc,
                              idBool               aKillServer);

    IDE_RC getFileSize(ULong *aFileSize);

    IDE_RC copy(idvSQL *aStatSQL ,SChar *aFileName);

    IDE_RC close();

    idBool exist();
    /************************************
    * PROJ-2133 incremental backup end
    ************************************/

    idBool isOpen();
    IDE_RC open();

    void setDir(SChar *aDir);
    const SChar* getDir();

    // 새로운 Checkpoint Image파일을 생성할 Checkpoint Path를 리턴한다.
    static IDE_RC makeDBDirForCreate(smmTBSNode * aTBSNode,
                                     UInt   aDBFileNo,
                                     SChar *aDBDir );
    

    static idBool findDBFile(smmTBSNode * aTBSNode,
                             UInt         aPingPongNum,
                             UInt         aFileNum,
                             SChar  *     aFileName,
                             SChar **     aFileDir);


    // 특정 DB파일이 Disk에 있는지 확인한다.
    static idBool isDBFileOnDisk( smmTBSNode * aTBSNode,
                                  UInt aPingPongNum,
                                  UInt aDBFileNo );

    // BUG-29607 Create DB, TBS 시 동일이름 파일이 존재하는지를
    //           앞으로 생성될 것 까지 감안해서 확인합니다.
    static IDE_RC chkExistDBFileByNode( smmTBSNode * aTBSNode );
    static IDE_RC chkExistDBFileByProp( const SChar * aTBSName );
    static IDE_RC chkExistDBFile( const SChar * aTBSName,
                                  const SChar * aChkptPath );

    ///////////////////////////////////////////////////////////////
    // PRJ-1548 User Memory TableSpace 개념 도입
    // 백업 & 미디어복구

    // 데이타파일 메터헤더 설정 
    void   setChkptImageHdr( smLSN                    * aMemRedoLSN, 
                             smLSN                    * aMemCreateLSN,
                             scSpaceID                * aSpaceID,
                             UInt                     * aSmVersion,
                             smiDataFileDescSlotID    * aDataFileDescSlotID );

    // 데이타파일 메터헤더 반환
    void   getChkptImageHdr( smmChkptImageHdr * aChkptImageHdr );

    // 데이타파일 속성 반환 
    void   getChkptImageAttr( smmTBSNode        * aTBSNode, 
                              smmChkptImageAttr * aChkptImageAttr );

    // 데이타파일 메타헤더를 기록한다. 
    IDE_RC flushDBFileHdr();

    // 데이타파일로부터 메타헤더를 판독한다.
    IDE_RC readChkptImageHdr( smmChkptImageHdr * aChkptImgageHdr );

    // 데이타파일의 메타헤더를 판독하여 미디어복구가 
    // 필요한지 판단한다.
    IDE_RC checkValidationDBFHdr( smmChkptImageHdr  * aChkptImageHdr,
                                  idBool            * aIsMediaFailure );

    // Drop 하려는 Checkpoint Path에서 Checkpoint Image가
    // 유효한 경로로 옮겨졌는지를 확인한다.
    static IDE_RC checkChkptImgInDropCPath( smmTBSNode       * aTBSNode,
                                            smmChkptPathNode * aDropPathNode );

    // 데이타파일 HEADER의 유효성을 검사한다. 
    IDE_RC checkValuesOfDBFHdr( smmChkptImageHdr  * aChkptImageHdr );


    // 데이타파일 HEADER의 유효성을 검사한다. (ASSERT버젼)
    IDE_RC assertValuesOfDBFHdr( smmChkptImageHdr*  aChkptImageHdr );
    
    //  미디어오류 플래그를 설정한다. 
    void   setIsMediaFailure( idBool   aFlag ) { mIsMediaFailure = aFlag; }
    //  미디어오류 플래그를 반환한다. 
    idBool getIsMediaFailure() { return mIsMediaFailure; }

    // 생성된 데이타파일에 대한 속성을 로그앵커에 추가한다. 
    IDE_RC addAttrToLogAnchorIfCrtFlagIsFalse( smmTBSNode * aTBSNode );

    // 미디어복구 플래그 설정 및 재수행 구간 계산하기
    IDE_RC prepareMediaRecovery( smiRecoverType        aRecoveryType, 
                                 smmChkptImageHdr    * aChkptImageHdr,
                                 smLSN               * aFromRedoLSN,
                                 smLSN               * aToRedoLSN );

    // 데이타파일 번호 반환
    UInt  getFileNum() { return mFileNum; }

    // 데이타파일 저장경로 반환
    SChar * getFileDir() { return mDir; }

    //테이블스페이스 번호 반환 
    scSpaceID getSpaceID() { return mSpaceID; }

    // 데이타파일의 페이지구간을 반환
    void  getPageRangeInFile( scPageID  * aFstPageID,
                              scPageID  * aLstPageID ) 
    { 
        *aFstPageID = mFstPageID;
        *aLstPageID = mLstPageID;
    }

    // 데이타파파일에 포함된  페이지 인지 체크한다. 
    idBool IsIncludePageInFile( scPageID  aPageID )
    {
        if ( (mFstPageID <= aPageID) && 
             (mLstPageID >= aPageID) )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    /************************************
    * PROJ-2133 incremental backup begin
    ************************************/
    static IDE_RC incrementalBackup( smriCTDataFileDescSlot * aDataFileDescSlot,
                                     smmDatabaseFile        * aDatabaseFile,
                                     smriBISlot             * aBackupInfo );

    static IDE_RC setDBFileHeaderByPath( SChar            * aChkptImagePath,
                                         smmChkptImageHdr * aChkptImageHdr );
           
    IDE_RC isNeedCreatePingPongFile( smriBISlot * aBISlot,
                                     idBool     * aResult );

    static IDE_RC readChkptImageHdrByPath( SChar            * aChkptImagePath,
                                           smmChkptImageHdr * aChkptImageHdr );
    /************************************
    * PROJ-2133 incremental backup end
    ************************************/
};

inline idBool smmDatabaseFile::isOpen()
{
    return ( mFile.getCurFDCnt() == 0 ) ? ID_FALSE : ID_TRUE;
}

inline IDE_RC smmDatabaseFile::getFileSize( ULong * aFileSize )
{
    return mFile.getFileSize( aFileSize );
}

inline IDE_RC smmDatabaseFile::close()
{
    return mFile.close();
}

inline idBool smmDatabaseFile::exist()
{
    return mFile.exist();
}

inline IDE_RC smmDatabaseFile::copy( idvSQL * aStatistics, 
                                     SChar  * aFileName )
{
    return mFile.copy( aStatistics, aFileName );
}


inline IDE_RC smmDatabaseFile::createUntilSuccess( 
                                    iduEmergencyFuncType aSetEmergencyFunc,
                                    idBool aKillServer  )
{
    return mFile.createUntilSuccess( aSetEmergencyFunc, aKillServer );
}

#endif  // _O_SMM_DATABASE_FILE_H_
