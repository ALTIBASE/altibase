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
 * $Id: acpThrRwlock.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_THR_RWLOCK_H_)
#define _O_ACP_THR_RWLOCK_H_

/**
 * @file
 * @ingroup CoreThr
 */

#include <acpThrCond.h>
#include <acpThrMutex.h>
#include <acpError.h>


ACP_EXTERN_C_BEGIN


/*
 * flag for thread rwlock attribute
 */
/**
 * default thread rwlock
 *
 * @see acpThrRwlockCreate()
 */
#define ACP_THR_RWLOCK_DEFAULT 0


/**
 * thread rwlock object
 */
typedef struct acp_thr_rwlock_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    /* serialize access */
    acp_thr_mutex_t mLock;

    /* writer threads waiting to acquire the lock */
    acp_thr_cond_t  mWaitingReaders;

    /* reader threads waiting to acquire the lock */
    acp_thr_cond_t  mWaitingWriters;

    /* number of waiting readers */
    acp_sint32_t    mWaitingReadersCount;

    /* number of waiting writers */
    acp_sint32_t    mWaitingWritersCount;

    /* If writer has the lock, VALUE is -1, else this keeps track of
     * the number of readres holding the lock
     */
    acp_sint32_t    mRefCount;
#else
    pthread_rwlock_t mRwlock;
#endif
} acp_thr_rwlock_t;

#if defined (ALTI_CFG_OS_WINDOWS) && !defined (ACP_CFG_DOXYGEN)

ACP_EXPORT acp_rc_t acpThrRwlockCreate(acp_thr_rwlock_t *aRwlock,
                                       acp_sint32_t      aFlag);
ACP_EXPORT acp_rc_t acpThrRwlockDestroy(acp_thr_rwlock_t *aRwlock);

ACP_EXPORT acp_rc_t acpThrRwlockLockRead(acp_thr_rwlock_t *aRwlock);
ACP_EXPORT acp_rc_t acpThrRwlockLockWrite(acp_thr_rwlock_t *aRwlock);

ACP_EXPORT acp_rc_t acpThrRwlockTryLockRead(acp_thr_rwlock_t *aRwlock);
ACP_EXPORT acp_rc_t acpThrRwlockTryLockWrite(acp_thr_rwlock_t *aRwlock);

ACP_EXPORT acp_rc_t acpThrRwlockUnlock(acp_thr_rwlock_t *aRwlock);

#else

/**
 * creates a thread rwlock
 *
 * @param aRwlock pointer to the thread rwlock
 * @param aFlag flag for thread rwlock attribute
 * @return result code
 *
 * @a aFlag value can be #ACP_THR_RWLOCK_DEFAULT which is same as zero
 */
ACP_INLINE acp_rc_t acpThrRwlockCreate(acp_thr_rwlock_t *aRwlock,
                                       acp_sint32_t      aFlag)
{
    if (aFlag != ACP_THR_RWLOCK_DEFAULT)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        return pthread_rwlock_init(&aRwlock->mRwlock, NULL);
    }
}

/**
 * destroys a thread rwlock
 *
 * @param aRwlock pointer to the thread rwlock
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrRwlockDestroy(acp_thr_rwlock_t *aRwlock)
{
    return pthread_rwlock_destroy(&aRwlock->mRwlock);
}

/**
 * locks a thread rwlock for read
 *
 * @param aRwlock pointer to the thread rwlock
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrRwlockLockRead(acp_thr_rwlock_t *aRwlock)
{
    return pthread_rwlock_rdlock(&aRwlock->mRwlock);
}

/**
 * locks a thread rwlock for write
 *
 * @param aRwlock pointer to the thread rwlock
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrRwlockLockWrite(acp_thr_rwlock_t *aRwlock)
{
    return pthread_rwlock_wrlock(&aRwlock->mRwlock);
}

/**
 * trys lock a thread rwlock for read
 *
 * @param aRwlock pointer to the thread rwlock
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrRwlockTryLockRead(acp_thr_rwlock_t *aRwlock)
{
    return pthread_rwlock_tryrdlock(&aRwlock->mRwlock);
}

/**
 * trys lock a thread rwlock for write
 *
 * @param aRwlock pointer to the thread rwlock
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrRwlockTryLockWrite(acp_thr_rwlock_t *aRwlock)
{
    return pthread_rwlock_trywrlock(&aRwlock->mRwlock);
}

/**
 * unlocks a thread rwlock
 *
 * @param aRwlock pointer to the thread rwlock
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrRwlockUnlock(acp_thr_rwlock_t *aRwlock)
{
    return pthread_rwlock_unlock(&aRwlock->mRwlock);
}

#endif

ACP_EXTERN_C_END


#endif
