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
 * $Id: acpSpinLock.h 11212 2010-06-10 06:15:30Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_SPIN_LOCK_H_)
#define _O_ACP_SPIN_LOCK_H_

/**
 * @file
 * @ingroup CoreAtomic
 * @ingroup CoreThr
 */

#include <acpAlign.h>
#include <acpAtomic.h>
#include <acpSpinWait.h>
#include <acpThr.h>


ACP_EXTERN_C_BEGIN


/**
 * spinlock object
 */
typedef struct acp_spin_lock_t
{
#if defined(ALTI_CFG_CPU_PARISC)
    volatile acp_sint32_t mLock[4];
#else
    volatile acp_sint32_t mLock;
#endif
    acp_sint32_t          mSpinCount;
} acp_spin_lock_t;


#if defined(ACP_CFG_DOXYGEN)

/**
 * initializer for a spinlock object
 * @param aSpinCount spin count prior to sleep
 *
 * if @a aSpinCount is -1, it will use default spin count
 */
#define ACP_SPIN_LOCK_INITIALIZER(aSpinCount)

#else

#if defined(ALTI_CFG_CPU_PARISC)
#define ACP_SPIN_LOCK_INITIALIZER(aSpinCount) { { 1, 1, 1, 1 }, (aSpinCount) }
#else
#define ACP_SPIN_LOCK_INITIALIZER(aSpinCount) { 1, (aSpinCount) }
#endif

#endif


/**
 * initializes a spinlock
 * @param aLock the spinlock object
 * @param aSpinCount spin count prior to sleep
 */
ACP_INLINE void acpSpinLockInit(acp_spin_lock_t *aLock, acp_sint32_t aSpinCount)
{
#if defined(ALTI_CFG_CPU_PARISC)
    aLock->mLock[0] = 1;
    aLock->mLock[1] = 1;
    aLock->mLock[2] = 1;
    aLock->mLock[3] = 1;
#else
    aLock->mLock = 1;
#endif
    aLock->mSpinCount = aSpinCount;
}

/**
 * trys a lock for a spinlock object
 * @param aLock the spinlock object
 * @return #ACP_TRUE if locked, #ACP_FALSE if not
 */
ACP_EXPORT acp_bool_t acpSpinLockTryLock(acp_spin_lock_t *aLock);

/**
 * returns a lock state of a spinlock object
 * @param aLock the spinlock object
 * @return #ACP_TRUE if locked, #ACP_FALSE if not
 */
ACP_INLINE acp_bool_t acpSpinLockIsLocked(acp_spin_lock_t *aLock)
{
#if defined(ALTI_CFG_CPU_PARISC)
    if ((aLock->mLock[0] == 0) || (aLock->mLock[1] == 0) ||
        (aLock->mLock[2] == 0) || (aLock->mLock[3] == 0))
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
#else
    return (aLock->mLock == 0) ? ACP_TRUE : ACP_FALSE;
#endif
}


/**
 * locks a spinlock
 * @param aLock the spinlock object
 */
ACP_INLINE void acpSpinLockLock(acp_spin_lock_t *aLock)
{
    acp_sint32_t sSpinLoop;
    acp_sint32_t sSpinCount;
    acp_uint32_t sSpinSleepTime;
    acp_bool_t sIsMyLock;

    
    /* set up conditions */
    sSpinSleepTime = ACP_SPIN_WAIT_SLEEP_TIME_MIN;
    sIsMyLock = ACP_FALSE;

    if (aLock->mSpinCount < 0)
    {
        sSpinCount = acpSpinWaitGetDefaultSpinCount();
    }
    else
    {
        sSpinCount = aLock->mSpinCount;
    }


    while (1)
    {
        /* Only read lock variable from hardware-cache or memory
         * to reduce system-bus locking.
         */
        for (sSpinLoop = 0;
             sSpinLoop <= sSpinCount; /* sSpinCount can be 0 in UP! */
             sSpinLoop++)
        {
            /* for cache hit, more frequency case is first */
            if (acpSpinLockIsLocked(aLock) == ACP_FALSE)
            {
                if (acpSpinLockTryLock(aLock) == ACP_TRUE)
                {
                    /* success to lock */
                    /* return; is better than goto for recucing jump */
                    sIsMyLock = ACP_TRUE;
                    break;
                }
                else
                {
                    /* Other process takes the lock.
                     * The process takes time in the critical section,
                     * therefore I better to sleep, not try lock.
                     * */
                    break;
                }
            }
            else
            {
                /* do loop */
            }
        }
    

        if (sIsMyLock == ACP_TRUE)
        {
            /* I win the lock */
            break;
        }
        else
        {
            /* sleep and try again to yield chance for others.
             * This can avoid excessive racing condition.
             */
            acpSleepUsec(sSpinSleepTime);
        }


        if (sSpinSleepTime < ACP_SPIN_WAIT_SLEEP_TIME_MAX)
        {
            sSpinSleepTime *= 2;
        }
        else
        {
            /* reach maximum sleep time */
        }

    }

    return;

}


/**
 * unlocks a spinlock object
 * @param aLock the spinlock object
 */
ACP_INLINE void acpSpinLockUnlock(acp_spin_lock_t *aLock)
{
    ACP_MEM_BARRIER();

#if defined(ALTI_CFG_CPU_PARISC)
    aLock->mLock[0] = 1;
    aLock->mLock[1] = 1;
    aLock->mLock[2] = 1;
    aLock->mLock[3] = 1;
#else
    (void)acpAtomicSet32(&(aLock->mLock), 1);
#endif
}

/**
 * sets a spinlock count for a spinlock object
 * @param aLock the spinlock object
 * @param aSpinCount the spinlock object
 */
ACP_INLINE void acpSpinLockSetCount(acp_spin_lock_t* aLock,
                                    acp_sint32_t     aSpinCount)
{
    aLock->mSpinCount = aSpinCount;
}

/**
 * gets a spinlock count from a spinlock object
 * @param aLock the spinlock object
 * @return old spin count
 */
ACP_INLINE acp_sint32_t acpSpinLockGetCount(acp_spin_lock_t* aLock)
{
    return aLock->mSpinCount;
}

ACP_EXTERN_C_END


#endif
