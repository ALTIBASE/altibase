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
 
/*******************************************************************************
 * $Id: acpShm.c 5857 2009-06-02 10:39:22Z djin $
 ******************************************************************************/

#include <acpEnv.h>
#include <acpFile.h>
#include <acpShm.h>
#include <acpPrintf.h>


#define ACP_SHM_MAX_PERM    (   \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

static acp_char_t *gAcpShmEnvTemp = "TEMP";

ACP_INLINE acp_rc_t acpShmMakePath(acp_char_t   *aPath,
                                   acp_sint32_t  aLen,
                                   acp_key_t     aKey)
{
    acp_rc_t    sRC;
    acp_char_t *sTempDir;

    sRC = acpEnvGet(gAcpShmEnvTemp, &sTempDir);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        return acpSnprintf(aPath, aLen,
                           "%s\\acpShm_%08x", sTempDir, aKey);
    }
}

ACP_EXPORT acp_rc_t acpShmCreate(
        acp_key_t   aKey,
        acp_size_t  aSize,
        acp_mode_t  aPermission)
{
    acp_file_t sFile;
    acp_rc_t   sRC;
    acp_char_t sPath[MAX_PATH];

    if(aPermission != (aPermission & ACP_SHM_MAX_PERM))
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* Do nothing */
    }

    sRC = acpShmMakePath(sPath, sizeof(sPath), aKey);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpFileOpen(&sFile,
                      sPath,
                      ACP_O_RDWR | ACP_O_CREAT | ACP_O_EXCL,
                      aPermission);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpFileTruncate(&sFile, aSize);

    (void)acpFileClose(&sFile);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpFileRemove(sPath);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpShmDestroy(acp_key_t aKey)
{
    acp_rc_t   sRC;
    acp_char_t sPath[MAX_PATH];

    sRC = acpShmMakePath(sPath, sizeof(sPath), aKey);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    return acpFileRemove(sPath);
}

ACP_EXPORT acp_rc_t acpShmAttach(acp_shm_t *aShm, acp_key_t aKey)
{
    DWORD      sLowFileSize  = 0;
    DWORD      sHighFileSize = 0;
    acp_file_t sFile;
    acp_rc_t   sRC;
    acp_char_t sPath[MAX_PATH];

    sRC = acpShmMakePath(sPath, sizeof(sPath), aKey);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /* Preserve current permission */
    sRC = acpFileOpen(&sFile, sPath, ACP_O_RDWR, 0);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sLowFileSize = GetFileSize(sFile.mHandle, &sHighFileSize);

    if (sLowFileSize == INVALID_FILE_SIZE)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        (void)acpFileClose(&sFile);

        return sRC;
    }
    else
    {
#if defined(ACP_CFG_COMPILE_64BIT)
        aShm->mSize = ((acp_size_t)sHighFileSize << 32) + sLowFileSize;
#elif defined(ACP_CFG_COMPILE_32BIT)
        if (sHighFileSize > 0)
        {
            (void)acpFileClose(&sFile);

            return ACP_RC_ENOMEM;
        }
        else
        {
            aShm->mSize = sLowFileSize;
        }
#else
#error unknown compile bit
#endif
    }

    aShm->mHandle = CreateFileMapping(sFile.mHandle,
                                      NULL,
                                      PAGE_READWRITE,
                                      sHighFileSize,
                                      sLowFileSize,
                                      NULL);

    sRC = ACP_RC_GET_OS_ERROR();

    (void)acpFileClose(&sFile);

    if (aShm->mHandle == NULL)
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    aShm->mAddr = MapViewOfFile(aShm->mHandle,
                                FILE_MAP_READ | FILE_MAP_WRITE,
                                0,
                                0,
                                aShm->mSize);

    if (aShm->mAddr == NULL)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        (void)CloseHandle(aShm->mHandle);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpShmDetach(acp_shm_t *aShm)
{
    (void)UnmapViewOfFile(aShm->mAddr);
    (void)CloseHandle(aShm->mHandle);

    return ACP_RC_SUCCESS;
}

#else

/**
 * creates a @a aSize byte(s) of shared memory at @a aPath
 *
 * @param aKey key to the shared memory to create
 * @param aSize size of shared memory to create
 * @param aPermission permissions granted
 *        to the owner, group and world
 * @return result code
 */
ACP_EXPORT acp_rc_t acpShmCreate(
        acp_key_t       aKey,
        acp_size_t      aSize,
        acp_mode_t      aPermission
        )
{
    acp_sint32_t sShmID;

    if(aPermission != (aPermission & ACP_SHM_MAX_PERM))
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* Do nothing */
    }

    if (aKey == IPC_PRIVATE)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    sShmID = shmget(aKey, aSize, IPC_CREAT | IPC_EXCL | aPermission);

    if (sShmID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

/**
 * destroys the shared memory
 *
 * @param aKey key to the shared memory to destroy
 * @return result code
 */
ACP_EXPORT acp_rc_t acpShmDestroy(acp_key_t aKey)
{
    acp_sint32_t sShmID;
    acp_sint32_t sRet;

    if (aKey == IPC_PRIVATE)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    sShmID = shmget(aKey, 0, SHM_R | SHM_W);

    if (sShmID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRet = shmctl(sShmID, IPC_RMID, NULL);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

/**
 * attaches the shared memory
 *
 * @param aShm pointer to the shared memory object
 * @param aKey key to the shared memory to attach
 * @return result code
 */
ACP_EXPORT acp_rc_t acpShmAttach(acp_shm_t *aShm, acp_key_t aKey)
{
    struct shmid_ds sStat;
    acp_sint32_t    sRet;
    acp_rc_t        sRC;

    aShm->mShmID = shmget(aKey, 0, SHM_R | SHM_W);

    if (aShm->mShmID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    aShm->mAddr = shmat(aShm->mShmID, NULL, 0);

    if (aShm->mAddr == (void *)-1)
    {
        aShm->mAddr = NULL;

        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRet = shmctl(aShm->mShmID, IPC_STAT, &sStat);

        if (sRet == -1)
        {
            sRC = ACP_RC_GET_OS_ERROR();

            (void)shmdt(aShm->mAddr);

            return sRC;
        }
        else
        {
            aShm->mSize = sStat.shm_segsz;
        }
    }

    return ACP_RC_SUCCESS;
}

/**
 * detaches the shared memory
 *
 * @param aShm pointer to the shared memory object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpShmDetach(acp_shm_t *aShm)
{
    acp_sint32_t sRet;

    sRet = shmdt(aShm->mAddr);

    if (sRet == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

#endif
