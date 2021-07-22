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
 * $Id: dumpbi.cpp 50206 2011-12-21 07:18:01Z sliod $
 **********************************************************************/

#include <idl.h>
#include <idp.h>
#include <smiDef.h>
#include <smriBackupInfoMgr.h>

void ShowCopyRight(void);
IDE_RC dumpbackupInfoFile( SChar*  aBackupInfoFilePath );


IDE_RC dumpBackupInfoFile( SChar * aBackupInfoFilePath )
{
    iduFile                     sFile;
    ULong                       sReadOffset = 0;
    smriBIFileHdr               sBIFileHdr;
    smriBISlot                * sBISlotArea;
    smriBISlot                * sBISlot;
    UInt                        sBISlotIdx;
    time_t                      sTime;
    struct tm                   sBackupTime;
    SChar                       sBeginBackupTime[ SMRI_BI_TIME_LEN ];
    SChar                       sEndBackupTime[ SMRI_BI_TIME_LEN ];
    

    IDE_DASSERT( aBackupInfoFilePath != NULL );

    IDE_TEST( sFile.initialize(IDU_MEM_SM_SMR,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL) 
              != IDE_SUCCESS );


    IDE_TEST( sFile.setFileName( aBackupInfoFilePath ) != IDE_SUCCESS );

    IDE_TEST( sFile.open() != IDE_SUCCESS );

    IDE_TEST( sFile.read(NULL,
                         SMRI_BI_HDR_OFFSET, 
                         (SChar*)&sBIFileHdr,
                         (UInt)ID_SIZEOF(smriBIFileHdr))
              != IDE_SUCCESS );

    sReadOffset += SMRI_BI_FILE_HDR_SIZE; 

    idlOS::printf("<< DUMP OF BACKUOINFO FILE - %s >>\n\n",
                  aBackupInfoFilePath );

    idlOS::printf("\n------------------------\n");
    idlOS::printf("[BACKUO INFO FILE HDR]\n");

    idlOS::printf("Backup info slot count        [ %"ID_UINT32_FMT" ]\n",
                   sBIFileHdr.mBISlotCnt );

    idlOS::printf("Last backup LSN               [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                   sBIFileHdr.mLastBackupLSN.mFileNo,
                   sBIFileHdr.mLastBackupLSN.mOffset );

    idlOS::printf("Database name                 [ %s ]\n",
                   sBIFileHdr.mDBName );
    idlOS::printf("\n------------------------\n");

    IDE_TEST_CONT( sBIFileHdr.mBISlotCnt == 0, skip_dump_slot );

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                 1,                             
                                 ID_SIZEOF( smriBISlot ) * sBIFileHdr.mBISlotCnt,       
                                 (void**)&sBISlotArea )             
              != IDE_SUCCESS );  

    
    IDE_TEST( sFile.read(NULL,
                         sReadOffset, 
                         (SChar*)sBISlotArea,
                         ID_SIZEOF( smriBISlot ) * sBIFileHdr.mBISlotCnt )
          != IDE_SUCCESS );


    idlOS::printf("[BACKUP INFO SLOT]\n");
    for( sBISlotIdx = 0; sBISlotIdx < sBIFileHdr.mBISlotCnt; sBISlotIdx++ )
    {
        sBISlot = &sBISlotArea[ sBISlotIdx ];
        
        sTime = (time_t)sBISlot->mBeginBackupTime;
        idlOS::localtime_r( (time_t *)&sTime, &sBackupTime );
        idlOS::snprintf( sBeginBackupTime, 
                         SMRI_BI_TIME_LEN,
                         "%04"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         " %02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT,
                         sBackupTime.tm_year + 1900,
                         sBackupTime.tm_mon + 1,
                         sBackupTime.tm_mday,
                         sBackupTime.tm_hour,
                         sBackupTime.tm_min,
                         sBackupTime.tm_sec );

        sTime = (time_t)sBISlot->mEndBackupTime;
        idlOS::localtime_r( (time_t *)&sTime, &sBackupTime );
        idlOS::snprintf( sEndBackupTime,
                         SMRI_BI_TIME_LEN,
                         "%04"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         " %02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT,
                         sBackupTime.tm_year + 1900,
                         sBackupTime.tm_mon + 1,
                         sBackupTime.tm_mday,
                         sBackupTime.tm_hour,
                         sBackupTime.tm_min,
                         sBackupTime.tm_sec );         
        idlOS::printf("\n------------------------\n");
        
        idlOS::printf("Slot index                       [ %"ID_UINT32_FMT" ]\n",
                      sBISlotIdx );

        idlOS::printf("Begin backup time                [ %s ]\n",
                      sBeginBackupTime );

        idlOS::printf("End backup time                  [ %s ]\n",
                      sEndBackupTime );

        idlOS::printf("Incremental backup chunk cnt     [ %"ID_UINT32_FMT" ]\n",
                   sBISlot->mIBChunkCNT );

        idlOS::printf("Incremental backup chunk size    [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mIBChunkSize );

        idlOS::printf("Backup target                    [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mBackupTarget );

        idlOS::printf("Backup level                     [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mBackupLevel );

        idlOS::printf("Backup Type                      [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mBackupType );

        idlOS::printf("Tablespace ID                    [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mSpaceID );

        idlOS::printf("File ID                          [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mFileNo );

        idlOS::printf("Original file size               [ %"ID_UINT32_FMT" ]\n",
                      sBISlot->mOriginalFileSize );

        idlOS::printf("Backup Tag                       [ %s ]\n",
                      sBISlot->mBackupTag );

        idlOS::printf("Backup file name                 [ %s ]\n",
                      sBISlot->mBackupFileName );

        idlOS::printf("------------------------\n");
    }

    IDE_TEST( iduMemMgr::free( sBISlotArea ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT(skip_dump_slot);

    IDE_TEST( sFile.close() != IDE_SUCCESS );

    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

int main(SInt argc, SChar *argv[])
{
    IDE_RC rc;

    ShowCopyRight();
    
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
   
    //fix TASK-3870
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);

    IDE_TEST(iduCond::initializeStatic() != IDE_SUCCESS);
 
    if(argc == 2)
    {
        rc = dumpBackupInfoFile(argv[1]);
        IDE_TEST_RAISE(rc != IDE_SUCCESS, err_file_invalid);
    }
    else
    {
        idlOS::printf("usage: dumpbi backupinfo_file\n");
        IDE_RAISE(err_arg_invalid);
    }

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS );
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );
   
    return 0;

    IDE_EXCEPTION(err_arg_invalid);
    {
    }
    IDE_EXCEPTION(err_file_invalid);
    {
    idlOS::printf("Check file type\n");
    }
    IDE_EXCEPTION_END;
    
    return -1;
}

void ShowCopyRight(void)
{
    SChar         sBuf[1024 + 1];
    SChar         sBannerFile[1024];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
    const SChar * sBanner = "DUMPBI.ban";

    sAltiHome = idlOS::getenv(IDP_HOME_ENV);
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home );

    // make full path banner file name
    idlOS::memset(sBannerFile, 0, ID_SIZEOF(sBannerFile));
    idlOS::snprintf(sBannerFile, ID_SIZEOF(sBannerFile), "%s%c%s%c%s"
        , sAltiHome, IDL_FILE_SEPARATOR, "msg", IDL_FILE_SEPARATOR, sBanner);

    sFP = idlOS::fopen(sBannerFile, "r");
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    sCount = idlOS::fread( (void*) sBuf, 1, 1024, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf("%s", sBuf);
    idlOS::fflush(stdout);

    idlOS::fclose(sFP);

    IDE_EXCEPTION( err_altibase_home );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_open );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_read );
    {
        idlOS::fclose(sFP);
    }
    IDE_EXCEPTION_END;
}
