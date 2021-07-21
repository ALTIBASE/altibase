/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmAll.h>

#if !defined(CM_DISABLE_IPC)

/*
 * IPC Channel Information
 */
PDL_thread_mutex_t gIpcMutex;
idBool             gIpcMutexInit = ID_FALSE;
idBool *           gIpcChannelList;
SInt               gIpcClientCount;
cmbShmInfo         gIpcShmInfo;
SChar *            gIpcShmBuffer;
key_t              gIpcShmKey;
PDL_HANDLE         gIpcShmID;
key_t *            gIpcSemChannelKey;
SInt  *            gIpcSemChannelID;
ASYS_TCHAR         gIpcLogFile[1024];

IDE_RC cmbShmInitializeStatic()
{
    /*
     * Initialize IPC Mutex
     */
    if (gIpcMutexInit == ID_FALSE)
    {
        IDE_TEST(idlOS::thread_mutex_init(&gIpcMutex) != 0);
        gIpcMutexInit = ID_TRUE;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbShmFinalizeStatic()
{
    /*
     * Destroy IPC Mutex
     */
    if (gIpcMutexInit == ID_TRUE)
    {
        IDE_TEST(idlOS::thread_mutex_destroy(&gIpcMutex) != 0);
        gIpcMutexInit = ID_FALSE;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC cmbShmRemoveOldCannel()
{
    SInt        sCount;
    FILE       *sIpcLogFile;

    key_t       sShmKey;
    key_t       sSemKey;
    union semun sArg;

    PDL_HANDLE  sShmID;
    SInt        sSemID;

    SInt        sShmSize;
    SInt        sSemCnt;

    SInt        sFlag = 0666;

    sIpcLogFile = idlOS::fopen(gIpcLogFile, PDL_TEXT("r"));
    IDE_TEST_RAISE(sIpcLogFile == NULL, file_open_error);

    sCount = idlOS::fread((void*)&sShmKey, sizeof(key_t), 1, sIpcLogFile);
    IDE_TEST_RAISE(sCount != 1, file_read_error);

    sCount = idlOS::fread( (void*) &sShmSize, sizeof(SInt), 1, sIpcLogFile);
    IDE_TEST_RAISE(sCount != 1, file_read_error);

    sShmID = idlOS::shmget(sShmKey, sShmSize, sFlag);
    IDE_TEST_RAISE(sShmID == PDL_INVALID_HANDLE, shmget_error);

    IDE_TEST_RAISE( idlOS::shmctl( sShmID, IPC_RMID, NULL ) != 0, shmctl_error);

    // PR-3024
    idlOS::memset(&sArg, 0, sizeof(union semun));

    while (1)
    {
        sCount = idlOS::fread((void*)&sSemKey, sizeof(key_t), 1, sIpcLogFile);
        if(sCount != 1)
        {
            break;
        }

        sCount = idlOS::fread((void*)&sSemCnt, sizeof(SInt), 1, sIpcLogFile);
        if(sCount != 1)
        {
            break;
        }

        sSemID = idlOS::semget(sSemKey, sSemCnt, sFlag);
        if (sSemID < 0)
        {
            break;
        }
        IDE_TEST_RAISE(idlOS::semctl( sSemID, 0, IPC_RMID, sArg) != 0,
                       shmctl_error );
    }

    (void)idlOS::fclose(sIpcLogFile);
    (void)idlOS::unlink(gIpcLogFile);

    return IDE_SUCCESS;

    IDE_EXCEPTION(file_read_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IPC_FILE_READ_ERROR);
    }
    IDE_EXCEPTION(shmget_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IPC_NO_SEMID);
    }
    IDE_EXCEPTION(shmctl_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IPC_NO_SEMKEY);
    }
    IDE_EXCEPTION(file_open_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IPC_FILE_OPEN_ERROR);
    }
    IDE_EXCEPTION_END;

    if (sIpcLogFile != NULL)
    {
        (void)idlOS::fclose(sIpcLogFile);
    }
    (void)idlOS::unlink(gIpcLogFile);

    return IDE_FAILURE;
}

