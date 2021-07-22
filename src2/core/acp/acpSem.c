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
 * $Id: acpSem.c 3978 2008-12-19 08:04:42Z djin $
 ******************************************************************************/

#include <aclHash.h>
#include <acpSem.h>
#include <acpThr.h>
#include <acpPrintf.h>

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

#define ACP_SEM_HASH_BUCKET_SIZE 10

static acl_hash_table_t gAcpSemHashTable;
static acp_thr_once_t   gAcpSemOnceControl = ACP_THR_ONCE_INITIALIZER;
static volatile acp_bool_t gAcpSemInitialized = ACP_FALSE;

static void acpSemHashInitialize(void)
{
    (void)aclHashCreate(&gAcpSemHashTable,
                        ACP_SEM_HASH_BUCKET_SIZE,
                        sizeof(acp_key_t),
                        aclHashHashInt32,
                        aclHashCompInt32,
                        ACP_TRUE);

    gAcpSemInitialized = ACP_TRUE;
}

ACP_EXPORT acp_rc_t acpSemCreate(acp_key_t aKey, acp_sint32_t aValue)
{
    acp_char_t            sName[30];
    acp_rc_t              sRC = ACP_RC_SUCCESS;
    SECURITY_ATTRIBUTES   sAttributes;
    HANDLE                sHandle;

    (void)acpSnprintf(sName,
                      sizeof(sName),
                      "Global\\acpSem_%08x",
                      aKey);

    sAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sAttributes.lpSecurityDescriptor = NULL;
    sAttributes.bInheritHandle       = ACP_FALSE;

    sHandle = CreateSemaphore(&sAttributes, aValue, ACP_SINT32_MAX, sName);

    sRC = ACP_RC_GET_OS_ERROR();

    if (sHandle == NULL)
    {
        return sRC;
    }
    else
    {
        if (ACP_RC_IS_SUCCESS(sRC))
        {
            acpThrOnce(&gAcpSemOnceControl, acpSemHashInitialize);

            sRC = aclHashAdd(&gAcpSemHashTable, &aKey, sHandle);
        }
        else if (sRC == ACP_RC_FROM_SYS_ERROR(ERROR_ALREADY_EXISTS))
        {
            (void)CloseHandle(sHandle);
        }
        else
        {
            /* do nothing */
        }

        return sRC;
    }
}

ACP_EXPORT acp_rc_t acpSemDestroy(acp_key_t aKey)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if(ACP_FALSE == gAcpSemInitialized)
    {
        /* Do nothing. No Semaphores were created */
    }
    else
    {
        DWORD    sRet;
        HANDLE   sHandle;

        sRC = aclHashFind(&gAcpSemHashTable, &aKey, &sHandle);

        /* Remove last existing semaphore */
        if (ACP_RC_IS_SUCCESS(sRC) == ACP_TRUE)
        {
            sRet = CloseHandle(sHandle);

            if (sRet == 0)
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                (void)aclHashRemove(&gAcpSemHashTable, &aKey, &sHandle);
            }
        }
        else
        {
            /* do nothing */
        }
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpSemOpen(acp_sem_t *aSem, acp_key_t aKey)
{
    acp_rc_t            sRC;
    acp_char_t          sName[30];
    HANDLE              sHandle;
    SECURITY_ATTRIBUTES sAttributes;

    (void)acpSnprintf(sName, 
                      sizeof(sName),
                      "Global\\acpSem_%08x",
                      aKey);

    sAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sAttributes.lpSecurityDescriptor = NULL;
    sAttributes.bInheritHandle       = ACP_FALSE;
    
    sHandle = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, sName); 
    if(NULL == sHandle)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aSem->mHandle = sHandle;
        sRC = ACP_RC_SUCCESS;
    }
    
    return sRC;
}

ACP_EXPORT acp_rc_t acpSemClose(acp_sem_t *aSem)
{
    BOOL sRet;

    sRet = CloseHandle(aSem->mHandle);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpSemAcquire(acp_sem_t *aSem)
{
    DWORD sRet;

    sRet = WaitForSingleObject(aSem->mHandle, INFINITE);

    if ((sRet == WAIT_ABANDONED) || (sRet == WAIT_OBJECT_0))
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_OS_ERROR();
    }
}

ACP_EXPORT acp_rc_t acpSemTryAcquire(acp_sem_t *aSem)
{
    DWORD sRet;

    sRet = WaitForSingleObject(aSem->mHandle, 0);

    if ((sRet == WAIT_ABANDONED) || (sRet == WAIT_OBJECT_0))
    {
        return ACP_RC_SUCCESS;
    }
    else if (sRet == WAIT_TIMEOUT)
    {
        return ACP_RC_EBUSY;
    }
    else
    {
        return ACP_RC_GET_OS_ERROR();
    }
}

