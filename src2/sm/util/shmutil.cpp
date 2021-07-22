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
 * $Id: shmutil.cpp 85760 2019-06-28 07:19:23Z et16 $
 **********************************************************************/

#include <smuUtility.h>
#include <smi.h>
#include <smm.h>
#include <sct.h>
#include <mmm.h>
#include <mmuProperty.h>
#include <mmuServerStat.h>

#ifdef VC_WIN32_NOT_YET_ADAPTED
int KILL_PROC_BY_NAME(const char *);
#include "../../../pd/src/win32/killProcByName.cpp"
#endif

#ifdef ALTIBASE_3
#undef ALTIBASE_3
#endif

void printMemBase(SChar *aMsg, smmTBSNode * aTBSNode);
IDE_RC printSHMinfo();
IDE_RC writeToDisk(smmTBSNode * aTBSNode, SChar * aDBPath );
IDE_RC writeAllPage(smmTBSNode * aTBSNode, SChar * aDBPath );
IDE_RC eraseSHM(smmTBSNode * aTBSNode);
IDE_RC askDBPath( SChar * aDBPath, UInt aDBPathMaxLen );
IDE_RC askRemoveYN(idBool *aDoIt);




SChar         gHomeDir[256];
SChar         gConfFile[256];
mmuServerStat gCheckStatus;

const SChar mHelpMsg[] =
    "shmutil {-p|w|e] [-d home_directory] [-f properties_file_path]\n"
    "         -p : Print Shared Memory DB Info \n"
    "         -w : Write Shared Memory DB to Disk\n"
    "         -e : Erase Shared Memory DB \n"
    "         -d : specify Altibase home directory path\n"
    "         -f : specify Altibase properties file path\n";

void mainCallbackFuncForFatal(
    SChar  *  file ,
    SInt      linenum ,
    SChar  *  msg  )
{
    smuUtility::outputMsg("%s:%d(%s)\n", file, linenum, msg);
    idlOS::exit(-1);
}

IDE_RC mainNullCallbackFuncForMessage(
    const SChar * /*_ msg  _*/,
    SInt          /*_ flag _*/,
    idBool        /*_ aLogMsg _*/)
{
    return IDE_SUCCESS;
}

void mainNullCallbackFuncForPanic(
    SChar  * /*_ file _*/,
    SInt     /*_ linenum _*/,
    SChar  * /*_ msg _*/ )
{
}

void mainNullCallbackFuncForErrlog(
    SChar  * /*_ file _*/,
    SInt     /*_ linenum _*/)
{
}