static IDE_RC cmbShmWriteLog(FILE* aLogFile, void* aBuffer, SInt aSize)
{
    SInt sSize;

    IDE_TEST_RAISE(aLogFile == NULL, err_null_fd);

    sSize = idlOS::fwrite((const void*) aBuffer, aSize, 1, aLogFile);
    IDE_TEST_RAISE(sSize != 1, fwrite_error);

    sSize = idlOS::fflush(aLogFile);
    IDE_TEST_RAISE(sSize != 0, fflush_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_null_fd);
    {
        IDE_WARNING(IDE_SERVER_0, "File Descriptor is NULL!!.\n");
    }
    IDE_EXCEPTION(fwrite_error);
    {
        IDE_WARNING(IDE_SERVER_0, "Write IPC(semaphore) Error.\n");
    }
    IDE_EXCEPTION(fflush_error);
    {
        IDE_WARNING(IDE_SERVER_0, "Flush Error.\n");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbShmCreate(SInt aMaxChannelCount)
{
    UInt   i;
    SInt   j;
    SInt   sFlag;
    FILE*  sLogFile = NULL;
    ULong  sShmSize;
    SInt   sSemCount;
    UInt   sMaxCount;
    union  semun sArg;

    /*
     * IPC 초기화
     */

    gIpcShmKey        = -1;
    gIpcShmID         = PDL_INVALID_HANDLE;
    gIpcSemChannelKey = NULL;
    gIpcSemChannelID  = NULL;
    gIpcShmBuffer     = NULL;
    gIpcChannelList   = NULL;
    gIpcClientCount   = 0;

    gIpcShmInfo.mMaxChannelCount = aMaxChannelCount;
    // fix BUG-18830
    // bug-27250 free Buf list can be crushed when client killed
    // mMaxBufferCount는  원래 semaphore값을 초기화하는데 사용된다.
    // 변경전: channel수 * extra_buffer 수(8)
    // 변경후: 고정값 10
    gIpcShmInfo.mMaxBufferCount  = CMB_SHM_SEMA_UNDO_VALUE;

    /*
     * 비정상 종료시 생성한 자원 해제
     */

    idlOS::snprintf(gIpcLogFile,
                    ID_SIZEOF(gIpcLogFile),
                    PDL_TEXT("%s%s%s"),
                    idp::getHomeDir(),
                    IDL_FILE_SEPARATORS,
                    cmuIpcLogFileFromHome());

    (void)cmbShmRemoveOldCannel();

    if (gIpcShmInfo.mMaxChannelCount > 0)
    {
        IDU_FIT_POINT_RAISE( "cmbShm::cmbShmCreate::malloc::IpcSemChannelKey",
                              InsufficientMemory );
        
        // initialize channel semaphore key
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_ID,
                                         sizeof(key_t) * gIpcShmInfo.mMaxChannelCount,
                                         (void**)&gIpcSemChannelKey)
                       != IDE_SUCCESS, InsufficientMemory );

        for (i = 0; i < gIpcShmInfo.mMaxChannelCount; i++)
        {
            gIpcSemChannelKey[i] = -1;
        }

        IDU_FIT_POINT_RAISE( "cmbShm::cmbShmCreate::malloc::IpcSemChannelID",
                              InsufficientMemory );
        
        // initialize channel semaphore ID
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_ID,
                                         sizeof(SInt) * gIpcShmInfo.mMaxChannelCount,
                                         (void**)&gIpcSemChannelID)
                       != IDE_SUCCESS, InsufficientMemory );

        for (i = 0; i < gIpcShmInfo.mMaxChannelCount; i++)
        {
            gIpcSemChannelID[i] = -1;
        }

        IDU_FIT_POINT_RAISE( "cmbShm::cmbShmCreate::malloc::IpcChannelList",
                              InsufficientMemory );
        
        // initialize channel list
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_ID,
                                         sizeof(idBool) * gIpcShmInfo.mMaxChannelCount,
                                         (void**)&gIpcChannelList)
                       != IDE_SUCCESS, InsufficientMemory );

        for ( i = 0; i < gIpcShmInfo.mMaxChannelCount; i++ )
        {
            gIpcChannelList[i] = ID_FALSE;
        }

        // create ipc log file

        sLogFile = idlOS::fopen(gIpcLogFile, PDL_TEXT("w"));
        IDE_TEST_RAISE(sLogFile == NULL, err_open_ipc_logfile);

        /* BUG-49060 set altibase_ipc.log permission as 622 */
