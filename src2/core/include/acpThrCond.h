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
 * $Id: acpThrCond.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_THR_COND_H_)
#define _O_ACP_THR_COND_H_


/**
 * @file
 * @ingroup CoreThr
 */

#include <acpThrMutex.h>
#include <acpTime.h>
#include <acpSpinLock.h>


ACP_EXTERN_C_BEGIN


#if defined (ALTI_CFG_OS_WINDOWS)
typedef enum ACP_THR_COND_SCOPE
{
    ACP_THR_COND_EVENT_SIGNAL = 0,
    ACP_THR_COND_EVENT_BROADCAST,
    ACP_THR_COND_EVENT_MAX
} ACP_THR_COND_SCOPE;
#endif

/**
 * thread conditional variable object
 */
typedef struct acp_thr_cond_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    volatile acp_sint32_t mIsBroadcast; /* broadcasting flag                */
    volatile acp_sint32_t mWaiterCount; /* number of waiter threads         */
    HANDLE                mWaitDone;    /* event for notify waiting is done */
    HANDLE                mSemaphore;   /* semaphore for wakeup waiters     */
#else
    pthread_cond_t mCond;
#endif
} acp_thr_cond_t;

#if defined(ALTI_CFG_OS_WINDOWS) && !defined (ACP_CFG_DOXYGEN)

ACP_EXPORT acp_rc_t acpThrCondCreate(acp_thr_cond_t *aCond);
ACP_EXPORT acp_rc_t acpThrCondDestroy(acp_thr_cond_t *aCond);

ACP_EXPORT acp_rc_t acpThrCondWait(acp_thr_cond_t  *aCond,
                                   acp_thr_mutex_t *aMutex);
ACP_EXPORT acp_rc_t acpThrCondTimedWait(acp_thr_cond_t  *aCond,
                                        acp_thr_mutex_t *aMutex,
                                        acp_time_t       aTimeout,
                                        acp_time_type_t  aTimeoutType);

ACP_EXPORT acp_rc_t acpThrCondSignal(acp_thr_cond_t *aCond);
ACP_EXPORT acp_rc_t acpThrCondBroadcast(acp_thr_cond_t *aCond);

#else

/**
 * creates a thread condition variable
 *
 * @param aCond pointer to the thread condition variable
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrCondCreate(acp_thr_cond_t *aCond)
{
    return pthread_cond_init(&aCond->mCond, NULL);
}

/**
 * destroys a thread condition variable
 *
 * @param aCond pointer to the thread condition variable
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrCondDestroy(acp_thr_cond_t *aCond)
{
    return pthread_cond_destroy(&aCond->mCond);
}

/**
 * waits for a thread condition variable
 *
 * @param aCond pointer to the thread condition variable
 * @param aMutex pointer to the thread mutex
 * @return result code
 *
 * @a aMutex must be locked before this function call to prevent race condition
 */
ACP_INLINE acp_rc_t acpThrCondWait(acp_thr_cond_t  *aCond,
                                   acp_thr_mutex_t *aMutex)
{
    return pthread_cond_wait(&aCond->mCond, &aMutex->mMutex);
}

/**
 * waits for a thread condition variable with timeout
 *
 * @param aCond pointer to the thread condition variable
 * @param aMutex pointer to the thread mutex
 * @param aTimeout timeout value for waiting
 * @param aTimeoutType type of timeout specified
 * @return result code
 *
 * @a aMutex must be locked before this function call to prevent race condition
 */
ACP_INLINE acp_rc_t acpThrCondTimedWait(acp_thr_cond_t  *aCond,
                                        acp_thr_mutex_t *aMutex,
                                        acp_time_t       aTimeout,
                                        acp_time_type_t  aTimeoutType)
{
    struct timespec sTime;

    if (aTimeoutType == ACP_TIME_REL)
    {
        aTimeout += acpTimeNow();
    }
    else
    {
        /* do nothing */
    }

    sTime.tv_sec  = (acp_slong_t)acpTimeToSec(aTimeout);
    sTime.tv_nsec = (acp_slong_t)acpTimeGetUsec(aTimeout) * 1000;

    return pthread_cond_timedwait(&aCond->mCond, &aMutex->mMutex, &sTime);
}

/**
 * signals a thread condition variable
 *
 * @param aCond pointer to the thread condition variable
 * @return result code
 *
 * to ensure fairness and correctness, the calling thread should lock the
 * associated mutex with the condition variable
 */
ACP_INLINE acp_rc_t acpThrCondSignal(acp_thr_cond_t *aCond)
{
    return pthread_cond_signal(&aCond->mCond);
}

/**
 * broadcasts a thread condition variable
 *
 * @param aCond pointer to the thread condition variable
 * @return result code
 *
 * to ensure fairness and correctness, the calling thread should lock the
 * associated mutex with the condition variable
 */
ACP_INLINE acp_rc_t acpThrCondBroadcast(acp_thr_cond_t *aCond)
{
    return pthread_cond_broadcast(&aCond->mCond);
}

#endif

ACP_EXTERN_C_END


#endif
