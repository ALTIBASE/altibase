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
 * $Id: acpProcMutex.c 2821 2008-07-01 07:51:38Z jykim $
 ******************************************************************************/

#include <acpPrintf.h>
#include <acpProcMutex.h>
#include <acpRand.h>
#include <acpSem.h>


#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_EXPORT acp_rc_t acpProcMutexCreate(acp_key_t aKey)
{
    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpProcMutexDestroy(acp_key_t aKey)
{
    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpProcMutexOpen(acp_proc_mutex_t *aMutex, acp_key_t aKey)
{
    acp_char_t sName[30];

    (void)acpSnprintf(sName,
                      sizeof(sName),
                      "Global\\acpProcMutex_%08x",
                      aKey);

    aMutex->mHandle = CreateMutex(NULL, FALSE, sName);

    if (aMutex->mHandle == NULL)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpProcMutexClose(acp_proc_mutex_t *aMutex)
{
    BOOL sRet;

    sRet = CloseHandle(aMutex->mHandle);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpProcMutexLock(acp_proc_mutex_t *aMutex)
{
    DWORD sRet;

    sRet = WaitForSingleObject(aMutex->mHandle, INFINITE);

    if ((sRet == WAIT_ABANDONED) || (sRet == WAIT_OBJECT_0))
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_OS_ERROR();
    }
}

ACP_EXPORT acp_rc_t acpProcMutexTryLock(acp_proc_mutex_t *aMutex)
{
    DWORD sRet;

    sRet = WaitForSingleObject(aMutex->mHandle, 0);

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

ACP_EXPORT acp_rc_t acpProcMutexUnlock(acp_proc_mutex_t *aMutex)
{
    BOOL sRet;

    sRet = ReleaseMutex(aMutex->mHandle);

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

/**
 * creates and opens a process mutex
 *
 * @param aKey key to the process mutex to create
 * @return result code
 *
 * it returns #ACP_RC_EEXIST if the process mutex already exists
 */
ACP_EXPORT acp_rc_t acpProcMutexCreate(acp_key_t aKey)
{
    return acpSemCreate(aKey, 1);
}

/**
 * destroys a process mutex
 *
 * @param aKey key to the process mutex to destroy
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcMutexDestroy(acp_key_t aKey)
{
    return acpSemDestroy(aKey);
}

/**
 * opens a process mutex
 *
 * @param aMutex pointer to the process mutex
 * @param aKey key to the process mutex
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if the process mutex does not exist
 */
ACP_EXPORT acp_rc_t acpProcMutexOpen(acp_proc_mutex_t *aMutex, acp_key_t aKey)
{
    return acpSemOpen(&aMutex->mSem, aKey);
}

/**
 * closes a process mutex
 *
 * @param aMutex pointer to the process mutex
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcMutexClose(acp_proc_mutex_t *aMutex)
{
    return acpSemClose(&aMutex->mSem);
}

/**
 * locks a process mutex
 *
 * @param aMutex pointer to the process mutex
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcMutexLock(acp_proc_mutex_t *aMutex)
{
    return acpSemAcquire(&aMutex->mSem);
}

/**
 * trys lock a process mutex
 *
 * @param aMutex pointer to the process mutex
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcMutexTryLock(acp_proc_mutex_t *aMutex)
{
    return acpSemTryAcquire(&aMutex->mSem);
}

/**
 * unlocks a process mutex
 *
 * @param aMutex pointer to the process mutex
 * @return result code
 */
ACP_EXPORT acp_rc_t acpProcMutexUnlock(acp_proc_mutex_t *aMutex)
{
    return acpSemRelease(&aMutex->mSem);
}

#endif
