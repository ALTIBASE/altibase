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
 * $$Id:$
 **********************************************************************/

#include <smErrorCode.h>
#include <smuProperty.h>
#include <sdbReq.h>
#include <sdbDWRecoveryMgr.h>
#include <sddDiskMgr.h>

/****************************************************************************
 * Abstraction :
 *  리커버리시에 본 함수 수행시킴.
 *  각 Double write File(DWFile)파일들이 존재하는 디렉토리로 가서
 *  그 파일들을 열어서, 각 파일에 대해 recoverDWFile을 수행하는 역활을 한다.
 ****************************************************************************/
IDE_RC sdbDWRecoveryMgr::recoverCorruptedPages()
{
    UInt            i;
    SInt            sRc;
    DIR            *sDir;
    struct dirent  *sDirEnt;
    struct dirent  *sResDirEnt;
    SInt            sState = 0;
    sddDWFile       sDWFile;
    idBool          sRightDWFile;
    SChar           sFullFileName[SM_MAX_FILE_NAME + 1];
    UInt            sDwFileNamePrefixLen;
    idBool          sFound = ID_FALSE;

    /* sdbDWRecoveryMgr_recoverCorruptedPages_malloc_DirEnt.tc */
    IDU_FIT_POINT("sdbDWRecoveryMgr::recoverCorruptedPages::malloc::DirEnt");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                               ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                               (void**)&sDirEnt,
                               IDU_MEM_FORCE)
             != IDE_SUCCESS);
    sState = 1;

    sDwFileNamePrefixLen = idlOS::strlen( SDD_DWFILE_NAME_PREFIX );

    for (i = 0; i < smuProperty::getDWDirCount(); i++)
    {
        sDir = idf::opendir(smuProperty::getDWDir(i));
        if (sDir == NULL)
        {
            ideLog::log(IDE_SERVER_0,
                        "Invalidate double write directory: %s",
                        smuProperty::getDWDir(i));
            continue;
        }

        while (1)
        {
	        errno = 0;
            sRc = idf::readdir_r(sDir, sDirEnt, &sResDirEnt);
            if ( (sRc != 0) && (errno != 0) )
            {
                ideLog::log(IDE_SERVER_0,
                            "Invalidate double write directory: %s",
                            smuProperty::getDWDir(i));
                break;
            }
            if (sResDirEnt == NULL)
            {
                break;
            }

            /* BUG-24946 Server Start시 DWFile load과정에서 발생한 에러코드를
             *           무시하고, Clear 해주어야함.
             * 프로퍼티에 정의된 DWDir 내의 모든 파일이나 디렉토리에 대해서 load
             * 해보지 않고, prefix가 일치하는 것에 대해서만 load해본다. */

            if ( idlOS::strncmp( sResDirEnt->d_name,
                                 SDD_DWFILE_NAME_PREFIX,
                                 sDwFileNamePrefixLen ) != 0 )
            {
                continue;
            }

            // full file name 만들기
            sFullFileName[0] = '\0';
            idlOS::sprintf(sFullFileName,
                           "%s%c%s",
                           smuProperty::getDWDir(i),
                           IDL_FILE_SEPARATOR,
                           sResDirEnt->d_name );

            if ( sDWFile.load(sFullFileName, &sRightDWFile) != IDE_SUCCESS )
            {
                /* 파일에 대한 permission 에러나 open 에러가 발생하여도
                 * 무시하겠다라면 다음 에러코드를 위해서 현재 설정된 에러코드를 제거한다.
                 * 그렇지 않으면 이후 Exception CallStack에 섞여 출력하게됨 */
                IDE_CLEAR();
            }

            if (sRightDWFile == ID_FALSE)
            {
                (void)sDWFile.destroy();
                continue;
            }

            sFound = ID_TRUE;

            IDE_TEST(recoverDWFile(&sDWFile) != IDE_SUCCESS);

            /* BUG-27776 the server startup can be fail since the dw file is 
             * removed after DW recovery. 
             * DWFile을 지우는 대신 Reset합니다.*/
            IDE_TEST( sDWFile.reset() != IDE_SUCCESS );
            (void)sDWFile.destroy();
        }
        idf::closedir(sDir);
    }

    IDE_ASSERT( smuProperty::getSMStartupTest() != 27776 );

    sState = 0;
    IDE_TEST(iduMemMgr::free(sDirEnt) != IDE_SUCCESS);
    
    /*
     * BUG-25957 [SD] 비정상종료후 restart시 DOUBLE_WRITE_DIRECTORY,
     *           DOUBLE_WRITE_DIRECTORY_COUNTproperty값을 변경하면 corrupt된 
     *           page들에대한 복구가 불완전하게 이뤄질수 있음.
     */
    if( sFound  == ID_FALSE )
    {
        IDE_RAISE( error_dw_file_not_found )
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( error_dw_file_not_found )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DW_FILE_NOT_FOUND ));
    }
    IDE_EXCEPTION_END;

    if (sState > 0)
    {
        IDE_ASSERT(iduMemMgr::free(sDirEnt) == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

/****************************************************************************
 * Abstraction :
 *  DWFile을 보고 해당 space ID와 page ID 가져와서 
 *  실제 DB에서 해당 하는 페이지가 제대로 되어 있는지 확인한다. 
 *  만약 깨져 있다면, DWFile에 해당 부분을 실제 DB에 Write한다.
 ****************************************************************************/
IDE_RC sdbDWRecoveryMgr::recoverDWFile(sddDWFile *aDWFile)
{
    SChar      *sAllocPtr;
    UChar      *sDWBuffer;
    UChar      *sDataBuffer;
    SInt        sState = 0;
    idBool      sIsPageValid;
    scPageID    sPageID;
    scSpaceID   sSpaceID;
    UInt        i;

    /* TC/FIT/Limit/sm/sdbDWRecoveryMgr_recoverDWFile_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbDWRecoveryMgr::recoverDWFile::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     aDWFile->getPageSize() * 3,
                                     (void**)&sAllocPtr) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    sDWBuffer = (UChar*)idlOS::align(sAllocPtr, aDWFile->getPageSize());
    sDataBuffer = sDWBuffer + aDWFile->getPageSize();

    for (i = 0; i < aDWFile->getPageCount(); i++)
    {
        IDE_TEST(aDWFile->read(NULL, (SChar*)sDWBuffer, i)
                 != IDE_SUCCESS);

        sPageID = smLayerCallback::getPageID( sDWBuffer );
        sSpaceID = ((sdbFrameHdr*)(sDWBuffer))->mSpaceID;

        // disk manager에 필요
        // 전혀 사용되지 않은 dw buffer frame일 경우
        // 무시한다. 초기화한 상태 그대로일 것임.
        if((sSpaceID == SC_NULL_SPACEID) && (sPageID == SC_NULL_PID))
        {
            continue;
        }

        // Fix BUG-17158 에 대한 내용
        // offline 혹은 Discard Disk TBS에 대해서 Double Write Buffer로부터
        // 복구를 진행하지 않는다.
        // 1. offline 연산 수행시 flush가 완료된 이후에 Offline 상태로 변경되기
        // 때문에 flush 했던 Page들은 모두 안전하게 데이타파일에 sync됨을
        // 보장한다.
        // 2. discard TBS는 Control 단계에서 결정이 되기 때문에 그 이후에는
        // 무시하면 된다.
        sIsPageValid = sddDiskMgr::isValidPageID( NULL,
                                                  sSpaceID,
                                                  sPageID );

        if (sIsPageValid != ID_TRUE)
        {
            continue;
        }

        /* BUG-19477 [SM-DISK] IOB영역에 Temp Tablespace의 Meta페이지가 있을 경우
         *           서버가 Media Recovery 중 비정상 종료
         *
         * Temp Tablespace의 Meta 페이지 또한 DWA(Double Write Area)에 기록이 된다.
         * 하지만 Temp Tablespace는 Media Recovery대상이 아니기때문에 Media Recovery시
         * Temp는 재생성이 되어 초기크기로 만들어진다. 그런데 Media Recovery시 초기크기
         * 이후 이미지가 DWA에 기록이 되어있으면 Media Recovery시 존재하지 않는 페이지
         * 에 대해서 Read를 요청하여 서버가 죽을 수 있습니다.
         *
         **/
        if ( smLayerCallback::isTempTableSpace( sSpaceID ) == ID_TRUE )
        {
            continue;
        }

        IDE_TEST( sddDiskMgr::read( NULL,
                                    sSpaceID,
                                    sPageID,
                                    sDataBuffer )
                  != IDE_SUCCESS );

        if ( smLayerCallback::isPageCorrupted( sDataBuffer ) == ID_TRUE )
        {
            if ( smLayerCallback::isPageCorrupted( sDWBuffer ) == ID_TRUE)
            {
                IDE_RAISE(page_corruption_error);
            }

            // offline TBS에 double write buffer로 복구하는 경우가 있다면
            // 그 원인은 offline 연산중에 깨진 것이 아니라
            // offline이 완료된 이후에 외부요인에 의해 깨진것이라 하겠다.
            // double write buffer는 buffer manager가 flush하는 도중에
            // crash가 발생하여 끼지는 것을 복구하려는 것이지 모든 Media
            // 오류에 대한 복구를 지원하는 것이 아니다.
            IDE_TEST(sddDiskMgr::write(NULL,
                                       sSpaceID,
                                       sPageID,
                                       sDWBuffer)
                     != IDE_SUCCESS);

            ideLog::log(SM_TRC_LOG_LEVEL_BUFFER,
                        SM_TRC_BUFFER_CORRUPTED_PAGE,
                        sSpaceID,
                        sPageID);
        }
    }

    sState = 0;
    IDE_TEST(iduMemMgr::free(sAllocPtr) != IDE_SUCCESS);

    IDE_TEST(sddDiskMgr::syncAllTBS(NULL,
                                    SDD_SYNC_NORMAL)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(page_corruption_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_PageCorrupted,
                                sSpaceID,
                                sPageID));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if (sState > 0)
    {
        IDE_ASSERT(iduMemMgr::free(sAllocPtr) == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