int main(int argc, char* argv[])
{
    SChar            sOptFlag  = ' ';
    SInt             sOptCount = 0;
    SInt             sOptR;
    UInt             sShmDBKey;
    smmTBSNode     * sTBSNode;
    idBool           sDoRemove;
    SChar            sDBPath[SM_MAX_FILE_NAME + 1];
    SChar          * envhome = NULL;

    (void) ideSetCallbackFunctions
        (
            mainNullCallbackFuncForMessage,
            mainCallbackFuncForFatal,
            mainNullCallbackFuncForPanic,
            mainNullCallbackFuncForErrlog
            );


    /* ---------------------------
     * Initialize
     * --------------------------*/
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic( IDU_CLIENT_TYPE );

    IDE_TEST( iduCond::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );


    smuUtility::outputUtilityHeader("ShmUtil");

    /* --------------------
     * [0] 환경변수 존재 검사
     * -------------------*/
    envhome = idlOS::getenv(IDP_HOME_ENV);

    IDE_TEST_RAISE( !(envhome && idlOS::strlen(envhome) > 0), env_not_exist_error);

    // BUG-29649 Initialize Global Variables of shmutil
    gHomeDir[0]  = '\0';
    gConfFile[0] = '\0';

    /* --------------------
     * [1] 옵션 처리
     * -------------------*/
    while ( (sOptR = idlOS::getopt(argc, argv, "d:f:pwe")) != EOF)
    {
        switch(sOptR)
        {
            case 'd':  // 홈 디렉토리 명시
                idlOS::strncpy(gHomeDir, optarg, 255);
                break;
            case 'f':  // Conf 화일 명시
                idlOS::strncpy(gConfFile, optarg, 255);
                break;
            case 'p':
                sOptFlag = 'p';
                sOptCount++;
                break;
            case 'w':
                sOptFlag = 'w';
                sOptCount++;
                break;
            case 'e':
                sOptFlag = 'e';
                sOptCount++;
                break;
        }
    }

    IDE_TEST_RAISE(sOptCount > 1 || sOptFlag == ' ', invalid_option_error);

    /* --------------------
     *  프로퍼티 로딩
     * -------------------*/
    IDE_TEST_RAISE(idp::initialize(gHomeDir, gConfFile) != IDE_SUCCESS,
                   load_property_error);
    iduProperty::load();
    mmuProperty::load();
    smuProperty::load();

    /* --------------------
     *  에러 Message 로딩
     * -------------------*/
    IDE_TEST_RAISE(smuUtility::loadErrorMsb(idp::getHomeDir(),
                                            (SChar*)"US7ASCII")
                   != IDE_SUCCESS, load_error_msb_error);

    if( gCheckStatus.isFileExist() == ID_TRUE )
    {
        /* ---------------------------
         *  Server Lock File 초기화
         * --------------------------*/
        IDE_TEST_RAISE(gCheckStatus.initialize() != IDE_SUCCESS,
                       status_init_error);
    }

    /* --------------------
     *  Make File Name
     * -------------------*/


    /* -------------------------------------------
     * [3] SHM KEY 프로퍼티 검사 및 attach() 가능?
     * ------------------------------------------*/
    IDE_ASSERT(idp::read("SHM_DB_KEY", (void**)&sShmDBKey, 0)
               == IDE_SUCCESS);

    IDE_TEST_RAISE(sShmDBKey == 0, shm_propery_error);

    /* -------------------------------------------
     *  Initialize Time Manager ( Used by smrLFThread )
     * ------------------------------------------*/
    IDE_TEST(idvManager::initializeStatic() != IDE_SUCCESS);
    IDE_TEST(idvManager::startupService() != IDE_SUCCESS);

    /* -----------------------
     * [4] SM 매니저 초기화
     * ----------------------*/
    IDE_TEST_RAISE(smiStartup(SMI_STARTUP_PRE_PROCESS,
                              SMI_STARTUP_NOACTION,
                              &mmm::mSmiGlobalCallBackList) != IDE_SUCCESS,
                   smi_init_error );

    IDE_TEST_RAISE(smiStartup(SMI_STARTUP_PROCESS,
                              SMI_STARTUP_NOACTION,
                              &mmm::mSmiGlobalCallBackList) != IDE_SUCCESS,
                   smi_init_error );

    IDE_TEST_RAISE(smiStartup(SMI_STARTUP_CONTROL,
                              SMI_STARTUP_NOACTION,
                              &mmm::mSmiGlobalCallBackList) != IDE_SUCCESS,
                   smi_init_error );
    /* -----------------------
     * [4-1] Shared Memory Attach 수행
     * ----------------------*/
    IDE_TEST(smmManager::prepareDB( SMM_PREPARE_OP_DONT_CHECK_DB_SIGNATURE_4SHMUTIL )


             != IDE_SUCCESS);


    switch(sOptFlag)
    {
        case 'e': // remove shared memory

            IDE_TEST( askRemoveYN( & sDoRemove ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sDoRemove == ID_FALSE, shm_remove_canceled );
            break;

        case 'w': // write to disk
            // 어느 디렉토리에 DB파일을 만들지 결정한다.
            IDE_TEST( askDBPath( sDBPath,
                                 SM_MAX_FILE_NAME ) // 최대 Path길이
                      != IDE_SUCCESS );
            break;
        case 'p': // print info
            (void)printSHMinfo();
            idlOS::exit(0);

    }

    // 모든 Tablespace에 대해 Option별 동작 실시
    sTBSNode = (smmTBSNode*)sctTableSpaceMgr::getFirstSpaceNode();
    while ( sTBSNode != NULL)
    {

        if(sctTableSpaceMgr::isMemTableSpace(sTBSNode->mHeader.mID) == ID_TRUE)
        {
            if ( sctTableSpaceMgr::hasState( & sTBSNode->mHeader,
                                             SCT_SS_SKIP_SHMUTIL_OPER)
                 == ID_TRUE )
            {
                // shmutil의 동작을 SKIP해야 하는 경우
                // do nothing..
            }
            else
            {
                IDE_TEST_RAISE(sTBSNode->mRestoreType !=
                               SMM_DB_RESTORE_TYPE_SHM_ATTACH,
                               restore_type_error);

                /* -----------------------
                 * [5] Option 별 수행
                 * ----------------------*/
                switch(sOptFlag)
                {
                    case 'e': // erase shm
                        IDE_TEST( eraseSHM(sTBSNode) != IDE_SUCCESS );
                        break;
                    case 'w': // write to disk
                        IDE_TEST( writeToDisk(sTBSNode, sDBPath) != IDE_SUCCESS );
                        break;
                }
            }

        }
        else
        {
            // do nothing..
            // Disk Tablespace에 대해서는 아무것도 처리할 것이 없다.
        }

        sTBSNode = (smmTBSNode*)sctTableSpaceMgr::getNextSpaceNode( sTBSNode->mHeader.mID );
    }

    switch(sOptFlag)
    {
        case 'e': // remove shared memory
            smuUtility::outputMsg("[SUCCESS] All Shared Memory Removed \n");
        case 'w': // write to disk
            smuUtility::outputMsg("[SUCCESS] Database File Saved \n");
            break;
    }


    /* ---------------------------
     * Finalize
     * --------------------------*/
    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS );
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );


    return IDE_SUCCESS;
    /* -----------------------------------------------------------------
     * E [0] : SM 에러 처리
     * ---------------------------------------------------------------*/
    IDE_EXCEPTION(shm_remove_canceled);
    {
        smuUtility::outputMsg("[Warning] Shared Memory Destruction Cancelled....\n");
        idlOS::exit(0);
    }
    IDE_EXCEPTION(load_error_msb_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't Load Error Files. \n");
        smuUtility::outputMsg("Check Directory in $ALTIBASE_HOME"IDL_FILE_SEPARATORS"msg. \n");
        idlOS::exit(0);
    }
    IDE_EXCEPTION(load_property_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't Load Property. \n");
        smuUtility::outputMsg("Check Directory in $ALTIBASE_HOME"IDL_FILE_SEPARATORS"conf \n");
        idlOS::exit(0);
    }
    IDE_EXCEPTION(invalid_option_error);
    {
        smuUtility::outputMsg(mHelpMsg);
        smuUtility::outputMsg("Error: \n"
                              " Specify Only One Options \n");
        idlOS::exit(0);
    }
    IDE_EXCEPTION(env_not_exist_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Please Make Environment Variable For [%s]\n",
                              envhome);
        idlOS::exit(0);
    }
    IDE_EXCEPTION(shm_propery_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "The Database is not shared-memory version."
                              "check altibase properties.\n");
        idlOS::exit(0);
    }
