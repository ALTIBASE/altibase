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
 * $Id: acpThrMutex.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpThrMutex.h>
#include <acpPrintf.h>
#include <acpOS.h>


#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

/*
 * lock
 */
ACP_EXPORT acp_rc_t acpThrMutexLockCS(acp_thr_mutex_t *aMutex)
{
    EnterCriticalSection(&(aMutex->mMech.mCSHandle));

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrMutexLockEventAndMutex(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC;
    DWORD    sRet;

    sRet = WaitForSingleObject(aMutex->mMech.mHandle, INFINITE);

    if ((sRet != WAIT_OBJECT_0) && (sRet != WAIT_ABANDONED))
    {
        if (sRet == WAIT_TIMEOUT)
        {
            sRC = ACP_RC_EBUSY;
        }
        else
        {
            /* WAIT_FAILED */
            sRC = ACP_RC_GET_OS_ERROR();
        }
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

/*
 * trylock
 */
ACP_EXPORT acp_rc_t acpThrMutexTryLockCS(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC;
    BOOL     sRet;

    sRet = TryEnterCriticalSection(&(aMutex->mMech.mCSHandle));

    if (sRet == 0)
    {
        sRC = ACP_RC_EBUSY;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrMutexTryLockEventAndMutex(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC;
    DWORD    sRet;

    sRet = WaitForSingleObject(aMutex->mMech.mHandle, 0);

    if ((sRet != WAIT_OBJECT_0) && (sRet != WAIT_ABANDONED))
    {
        if (sRet == WAIT_TIMEOUT)
        {
            sRC = ACP_RC_EBUSY;
        }
        else
        {
            /* WAIT_FAILED */
            sRC = ACP_RC_GET_OS_ERROR();
        }
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

/*
 * unlock
 */
ACP_EXPORT acp_rc_t acpThrMutexUnlockCS(acp_thr_mutex_t *aMutex)
{
    LeaveCriticalSection(&(aMutex->mMech.mCSHandle));

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrMutexUnlockEvent(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC;
    DWORD    sRet;

    sRet = SetEvent(aMutex->mMech.mHandle);

    if (sRet == 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrMutexUnlockMutex(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC;
    DWORD    sRet;

    sRet = ReleaseMutex(aMutex->mMech.mHandle);

    if (sRet == 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
}

/*
 * destroy
 */
ACP_EXPORT acp_rc_t acpThrMutexDestroyCS(acp_thr_mutex_t *aMutex)
{
    DeleteCriticalSection(&aMutex->mMech.mCSHandle);

    aMutex->mInitFlag = 0;

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrMutexDestroyEventAndMutex(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC;
    DWORD    sRet;

    sRet = CloseHandle(aMutex->mMech.mHandle);

    if (sRet == 0)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aMutex->mInitFlag = 0;
        sRC               = ACP_RC_SUCCESS;
    }

    return sRC;
}

static acpThrMutexFunc gAcpThrMutexCS =
{
    acpThrMutexLockCS,
    acpThrMutexTryLockCS,
    acpThrMutexUnlockCS,
    acpThrMutexDestroyCS
};

static acpThrMutexFunc gAcpThrMutexEvent =
{
    acpThrMutexLockEventAndMutex,
    acpThrMutexTryLockEventAndMutex,
    acpThrMutexUnlockEvent,
    acpThrMutexDestroyEventAndMutex
};

static acpThrMutexFunc gAcpThrMutexMutex =
{
    acpThrMutexLockEventAndMutex,
    acpThrMutexTryLockEventAndMutex,
    acpThrMutexUnlockMutex,
    acpThrMutexDestroyEventAndMutex
};

ACP_EXPORT acp_rc_t acpThrMutexCreate(acp_thr_mutex_t *aMutex,
                                      acp_sint32_t     aFlag)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if (aFlag == ACP_THR_MUTEX_DEFAULT)
    {
        aMutex->mMech.mHandle = CreateEvent(NULL, FALSE, TRUE, NULL);

        if (aMutex->mMech.mHandle == NULL)
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
        else
        {
            aMutex->mOp = &gAcpThrMutexEvent;
        }
    }
    else
    {
        if (acpOSGetVersion() >= ACP_OS_WIN_NT)
        {
            InitializeCriticalSection(&(aMutex->mMech.mCSHandle));
            aMutex->mOp = &gAcpThrMutexCS;
        }
        else
        {
            aMutex->mMech.mHandle = CreateMutex(NULL, FALSE, NULL);

            if (aMutex->mMech.mHandle == NULL)
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            else
            {
                aMutex->mOp = &gAcpThrMutexMutex;
            }
        }
    }

    /* mutex object was created completly */
    aMutex->mInitFlag = 2;

    return sRC;
}

#else

/**
 * creates a thread mutex
 *
 * @param aMutex pointer to the thread mutex
 * @param aFlag flag for mutex attributes
 * @return result code
 *
 * @a aFlag can be one of
 * #ACP_THR_MUTEX_DEFAULT for default fastest mutex,
 * #ACP_THR_MUTEX_ERRORCHECK for error checking mutex,
 * #ACP_THR_MUTEX_RECURSIVE for recursive mutex.
 * #ACP_THR_MUTEX_DEFAULT is used if @a aFlag is zero.
 */
ACP_EXPORT acp_rc_t acpThrMutexCreate(acp_thr_mutex_t *aMutex,
                                      acp_sint32_t     aFlag)
{
    pthread_mutexattr_t sAttr;
    acp_sint32_t        sRet;

#define ACP_RETURN_IF_ERROR(aRet, aCleanup)     \
    do                                          \
    {                                           \
        if ((aRet) != 0)                        \
        {                                       \
            aCleanup ;                          \
                                                \
            return (aRet);                      \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

    if (aFlag != ACP_THR_MUTEX_DEFAULT)
    {
        sRet = pthread_mutexattr_init(&sAttr);

        ACP_RETURN_IF_ERROR(sRet, );

        switch (aFlag)
        {
            case ACP_THR_MUTEX_ERRORCHECK:
                sRet = pthread_mutexattr_settype(&sAttr,
                                                 PTHREAD_MUTEX_ERRORCHECK);
                break;
            case ACP_THR_MUTEX_RECURSIVE:
                sRet = pthread_mutexattr_settype(&sAttr,
                                                 PTHREAD_MUTEX_RECURSIVE);
                break;
            default:
                sRet = EINVAL;
                break;
        }

        ACP_RETURN_IF_ERROR(sRet, (void)pthread_mutexattr_destroy(&sAttr));

        sRet = pthread_mutex_init(&aMutex->mMutex, &sAttr);

        (void)pthread_mutexattr_destroy(&sAttr);
    }
    else
    {
        sRet = pthread_mutex_init(&aMutex->mMutex, NULL);
    }

    return sRet;

#undef ACP_RETURN_IF_ERROR
}

#endif