#if defined( ALTI_CFG_OS_WINDOWS )
        // Unable to set file permission at Windows OS
#else
        (void) idlOS::fchmod( fileno( sLogFile ), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
#endif

        // set shared memory key and semaphore key
        gIpcShmKey = idlOS::getpid() % 10000 + 10000;
        gIpcSemChannelKey[0] = gIpcShmKey + 10000;

        /* ------------------------------------------------
         *  HP 64bit의 경우 32비트 Clinet에서
         *  Shared Memory를 Attch 할 수 있도록,
         *  compatible option : IPC_SHARE32를 주어야 한다.
         *  by gamestar
         * ----------------------------------------------*/
#if (defined(HP_HPUX) ||defined(IA64_HP_HPUX))   && defined(COMPILE_64BIT)
        sFlag = 0666 | IPC_CREAT | IPC_EXCL | IPC_SHARE32;
#else
        sFlag = 0666 | IPC_CREAT | IPC_EXCL;
#endif

        // create shared memory
        sShmSize = cmbShmGetFullSize(gIpcShmInfo.mMaxChannelCount);

#if !defined(COMPILE_64BIT)
        IDE_TEST_RAISE(sShmSize >= ID_UINT_MAX, overflow_memory_size_32bit);
#endif

        while (gIpcShmID == PDL_INVALID_HANDLE)
        {
            gIpcShmID = idlOS::shmget(gIpcShmKey, sShmSize, sFlag);
            if (gIpcShmID != PDL_INVALID_HANDLE)
            {
                break;
            }
            gIpcShmKey++;
            switch(errno)
            {
                case EEXIST:
                case EACCES:
                    break; /* continue finding*/
                case ENOSPC:
                case ENOMEM:
                    IDE_RAISE(no_shm_space_error);
                default:
                    IDE_RAISE(normal_shmget_error);
            }
        } // while

        IDE_TEST(cmbShmWriteLog(sLogFile, (void*)&gIpcShmKey, ID_SIZEOF(key_t)) != IDE_SUCCESS);
        IDE_TEST(cmbShmWriteLog(sLogFile, (void*)&sShmSize, ID_SIZEOF(SInt)) != IDE_SUCCESS);

        gIpcShmBuffer = (SChar*) idlOS::shmat( gIpcShmID, NULL, 0 );
        IDE_TEST_RAISE( gIpcShmBuffer == (void*) -1, err_attach_shm );

        // Shared Memory Buffer 초기화
        sMaxCount = gIpcShmInfo.mMaxChannelCount;
        
        // create channel semaphores

        sSemCount = IPC_SEMCNT_PER_CHANNEL;

        sArg.val = 1;

        for (i = 0; i < sMaxCount; i++)
        {
            while (gIpcSemChannelID[i] < 0)
            {
                gIpcSemChannelID[i] = idlOS::semget(gIpcSemChannelKey[i],
                                                    sSemCount,
                                                    sFlag);
                if (gIpcSemChannelID[i] >= 0)
                {
                    if ((i+1) < gIpcShmInfo.mMaxChannelCount)
                    {
                        gIpcSemChannelKey[i+1] = gIpcSemChannelKey[i] + 1;
                    }
                    for (j=0; j<sSemCount; j++ )
                    {
                        IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[i],
                                                     j,
                                                     SETVAL,
                                                     sArg)
                                        != 0, err_sem_initial);
                    }

                    break;
                }
                gIpcSemChannelKey[i]++;
                switch(errno)
                {
                case EEXIST:
                case EACCES:
                    break; /* continue finding*/
                case ENOSPC:
                case ENOMEM:
                    IDE_RAISE(no_sem_space_error);
                default:
                    IDE_RAISE(normal_semget_error);
                }

            } // while

            /* ------------------------------------------------
             *  IPC2 지원할 경우
             * ----------------------------------------------*/
            {
                cmbShmChannelInfo *sShmHeader;
                sShmHeader = cmbShmGetChannelInfo(gIpcShmBuffer, i);

                sShmHeader->mPID               = 0;
                sShmHeader->mTicketNum         = 0;  // BUG-32398 타임스탬프에서 티켓번호로 변경
            }
            IDE_TEST(cmbShmWriteLog(sLogFile, (void*)&gIpcSemChannelKey[i], ID_SIZEOF(key_t)) != IDE_SUCCESS);
            IDE_TEST(cmbShmWriteLog(sLogFile, (void*)&sSemCount, ID_SIZEOF(SInt)) != IDE_SUCCESS);
        } // for

        /* bug-31572: IPC dispatcher hang might occur on AIX.
         * Each semid for some channel_ids printed for debugging.
         * This does not have any big meaning. */
        for (i = 0; i < sMaxCount; i++)
        {
            /* print only 0, 10, 20, ... among IDs */
            if ((i%10) == 0)
            {
                ideLog::log(IDE_SERVER_2, "[IPC channel info] "
                            "channel[%3d] "
                            "semid: %d",
                            i,
                            gIpcSemChannelID[i]);
            }
        }

        (void)idlOS::fclose(sLogFile);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION(err_open_ipc_logfile);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_FILE_OPEN, gIpcLogFile));
    }
    IDE_EXCEPTION(err_attach_shm);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idc_SHM_ATTACH));
    }
    IDE_EXCEPTION(err_sem_initial);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idc_SEM_INIT_OP));
    }
    IDE_EXCEPTION(no_shm_space_error);
    {
        ideLog::log(IDE_SERVER_0, "Not Enough Shared Memory : errno=%u\n", errno);
    }