//      IDE_EXCEPTION(make_dbname_error);
//      {
//          smuUtility::outputMsg("ERROR: \n"
//                                "Can't Make Database File Name.\n");
//          smuUtility::outputMsg("Check and Retry.\n");
//          idlOS::exit(0);
//      }
    IDE_EXCEPTION(restore_type_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Restore Type is invalid. Something wrong?\n");
        smuUtility::outputMsg("Check and Retry.\n");
    }

    IDE_EXCEPTION(smi_init_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Storage Manager Initialzing Failed.\n");
        smuUtility::outputMsg("Check Error Message.\n");
    }
    IDE_EXCEPTION(status_init_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't Initialize FileLock Class....\n");
    }
    IDE_EXCEPTION_END;

    ideDump();
    return IDE_FAILURE;
}

void printMemBase(SChar *aMsg, smmTBSNode * aTBSNode)
{
    PDL_Time_Value     sTimeStamp;
    smmSCH            *sBaseSCH;
    smmSCH            *sSCH;
    key_t              sKey;
    UInt               i;
    scPageID           total_page;
    scPageID           used_page;
    scPageID           free_page;
    SDouble            total_size_m;
    SDouble            free_size_m;
    SDouble            used_size_m;
    smmMemBase *       sMemBase = aTBSNode->mMemBase;

    sTimeStamp.set(sMemBase->mTimestamp);

    smuUtility::outputMsg("\n %s \n", aMsg);

    smuUtility::outputMsg(" #################### IPC Information ###################### \n");

    sBaseSCH     = smmFixedMemoryMgr::getBaseSCH(aTBSNode);
    IDE_ASSERT( sBaseSCH != NULL);

    smuUtility::outputMsg("   SHM BASE KEY = %d(0x%x) ID = %d(0x%x) \n",
                          smmFixedMemoryMgr::getShmKey(aTBSNode),
                          smmFixedMemoryMgr::getShmKey(aTBSNode),
                          sBaseSCH->m_next->m_shm_id,
                          sBaseSCH->m_next->m_shm_id);


    smuUtility::outputMsg("\n #################### Brief Information ###################### \n");

    total_page = aTBSNode->mMemBase->mAllocPersPageCount;

    // 모든 Free Page List의 총 Free Page수를 구한다.
    free_page = 0;
    for ( i=0; i< aTBSNode->mMemBase->mFreePageListCount; i++ )
    {
        free_page += aTBSNode->mMemBase->mFreePageLists[i].mFreePageCount ;
    }

    used_page  = total_page - free_page;

#if (_MSC_VER < 1300 && _MSC_VER >= 1200)
    total_size_m = (SDouble)(SM_PAGE_SIZE * (vSLong)total_page) / (SDouble)(1024 * 1024);
    free_size_m  = (SDouble)(SM_PAGE_SIZE * (vSLong)sFreePageCnt) /  (SDouble)(1024 * 1024);
#else
    total_size_m = (SDouble)(SM_PAGE_SIZE * total_page) / (SDouble)(1024 * 1024);
    free_size_m  = (SDouble)(SM_PAGE_SIZE * free_page) /  (SDouble)(1024 * 1024);
#endif
    used_size_m  = total_size_m - free_size_m;

    smuUtility::outputMsg("             SIZE(MB)     PAGE(#) \n");
    smuUtility::outputMsg("   TOTAL %10.4f %10"ID_vULONG_FMT" \n",
                          total_size_m, total_page);
    smuUtility::outputMsg("   USED  %10.4f %10"ID_vULONG_FMT" \n",
                          used_size_m, used_page);
    smuUtility::outputMsg("   FREE  %10.4f %10"ID_vULONG_FMT" \n",
                          free_size_m, free_page);
    smuUtility::outputMsg("   Timestamp is %d(sec) / %d(usec) \n\n",
                          sTimeStamp.sec(), sTimeStamp.usec());


    smuUtility::outputMsg("\n #################### Detail Information ###################### \n");
    sSCH = sBaseSCH->m_next;          // 첫번째 SCH로 옮긴다.
    sKey = smmFixedMemoryMgr::getShmKey(aTBSNode); // Base key를 얻음

    while(sSCH != NULL)
    {
        smmSCH *sNextSCH = sSCH->m_next; // to prevent FMR
        SDouble used_size;

#if (_MSC_VER < 1300 && _MSC_VER >= 1200)
        used_size  = (SDouble)((vSLong)(sSCH->m_header->m_page_count) * SM_PAGE_SIZE)
            / (SDouble)(1024 * 1024);
#else
        used_size  = (SDouble)(sSCH->m_header->m_page_count * SM_PAGE_SIZE)
            / (SDouble)(1024 * 1024);
#endif

        smuUtility::outputMsg("     ## Shared Memory Chunk ## \n");
        smuUtility::outputMsg("        Shm Key    = %"ID_UINT32_FMT" \n",
                              sKey);
        smuUtility::outputMsg("        Shm Id     = %"ID_UINT32_FMT" \n",
                              (UInt)sSCH->m_shm_id);
        smuUtility::outputMsg("        Page Count = %"ID_vULONG_FMT" \n",
                              sSCH->m_header->m_page_count );
        smuUtility::outputMsg("        Size(MB)   = %0.4f \n", used_size);
        smuUtility::outputMsg("\n");
        sKey = sSCH->m_header->m_next_key;
        sSCH = sNextSCH;
    }
    smuUtility::outputMsg("\n");
}

