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
 * $Id: smiTableSpace.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMI_TABLE_SPACE_H_
# define _O_SMI_TABLE_SPACE_H_ 1

# include <smiDef.h>

class smiTrans;

class smiTableSpace
{
public:

    static IDE_RC createDiskTBS(idvSQL            * aStatistics,
                                smiTrans          * aTrans,
                                smiTableSpaceAttr * aTableSpaceAttr,
                                smiDataFileAttr  ** aDataFileAttrList,
                                UInt                aFileAttrCnt,
                                UInt                aExtPageCnt,
				                ULong               aExtSize);

    // 메모리 Tablespace를 생성한다.
    static IDE_RC createMemoryTBS(smiTrans             * aTrans,
                                  SChar                * aName,
                                  UInt                   aAttrFlag,
                                  smiChkptPathAttrList * aChkptPathList,
                                  ULong                  aSplitFileSize,
                                  ULong                  aInitSize,
                                  idBool                 aIsAutoExtend,
                                  ULong                  aNextSize,
                                  ULong                  aMaxSize,
                                  idBool                 aIsOnline,
                                  scSpaceID            * aTBSID );

    /* PROJ-1594 Volatile TBS */
    static IDE_RC createVolatileTBS( smiTrans   * aTrans,
                                     SChar      * aName,
                                     UInt         aAttrFlag,
                                     ULong        aInitSize,
                                     idBool       aIsAutoExtend,
                                     ULong        aNextSize,
                                     ULong        aMaxSize,
                                     UInt         aState,
                                     scSpaceID  * aTBSID );

    // 메모리/디스크 Tablespace를 Drop한다.
    static IDE_RC drop( idvSQL*      aStatistics,
                        smiTrans *   aTrans,
                        scSpaceID    aTblSpaceID,
                        smiTouchMode aTouchMode );



    // 메모리/디스크 TableSpace를 DISCARD한다
    static IDE_RC alterDiscard( scSpaceID aTableSpaceID );


    // ALTER TABLESPACE ONLINE/OFFLINE을 수행
    static IDE_RC alterStatus(idvSQL     * aStatistics,
                              smiTrans   * aTrans,
                              scSpaceID    aID,
                              UInt         aStatus);

    // Tablespace의 Attribute Flag를 변경한다.
    static IDE_RC alterTBSAttrFlag(smiTrans  * aTrans,
                                   scSpaceID   aTableSpaceID,
                                   UInt        aAttrFlagMask,
                                   UInt        aAttrFlagValue );
    
    static IDE_RC addDataFile(idvSQL           * aStatistics,
                              smiTrans*          aTrans,
                              scSpaceID          aID,
                              smiDataFileAttr ** aDataFileAttr,
                              UInt               aDataFileAttrCnt);


    static IDE_RC alterDataFileAutoExtend( smiTrans * aTrans,
                                           scSpaceID  aSpaceID,
                                           SChar *    aFileName,
                                           idBool     aAutoExtend,
                                           ULong      aNextSize,
                                           ULong      aMaxSize,
                                           SChar *    aValidFileName);

    // alter tablespace tbs-name alter datafile datafile-name online/offline
    static IDE_RC alterDataFileOnLineMode(scSpaceID   aSpaceID,
                                          SChar *     aFileName,
                                          UInt        aOnline);

    static  IDE_RC resizeDataFile(idvSQL    * aStatistics,
                                  smiTrans  * aTrans,
                                  scSpaceID   aTblSpaceID,
                                  SChar     * aFileName,
                                  ULong       aSizeWanted,
                                  ULong     * aSizeChanged,
                                  SChar     * aValidFileName);

    // alter tablespace rename datafile
    static IDE_RC  renameDataFile(scSpaceID  aTblSpaceID,
                                  SChar *    aOldFileName,
                                  SChar *    aNewFileName);

    static IDE_RC  removeDataFile(smiTrans * aTrans,
                                  scSpaceID aTblSpaceID,
                                  SChar*    aDataFile,
                                  SChar*    aValidFileName);

    // BUG-29812
    // 기존의 getAbsPath를 Memory TBS에서도 사용하도록 변경
    // 이를 위해 TBS를 Memory와 Disk로 구분하기 위해서 aTBSType을 추가
    static IDE_RC getAbsPath( SChar         * aRelName,
                              SChar         * aAbsName,
                              smiTBSLocation  aTBSLocation );

    static IDE_RC getAttrByID( scSpaceID           aTableSpaceID,
                               smiTableSpaceAttr * aTBSAttr );

    static IDE_RC getAttrByName( SChar             * aTableSpaceName,
                                 smiTableSpaceAttr * aTBSAttr );

    static IDE_RC existDataFile( scSpaceID         aTableSpaceID,
                                 SChar*            aDataFileName,
                                 idBool*           aExist);

    static IDE_RC existDataFile( SChar*            aDataFileName,
                                 idBool*           aExist);


    // fix BUG-13646
    static IDE_RC getExtentAnTotalPageCnt(scSpaceID  aTableSpaceID,
                                          UInt*      aExtentPageCount,
                                          ULong*     aTotalPageCount);

    // 시스템 테이블스페이스 여부 반환
    static idBool isSystemTableSpace( scSpaceID aSpaceID );


    static idBool isMemTableSpace( scSpaceID aTableSpaceID );
    static idBool isDiskTableSpace( scSpaceID  aTableSpaceID );
    static idBool isTempTableSpace( scSpaceID  aTableSpaceID );
    static idBool isVolatileTableSpace( scSpaceID aTableSpaceID );