#if !defined(COMPILE_64BIT)
    IDE_EXCEPTION(overflow_memory_size_32bit);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_NO_SHM_MEM, sShmSize);
    }
#endif
    IDE_EXCEPTION(normal_shmget_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IPC_SHMGET_ERROR, errno);
        /* bug-36515 mismatched err-msg when startup failed */
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SHMGET_ERROR));
    }
    IDE_EXCEPTION(no_sem_space_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_NO_SEM_MEM, errno);
    }
    IDE_EXCEPTION(normal_semget_error);
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IPC_SEMGET_ERROR, errno);
    }
    IDE_EXCEPTION_END;

    // BUG-18832
    union semun sSemCtlArg;

    idlOS::memset(&sSemCtlArg, 0, ID_SIZEOF(union semun));

    if (gIpcShmID != PDL_INVALID_HANDLE)
    {
        (void)idlOS::shmctl(gIpcShmID, IPC_RMID, NULL);
        gIpcShmID = PDL_INVALID_HANDLE;
    }

    gIpcShmKey = -1;

    if (gIpcSemChannelID != NULL)
    {
        for (i = 0; i < gIpcShmInfo.mMaxChannelCount; i++)
        {
            if (gIpcSemChannelID[i] >= 0)
            {
                (void)idlOS::semctl(gIpcSemChannelID[i], 0, IPC_RMID, sSemCtlArg);
                gIpcSemChannelID[i] = PDL_INVALID_HANDLE;
            }
        }
        (void)iduMemMgr::free(gIpcSemChannelID);
        gIpcSemChannelID = NULL;
    }

    if (gIpcSemChannelKey != NULL)
    {
        (void)iduMemMgr::free(gIpcSemChannelKey);
        gIpcSemChannelKey = NULL;
    }

    /* BUG-36775 CodeSonar warning : Leak */
    if( sLogFile != NULL )
    {
        (void)idlOS::fclose(sLogFile);
    }

    return IDE_FAILURE;
}