IDE_RC printSHMinfo()
{
    smmTBSNode * sTBSNode;

    IDE_ASSERT(sctTableSpaceMgr::findSpaceNodeBySpaceID(
                   SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                   (void**)&sTBSNode) == IDE_SUCCESS);

    printMemBase((SChar *)"", sTBSNode);

    return IDE_SUCCESS;
}

IDE_RC writeAllPage(smmTBSNode * aTBSNode, SChar *aDBPath )
{
    scPageID           i;
    scPageID           sCurPID;
    smmDatabaseFile   **sDatabaseFile;
    smmSCH            *sBaseSCH;
    smmSCH            *sSCH;
    SInt               sDbFileCount;
    SInt               sDbFileNo;
    SInt               sCurrentDB;

    sBaseSCH     = smmFixedMemoryMgr::getBaseSCH(aTBSNode);
    IDE_ASSERT( sBaseSCH != NULL);

    sCurrentDB = smmManager::getCurrentDB(aTBSNode);
    sDbFileCount = smmManager::getDbFileCount(aTBSNode, sCurrentDB);

    sDatabaseFile = (smmDatabaseFile **)idlOS::calloc(sDbFileCount,
                                                      ID_SIZEOF(smmDatabaseFile *));

    for ( i = 0; i < (scPageID)sDbFileCount; i++ )
    {
        sDatabaseFile[i] = (smmDatabaseFile*)idlOS::malloc(ID_SIZEOF(smmDatabaseFile));

        IDE_TEST_RAISE(sDatabaseFile[i] == NULL, memory_error);

        sDatabaseFile[i] = new (sDatabaseFile[i]) smmDatabaseFile();

        IDE_TEST(sDatabaseFile[i]->initialize(aTBSNode->mHeader.mID,
                                              sCurrentDB,
                                              i ) != IDE_SUCCESS);

        IDE_TEST(sDatabaseFile[i]->setFileName(aDBPath,
                                               aTBSNode->mHeader.mName,
                                               sCurrentDB,/*Ping Pong Number*/
                                               i)
                 != IDE_SUCCESS);

        IDE_TEST(sDatabaseFile[i]->createUntilSuccess(smiSetEmergency,
                                                      ID_TRUE) != IDE_SUCCESS);
        IDE_TEST(sDatabaseFile[i]->open() != IDE_SUCCESS);
    }


    sSCH = sBaseSCH->m_next;
    while(sSCH != NULL)
    {
        smpPersPage *sBasePage = (smpPersPage *)((UChar *)sSCH->m_header +
                                                 SMM_CACHE_ALIGNED_SHM_HEADER_SIZE);

        for (i = 0; i < sSCH->m_header->m_page_count; i++)
        {
            smpPersPage *sCurPage = sBasePage + i;
            sCurPID = sCurPage->mHeader.mSelfPageID;
#if defined(ALTIBASE_3)
            if ( ((smcTempPage *)sCurPage)->m_header.mSelf !=
                 (smcTempPage *)SMM_SHM_LOCATION_FREE)
#else
            if ( ((smmTempPage *)sCurPage)->m_header.m_self !=
                 (smmTempPage *)SMM_SHM_LOCATION_FREE)
#endif
                {

                    sDbFileNo = smmManager::getDbFileNo(aTBSNode, sCurPID);
                    IDE_TEST_RAISE( sDbFileNo >= sDbFileCount,
                                    page_corrupted_error );
                    IDE_TEST_RAISE(sDatabaseFile[sDbFileNo]->writePage(
                                       aTBSNode,
                                       sCurPID,
                                       (UChar *)sCurPage)
                                   != IDE_SUCCESS, write_page_error);
                }
        }
        sSCH = sSCH->m_next;
    }

    for ( i = 0; i < (scPageID)sDbFileCount; i++ )
    {
        IDE_TEST_RAISE(sDatabaseFile[i]->close() != IDE_SUCCESS, close_error);
    }
    free(sDatabaseFile);


    return IDE_SUCCESS;

    IDE_EXCEPTION( page_corrupted_error );
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Shared Memory Page Corrupted.\n");
    }
    IDE_EXCEPTION(write_page_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Write page Failed.\n");
    }
    IDE_EXCEPTION(close_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Database File Close Failed.\n");
    }
    IDE_EXCEPTION(memory_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't allocate Memory.\n");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC askDBPath( SChar * aDBPath, UInt aDBPathMaxLen )
{
    UInt  sLoop;

    idlOS::memset(aDBPath,      0, SM_MAX_DB_NAME);

    smuUtility::outputMsg("\n\n !!!!!! WARNING !!!!!!\n");
    smuUtility::outputMsg(" Duplicated DB-Name will overwrite original DB-file.\n");
    smuUtility::outputMsg(" Use Different DB-Path.\n");

    for( sLoop = 0; sLoop != mmuProperty::getDBDirCount(); ++sLoop )
    {
        smuUtility::outputMsg(" Orignal DB Path %d)  => %s\n",
                              sLoop+1,
                              mmuProperty::getDBDir(sLoop));
    }

    smuUtility::outputMsg(" Input DB-Path => "); idlOS::fflush(stdout);
    idlOS::gets(aDBPath, aDBPathMaxLen);

    IDE_TEST_RAISE(idlOS::strlen(aDBPath) < 1, too_short_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(too_short_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Database File Path is too short.\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC askRemoveYN(idBool *aDoIt)
{
    SChar flag[2];

    smuUtility::outputMsg("\nReady for Destroying Shared Memory? (y/N)");
    idlOS::fflush(stdout);
    idlOS::gets(flag, 2);

    if (flag[0] == 'y' || flag[0] == 'Y')
    {
        *aDoIt = ID_TRUE;
#ifdef VC_WIN32_NOT_YET_ADAPTED
        KILL_PROC_BY_NAME("ipc-daemon.exe");
#endif
    }
    else
    {
        *aDoIt = ID_FALSE;
    }

    return IDE_SUCCESS;
}



IDE_RC writeToDisk(smmTBSNode * aTBSNode, SChar * aDBPath )
{

    idBool sRunningFlag;

    if( gCheckStatus.isFileExist() == ID_TRUE )
    {
        IDE_TEST_RAISE(gCheckStatus.checkServerRunning(&sRunningFlag) != IDE_SUCCESS, check_server_error);
    }
    else
    {
        sRunningFlag = ID_FALSE;
    }

    IDE_TEST_RAISE(sRunningFlag == ID_TRUE, server_is_running_error);

    smuUtility::outputMsg("Tablespace File saving...%s\n", aTBSNode->mHeader.mName);

    IDE_TEST(writeAllPage(aTBSNode, aDBPath) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(server_is_running_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't write on on-line state. Shutdown altibase first.\n");
    }
    IDE_EXCEPTION(check_server_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't get the server status."
                              " any problem on socket?\n\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC eraseSHM(smmTBSNode * aTBSNode)
{
    idBool sRunningFlag;

    /* -------------------------
     * [1] 서버가 동작중인지 검사
     * ------------------------*/

    if( gCheckStatus.isFileExist() == ID_TRUE )
    {
        IDE_TEST_RAISE(gCheckStatus.checkServerRunning(&sRunningFlag) != IDE_SUCCESS, check_server_error);
    }
    else
    {
        sRunningFlag = ID_FALSE;
    }

    IDE_TEST_RAISE(sRunningFlag == ID_TRUE, server_is_running_error);

    IDE_TEST(smmFixedMemoryMgr::remove(aTBSNode) != IDE_SUCCESS);
    smuUtility::outputMsg("Destroyed Shared Memory of Tablespace %s.\n",
                          aTBSNode->mHeader.mName);

    return IDE_SUCCESS;

    IDE_EXCEPTION(check_server_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't get the server status."
                              " any problem on socket?\n\n");
    }
    IDE_EXCEPTION(server_is_running_error);
    {
        smuUtility::outputMsg("ERROR: \n"
                              "Can't Erase on on-line state. Shutdown altibase first.\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