    static idBool isMemTableSpaceType( smiTableSpaceType  aType );
    static idBool isVolatileTableSpaceType( smiTableSpaceType aType );
    static idBool isDiskTableSpaceType( smiTableSpaceType  aType );
    static idBool isTempTableSpaceType( smiTableSpaceType  aType );
    static idBool isDataTableSpaceType( smiTableSpaceType  aType );

    ////////////////////////////////////////////////////////////////////
    // PROJ-1548-M3 ALTER TABLESPACE
    ////////////////////////////////////////////////////////////////////
    //  ALTER TABLESPACE TBSNAME ADD CHECKPOINT PATH ... 를 실행
    static IDE_RC alterMemoryTBSAddChkptPath( scSpaceID      aSpaceID,
                                              SChar        * aChkptPath );

    // ALTER TABLESPACE TBSNAME RENAME CHECKPOINT PATH ... 를 실행
    static IDE_RC alterMemoryTBSRenameChkptPath( scSpaceID    aSpaceID,
                                                 SChar      * aOrgChkptPath,
                                                 SChar      * aNewChkptPath );


    //  ALTER TABLESPACE TBSNAME DROP CHECKPOINT PATH ... 를 실행
    static IDE_RC alterMemoryTBSDropChkptPath( scSpaceID    aSpaceID,
                                               SChar      * aChkptPath );

    // ALTER TABLESPACE TBSNAME AUTOEXTEND ... 를 수행한다
    static IDE_RC alterMemoryTBSAutoExtend(smiTrans*   aTrans,
                                           scSpaceID   aSpaceID,
                                           idBool      aAutoExtend,
                                           ULong       aNextSize,
                                           ULong       aMaxSize );

    /* PROJ-1594 Volatile TBS */
    static IDE_RC alterVolatileTBSAutoExtend(smiTrans*   aTrans,
                                             scSpaceID   aSpaceID,
                                             idBool      aAutoExtend,
                                             ULong       aNextSize,
                                             ULong       aMaxSize );

    ////////////////////////////////////////////////////////////////////////////
    // BUG-14897 sys_tbs_data에 파일 추가시 user tablespace의 설정을 참고합니다.
    ////////////////////////////////////////////////////////////////////////////
    // BUG-14897 - 속성 SYS_DATA_TBS_EXTENT_SIZE 값 리턴.
    static ULong  getSysDataTBSExtentSize();

    // BUG-14897 - 속성 SYS_DATA_FILE_INIT_SIZE 값 리턴.
    static ULong  getSysDataFileInitSize();

    // BUG-14897 - 속성 SYS_DATA_FILE_MAX_SIZE 값 리턴.
    static ULong  getSysDataFileMaxSize();

    // BUG-14897 - 속성 SYS_DATA_FILE_NEXT_SIZE 값 리턴.
    static ULong  getSysDataFileNextSize();

    // BUG-14897 - 속성 SYS_UNDO_TBS_EXTENT_SIZE 값 리턴.
    static ULong  getSysUndoTBSExtentSize();

    // BUG-14897 - 속성 SYS_UNDO_FILE_INIT_SIZE 값 리턴.
    static ULong  getSysUndoFileInitSize();

    // BUG-14897 - 속성 SYS_UNDO_FILE_MAX_SIZE 값 리턴.
    static ULong  getSysUndoFileMaxSize();

    // BUG-14897 - 속성 SYS_UNDO_FILE_NEXT_SIZE 값 리턴.
    static ULong  getSysUndoFileNextSize();

    // BUG-14897 - 속성 SYS_TEMP_TBS_EXTENT_SIZE 값 리턴.
    static ULong  getSysTempTBSExtentSize();

    // BUG-14897 - 속성 SYS_TEMP_FILE_INIT_SIZE 값 리턴.
    static ULong  getSysTempFileInitSize();

    // BUG-14897 - 속성 SYS_TEMP_FILE_MAX_SIZE 값 리턴.
    static ULong  getSysTempFileMaxSize();

    // BUG-14897 - 속성 SYS_TEMP_FILE_NEXT_SIZE 값 리턴.
    static ULong  getSysTempFileNextSize();

    // BUG-14897 - 속성 USER_DATA_TBS_EXTENT_SIZE 값 리턴.
    static ULong  getUserDataTBSExtentSize();

    // BUG-14897 - 속성 USER_DATA_FILE_INIT_SIZE 값 리턴.
    static ULong  getUserDataFileInitSize();

    // BUG-14897 - 속성 USER_DATA_FILE_MAX_SIZE 값 리턴.
    static ULong  getUserDataFileMaxSize();

    // BUG-14897 - 속성 USER_DATA_FILE_NEXT_SIZE 값 리턴.
    static ULong  getUserDataFileNextSize();

    // BUG-14897 - 속성 USER_TEMP_TBS_EXTENT_SIZE 값 리턴.
    static ULong  getUserTempTBSExtentSize();

    // BUG-14897 - 속성 USER_TEMP_FILE_INIT_SIZE 값 리턴.
    static ULong  getUserTempFileInitSize();

    // BUG-14897 - 속성 USER_TEMP_FILE_MAX_SIZE 값 리턴.
    static ULong  getUserTempFileMaxSize();

    // BUG-14897 - 속성 USER_TEMP_FILE_NEXT_SIZE 값 리턴.
    static ULong  getUserTempFileNextSize();

private:
    //OS limit,file header를 고려한 적절한 크기를 얻어내도록 함.
    static ULong  getValidSize4Disk( ULong aSizeInBytes );

};

#endif /* _O_SMI_TABLE_SPACE_H_ */
