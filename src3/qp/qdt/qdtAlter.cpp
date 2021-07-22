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
 * $Id: qdtAlter.cpp 88963 2020-10-19 03:33:18Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qcmTableSpace.h>
#include <qdtAlter.h>
#include <qdtCommon.h>
#include <qdpPrivilege.h>
#include <smiTableSpace.h>
#include <qdpRole.h>

/*
    Disk Data tablespace, Disk Temp Tablespace용 ALTER구문에 대한
    SM의 Tablespace Type조회

    [IN] aQueryTBSType - SQL Parsing시 설정된 Tablespace type
    [IN] aStorageTBSType - SM에 저장된 실제 Tablespace type
 */
IDE_RC qdtAlter::isDiskTBSType(smiTableSpaceType aQueryTBSType,
                               smiTableSpaceType aStorageTBSType )
{
    // PROJ-1548 User Memory Tablespace
    //
    // Memory Tablespace에 Disk Tablespace용 ALTER구문이 들어오면 에러
    if ( ( aStorageTBSType != SMI_DISK_SYSTEM_DATA ) &&
         ( aStorageTBSType != SMI_DISK_SYSTEM_UNDO ) &&
         ( aStorageTBSType != SMI_DISK_SYSTEM_TEMP ) &&
         ( aStorageTBSType != SMI_DISK_USER_DATA )   &&
         ( aStorageTBSType != SMI_DISK_USER_TEMP ) )
    {
        // SM의 실제 Tablespace Type이 Disk Tablespace가 아니면 에러
        IDE_RAISE( ERR_INVALID_ALTER_ON_DISK_TBS );
    }

    //-----------------------------------------------------
    // To Fix BUG-10414
    // add 대상 file이 저장될 table space의 정당성 검사
    // ( data file은 data table space에
    //   temp file은 temp table space에 add 해야 함 )
    // 
    // Parsing 단계
    //    : 어떤 file( data file / temp file )을 add 할 것인가에 따라
    //      table space type 설정 
    //      < parsing 과정에서 설정되는 table space 종류 >
    //        SMI_DISK_USER_DATA : data file을 add 하려고 할 경우 
    //        SMI_DISK_USER_TEMP : temp file을 add 하려고 할 경우
    // Validation 단계
    //    : add할 file이 저장될 table space 공간의 정당성 검사 수행
    //      < qcmTablespace::getTBSAttrByName()에서 얻어오는 table space 종류 >
    //        SMI_MEMORY_SYSTEM_DICTIONARY
    //        SMI_MEMORY_SYSTEM_DATA,            
    //        SMI_MEMORY_USER_DATA,
    //        SMI_DISK_SYSTEM_DATA,
    //        SMI_DISK_USER_DATA,
    //        SMI_DISK_SYSTEM_TEMP,
    //        SMI_DISK_USER_TEMP,
    //        SMI_DISK_SYSTEM_UNDO,
    //        SMI_TABLESPACE_TYPE_MAX
    
    //        SMI_MEMORY_ALL_DATA
    //        SMI_DISK_SYSTEM_DATA
    //        SMI_DISK_USER_DATA
    //        SMI_DISK_SYSTEM_TEMP
    //        SMI_DISK_USER_TEMP
    //        SMI_DISK_SYSTEM_UNDO
    //        SMI_TABLESPACE_TYPE_MAX
    //-----------------------------------------------------
    if ( aQueryTBSType == SMI_DISK_USER_DATA )
    {
        // Data File을 Add 하려고 하는 경우
        if ( (aStorageTBSType == SMI_DISK_SYSTEM_DATA) ||
             (aStorageTBSType == SMI_DISK_USER_DATA) ||
             (aStorageTBSType == SMI_DISK_SYSTEM_UNDO) )
        {
            // nothing to do 
        }
        else
        {
            // Data File을 add 할 수 없는 TableSpace 임
            IDE_RAISE(ERR_MISMATCH_TBS_TYPE);
        }
    }
    else
    {
        // Temp File을 Add 하려고 하는 경우
        if ( (aStorageTBSType == SMI_DISK_SYSTEM_TEMP) ||
             (aStorageTBSType == SMI_DISK_USER_TEMP) )
        {
            // nothing to do
        }
        else
        {
            // Temp File을 add 할 수 없는 TableSpace임. 
            IDE_RAISE(ERR_MISMATCH_TBS_TYPE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_ALTER_ON_DISK_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_INVALID_ALTER_ON_DISK_TBS));
    }
    IDE_EXCEPTION(ERR_MISMATCH_TBS_TYPE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_MISMATCH_TBS_TYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Memory data tablespace용 ALTER구문에 대한
    SM의 Tablespace Type조회

    [IN] aQueryTBSType - SQL Parsing시 설정된 Tablespace type
    [IN] aStorageTBSType - SM에 저장된 실제 Tablespace type
 */
IDE_RC qdtAlter::isMemTBSType(smiTableSpaceType aTBSType)
{
    // PROJ-1548 User Memory Tablespace
    //
    // Disk Tablespace에 Memory Tablespace용 ALTER구문이 들어오면 에러
    if ( ( aTBSType != SMI_MEMORY_USER_DATA )         &&
         ( aTBSType != SMI_MEMORY_SYSTEM_DICTIONARY ) &&
         ( aTBSType != SMI_MEMORY_SYSTEM_DATA ) )
    {
        // SM의 실제 Tablespace Type이 Disk Tablespace이면 에러 
        IDE_RAISE( ERR_INVALID_ALTER_ON_MEM_TBS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_ALTER_ON_MEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_INVALID_ALTER_ON_MEM_TBS));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdtAlter::validateAddFile(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ADD ...FILE 의 validation 수행
 *
 * Implementation :
 *    1. 권한 검사 qdpPrivilege::checkDDLAlterTableSpacePriv()
 *    2. 테이블스페이스 이름 존재 유무 검사
 *    3. file specification validation 함수 호출 (CREATE TABLESPCE 와 동일)
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::validateAddFile"

    qdAlterTBSParseTree   * sParseTree;
    UInt                    sDataFileCnt;
    ULong                   sExtentSize;
    smiTableSpaceAttr       sTBSAttr;

    qdTBSFilesSpec        * sFilesSpec;
    smiDataFileAttr       * sFileAttr;
    idBool                  sFileExist = ID_FALSE;
    SChar                   sFileName[SMI_MAX_DATAFILE_NAME_LEN + 1];

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    // 동일한 tablespace name이 존재하는지 검사 
    IDE_TEST ( qcmTablespace::getTBSAttrByName(
        aStatement,
        sParseTree->TBSAttr->mName,
        sParseTree->TBSAttr->mNameLength,
        & sTBSAttr ) != IDE_SUCCESS );

    // To Fix BUG-10414
    // add 대상 file이 저장될 table space의 정당성 검사
    // Parse단계에서는 USER_DATE 혹은 USER_TEMP 둘중 하나로 세팅함
    // isDiskTBSType()은 USER_DATA 혹은 USER_TEMP중 하나만 받음.
    // 따라서 아직 정확한 타입을 세팅하면 안됨
    IDE_TEST( isDiskTBSType(  sParseTree->TBSAttr->mType,
                              sTBSAttr.mType )
              != IDE_SUCCESS );

    // 요기서 부터 정확한 TBS 타입 세팅하여야 함
    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    // To Fix PR-10780
    // Extent Size의 계산
    sExtentSize = (ULong) sTBSAttr.mDiskAttr.mExtPageCount *
        smiGetPageSize(sParseTree->TBSAttr->mType);

    // datafile의 크기 설정 및 auto extend절 정당성 검사  
    IDE_TEST( qdtCommon::validateFilesSpec( aStatement,
                                            sParseTree->TBSAttr->mType,
                                            sParseTree->diskFilesSpec,
                                            sExtentSize,
                                            & sDataFileCnt) != IDE_SUCCESS );

    // validate reuse
    for ( sFilesSpec = sParseTree->diskFilesSpec;
          sFilesSpec != NULL;
          sFilesSpec = sFilesSpec->next )
    {
        sFileAttr = sFilesSpec->fileAttr;

        IDE_TEST( smiTableSpace::getAbsPath( sFileAttr->mName,
                                             sFileName,
                                             SMI_TBS_DISK)
                  != IDE_SUCCESS );

        // Data File Name이 존재하는 지 검사
        IDE_TEST( qcmTablespace::existDataFileInDB( sFileName,
                                                    idlOS::strlen(sFileName),
                                                    & sFileExist )
                  != IDE_SUCCESS );

        if ( sFileExist == ID_TRUE )
        {
            //------------------------------------------
            // 존재하는 File Name임.
            //------------------------------------------

            // REUSE option일 경우
            // 자신의 TBS에 존재하는 file이라면 사용할 수 있다.
            if ( sFileAttr->mCreateMode == SMI_DATAFILE_REUSE )
            {
                // 자신의 TBS에 존재하는 File Name일 경우
                // REUSE 이므로 사용할 수 있다.
                IDE_TEST ( qcmTablespace::existDataFileInTBS(
                    sTBSAttr.mID,
                    sFileName,
                    idlOS::strlen(sFileName),
                    & sFileExist )
                           != IDE_SUCCESS );

                IDE_TEST_RAISE( sFileExist == ID_FALSE, ERR_EXIST_FILE );
            }
            else
            {
                // 이미 존재하는 File임
                IDE_RAISE( ERR_EXIST_FILE );
            }
        }
        else
        {
            // 존재하지 않는 File Name임.
            // Nothing To Do
        }
    }

    // To Fix BUG-10498
    sParseTree->fileCount = sDataFileCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_FILE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_EXIST_FILE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::validateRenameOrDropFile(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... RENAME/DROP ...FILE 의 validation 수행
 *
 * Implementation :
 *    1. 권한 검사 qdpPrivilege::checkDDLAlterTableSpacePriv()
 *    2. 테이블스페이스 이름 존재 유무 검사
 *    3. 구문에 DATAFILE 로 명시한 경우 temporary tablespace 이면 오류
 *       구문에 TEMPFILE 로 명시한 경우 data tablespace 이면 오류
 *    4. file 이 존재하는지 체크
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::validateRenameOrDropFile"

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    qdTBSFileNames         *sTmpFiles;
    idBool                  sFileExist;
    SChar                   sRelName[SMI_MAX_DATAFILE_NAME_LEN + 1];
    SChar                   sFileName[SMI_MAX_DATAFILE_NAME_LEN + 1];

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr->mName,
                  sParseTree->TBSAttr->mNameLength,
                  &sTBSAttr) != IDE_SUCCESS );

    // To Fix BUG-10414
    // add 대상 file이 저장될 table space의 정당성 검사
    IDE_TEST( isDiskTBSType(  sParseTree->TBSAttr->mType,
                              sTBSAttr.mType )
              != IDE_SUCCESS );

    for ( sTmpFiles=  sParseTree->newFileNames;
          sTmpFiles != NULL;
          sTmpFiles = sTmpFiles->next )
    {
        idlOS::memcpy( sRelName,
                       sTmpFiles->fileName.stmtText + sTmpFiles->fileName.offset + 1,
                       sTmpFiles->fileName.size - 2 );
        sRelName[sTmpFiles->fileName.size - 2] = 0;

        IDE_TEST( smiTableSpace::getAbsPath(
                    sRelName,
                    sFileName,
                    SMI_TBS_DISK)
                  != IDE_SUCCESS );

        IDE_TEST( qcmTablespace::existDataFileInDB(  sFileName,
                                                     idlOS::strlen(sFileName),
                                                     & sFileExist )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sFileExist == ID_TRUE, ERR_EXIST_FILE );
    }

    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_FILE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_EXIST_FILE));
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::validateModifyFile(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ALTER ...FILE  의 validation 수행
 *
 * Implementation :
 *    1. 권한 검사 qdpPrivilege::checkDDLAlterTableSpacePriv()
 *    2. 테이블스페이스 이름 존재 유무 검사
 *    3. 구문에 DATAFILE 로 명시한 경우 temporary tablespace 이면 오류
 *       구문에 TEMPFILE 로 명시한 경우 data tablespace 이면 오류
 *    4. file 이 존재하는지 체크
 *    5. autoextend clause validation 은 CREATE TABLESPACE 문과 동일
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::validateModifyFile"

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;
    UInt                    sDataFileCnt;
    ULong                   sExtentSize;
    SChar                   sRelName[SMI_MAX_DATAFILE_NAME_LEN+1];

    idBool                  sExist;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( qcmTablespace::getTBSAttrByName(
        aStatement,
        sParseTree->TBSAttr->mName,
        sParseTree->TBSAttr->mNameLength,
        & sTBSAttr ) != IDE_SUCCESS );

    // To Fix BUG-10414
    // add 대상 file이 저장될 table space의 정당성 검사
    // Parse단계에서는 USER_DATE 혹은 USER_TEMP 둘중 하나로 세팅함
    // isDiskTBSType()은 USER_DATA 혹은 USER_TEMP중 하나만 받음.
    // 따라서 아직 정확한 타입을 세팅하면 안됨
    IDE_TEST( isDiskTBSType(  sParseTree->TBSAttr->mType,
                              sTBSAttr.mType )
              != IDE_SUCCESS );

    // 요기서 부터 정확한 TBS 타입 세팅하여야 함
    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;


    idlOS::memset( sRelName, 0, SMI_MAX_DATAFILE_NAME_LEN + 1 );
    idlOS::memcpy( sRelName,
                   sParseTree->diskFilesSpec->fileAttr->mName,
                   sParseTree->diskFilesSpec->fileAttr->mNameLength );

    IDE_TEST( smiTableSpace::getAbsPath(
                        sRelName,
                        sParseTree->diskFilesSpec->fileAttr->mName,
                        SMI_TBS_DISK)
              != IDE_SUCCESS );

    sParseTree->diskFilesSpec->fileAttr->mNameLength =
        idlOS::strlen(sParseTree->diskFilesSpec->fileAttr->mName);

    IDE_TEST( qcmTablespace::existDataFileInTBS(
        sTBSAttr.mID,
        sParseTree->diskFilesSpec->fileAttr->mName,
        sParseTree->diskFilesSpec->fileAttr->mNameLength,
        & sExist )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NOT_EXIST_FILE );

    // To Fix PR-10780
    // Extent Size의 계산
    sExtentSize = (ULong) sTBSAttr.mDiskAttr.mExtPageCount *
        smiGetPageSize(sParseTree->TBSAttr->mType);

    IDE_TEST( qdtCommon::validateFilesSpec( aStatement,
                                            sParseTree->TBSAttr->mType,
                                            sParseTree->diskFilesSpec,
                                            sExtentSize,
                                            &sDataFileCnt)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_FILE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NOT_EXIST_FILE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::validateTBSOnlineOrOffline(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ONLINE/OFFLINE 의 execution 수행
 *
 * Implementation :
 *    1. 권한 검사 qdpPrivilege::checkDDLAlterTableSpacePriv()
 *    2. 테이블스페이스 이름 존재 유무 검사
 *    3. Alter Online / Offline가능한 테이블 스페이스인지 확인
 *
 ***********************************************************************/

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;
    qcmTableInfoList      * sTableInfoList = NULL;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );
    
    IDE_TEST( qcmTablespace::getTBSAttrByName(
        aStatement,
        sParseTree->TBSAttr->mName,
        sParseTree->TBSAttr->mNameLength,
        &sTBSAttr) != IDE_SUCCESS );

    IDE_TEST_RAISE( smiTableSpace::isSystemTableSpace( sTBSAttr.mID )
                    == ID_TRUE,
                    ERR_CANNOT_ONOFFLINE );

    IDE_TEST_RAISE( smiTableSpace::isTempTableSpace( sTBSAttr.mID )
                    == ID_TRUE,
                    ERR_CANNOT_ONOFFLINE );

    IDE_TEST_RAISE( smiTableSpace::isVolatileTableSpace( sTBSAttr.mID )
                    == ID_TRUE,
                    ERR_CANNOT_ONOFFLINE );

    // PROJ-1567
    // TBS에 있는 테이블 또는 파티션에 이중화가 걸려있을 경우
    // TBS를 Online 또는 Offline으로 변경할 수 없다.

    // 논-파티션드 테이블, 파티션드 테이블
    IDE_TEST( qcmTablespace::findTableInfoListInTBS(
                aStatement,
                sTBSAttr.mID,
                ID_TRUE,
                &sTableInfoList) != IDE_SUCCESS );

    for( ;
         sTableInfoList != NULL;
         sTableInfoList = sTableInfoList->next )
    {
        // if specified tables is replicated, the error
        IDE_TEST_RAISE(sTableInfoList->tableInfo->replicationCount > 0,
                       ERR_DDL_WITH_REPLICATED_TABLE);
        //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
        IDE_DASSERT(sTableInfoList->tableInfo->replicationRecoveryCount == 0);
    }

    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_CANNOT_ONOFFLINE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_CANNOT_ONOFFLINE));
    }
    IDE_EXCEPTION(ERR_DDL_WITH_REPLICATED_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DDL_WITH_REPLICATED_TBL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdtAlter::executeTBSOnlineOrOffline(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ONLINE/OFFLINE 의 execution 수행
 *
 * Implementation :
 *    1. smiTableSpace::setStatus() 함수 호출
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeTBSOnlineOrOffline"

    qdAlterTBSParseTree   * sParseTree;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    /*
        이 안에서 Tablespace에 X락 획득후
        Drop/Discard되었는지 체크한다.
        QP에서는 Execute 직전에 이러한 사항을 별도 체크할 필요 없다.
     */
    IDE_TEST( smiTableSpace::alterStatus(
                                 aStatement->mStatistics,
                                 (QC_SMI_STMT( aStatement ))->getTrans(),
                                 sParseTree->TBSAttr->mID,
                                 sParseTree->TBSAttr->mTBSStateOnLA)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeAddFile(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ADD ...FILE 의 execution 수행
 *
 * Implementation :
 *    1. smiTableSpace::addDataFile() 함수 호출
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeAddFile"

    qdAlterTBSParseTree   * sParseTree;
    smiDataFileAttr      ** sDataFileAttr = NULL;
    qdTBSFilesSpec        * sFileSpec;
    UInt                    i;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    //------------------------------------------------------------------
    // To Fix BUG-10498
    // SM에 내려줄 add 할 datafile들의 attribute 정보들을 생성한다.
    // ( Parse Tree에서 관리하는 data file attribute 구조와
    //   SM에서 요구하는 data file attribute 구조가 맞지 않기 때문에 )
    //
    // < Parse Tree에서 관리하는 attribute 정보 구조 >
    // ----------------------
    // | qdAlterTBSParseTree|
    // ----------------------   ------------------
    // |    fileSpec        |-->| qdTBSFilesSpec |
    // |     ...            |   ------------------   ------------------
    // ---------------------    |  fileAttr------|-->| smiDataFileAttr |
    //                          |  fileNo        |   ------------------
    //                          |  next--|       |   |  ...            |
    //                          ---------|-------    ------------------
    //                                   V
    //                          ------------------
    //                          | qdTBSFilesSpec |
    //                          ------------------    ------------------
    //                          |  fileAttr -----|--> | smiDataFileAttr |
    //                          |  fileNo        |    ------------------
    //                          |  next          |    |  ...            |
    //                          -----------------     ------------------
    //
    // < SM Interface에서 요구하는 attribute 정보 구조 >
    // (smiDataFileAttr*)의 배열
    //------------------------------------------------------------------
    
    IDU_FIT_POINT( "qdtAlter::executeAddFile::alloc::sDataFileAttr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aStatement->qmxMem->alloc(
            ID_SIZEOF(smiDataFileAttr*) * sParseTree->fileCount,
            (void **) & sDataFileAttr )
        != IDE_SUCCESS);

    i = 0;
    for ( sFileSpec = sParseTree->diskFilesSpec;
          sFileSpec != NULL;
          sFileSpec = sFileSpec->next )
    {
        sDataFileAttr[i++] = sFileSpec->fileAttr;
    }
    
    IDE_TEST(
        smiTableSpace::addDataFile( aStatement->mStatistics,
                                    (QC_SMI_STMT( aStatement))->getTrans(),
                                    sParseTree->TBSAttr->mID,
                                    sDataFileAttr,
                                    sParseTree->fileCount )
        != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeRenameFile(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... RENAME ...FILE 의 execution 수행
 *
 * Implementation :
 *    1. smiTableSpace::renameDataFile() 함수 호출
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeRenameFile"

    qdAlterTBSParseTree   * sParseTree;
    SChar                 * sSqlStr;
    SChar                 * sOldFileName;
    SChar                 * sNewFileName;
    qdTBSFileNames        * sOldFileNames;
    qdTBSFileNames        * sNewFileNames;
    smiTableSpaceAttr       sTBSAttr;
    qdTBSFileNames         *sTmpFiles;
    idBool                  sFileExist;
    SChar                   sRelName[SMI_MAX_DATAFILE_NAME_LEN + 1];
    SChar                   sFileName[SMI_MAX_DATAFILE_NAME_LEN + 1];

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;


    IDE_TEST( qcmTablespace::getTBSAttrByName(
        aStatement,
        sParseTree->TBSAttr->mName,
        sParseTree->TBSAttr->mNameLength,
        &sTBSAttr) != IDE_SUCCESS );

    // To Fix BUG-10414
    // add 대상 file이 저장될 table space의 정당성 검사
    IDE_TEST( isDiskTBSType(  sParseTree->TBSAttr->mType,
                              sTBSAttr.mType )
              != IDE_SUCCESS );

    for ( sTmpFiles=  sParseTree->newFileNames;
          sTmpFiles != NULL;
          sTmpFiles = sTmpFiles->next )
    {
        idlOS::memcpy( sRelName,
                       sTmpFiles->fileName.stmtText + sTmpFiles->fileName.offset + 1,
                       sTmpFiles->fileName.size - 2 );
        sRelName[sTmpFiles->fileName.size - 2] = 0;

        IDE_TEST( smiTableSpace::getAbsPath(
                    sRelName,
                    sFileName,
                    SMI_TBS_DISK )
                  != IDE_SUCCESS );

        IDE_TEST( qcmTablespace::existDataFileInDB(  sFileName,
                                                     idlOS::strlen(sFileName),
                                                     & sFileExist )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sFileExist == ID_TRUE, ERR_EXIST_FILE );
    }

    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    IDU_FIT_POINT( "qdtAlter::executeRenameFile::alloc::sOldFileName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    SMI_MAX_DATAFILE_NAME_LEN + 1,
                                    &sOldFileName)
             != IDE_SUCCESS);

    IDU_FIT_POINT( "qdtAlter::executeRenameFile::alloc::sNewFileName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    SMI_MAX_DATAFILE_NAME_LEN + 1,
                                    &sNewFileName)
             != IDE_SUCCESS);

    IDU_FIT_POINT( "qdtAlter::executeRenameFile::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    sOldFileNames = sParseTree->oldFileNames;
    sNewFileNames = sParseTree->newFileNames;

    while (sOldFileNames != NULL)
    {
        idlOS::memcpy((void*) sOldFileName,
                      sOldFileNames->fileName.stmtText + sOldFileNames->fileName.offset + 1,
                      sOldFileNames->fileName.size - 2);
        sOldFileName[ sOldFileNames->fileName.size - 2 ] = '\0';

        idlOS::memcpy((void*) sNewFileName,
                      sNewFileNames->fileName.stmtText + sNewFileNames->fileName.offset + 1,
                      sNewFileNames->fileName.size - 2);
        sNewFileName[ sNewFileNames->fileName.size - 2 ] = '\0';
        
        IDE_TEST( smiTableSpace::renameDataFile(
                          sParseTree->TBSAttr->mID,
                          sOldFileName, sNewFileName )
                  != IDE_SUCCESS );

        sOldFileNames = sOldFileNames->next;
        sNewFileNames = sNewFileNames->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_FILE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_EXIST_FILE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeModifyFileAutoExtend(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ALTER ...FILE AUTOEXTEND ... 의 execution 수행
 *
 * Implementation :
 *    1. 변경 속성에 따른 smiTableSpace 함수 호출
 *      1.1 autoextend 속성 변경시
 *          smiTableSpace::alterDataFileAutoExtend
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeModifyFileAutoExtend"

    qdAlterTBSParseTree   * sParseTree;
    SChar                 * sFileName;
    smiDataFileAttr       * sFileAttr;
    SChar                   sValidFileName[SMI_MAX_DATAFILE_NAME_LEN+1];

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;
    sFileAttr  = sParseTree->diskFilesSpec->fileAttr;

    IDU_FIT_POINT( "qdtAlter::executeModifyFileAutoExtend::alloc::sFileName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    sFileAttr->mNameLength + 1,
                                    &sFileName)
             != IDE_SUCCESS);

    idlOS::memcpy( (void*) sFileName,
                   (void*) sFileAttr->mName,
                   sFileAttr->mNameLength );
    sFileName[ sFileAttr->mNameLength ] = '\0';

    idlOS::memset(sValidFileName, 0x00, SMI_MAX_DATAFILE_NAME_LEN+1);
    IDE_TEST( smiTableSpace::alterDataFileAutoExtend(
                  (QC_SMI_STMT( aStatement ))->getTrans(),
                  sParseTree->TBSAttr->mID,
                  sFileName,
                  sFileAttr->mIsAutoExtend,
                  sFileAttr->mNextSize,
                  sFileAttr->mMaxSize,
                  (SChar*)sValidFileName)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeModifyFileSize(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... ALTER ...FILE AUTOEXTEND ... 의 execution 수행
 *
 * Implementation :
 *    1. 변경 속성에 따른 smiTableSpace 함수 호출
 *      1.2 SIZE 속성 변경시
 *          smiTableSpace::resizeDataFile
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeModifyFileSize"

    qdAlterTBSParseTree   * sParseTree;
    SChar                 * sFileName;
    smiDataFileAttr       * sFileAttr;
    SChar                   sValidFileName[SMI_MAX_DATAFILE_NAME_LEN+1];
    ULong                   sSizeChanged;
    

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;
    sFileAttr  = sParseTree->diskFilesSpec->fileAttr;

    IDU_FIT_POINT( "qdtAlter::executeModifyFileSize::alloc::sFileName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    sFileAttr->mNameLength + 1,
                                    &sFileName)
             != IDE_SUCCESS);

    idlOS::memcpy( (void*) sFileName,
                   (void*) sFileAttr->mName,
                   sFileAttr->mNameLength );
    sFileName[ sFileAttr->mNameLength ] = '\0';

    idlOS::memset(sValidFileName, 0x00, SMI_MAX_DATAFILE_NAME_LEN+1);
    
    IDE_TEST( smiTableSpace::resizeDataFile(
                  aStatement->mStatistics,
                  (QC_SMI_STMT( aStatement ))->getTrans(),
                  sParseTree->TBSAttr->mID,
                  sFileName,
                  sFileAttr->mInitSize,
                  &sSizeChanged,
                  (SChar*)sValidFileName)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeModifyFileOnOffLine(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description : PROJ-1548-M3-1
 *    ALTER TABLESPACE ... ALTER ...FILE AUTOEXTEND ... 의 execution 수행
 *
 * Implementation :
 *    1. 변경 속성에 따른 smiTableSpace 함수 호출
 *      1.3 FILE 의 ONLINE/OFFLINE 속성 변경시
 *          smiTableSpace::alterDataFileOnLineMode
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeModifyFileOnOffLine"

    qdAlterTBSParseTree   * sParseTree;
    SChar                 * sFileName;
    smiDataFileAttr       * sFileAttr;
    smiTableSpaceAttr       sTBSAttr;
    SChar                   sRelName[SMI_MAX_DATAFILE_NAME_LEN+1];
    idBool                  sExist;
    
    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;
    sFileAttr  = sParseTree->diskFilesSpec->fileAttr;
    
    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr->mName,
                  sParseTree->TBSAttr->mNameLength,
                  & sTBSAttr ) != IDE_SUCCESS );

    // To Fix BUG-10414
    // add 대상 file이 저장될 table space의 정당성 검사
    IDE_TEST( isDiskTBSType(  sParseTree->TBSAttr->mType,
                              sTBSAttr.mType )
              != IDE_SUCCESS );
    
    idlOS::memset( sRelName, 0, SMI_MAX_DATAFILE_NAME_LEN + 1 );
    idlOS::memcpy( sRelName,
                   sParseTree->diskFilesSpec->fileAttr->mName,
                   sParseTree->diskFilesSpec->fileAttr->mNameLength );

    IDE_TEST( smiTableSpace::getAbsPath(
                        sRelName,
                        sParseTree->diskFilesSpec->fileAttr->mName,
                        SMI_TBS_DISK )
              != IDE_SUCCESS );

    sParseTree->diskFilesSpec->fileAttr->mNameLength =
        idlOS::strlen(sParseTree->diskFilesSpec->fileAttr->mName);

    IDE_TEST( qcmTablespace::existDataFileInTBS(
        sTBSAttr.mID,
        sParseTree->diskFilesSpec->fileAttr->mName,
        sParseTree->diskFilesSpec->fileAttr->mNameLength,
        & sExist )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NOT_EXIST_FILE );

    // To Fix PR-10780
    // Extent Size의 계산

    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    IDU_LIMITPOINT("qdtAlter::executeModifyFileOnOffLine::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    sFileAttr->mNameLength + 1,
                                    &sFileName)
             != IDE_SUCCESS);

    idlOS::memcpy( (void*) sFileName,
                   (void*) sFileAttr->mName,
                   sFileAttr->mNameLength );
    sFileName[ sFileAttr->mNameLength ] = '\0';
    
    IDE_TEST( smiTableSpace::alterDataFileOnLineMode(
                          sParseTree->TBSAttr->mID,
                          sFileName,
                          sFileAttr->mState )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_FILE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NOT_EXIST_FILE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeDropFile(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE ... DROP ...FILE 의 execution 수행
 *
 * Implementation :
 *    1. smiTableSpace::removeDataFile() 함수 호출
 *
 ***********************************************************************/

#define IDE_FN "qdtAlter::executeDropFile"

    qdAlterTBSParseTree   * sParseTree;
    SChar                 * sSqlStr;
    SChar                 * sInputFileName;
    qdTBSFileNames        * sFileNames;
    UInt                    sRemoveCnt;
    SChar                   sValidFileName[SMI_MAX_DATAFILE_NAME_LEN+1];

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    IDU_FIT_POINT( "qdtAlter::executeDropFile::alloc::sInputFileName",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    SMI_MAX_DATAFILE_NAME_LEN + 1,
                                    &sInputFileName)
             != IDE_SUCCESS);

    IDU_FIT_POINT( "qdtAlter::executeDropFile::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    sRemoveCnt = 0;
    sFileNames = sParseTree->oldFileNames;

    while (sFileNames != NULL)
    {
        idlOS::memcpy((void*) sInputFileName,
                      sFileNames->fileName.stmtText + sFileNames->fileName.offset + 1,
                      sFileNames->fileName.size - 2);
        sInputFileName[ sFileNames->fileName.size - 2 ] = '\0';

        idlOS::memset(sValidFileName, 0x00, SMI_MAX_DATAFILE_NAME_LEN+1);
        IDE_TEST( smiTableSpace::removeDataFile(
                      (QC_SMI_STMT( aStatement ))->getTrans(),
                      sParseTree->TBSAttr->mID,
                      sInputFileName,
                      (SChar*)sValidFileName)
                  != IDE_SUCCESS );

        sRemoveCnt++;
        sFileNames = sFileNames->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeTBSBackup( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : To fix BUG-11952
 *    ALTER TABLESPACE [tbs_name] [BEGIN|END] BACKUP 의 execution 수행
 *
 * Implementation :
 *
 *
 ***********************************************************************/
#define IDE_FN "IDE_RC qdtAlter::executeTBSBackup"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // check privileges (sysdba만 가능)
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_IS_SYSDBA(aStatement) != ID_TRUE,
                    ERR_NO_GRANT_SYSDBA );
    
    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr->mName,
                  sParseTree->TBSAttr->mNameLength,
                  &sTBSAttr) != IDE_SUCCESS );

    // TODO: sm interface를 사용하여 구현해야 함
    switch( sParseTree->backupState )
    {
        case QD_TBS_BACKUP_BEGIN:
        {
            // code 추가
            IDE_TEST(smiBackup::beginBackupTBS( sTBSAttr.mID )
                     != IDE_SUCCESS);
            break;
            
        }
        case QD_TBS_BACKUP_END:    
        {
            // code 추가
            IDE_TEST(smiBackup::endBackupTBS( sTBSAttr.mID )
                     != IDE_SUCCESS);
            break;
        }
        default:
            IDE_DASSERT(0);
            break;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NO_GRANT_SYSDBA);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_SYSDBA_STR));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtAlter::executeAlterMemoryTBSChkptPath( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-1548-M3-1
 *    ALTER TABLESPACE TBSNAME ADD/RENAME/DROP CHECKPOINT PATH ...
 *    를 실행
 *
 * 들어올 수 있는 문장 종류:
 *    - ALTER TABLESPACE TBSNAME ADD CHECKPOINT PATH '/path/to/dir';
 *    - ALTER TABLESPACE TBSNAME RENAME CHECKPOINT PATH
 *         '/path/to/old' to '/path/to/new';
 *    - ALTER TABLESPACE TBSNAME DROP CHECKPOINT PATH '/path/to/dir';
 *
 * Implementation :
 *   (1) Tablespace이름 => Tablespace ID조회, 존재하는 Tablespace인지 체크
 *   (2) ADD/RENAME/DROP CHECKPOINT PATH 수행 
 *
 ***********************************************************************/

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;
    qdAlterChkptPath      * sAlterCPathClause;
    scSpaceID               sSpaceID;
    SChar                 * sFromChkptPath = NULL;
    SChar                 * sToChkptPath = NULL;
    smiChkptPathAttr      * sFromChkptPathAttr;
    smiChkptPathAttr      * sToChkptPathAttr;

    sParseTree        = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;
    sAlterCPathClause = sParseTree->memAlterChkptPath;

    // tablespace name 존재 유무 검사 
    IDE_TEST( qcmTablespace::getTBSAttrByName(
        aStatement,
        sParseTree->TBSAttr->mName,
        sParseTree->TBSAttr->mNameLength,
        & sTBSAttr ) != IDE_SUCCESS );

    // ALTER하려는 Tablespace가 Memory Tablespace인지 검사 
    IDE_TEST( isMemTBSType( sTBSAttr.mType )
              != IDE_SUCCESS );
    
    // Tablespace type을 정한다.
    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    // ADD/RENAME/DROP CHECKPOINT PATH 수행 
    sSpaceID = sParseTree->TBSAttr->mID ;

    sFromChkptPathAttr = sAlterCPathClause->fromChkptPathAttr;
    sToChkptPathAttr   = sAlterCPathClause->toChkptPathAttr;

    sFromChkptPath = sFromChkptPathAttr->mChkptPath; 

    // Memory TBS는 getAbsPath의 인자를 IN/OUT으로 사용하기 위해
    // 동일한 주소를 IN/OUT으로 각각 넘겨준다.
    IDE_TEST( smiTableSpace::getAbsPath( sFromChkptPath,
                                         sFromChkptPath,
                                         SMI_TBS_MEMORY )
              != IDE_SUCCESS );

    // BUG-29812
    // Memory TBS의 Checkpoint Path를 절대경로로 변환한다.
    if( sToChkptPathAttr != NULL )
    {
        sToChkptPath = sToChkptPathAttr->mChkptPath; 

        // Memory TBS는 getAbsPath의 인자를 IN/OUT으로 사용하기 위해
        // 동일한 주소를 IN/OUT으로 각각 넘겨준다.
        IDE_TEST( smiTableSpace::getAbsPath( sToChkptPath,
                                             sToChkptPath,
                                             SMI_TBS_MEMORY )
                  != IDE_SUCCESS );
    }

    switch( sAlterCPathClause->alterOp )
    {
        case QD_ADD_CHECKPOINT_PATH :
            IDE_TEST( smiTableSpace::alterMemoryTBSAddChkptPath(
                          sSpaceID,
                          sFromChkptPath )
                      != IDE_SUCCESS );
            break;


        case QD_RENAME_CHECKPOINT_PATH :
            IDE_TEST( smiTableSpace::alterMemoryTBSRenameChkptPath(
                          sSpaceID,
                          sFromChkptPath,
                          sToChkptPath )
                      != IDE_SUCCESS );
            break;
            
        case QD_DROP_CHECKPOINT_PATH :
            IDE_TEST( smiTableSpace::alterMemoryTBSDropChkptPath(
                          sSpaceID,
                          sFromChkptPath )
                      != IDE_SUCCESS );
            break;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdtAlter::executeAlterTBSDiscard( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-1548-M5-2
 *    ALTER TABLESPACE TBSNAME DISCARD 를 실행
 *
 * Implementation :
 *   (1) Tablespace이름 => Tablespace ID조회, 존재하는 Tablespace인지 체크
 *   (2) ALTER TABLESPACE TBSNAME DISCARD 수행
 *
 ***********************************************************************/

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    sParseTree        = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // tablespace name 존재 유무 검사 
    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr->mName,
                  sParseTree->TBSAttr->mNameLength,
                  & sTBSAttr ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sTBSAttr.mType != SMI_MEMORY_USER_DATA &&
                    sTBSAttr.mType != SMI_DISK_USER_DATA &&
                    sTBSAttr.mType != SMI_DISK_USER_TEMP,
                    ERR_CANNOT_DISCARD );
    
    sParseTree->TBSAttr->mID = sTBSAttr.mID;

    // ALTER TABLESPACE DISCARD 수행 
    IDE_TEST( smiTableSpace::alterDiscard( sParseTree->TBSAttr->mID )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CANNOT_DISCARD)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_CANNOT_DISCARD));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdtAlter::validateAlterMemVolTBSAutoExtend(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description : PROJ-1548-M3-2
 *    ALTER TABLESPACE TBSNAME AUTOEXTEND ... 의 validation 수행
 *
 * Implementation :
 *    (1) alter tablespace 권한이 있는지 검사
 *    (2) 동일한 tablespace name 존재 유무 검사
 *    ** 원래 이 함수는 memory TBS에 대해서만 고려되었으나,
 *       volatile TBS에 대해서도 고려되어야 하기 때문에
 *       함수 이름도 MemVol...로 바꾸었다.
 *       파싱 단계에서는 memory TBS인지 volatile TBS인지 알 수 없으며
 *       validation 단계인 본 함수에서 tbs ID로 타입을 얻어와서
 *       validation check를 수행한다. 
 ***********************************************************************/

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    // 동일한 tablespace name 존재 유무 검사 
    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr->mName,
                  sParseTree->TBSAttr->mNameLength,
                  & sTBSAttr ) != IDE_SUCCESS );

    // BUG-32255
    // Memory system TBS verification is the first. 
    IDE_TEST_RAISE( smiTableSpace::isSystemTableSpace( sTBSAttr.mID )
                    == ID_TRUE,
                    ERR_CANNOT_ALTER_SYSTEM_TABLESPACE );

    IDE_TEST_RAISE( (sTBSAttr.mType != SMI_MEMORY_USER_DATA) &&
                    (sTBSAttr.mType != SMI_VOLATILE_USER_DATA),
                    ERR_NEITHER_VOLATILE_NOR_MEMORY );

    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NEITHER_VOLATILE_NOR_MEMORY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_INVALID_ALTER_ON_MEM_OR_VOL_TBS));
    }
    IDE_EXCEPTION(ERR_CANNOT_ALTER_SYSTEM_TABLESPACE )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_CANNOT_ALTER_SYSTEM_TABLESPACE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdtAlter::executeAlterMemVolTBSAutoExtend( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-1548-M3-2
 *    ALTER TABLESPACE TBSNAME AUTOEXTEND ... 을 실행
 *
 * 들어올 수 있는 문장 종류:
 *    - ALTER TABLESPACE TBSNAME AUTOEXTEND OFF
 *    - ALTER TABLESPACE TBSNAME AUTOEXTEND ON NEXT 10M
 *    - ALTER TABLESPACE TBSNAME AUTOEXTEND ON MAXSIZE 10M/UNLIMITTED
 *    - ALTER TABLESPACE TBSNAME AUTOEXTEND ON NEXT 10M MAXSIZE 10M/UNLIMITTED
 * 
 * Implementation :
 *
 ***********************************************************************/

    qdAlterTBSParseTree   * sParseTree;
    smiDataFileAttr       * sFileAttr;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;
    sFileAttr  = sParseTree->diskFilesSpec->fileAttr;

    if (sParseTree->TBSAttr->mType == SMI_MEMORY_USER_DATA)
    {
        IDE_TEST( smiTableSpace::alterMemoryTBSAutoExtend(
                          (QC_SMI_STMT( aStatement ))->getTrans(),
                          sParseTree->TBSAttr->mID,
                          sFileAttr->mIsAutoExtend,
                          sFileAttr->mNextSize,
                          sFileAttr->mMaxSize )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( sParseTree->TBSAttr->mType == SMI_VOLATILE_USER_DATA);

        IDE_TEST( smiTableSpace::alterVolatileTBSAutoExtend(
                          (QC_SMI_STMT( aStatement ))->getTrans(),
                          sParseTree->TBSAttr->mID,
                          sFileAttr->mIsAutoExtend,
                          sFileAttr->mNextSize,
                          sFileAttr->mMaxSize )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *   Tablespace의 Attribute Flag 설정에 대한 Validation함수
 *
 * => 모든 Tablespace에 공통적으로 수행된다.
 *  
 * EX> Alter Tablespace MyTBS Alter LOG_COMPRESS ON;
 *
 * Implementation :
 *    (1) alter tablespace 권한이 있는지 검사
 *    (2) 동일한 tablespace name 존재 유무 검사
 ***********************************************************************/
IDE_RC qdtAlter::validateAlterTBSAttrFlag(qcStatement * aStatement)
{

    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    // 동일한 tablespace name 존재 유무 검사 
    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr->mName,
                  sParseTree->TBSAttr->mNameLength,
                  & sTBSAttr ) != IDE_SUCCESS );

    sParseTree->TBSAttr->mID = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    IDE_ASSERT( sParseTree->attrFlagToAlter != NULL );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *    Tablespace의 Attribute Flag 설정에 대한 실행함수
 *
 * => 모든 Tablespace에 공통적으로 수행된다.
 *
 * EX> Alter Tablespace MyTBS Alter LOG_COMPRESS ON;
 * 
 ***********************************************************************/
IDE_RC qdtAlter::executeAlterTBSAttrFlag( qcStatement * aStatement )
{

    qdAlterTBSParseTree   * sParseTree;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    IDE_ASSERT( sParseTree->attrFlagToAlter != NULL );
    
    IDE_TEST( smiTableSpace::alterTBSAttrFlag(
                          (QC_SMI_STMT( aStatement ))->getTrans(),
                          sParseTree->TBSAttr->mID,
                          sParseTree->attrFlagToAlter->attrMask,
                          sParseTree->attrFlagToAlter->attrValue )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Tablespace Rename에 대한 validation
 *
 * EX> ALTER TABLESPACE MyTBS RENAME TO YourTBS;
 *
 *    현재는 여기서 아무것도 할 것이 없고 모든 검사를 execution시에 한다.
 *    나중에 바뀔 수도 있으므로 함수는 남겨둠.
 ***********************************************************************/
IDE_RC qdtAlter::validateAlterTBSRename( qcStatement * /* aStatement */ )
{
    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description :
 *    Tablespace Rename에 대한 execution
 *
 * EX> ALTER TABLESPACE MyTBS RENAME TO YourTBS;
 *
 * <<CHECKLIST>>
 * 1.tablespace존재하는지
 * 2.system tablespace인지(system tablespace이면 rename 불가능)
 * 3.online인지(online이 아니면 rename 불가능. sm에서 체크)
 * 4.중복이름체크(이건 sm에서 체크가 되어야 함)
 ***********************************************************************/
IDE_RC qdtAlter::executeAlterTBSRename( qcStatement * aStatement )
{
    qdAlterTBSParseTree   * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    sParseTree = (qdAlterTBSParseTree *)aStatement->myPlan->parseTree;

    // 권한 검사
    IDE_TEST( qdpRole::checkDDLAlterTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    // tablespace존재해야 함.
    IDE_TEST( qcmTablespace::getTBSAttrByName(
                  aStatement,
                  sParseTree->TBSAttr[0].mName,
                  sParseTree->TBSAttr[0].mNameLength,
                  & sTBSAttr ) != IDE_SUCCESS );

    // system tablespace인지 검사.
    IDE_TEST_RAISE( smiTableSpace::isSystemTableSpace( sTBSAttr.mID )
                    == ID_TRUE,
                    ERR_SYS_TBS );

    /* To Fix BUG-17292 [PROJ-1548] Tablespace DDL시 Tablespace에 X락 잡는다
     * Tablespace에 Lock을 잡지 않고 Tablespace안의 Table을 조회하게 되면
     * 그 사이에 새로운 Table이 생겨날 수 있다.
     * 이러한 문제를 미연에 방지하기 위해
     * Tablespace에 X Lock을 먼저 잡고 Tablespace Validation/Execution을 수행 */
    IDE_TEST( smiValidateAndLockTBS(
                  QC_SMI_STMT( aStatement ),
                  sTBSAttr.mID,
                  SMI_TBS_LOCK_EXCLUSIVE,
                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

    
    // sm에서 해야 할 것으로 보이는 것.
    // online인지 offline인지(online이어야만 함)
    // 바꿀 이름이 이미 존재하는지
    // => atomic하게 tablespace 이름이 바뀌어야 하므로 QP에서 체크하기 어려움
    
    /////////////////////////////////////////////////
    // 여기에서 sm 인터페이스를 호출해야 함.
    // Error시 중복이름으로 인한 에러인지 offline에러인지
    // 다른 이유로 인한 에러인지 구분필요.
    /////////////////////////////////////////////////
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SYS_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_CANNOT_RENAME_SYS_TBS));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


