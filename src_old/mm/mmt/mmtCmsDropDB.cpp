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

#include <idl.h>
#include <smi.h>
#include <qci.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmtServiceThread.h>
#include <mmuProperty.h>


static IDE_RC doDropDatabase(idvSQL */*aStatistics*/, qciArgDropDB *aArg)
{
    SChar *sDBName;
    UInt   sDBNameLen;
    SChar *sCurFile;
    SChar  sLogAnchorPath[512];
    UInt   i;
    UInt   sShmDBKey;
    SChar *sLogAnchorDir;
    SChar  sValidLogAnchorName[SM_MAX_FILE_NAME];
    UInt   sValidLogAnchorNo;
    SChar  sOutputMsg[256];
    UInt   sLoop;

    iduFile           sLogAnchorFile;
    UInt              sCurOffset;
    ULong             sFileSize;
    smrLogAnchor      sLogAnchorHeader;
    smiTableSpaceAttr sTBSAttr;
    smiDataFileAttr   sDataFileAttr;
    smiChkptPathAttr  sChkptPathAttr;
    smiNodeAttrType   sAttrType;
    SChar**           sTBSNameArray = NULL;
    idBool            sAllocated = ID_FALSE;
    /* PROJ-2102  */
    smiSBufferFileAttr  sFileAttr;

    sOutputMsg[0] = 0;

    /* ---------------------------
     *  DB 화일명 길이 검사
     * --------------------------*/
    sDBName    = mmuProperty::getDbName();
    sDBNameLen = idlOS::strlen(sDBName);

    IDE_TEST_RAISE((sDBNameLen == 0) || (sDBNameLen >= (SInt)SM_MAX_DB_NAME), DBNameLengthError);

    // fix BUG-10470 DBName 검사
    IDE_TEST_RAISE(idlOS::strCaselessMatch(sDBName,
                                           sDBNameLen,
                                           aArg->mDBName,
                                           aArg->mDBNameLen) != IDE_SUCCESS,
                   DBNameError);

    /* -------------------------------------------
     *  SHM 존재 검사 : 존재하면 에러!!!
     * ------------------------------------------*/
    sShmDBKey = smuProperty::getShmDBKey();

    if (sShmDBKey != 0)
    {
        idBool            sShmExist;

        IDE_TEST(smmFixedMemoryMgr::checkExist(sShmDBKey, sShmExist, NULL)
                 != IDE_SUCCESS);

        IDE_TEST_RAISE(sShmExist != ID_FALSE, ShmAlreadyExistError);
    }

    IDE_CALLBACK_SEND_MSG("Checking Log Anchor files");
    IDE_TEST(smrLogAnchorMgr::checkAndGetValidAnchorNo(&sValidLogAnchorNo) != IDE_SUCCESS);

    for (i = 0; i < 3; i++)
    {
        sLogAnchorDir = (SChar *)smuProperty::getLogAnchorDir(i);

        idlOS::snprintf(sLogAnchorPath, ID_SIZEOF(sLogAnchorPath), "%s%c%s%d",
                        sLogAnchorDir,
                        IDL_FILE_SEPARATOR,
                        SMR_LOGANCHOR_NAME,
                        i);
        sCurFile = sLogAnchorPath;
        IDE_TEST_RAISE(idf::access(sLogAnchorPath, F_OK | R_OK | W_OK) != 0, FileExistError);
        idlOS::snprintf(sOutputMsg,
                        ID_SIZEOF(sOutputMsg), "[Ok] %s Exist. \n", sCurFile);
        IDE_CALLBACK_SEND_MSG(sOutputMsg);

        if (i == sValidLogAnchorNo)
        {
            idlOS::strcpy(sValidLogAnchorName, sLogAnchorPath);
        }
    }


    IDE_TEST( sLogAnchorFile.initialize(IDU_MEM_SM_SMR,
                                        1, /* Max Open FD Count */
                                        IDU_FIO_STAT_OFF,
                                        IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );
    
    IDE_TEST( sLogAnchorFile.setFileName(sValidLogAnchorName) != IDE_SUCCESS );
    IDE_TEST( sLogAnchorFile.open() != IDE_SUCCESS );
    IDE_TEST( sLogAnchorFile.getFileSize(&sFileSize) != IDE_SUCCESS );

    sCurOffset = 0;

    IDE_TEST( smrLogAnchorMgr::readLogAnchorHeader(&sLogAnchorFile,
                                                   &sCurOffset,
                                                   &sLogAnchorHeader)
              != IDE_SUCCESS );

    if ( sLogAnchorHeader.mNewTableSpaceID > 0 )
    {
        IDU_FIT_POINT( "mmtCmsDropDB::doDropDatabase::calloc::TBSNameArray" ); 

        //fix BUG-27437 MEM_FORCE를 쓰면 안된다.
        IDE_TEST( iduMemMgr::calloc(IDU_MEM_SM_SMR,
                                    sLogAnchorHeader.mNewTableSpaceID,
                                    ID_SIZEOF(SChar*),
                                    (void**)&sTBSNameArray,
                                    IDU_MEM_IMMEDIATE) != IDE_SUCCESS );
        sAllocated = ID_TRUE;

        /* --------------------------------------------------
           remove database(contained internal database file)
           --------------------------------------------------*/

        IDE_CALLBACK_SEND_MSG("Removing DB files");

        // 가변영역의 첫번째 Node Attribute Type을 판독한다.
        IDE_TEST( smrLogAnchorMgr::getFstNodeAttrType( &sLogAnchorFile,
                                                       &sCurOffset,
                                                       &sAttrType ) != IDE_SUCCESS);

        /* --------------------------------------------------
           remove database(contained internal database file)
           --------------------------------------------------*/
        while ( 1  )
        {
            switch( sAttrType )
            {
                case SMI_TBS_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readTBSNodeAttr(
                                      &sLogAnchorFile,
                                      &sCurOffset,
                                      &sTBSAttr)
                                  != IDE_SUCCESS );

                        IDE_ASSERT( sLogAnchorHeader.mNewTableSpaceID
                                    > sTBSAttr.mID );
                        IDE_ASSERT( sTBSNameArray[ sTBSAttr.mID ] == NULL );

                        IDU_FIT_POINT( "mmtCmsDropDB::doDropDatabase::calloc::TBSNameArrayID" );

                        // PRJ-1548 SM - User Memory TableSpace 개념도입
                        // 관련 Chkpt Images들을 완벽하게 삭제하기 위해서
                        // 테이블스페이스 이름을 Array에 Caching 한다.
                        //fix BUG-27437 MEM_FORCE를 쓰면 안된다.
                        IDE_TEST( iduMemMgr::calloc(
                                      IDU_MEM_SM_SMR,
                                      (SMI_MAX_TABLESPACE_NAME_LEN + 1),
                                      ID_SIZEOF(SChar),
                                      (void**)&sTBSNameArray[sTBSAttr.mID],
                                      IDU_MEM_IMMEDIATE) != IDE_SUCCESS );

                        idlOS::strncpy( sTBSNameArray[sTBSAttr.mID],
                                        sTBSAttr.mName,
                                        sTBSAttr.mNameLength );

                        sTBSNameArray[sTBSAttr.mID][sTBSAttr.mNameLength] = '\0';

                        break;
                    }
                case SMI_DBF_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readDBFNodeAttr(
                                      &sLogAnchorFile,
                                      &sCurOffset,
                                      &sDataFileAttr)
                                  != IDE_SUCCESS );

                        /* BUG-25922
                         * 사용자 테이블스페이스 생성 및 drop 하고
                         * server kill 하고 destroydb 시 서버가 비정상 종료합니다. */
                        if( SMI_FILE_STATE_IS_NOT_DROPPED(sDataFileAttr.mState) )
                        {
                            // remove it
                            IDE_TEST_RAISE(smrBackupMgr::unlinkDataFile(
                                               sDataFileAttr.mName )
                                           != IDE_SUCCESS, UnlinkError);
                        }
                        break;
                    }
                case SMI_CHKPTPATH_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readChkptPathNodeAttr(
                                      &sLogAnchorFile,
                                      &sCurOffset,
                                      &sChkptPathAttr)
                                  != IDE_SUCCESS );

                        IDE_ASSERT( sLogAnchorHeader.mNewTableSpaceID
                                    > sChkptPathAttr.mSpaceID );
                        IDE_ASSERT( sTBSNameArray[ sChkptPathAttr.mSpaceID ]
                                    != NULL );

                        // remove it
                        IDE_TEST_RAISE(smrBackupMgr::unlinkChkptImages(
                                           sChkptPathAttr.mChkptPath,
                                           sTBSNameArray[sChkptPathAttr.mSpaceID])
                                       != IDE_SUCCESS, UnlinkError);
                        break;
                    }
                case SMI_CHKPTIMG_ATTR:
                    {
                        sCurOffset +=
                            smrLogAnchorMgr::getChkptImageAttrSize();
                        // Nothing To do ...
                        break;
                    }
                case SMI_SBUFFER_ATTR:
                    {
                        IDE_TEST( smrLogAnchorMgr::readSBufferFileAttr(
                                                                &sLogAnchorFile,
                                                                &sCurOffset,
                                                                &sFileAttr)
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( smrBackupMgr::unlinkDataFile( 
                                                               sFileAttr.mName )
                                        != IDE_SUCCESS, UnlinkError);

                    }

                        break;
                default:
                    {
                        IDE_ASSERT( 0 );
                        break;
                    }
            }

            // EOF
            if ( sFileSize <= ((ULong)sCurOffset) )
            {
                break;
            }

            // 다음 attribute type 판독
            IDE_TEST( smrLogAnchorMgr::getNxtNodeAttrType( &sLogAnchorFile,
                                                           sCurOffset,
                                                           &sAttrType )
                      != IDE_SUCCESS);

        }


        for ( sLoop = 0; sLoop < sLogAnchorHeader.mNewTableSpaceID; sLoop++ )
        {
            if ( sTBSNameArray[sLoop] != NULL )
            {
                IDE_TEST( iduMemMgr::free( sTBSNameArray[sLoop] ) != IDE_SUCCESS );
            }
        }

        sAllocated = ID_FALSE;
        IDE_TEST( iduMemMgr::free(sTBSNameArray) != IDE_SUCCESS );
    }

    IDE_TEST( sLogAnchorFile.close() != IDE_SUCCESS );
    IDE_TEST( sLogAnchorFile.destroy() != IDE_SUCCESS );

    // PROJ-2133 incremental backup 
    /* ------------------------------------------
     * remove change tracking file and backup info file
     * ------------------------------------------*/
    if( sLogAnchorHeader.mCTFileAttr.mCTMgrState == SMRI_CT_MGR_ENABLED )
    {
        IDE_CALLBACK_SEND_MSG("Removing Change Tracking file");

        IDE_TEST_RAISE( smrBackupMgr::unlinkChangeTrackingFile(
                                sLogAnchorHeader.mCTFileAttr.mCTFileName )
                      != IDE_SUCCESS, UnlinkError );
    }

    if( sLogAnchorHeader.mBIFileAttr.mBIMgrState != SMRI_BI_MGR_FILE_REMOVED )
    {

        IDE_CALLBACK_SEND_MSG("Removing Backup Info file");

        IDE_TEST_RAISE( smrBackupMgr::unlinkBackupInfoFile(
                                sLogAnchorHeader.mBIFileAttr.mBIFileName)
                      != IDE_SUCCESS, UnlinkError );
    }



    /* ------------------------------------------
       remove log file and log anchor file
       ------------------------------------------*/
    IDE_CALLBACK_SEND_MSG("Removing Log files");

    IDE_TEST_RAISE( smrBackupMgr::unlinkAllLogFiles(
                        (SChar *)smuProperty::getLogDirPath()) 
                    != IDE_SUCCESS, UnlinkError );

    IDE_CALLBACK_SEND_MSG("Removing Log Anchor files");
    for (i = 0; i < 3; i++)
    {
        sLogAnchorDir = (SChar *)smuProperty::getLogAnchorDir(i);

        IDE_TEST_RAISE(smrBackupMgr::unlinkAllLogFiles(sLogAnchorDir)
                       != IDE_SUCCESS,
                       UnlinkError);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(DBNameLengthError);
    {
        idlOS::snprintf(sOutputMsg, ID_SIZEOF(sOutputMsg), "\nInvalid Database Name   \n"
                        "Usage: Database Name Range = 1 ~ %d \n",
                        SM_MAX_DB_NAME - 1);
    }
    IDE_EXCEPTION(DBNameError);
    {
        idlOS::snprintf(sOutputMsg, ID_SIZEOF(sOutputMsg), "\nInvalid Database File Name.\n"
                        "Check Property and Retry.\n");
    }
    IDE_EXCEPTION(ShmAlreadyExistError);
    {
        idlOS::snprintf(sOutputMsg, ID_SIZEOF(sOutputMsg), "\nCan't Destroy DB. "
                        "Remove Shared Memory First.\n");
    }
    IDE_EXCEPTION(FileExistError);
    {
        idlOS::snprintf(sOutputMsg, ID_SIZEOF(sOutputMsg), "\n[%s]\n"
                        "Not Exist Or Permission Missing.\n"
                        "Check and Delete it Manually.\n", sCurFile);
    }
    IDE_EXCEPTION(UnlinkError);
    {
        idlOS::snprintf(sOutputMsg, ID_SIZEOF(sOutputMsg), "\nCan't deltele file %s  !!!\n",
                        sCurFile);
    }
    IDE_EXCEPTION_END;
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DROP_DB_ERROR));

        if (sOutputMsg[0] != 0)
        {
            IDE_CALLBACK_SEND_MSG(sOutputMsg);
        }
    }

    {
        IDE_PUSH();

        if ( sAllocated == ID_TRUE )
        {
            for ( sLoop = 0; sLoop < sLogAnchorHeader.mNewTableSpaceID; sLoop++ )
            {
                if ( sTBSNameArray[sLoop] != NULL )
                {
                    IDE_ASSERT( iduMemMgr::free( sTBSNameArray[sLoop] )
                                == IDE_SUCCESS );
                }
            }

            IDE_ASSERT( iduMemMgr::free(sTBSNameArray) == IDE_SUCCESS );
        }

        IDE_POP();
    }

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::dropDatabase(idvSQL *aStatistics, void *aArg)
{
    IDE_TEST_RAISE(mmm::getCurrentPhase() != MMM_STARTUP_PROCESS, PhaseError);

    IDE_TEST(doDropDatabase(aStatistics, (qciArgDropDB *)aArg) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(PhaseError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SERVER_PHASE_MISMATCHES_QUERY_TYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