IDE_RC cmbShmDestroy()
{
    UInt        i;
    union semun sSemCtlArg;

    if (gIpcShmInfo.mMaxChannelCount > 0)
    {
        idlOS::memset(&sSemCtlArg, 0, ID_SIZEOF(union semun));

        // Channel Semaphore 제거
        if (gIpcSemChannelID != NULL)
        {
            for (i = 0; i < gIpcShmInfo.mMaxChannelCount; i++)
            {
                IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[i],
                                             0,
                                             IPC_RMID,
                                             sSemCtlArg)
                               != 0, SemRemoveError);
                gIpcSemChannelID[i] = PDL_INVALID_HANDLE;
            }
        }

        gIpcShmInfo.mMaxChannelCount = 0;

        // Channel 메모리 제거
        if (gIpcSemChannelID != NULL)
        {
            IDE_TEST(iduMemMgr::free(gIpcSemChannelID) != IDE_SUCCESS);
            gIpcSemChannelID  = NULL;
        }

        if (gIpcSemChannelKey != NULL)
        {
            IDE_TEST(iduMemMgr::free(gIpcSemChannelKey) != IDE_SUCCESS);
            gIpcSemChannelKey = NULL;
        }

        if (gIpcChannelList != NULL)
        {
            IDE_TEST(iduMemMgr::free(gIpcChannelList) != IDE_SUCCESS);
            gIpcChannelList = NULL;
        }

        if (gIpcShmID != PDL_INVALID_HANDLE)
        {
            IDE_TEST_RAISE(idlOS::shmdt(gIpcShmBuffer) != 0, ShmRemoveError);

            /*
             * BUG-32403 (for Windriver)
             *
             * Windriver OS는 shmdt() 과정에서 공유메모리가
             * 삭제되어 shmctl()에서 항상 서버가 비정상 종료된다.
             * 공유메모리 상태를 체크하고 삭제하자.
             */
            if (idlOS::shmget(gIpcShmKey, 0, 0) != -1)
            {
                IDE_TEST_RAISE(idlOS::shmctl(gIpcShmID, IPC_RMID, NULL)
                               != 0, ShmRemoveError);
            }
            gIpcShmID = PDL_INVALID_HANDLE;
        }

        gIpcShmKey = -1;

        (void)idlOS::unlink(gIpcLogFile);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ShmRemoveError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SHM_CTL));
    }
    IDE_EXCEPTION(SemRemoveError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_CTL));
    }

    IDE_EXCEPTION_END;

    if (gIpcShmID != PDL_INVALID_HANDLE)
    {
        (void)idlOS::shmctl(gIpcShmID, IPC_RMID, NULL);
        gIpcShmID = PDL_INVALID_HANDLE;
    }

    gIpcShmKey = -1;

    if (gIpcSemChannelID != NULL)
    {
        for (i = 0; i < gIpcShmInfo.mMaxChannelCount; i++)
        {
            if (gIpcSemChannelID[i] >= 0)
            {
                (void)idlOS::semctl(gIpcSemChannelID[i], 0, IPC_RMID, sSemCtlArg);
                gIpcSemChannelID[i] = PDL_INVALID_HANDLE;
            }
        }
        (void)iduMemMgr::free(gIpcSemChannelID);
        gIpcSemChannelID = NULL;
    }

    if (gIpcSemChannelKey != NULL)
    {
        (void)iduMemMgr::free(gIpcSemChannelKey);
        gIpcSemChannelKey = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC cmbShmAttach()
{
    ULong sShmSize;
    SInt  sFlag = 0666;

    if (gIpcShmID == PDL_INVALID_HANDLE)
    {
        sShmSize = cmbShmGetFullSize(gIpcShmInfo.mMaxChannelCount);
        gIpcShmID = idlOS::shmget(gIpcShmKey, sShmSize, sFlag);
        IDE_TEST(gIpcShmID == PDL_INVALID_HANDLE);
    }

    if (gIpcShmBuffer == NULL)
    {
        gIpcShmBuffer = (SChar*)idlOS::shmat(gIpcShmID, NULL, 0);
        IDE_TEST((void*)-1 == gIpcShmBuffer);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbShmDetach()
{
    if (gIpcShmBuffer != NULL)
    {
        (void)idlOS::shmdt(gIpcShmBuffer);
        gIpcShmBuffer = NULL;
    }

    return IDE_SUCCESS;
}

#endif
