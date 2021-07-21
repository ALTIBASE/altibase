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
 * $Id: acpThrRwlock.c 1102 2007-10-31 01:45:29Z shsuh $
 ******************************************************************************/

#include <acpThrRwlock.h>

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

#define ACP_THR_RWLOCK_COULD_LOCK_READ(aRwlock)                                \
    (((aRwlock)->mRefCount >= 0) && ((aRwlock)->mWaitingWritersCount == 0))
#define ACP_THR_RWLOCK_COULD_LOCK_WRITE(aRwlock) ((aRwlock)->mRefCount == 0)

ACP_EXPORT acp_rc_t acpThrRwlockCreate(acp_thr_rwlock_t *aRwlock,
                                       acp_sint32_t      aFlag)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if (aFlag != ACP_THR_RWLOCK_DEFAULT)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* successed, do nothing */
    }

    sRC = acpThrMutexCreate(&aRwlock->mLock, ACP_THR_MUTEX_DEFAULT);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* successed, do nothing */
    }

    sRC = acpThrCondCreate(&aRwlock->mWaitingReaders);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpThrMutexDestroy(&aRwlock->mLock);

        return sRC;
    }
    else
    {
        /* successed, do nothing */
    }

    sRC = acpThrCondCreate(&aRwlock->mWaitingWriters);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpThrMutexDestroy(&aRwlock->mLock);
        (void)acpThrCondDestroy(&aRwlock->mWaitingReaders);

        return sRC;
    }
    else
    {
        /* successed, do nothing */
    }

    aRwlock->mRefCount            = 0;
    aRwlock->mWaitingWritersCount = 0;
    aRwlock->mWaitingReadersCount = 0;

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrRwlockDestroy(acp_thr_rwlock_t *aRwlock)
{
    (void)acpThrMutexDestroy(&aRwlock->mLock);
    (void)acpThrCondDestroy(&aRwlock->mWaitingReaders);
    (void)acpThrCondDestroy(&aRwlock->mWaitingWriters);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrRwlockLockRead(acp_thr_rwlock_t *aRwlock)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    sRC = acpThrMutexLock(&aRwlock->mLock);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        /* if writer is already locked  */
        if (ACP_THR_RWLOCK_COULD_LOCK_READ(aRwlock))
        {
            aRwlock->mRefCount++;
        }
        else
        {
            aRwlock->mWaitingReadersCount++;

            while (1)
            {
                sRC = acpThrCondWait(&aRwlock->mWaitingReaders,
                                     &aRwlock->mLock);

                if (ACP_RC_NOT_SUCCESS(sRC))
                {
                    break;
                }
                else
                {
                    if (ACP_THR_RWLOCK_COULD_LOCK_READ(aRwlock))
                    {
                        aRwlock->mRefCount++;
                        break;
                    }
                    else
                    {
                        /* continue */
                    }
                }
            }

            aRwlock->mWaitingReadersCount--;
        }

        (void)acpThrMutexUnlock(&aRwlock->mLock);
    }
    else
    {
        /* failed, do nothing */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrRwlockLockWrite(acp_thr_rwlock_t *aRwlock)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    sRC = acpThrMutexLock(&aRwlock->mLock);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        if (ACP_THR_RWLOCK_COULD_LOCK_WRITE(aRwlock))
        {
            aRwlock->mRefCount = -1;
        }
        else
        {
            aRwlock->mWaitingWritersCount++;

            while (1)
            {
                sRC = acpThrCondWait(&aRwlock->mWaitingWriters,
                                     &aRwlock->mLock);

                if (ACP_RC_NOT_SUCCESS(sRC))
                {
                    break;
                }
                else
                {
                    if (ACP_THR_RWLOCK_COULD_LOCK_WRITE(aRwlock))
                    {
                        aRwlock->mRefCount = -1;
                        break;
                    }
                    else
                    {
                        /* continue */
                    }
                }
            }

            aRwlock->mWaitingWritersCount--;
        }

        (void)acpThrMutexUnlock(&aRwlock->mLock);
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrRwlockTryLockRead(acp_thr_rwlock_t *aRwlock)
{
    acp_rc_t sRC;

    sRC = acpThrMutexLock(&aRwlock->mLock);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        if (ACP_THR_RWLOCK_COULD_LOCK_READ(aRwlock))
        {
            aRwlock->mRefCount++;
        }
        else
        {
            sRC = ACP_RC_EBUSY;
        }

        (void)acpThrMutexUnlock(&aRwlock->mLock);
    }
    else
    {
        /* failed, do nothing */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrRwlockTryLockWrite(acp_thr_rwlock_t *aRwlock)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    sRC = acpThrMutexLock(&aRwlock->mLock);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        if (ACP_THR_RWLOCK_COULD_LOCK_WRITE(aRwlock))
        {
            aRwlock->mRefCount = -1;
        }
        else
        {
            sRC = ACP_RC_EBUSY;
        }

        (void)acpThrMutexUnlock(&aRwlock->mLock);
    }
    else
    {
        /* failed, do nothing */
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrRwlockUnlock(acp_thr_rwlock_t *aRwlock)
{
    acp_rc_t sRC;

    sRC = acpThrMutexLock(&aRwlock->mLock);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* successed, do nothing */
    }

    if (aRwlock->mRefCount > 0)
    {
        aRwlock->mRefCount--;
    }
    else
    {
        if (aRwlock->mRefCount == -1)
        {
            aRwlock->mRefCount = 0;
        }
        else
        {
            /* "FAILED",
             * mRefCount should not be 0, there should be
             * called rdlock or wrlock before unlocking
             */
            (void)acpThrMutexUnlock(&aRwlock->mLock);
            return ACP_RC_EPERM;
        }
    }

    if ((aRwlock->mWaitingWritersCount > 0) && (aRwlock->mRefCount == 0))
    {
        /* unlock writer thread */
        sRC = acpThrCondSignal(&aRwlock->mWaitingWriters);
    }
    else
    {
        if ((aRwlock->mWaitingReadersCount > 0) &&
            (aRwlock->mWaitingWritersCount == 0))
        {
            /* unlock reader threads */
            sRC = acpThrCondBroadcast(&aRwlock->mWaitingReaders);
        }
        else
        {
            /* do nothing */
        }
    }

    (void)acpThrMutexUnlock(&aRwlock->mLock);

    return sRC;
}

#endif