ACP_EXPORT acp_rc_t acpSemRelease(acp_sem_t *aSem)
{
    BOOL sRet;

    sRet = ReleaseSemaphore(aSem->mHandle, 1, NULL);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

#else

typedef union acpSemMutexCtl
{
    acp_sint32_t     mVal;
    struct semid_ds *mBuf;
    unsigned short  *mArray;
} acpSemMutexCtl;

/**
 * creates a semaphore
 *
 * @param aKey key to the semaphore to create
 * @param aValue value of the semahphore
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSemCreate(acp_key_t aKey, acp_sint32_t aValue)
{
    acpSemMutexCtl sCtl;
    acp_sint32_t   sSemID;
    acp_sint32_t   sRet;
    acp_rc_t       sRC;

    if (aKey == IPC_PRIVATE)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    sSemID = semget(aKey, 1, IPC_CREAT | IPC_EXCL | 0666);

    if (sSemID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sCtl.mVal = aValue;

    sRet = semctl(sSemID, 0, SETVAL, sCtl);

    if (sRet != 0)
    {
        sRC       = ACP_RC_GET_OS_ERROR();
        sCtl.mVal = 0;

        (void)semctl(sSemID, 0, IPC_RMID, sCtl);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

/**
 * destroys a semaphore
 * the process that created the semaphore must destroy it
 *
 * @param aKey key to the semaphore
 * @return result code
 *
 * it returns #ACP_RC_EEXIST if the semaphore already exists
 */
ACP_EXPORT acp_rc_t acpSemDestroy(acp_key_t aKey)
{
    acpSemMutexCtl sCtl;
    acp_sint32_t   sSemID;
    acp_sint32_t   sRet;

    if (aKey == IPC_PRIVATE)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    sSemID = semget(aKey, 1, 0666);

    if (sSemID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sCtl.mVal = 0;

    sRet = semctl(sSemID, 0, IPC_RMID, sCtl);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

/**
 * opens a semphoare
 *
 * @param aSem poiter to the semaphore
 * @param aKey key to the semaphore
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if the semaphore does not exist
 */
ACP_EXPORT acp_rc_t acpSemOpen(acp_sem_t *aSem, acp_key_t aKey)
{
    if (aKey == IPC_PRIVATE)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    aSem->mSemID = semget(aKey, 1, 0666);

    if (aSem->mSemID == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

/**
 * closes a semaphore
 *
 * @param aSem pointer to the semaphore
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSemClose(acp_sem_t *aSem)
{
    aSem->mSemID = -1;

    return ACP_RC_SUCCESS;
}

/**
 * acquires a semaphore
 *
 * @param aSem pointer to the semaphore
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSemAcquire(acp_sem_t *aSem)
{
    struct sembuf sOp;
    acp_sint32_t  sRet;
    acp_rc_t      sRC;

    sOp.sem_num = 0;
    sOp.sem_op  = -1;
    sOp.sem_flg = SEM_UNDO;

    while (1)
    {
        sRet = semop(aSem->mSemID, &sOp, 1);

        if (sRet != 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if (ACP_RC_IS_EINTR(sRC))
            {
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
            break;
        }
    }

    return sRC;
}

/**
 * trys to acquire a semaphore
 *
 * @param aSem pointer to the semaphore
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSemTryAcquire(acp_sem_t *aSem)
{
    struct sembuf sOp;
    acp_sint32_t  sRet;
    acp_rc_t      sRC;

    sOp.sem_num = 0;
    sOp.sem_op  = -1;
    sOp.sem_flg = SEM_UNDO | IPC_NOWAIT;

    while (1)
    {
        sRet = semop(aSem->mSemID, &sOp, 1);

        if (sRet != 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if (ACP_RC_IS_EINTR(sRC))
            {
                continue;
            }
            else if (ACP_RC_IS_EAGAIN(sRC))
            {
                sRC = ACP_RC_EBUSY;
                break;
            }
            else
            {
                break;
            }
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
            break;
        }
    }

    return sRC;
}

/**
 * releases a semaphore
 *
 * @param aSem pointer to the semaphore
 * @return result code
 */
ACP_EXPORT acp_rc_t acpSemRelease(acp_sem_t *aSem)
{
    struct sembuf sOp;
    acp_sint32_t  sRet;
    acp_rc_t      sRC;

    sOp.sem_num = 0;
    sOp.sem_op  = 1;
    sOp.sem_flg = SEM_UNDO;

    while (1)
    {
        sRet = semop(aSem->mSemID, &sOp, 1);

        if (sRet != 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if (ACP_RC_IS_EINTR(sRC))
            {
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
            break;
        }
    }

    return sRC;
}

#endif
