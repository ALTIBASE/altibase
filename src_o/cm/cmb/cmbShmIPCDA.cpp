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

#if !defined(CM_DISABLE_IPCDA)

/* IPC Channel Information */
cmbShmIPCDAInfo    gIPCDAShmInfo;
iduMutex           gIPCDALock;
ASYS_TCHAR         gIPCDALogFile[1024];



IDE_RC cmbShmIPCDAInitializeStatic()
{
    IDE_TEST(gIPCDALock.initialize((SChar *)"CMN_LINKPEER_IPCDA_MUTEX",
                                   IDU_MUTEX_KIND_POSIX,
                                   IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC cmbShmIPCDAFinalizeStatic()
{
    IDE_TEST(gIPCDALock.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

static IDE_RC cmbShmIPCDARemoveOldCannel()
{
    SInt         sCount                = 0;
    FILE        *sIpcLogFile           = NULL;

    key_t        sShmKey               = 0;
    key_t        sShmKeyForSimpleQuery = 0;
    key_t        sSemKey               = 0;
    union semun sArg;

    PDL_HANDLE   sShmID               = PDL_INVALID_HANDLE;
    PDL_HANDLE   sShmIDForSimpleQuery = PDL_INVALID_HANDLE;
    SInt         sSemID               = 0;

    SInt         sShmSize             = 0;
    SInt         sSemCnt              = 0;

    SInt         sFlag = 0666;

    sIpcLogFile = idlOS::fopen(gIPCDALogFile, PDL_TEXT("r"));
    IDE_TEST_RAISE(sIpcLogFile == NULL, file_open_error);

    sCount = idlOS::fread((void*)&sShmKey, sizeof(key_t), 1, sIpcLogFile);
    IDE_TEST_RAISE(sCount != 1, file_read_error);

    sCount = idlOS::fread((void*)&sShmKeyForSimpleQuery, sizeof(key_t), 1, sIpcLogFile);
    IDE_TEST_RAISE(sCount != 1, file_read_error);

    sCount = idlOS::fread( (void*) &sShmSize, sizeof(SInt), 1, sIpcLogFile);
    IDE_TEST_RAISE(sCount != 1, file_read_error);

    sShmID = idlOS::shmget( sShmKey, sShmSize, sFlag);
    IDE_TEST_RAISE(sShmID == PDL_INVALID_HANDLE, shmget_error);
    IDE_TEST_RAISE( idlOS::shmctl( sShmID, IPC_RMID, NULL ) != 0, shmctl_error);

    sShmIDForSimpleQuery = idlOS::shmget( sShmKeyForSimpleQuery, sShmSize, sFlag);
    IDE_TEST_RAISE(sShmIDForSimpleQuery == PDL_INVALID_HANDLE, shmget_error);
    IDE_TEST_RAISE( idlOS::shmctl( sShmIDForSimpleQuery, IPC_RMID, NULL ) != 0, shmctl_error);

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
    (void)idlOS::unlink(gIPCDALogFile);

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
    (void)idlOS::unlink(gIPCDALogFile);

    return IDE_FAILURE;
}

static IDE_RC cmbShmIPCDAWriteLog(FILE* aLogFile, void* aBuffer, SInt aSize)
{
    SInt sSize = 0;

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


IDE_RC cmbShmIPCDACreate(SInt aMaxChannelCount)
{
    UInt   i                      = 0;
    SInt   j                      = 0;
    SInt   sFlag                  = 0;
    FILE*  sLogFile               = NULL;
    ULong  sShmSize               = 0;
    ULong  sShmIPCDADataBlockSize = 0;
    SInt   sSemCount              = 0;
    UInt   sMaxCount              = 0;
    union  semun sArg;

    /* IPCDA 초기화 */
    gIPCDAShmInfo.mShmKey          = CMB_INVALID_IPCDA_SHM_KEY;
    gIPCDAShmInfo.mShmID           = PDL_INVALID_HANDLE;
    gIPCDAShmInfo.mSemChannelKey   = NULL;
    gIPCDAShmInfo.mSemChannelID    = NULL;
    gIPCDAShmInfo.mShmBuffer       = NULL;
    gIPCDAShmInfo.mChannelList     = NULL;
    gIPCDAShmInfo.mClientCount     = 0;
    gIPCDAShmInfo.mMaxChannelCount = aMaxChannelCount;

    gIPCDAShmInfo.mShmKeyForSimpleQuery    = CMB_INVALID_IPCDA_SHM_KEY;
    gIPCDAShmInfo.mShmIDForSimpleQuery     = PDL_INVALID_HANDLE;
    gIPCDAShmInfo.mShmBufferForSimpleQuery = NULL;

    // fix BUG-18830
    // bug-27250 free Buf list can be crushed when client killed
    // mMaxBufferCount는  원래 semaphore값을 초기화하는데 사용된다.
    // 변경전: channel수 * extra_buffer 수(8)
    // 변경후: 고정값 10
    gIPCDAShmInfo.mMaxBufferCount  = CMB_SHM_SEMA_UNDO_VALUE;

    /* 비정상 종료시 생성한 자원 해제 */
    idlOS::snprintf(gIPCDALogFile,
                    ID_SIZEOF(gIPCDALogFile),
                    PDL_TEXT("%s%s%s"),
                    idp::getHomeDir(),
                    IDL_FILE_SEPARATORS,
                    cmuIPCDALogFileFromHome());

    (void)cmbShmIPCDARemoveOldCannel();

    if (gIPCDAShmInfo.mMaxChannelCount > 0)
    {
        /* PROJ-2065 Limit-Enviroment Test
         *
         * TC/Server/LimitEnv/cm/cmb/cmbShmIPCDA_cmbShmIPCDACreate1.sql
         */
        IDU_LIMITPOINT_RAISE("cmbShmIPCDA::cmbShmIPCDACreate::malloc::IPCDASemChannelKey", InsufficientMemory );
        
        // initialize channel semaphore key
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_ID,
                                         sizeof(key_t) * gIPCDAShmInfo.mMaxChannelCount,
                                         (void**)&gIPCDAShmInfo.mSemChannelKey)
                       != IDE_SUCCESS, InsufficientMemory );

        for (i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++)
        {
            gIPCDAShmInfo.mSemChannelKey[i] = -1;
        }

        IDU_FIT_POINT_RAISE( "cmbShmIPCDA::cmbShmIPCDACreate::malloc::IPCDASemChannelID",
                              InsufficientMemory );
        
        // initialize channel semaphore ID
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_ID,
                                         sizeof(SInt) * gIPCDAShmInfo.mMaxChannelCount,
                                         (void**)&gIPCDAShmInfo.mSemChannelID)
                       != IDE_SUCCESS, InsufficientMemory );

        for (i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++)
        {
            gIPCDAShmInfo.mSemChannelID[i] = -1;
        }

        IDU_FIT_POINT_RAISE( "cmbShmIPCDA::cmbShmIPCDACreate::malloc::IPCDAChannelList",
                              InsufficientMemory );

        // initialize channel list
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_ID,
                                         sizeof(idBool) * gIPCDAShmInfo.mMaxChannelCount,
                                         (void**)&gIPCDAShmInfo.mChannelList)
                       != IDE_SUCCESS, InsufficientMemory );

        for ( i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++ )
        {
            gIPCDAShmInfo.mChannelList[i] = ID_FALSE;
        }

        // create ipc log file

        sLogFile = idlOS::fopen(gIPCDALogFile, PDL_TEXT("w"));
        IDE_TEST_RAISE(sLogFile == NULL, err_open_ipc_logfile);

        // set shared memory key and semaphore key
        gIPCDAShmInfo.mShmKey           = idlOS::getpid() % 10000 + 20000;
        gIPCDAShmInfo.mSemChannelKey[0] = gIPCDAShmInfo.mShmKey + 20000;

        /* ------------------------------------------------
         *  HP 64bit의 경우 32비트 Clinet에서
         *  Shared Memory를 Attch 할 수 있도록,
         *  compatible option : IPC_SHARE32를 주어야 한다.
         *  by gamestar
         * ----------------------------------------------*/
#if (defined(HP_HPUX) || defined(IA64_HP_HPUX)) && defined(COMPILE_64BIT)
        sFlag = 0666 | IPC_CREAT | IPC_EXCL | IPC_SHARE32;
#else
        sFlag = 0666 | IPC_CREAT | IPC_EXCL;
#endif

        // create shared memory
        sShmSize = cmbShmIPCDAGetSize(gIPCDAShmInfo.mMaxChannelCount);
        sShmIPCDADataBlockSize = cmbShmIPCDAGetDataBlockSize(gIPCDAShmInfo.mMaxChannelCount);

#if !defined(COMPILE_64BIT)
        IDE_TEST_RAISE(sShmSize >= ID_UINT_MAX, overflow_memory_size_32bit);
#endif

        while (gIPCDAShmInfo.mShmID == PDL_INVALID_HANDLE)
        {
            gIPCDAShmInfo.mShmID = idlOS::shmget(gIPCDAShmInfo.mShmKey, sShmSize, sFlag);
            if (gIPCDAShmInfo.mShmID != PDL_INVALID_HANDLE)
            {
                break;
            }
            gIPCDAShmInfo.mShmKey++;
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

        // set shared memory key and semaphore key for IPCDA_data_shm
        gIPCDAShmInfo.mShmKeyForSimpleQuery = gIPCDAShmInfo.mShmKey + 1;

        // Data channel for simpleQuery Fetch
        while (gIPCDAShmInfo.mShmIDForSimpleQuery == PDL_INVALID_HANDLE)
        {
            gIPCDAShmInfo.mShmIDForSimpleQuery = idlOS::shmget(gIPCDAShmInfo.mShmKeyForSimpleQuery, sShmIPCDADataBlockSize, sFlag);
            if (gIPCDAShmInfo.mShmIDForSimpleQuery != PDL_INVALID_HANDLE)
            {
                break;
            }
            gIPCDAShmInfo.mShmKeyForSimpleQuery++;
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

        IDE_TEST(cmbShmIPCDAWriteLog(sLogFile, (void*)&gIPCDAShmInfo.mShmKey, ID_SIZEOF(key_t)) != IDE_SUCCESS);
        IDE_TEST(cmbShmIPCDAWriteLog(sLogFile, (void*)&gIPCDAShmInfo.mShmKeyForSimpleQuery, ID_SIZEOF(key_t)) != IDE_SUCCESS);
        IDE_TEST(cmbShmIPCDAWriteLog(sLogFile, (void*)&sShmSize, ID_SIZEOF(SInt)) != IDE_SUCCESS);

        gIPCDAShmInfo.mShmBuffer = (UChar*) idlOS::shmat( gIPCDAShmInfo.mShmID, NULL, 0 );
        IDE_TEST_RAISE( gIPCDAShmInfo.mShmBuffer == (void*) -1, err_attach_shm );
        gIPCDAShmInfo.mShmBufferForSimpleQuery = (UChar*) idlOS::shmat( gIPCDAShmInfo.mShmIDForSimpleQuery, NULL, 0 );
        IDE_TEST_RAISE( gIPCDAShmInfo.mShmBufferForSimpleQuery == (void*) -1, err_attach_shm );

        // Shared Memory Buffer 초기화
        sMaxCount = gIPCDAShmInfo.mMaxChannelCount;
        
        // create channel semaphores

        sSemCount = IPCDA_SEM_CNT_PER_CHANNEL;

        sArg.val = 1;

        for (i = 0; i < sMaxCount; i++)
        {
            while (gIPCDAShmInfo.mSemChannelID[i] < 0)
            {
                gIPCDAShmInfo.mSemChannelID[i] = idlOS::semget(gIPCDAShmInfo.mSemChannelKey[i],
                                                               sSemCount,
                                                               sFlag);
                if (gIPCDAShmInfo.mSemChannelID[i] >= 0)
                {
                    if ((i+1) < gIPCDAShmInfo.mMaxChannelCount)
                    {
                        gIPCDAShmInfo.mSemChannelKey[i+1] = gIPCDAShmInfo.mSemChannelKey[i] + 1;
                    }
                    for (j=0; j<sSemCount; j++ )
                    {
                        IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[i],
                                                     j,
                                                     SETVAL,
                                                     sArg)
                                        != 0, err_sem_initial);
                    }

                    break;
                }
                gIPCDAShmInfo.mSemChannelKey[i]++;
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
                cmbShmIPCDAChannelInfo *sShmHeader = NULL;
                sShmHeader = cmbShmIPCDAGetChannelInfo(gIPCDAShmInfo.mShmBuffer, i);

                sShmHeader->mPID               = 0;
                sShmHeader->mTicketNum         = 0;  // BUG-32398 타임스탬프에서 티켓번호로 변경
            }
            IDE_TEST(cmbShmIPCDAWriteLog(sLogFile, (void*)&gIPCDAShmInfo.mSemChannelKey[i], ID_SIZEOF(key_t)) != IDE_SUCCESS);
            IDE_TEST(cmbShmIPCDAWriteLog(sLogFile, (void*)&sSemCount, ID_SIZEOF(SInt)) != IDE_SUCCESS);
        } // for

        /* bug-31572: IPC dispatcher hang might occur on AIX.
         * Each semid for some channel_ids printed for debugging.
         * This does not have any big meaning. */
        for (i = 0; i < sMaxCount; i++)
        {
            /* print only 0, 10, 20, ... among IDs */
            if ((i%10) == 0)
            {
                ideLog::log(IDE_SERVER_2, "[IPCDA channel info] "
                            "channel[%3"ID_INT32_FMT"] "
                            "semid: %"ID_INT32_FMT,
                            i,
                            gIPCDAShmInfo.mSemChannelID[i]);
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
        IDE_SET(ideSetErrorCode(idERR_FATAL_FILE_OPEN, gIPCDALogFile));
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
        ideLog::log(IDE_SERVER_0, "Not Enough Shared Memory : errno=%"ID_UINT32_FMT"\n", errno);
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

    if (gIPCDAShmInfo.mShmID != PDL_INVALID_HANDLE)
    {
        (void)idlOS::shmctl(gIPCDAShmInfo.mShmID, IPC_RMID, NULL);
        gIPCDAShmInfo.mShmID = PDL_INVALID_HANDLE;
    }

    if (gIPCDAShmInfo.mShmIDForSimpleQuery != PDL_INVALID_HANDLE)
    {
        (void)idlOS::shmctl(gIPCDAShmInfo.mShmIDForSimpleQuery, IPC_RMID, NULL);
        gIPCDAShmInfo.mShmIDForSimpleQuery = PDL_INVALID_HANDLE;
    }

    gIPCDAShmInfo.mShmKey               = CMB_INVALID_IPCDA_SHM_KEY;
    gIPCDAShmInfo.mShmKeyForSimpleQuery = CMB_INVALID_IPCDA_SHM_KEY;

    if (gIPCDAShmInfo.mSemChannelID != NULL)
    {
        for (i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++)
        {
            if (gIPCDAShmInfo.mSemChannelID[i] >= 0)
            {
                (void)idlOS::semctl(gIPCDAShmInfo.mSemChannelID[i], 0, IPC_RMID, sSemCtlArg);
                gIPCDAShmInfo.mSemChannelID[i] = PDL_INVALID_HANDLE;
            }
        }
        (void)iduMemMgr::free(gIPCDAShmInfo.mSemChannelID);
        gIPCDAShmInfo.mSemChannelID = NULL;
    }

    if (gIPCDAShmInfo.mSemChannelKey != NULL)
    {
        (void)iduMemMgr::free(gIPCDAShmInfo.mSemChannelKey);
        gIPCDAShmInfo.mSemChannelKey = NULL;
    }

    /* BUG-36775 CodeSonar warning : Leak */
    if( sLogFile != NULL )
    {
        (void)idlOS::fclose(sLogFile);
    }

    return IDE_FAILURE;
}

IDE_RC cmbShmIPCDADestroy()
{
    UInt        i = 0;
    union semun sSemCtlArg;

    if (gIPCDAShmInfo.mMaxChannelCount > 0)
    {
        idlOS::memset(&sSemCtlArg, 0, ID_SIZEOF(union semun));

        // Channel Semaphore 제거
        if (gIPCDAShmInfo.mSemChannelID != NULL)
        {
            for (i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++)
            {
                IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[i],
                                             0,
                                             IPC_RMID,
                                             sSemCtlArg)
                               != 0, SemRemoveError);
                gIPCDAShmInfo.mSemChannelID[i] = PDL_INVALID_HANDLE;
            }
        }

        gIPCDAShmInfo.mMaxChannelCount = 0;

        // Channel 메모리 제거
        if (gIPCDAShmInfo.mSemChannelID != NULL)
        {
            IDE_TEST(iduMemMgr::free(gIPCDAShmInfo.mSemChannelID) != IDE_SUCCESS);
            gIPCDAShmInfo.mSemChannelID  = NULL;
        }

        if (gIPCDAShmInfo.mSemChannelKey != NULL)
        {
            IDE_TEST(iduMemMgr::free(gIPCDAShmInfo.mSemChannelKey) != IDE_SUCCESS);
            gIPCDAShmInfo.mSemChannelKey = NULL;
        }

        if (gIPCDAShmInfo.mChannelList != NULL)
        {
            IDE_TEST(iduMemMgr::free(gIPCDAShmInfo.mChannelList) != IDE_SUCCESS);
            gIPCDAShmInfo.mChannelList = NULL;
        }

        if (gIPCDAShmInfo.mShmID != PDL_INVALID_HANDLE)
        {
            IDE_TEST_RAISE(idlOS::shmdt(gIPCDAShmInfo.mShmBuffer) != 0, ShmRemoveError);

            /*
             * BUG-32403 (for Windriver)
             *
             * Windriver OS는 shmdt() 과정에서 공유메모리가
             * 삭제되어 shmctl()에서 항상 서버가 비정상 종료된다.
             * 공유메모리 상태를 체크하고 삭제하자.
             */
            if (idlOS::shmget(gIPCDAShmInfo.mShmKey, 0, 0) != -1)
            {
                IDE_TEST_RAISE(idlOS::shmctl(gIPCDAShmInfo.mShmID, IPC_RMID, NULL)
                               != 0, ShmRemoveError);
            }
            gIPCDAShmInfo.mShmID = PDL_INVALID_HANDLE;
        }

        if (gIPCDAShmInfo.mShmIDForSimpleQuery != PDL_INVALID_HANDLE)
        {
            IDE_TEST_RAISE(idlOS::shmdt(gIPCDAShmInfo.mShmBufferForSimpleQuery) != 0, ShmRemoveError);

            /*
             * BUG-32403 (for Windriver)
             *
             * Windriver OS는 shmdt() 과정에서 공유메모리가
             * 삭제되어 shmctl()에서 항상 서버가 비정상 종료된다.
             * 공유메모리 상태를 체크하고 삭제하자.
             */
            if (idlOS::shmget(gIPCDAShmInfo.mShmKeyForSimpleQuery, 0, 0) != -1)
            {
                IDE_TEST_RAISE(idlOS::shmctl(gIPCDAShmInfo.mShmIDForSimpleQuery, IPC_RMID, NULL)
                               != 0, ShmRemoveError);
            }
            gIPCDAShmInfo.mShmIDForSimpleQuery = PDL_INVALID_HANDLE;
        }

        gIPCDAShmInfo.mShmKey               = CMB_INVALID_IPCDA_SHM_KEY;
        gIPCDAShmInfo.mShmKeyForSimpleQuery = CMB_INVALID_IPCDA_SHM_KEY;

        (void)idlOS::unlink(gIPCDALogFile);
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

    if (gIPCDAShmInfo.mShmID != PDL_INVALID_HANDLE)
    {
        (void)idlOS::shmctl(gIPCDAShmInfo.mShmID, IPC_RMID, NULL);
        gIPCDAShmInfo.mShmID = PDL_INVALID_HANDLE;
    }

    if (gIPCDAShmInfo.mShmIDForSimpleQuery != PDL_INVALID_HANDLE)
    {
        (void)idlOS::shmctl(gIPCDAShmInfo.mShmIDForSimpleQuery, IPC_RMID, NULL);
        gIPCDAShmInfo.mShmIDForSimpleQuery = PDL_INVALID_HANDLE;
    }

    gIPCDAShmInfo.mShmKey               = CMB_INVALID_IPCDA_SHM_KEY;
    gIPCDAShmInfo.mShmKeyForSimpleQuery = CMB_INVALID_IPCDA_SHM_KEY;

    if (gIPCDAShmInfo.mSemChannelID != NULL)
    {
        for (i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++)
        {
            if (gIPCDAShmInfo.mSemChannelID[i] >= 0)
            {
                (void)idlOS::semctl(gIPCDAShmInfo.mSemChannelID[i], 0, IPC_RMID, sSemCtlArg);
                gIPCDAShmInfo.mSemChannelID[i] = PDL_INVALID_HANDLE;
            }
        }
        (void)iduMemMgr::free(gIPCDAShmInfo.mSemChannelID);
        gIPCDAShmInfo.mSemChannelID = NULL;
    }

    if (gIPCDAShmInfo.mSemChannelKey != NULL)
    {
        (void)iduMemMgr::free(gIPCDAShmInfo.mSemChannelKey);
        gIPCDAShmInfo.mSemChannelKey = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC cmbShmIPCDAAttach()
{
    ULong sShmSize          = 0;
    ULong sShmIPCDADataBlockSize = 0;
    SInt  sFlag             = 0666;

    if (gIPCDAShmInfo.mShmID == PDL_INVALID_HANDLE)
    {
        sShmSize = cmbShmIPCDAGetSize(gIPCDAShmInfo.mMaxChannelCount);
        gIPCDAShmInfo.mShmID = idlOS::shmget(gIPCDAShmInfo.mShmKey, sShmSize, sFlag);
        IDE_TEST(gIPCDAShmInfo.mShmID == PDL_INVALID_HANDLE);
    }

    if (gIPCDAShmInfo.mShmIDForSimpleQuery == PDL_INVALID_HANDLE)
    {
        sShmIPCDADataBlockSize  = cmbShmIPCDAGetDataBlockSize(gIPCDAShmInfo.mMaxChannelCount);
        gIPCDAShmInfo.mShmIDForSimpleQuery = idlOS::shmget(gIPCDAShmInfo.mShmKeyForSimpleQuery, sShmIPCDADataBlockSize, sFlag);
        IDE_TEST(gIPCDAShmInfo.mShmIDForSimpleQuery == PDL_INVALID_HANDLE);
    }

    if (gIPCDAShmInfo.mShmBuffer == NULL)
    {
        gIPCDAShmInfo.mShmBuffer = (UChar*)idlOS::shmat(gIPCDAShmInfo.mShmID, NULL, 0);
        IDE_TEST_RAISE((void*)-1 == gIPCDAShmInfo.mShmBuffer, ShmAttachError);
    }

    if (gIPCDAShmInfo.mShmBufferForSimpleQuery == NULL)
    {
        gIPCDAShmInfo.mShmBufferForSimpleQuery = (UChar*)idlOS::shmat(gIPCDAShmInfo.mShmIDForSimpleQuery, NULL, 0);
        IDE_TEST_RAISE((void*)-1 == gIPCDAShmInfo.mShmBufferForSimpleQuery, ShmAttachError);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ShmAttachError)
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SHM_ATTACH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void cmbShmIPCDADetach()
{
    if (gIPCDAShmInfo.mShmBuffer != NULL)
    {
        (void)idlOS::shmdt(gIPCDAShmInfo.mShmBuffer);
        gIPCDAShmInfo.mShmBuffer = NULL;
    }

    if (gIPCDAShmInfo.mShmBufferForSimpleQuery != NULL)
    {
        (void)idlOS::shmdt(gIPCDAShmInfo.mShmBufferForSimpleQuery);
        gIPCDAShmInfo.mShmBufferForSimpleQuery = NULL;
    }
}

#endif
