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
 * $Id: acpThrCond.c 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#include <acpAtomic.h>
#include <acpSpinWait.h>
#include <acpThrCond.h>


#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

/*
 * Windows Vista 이전 버전에서는 condition variable API가 존재하지 않기 때문에
 * Win32 Event 객체와 atomic operation을 통해 구현한다.
 *
 * Win32 condition variable 구현에서 다음의 문제들이 발생할 수 있으므로
 * 주의해야 한다.
 *
 * - lost wakeup
 *   cond_wait함수가 mutex unlock후 event wait전에 다른 쓰레드가 PulseEvent()
 *   함수를 통해 signal을 전송하면 cond_wait함수는 깨어나지 않는다.
 *   mutex와 event가 atomic하게 unlock-wait-lock되지 않기 때문에 PulseEvent()
 *   함수를 쓰지 않는 것이 좋다.
 *
 * - unfairness
 *   cond_broadcast함수는 cond_wait하고 있던 모든 쓰레드를 깨워준다. 하지만,
 *   Win32 Event에서 모든 쓰레드를 한번에 깨울려면 auto-reset event를 사용할
 *   수 없고 manual-reset event를 사용해야 한다. 이 때, 모든 cond_wait 쓰레드가
 *   깨어나는 것이 보장되어야 한다.
 *
 * - incorrectness
 *   PulseEvent()함수를 사용하지 않는 한 Win32 Event는 reliable하다.
 *   즉, event wait전에 set event가 발생하더라도 event wait은 event를 얻어온다.
 *   다른 쓰레드가 cond_signal이나 cond_broadcast호출 후에 cond_wait을 시작하는
 *   쓰레드가 wakeup되면 안된다.
 *
 * - busy waiting
 *   unfairness와(나) incorrectness를 보장하기 위해 busy wait을 해야하는 경우가
 *   있을 수 있다. 최소한의 busy wait만 하도록 구현해야 한다.
 */

static acp_rc_t acpThrCondTimedWaitInternal(acp_thr_cond_t  *aCond,
                                            acp_thr_mutex_t *aMutex,
                                            DWORD            aMsec)
{
    DWORD        sRet;
    acp_sint32_t sWaiterRemain;
    acp_sint32_t sIsBroadcast;
    acp_rc_t     sRC = ACP_RC_SUCCESS;

    /*
     * [1] WaiterCount Increase
     */
    (void)acpAtomicInc32(&aCond->mWaiterCount);

    /*
     * [2] Mutex Unlock
     */
    (void)acpThrMutexUnlock(aMutex);

    /*
     * [3] Signal Wait
     *
     * cond_signal/cond_broadcast가 호출될 때까지 주어진 timeout만큼 대기
     */
    sRet = WaitForSingleObject(aCond->mSemaphore, aMsec);

    /*
     * [4] WaiterCount Decrease
     */
    sWaiterRemain = acpAtomicDec32(&aCond->mWaiterCount);

    /*
     * [5] Event To Signaling Thread
     *
     * cond_signal/cond_broadcast쓰레드에 wakeup이 완료되었음을 event로 알려줌
     *
     * cond_broadcast의 경우 cond_wait하던 모든 쓰레드가 깨어났을때 event를 보냄
     */
    sIsBroadcast = acpAtomicGet32(&aCond->mIsBroadcast);
    switch(sRet)
    {
        case WAIT_OBJECT_0:
            if ((sWaiterRemain == 0) ||
                (sIsBroadcast == 0))
            {
                ACP_TEST_RAISE(SetEvent(aCond->mWaitDone) == 0, ERROR_SETEVENT);
            }
            else
            {
                /* do nothing */
            }
            break;
        case WAIT_TIMEOUT:
            if ((sWaiterRemain == 0) &&
                (sIsBroadcast == 1))
            {
                /*
                 * This condition could be reached only in case when
                 * semaphore released but we already stoped waiting it.
                 * So we should take semaphore and report to broadcast
                 * thread about it to avoid deadlock.
                 */
                sRet = WaitForSingleObject(aCond->mSemaphore, INFINITE);
                /* Set event without check of sRC to avoid deadlock */
                ACP_TEST_RAISE(SetEvent(aCond->mWaitDone) == 0, ERROR_SETEVENT);
                if (sRet != WAIT_OBJECT_0)
                {
                    /*
                     * If no semaphore set at this moment
                     * than somthing strange happens.
                     */
                    sRC = ACP_RC_EINVAL;
                }
                else
                {
                    /* do nothing */
                }
            }
            else
            {
                sRC = ACP_RC_ETIMEDOUT;
            }
            break;
        case WAIT_FAILED:
            {
                sRC = ACP_RC_GET_OS_ERROR();
            }
            break;
        default:
            {
                sRC = ACP_RC_EINVAL;
            }
            break;
    }

    /*
     * [6] Mutex Lock & Return
     */
    (void)acpThrMutexLock(aMutex);

    ACP_EXCEPTION(ERROR_SETEVENT)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_EXPORT acp_rc_t acpThrCondCreate(acp_thr_cond_t *aCond)
{
    acp_rc_t sRC;

    aCond->mIsBroadcast = 0;
    aCond->mWaiterCount = 0;

    /*
     * cond_signal/cond_broadcast가
     * cond_wait쓰레드를 깨우기 위해 사용하는 semaphore
     */
    aCond->mSemaphore = CreateSemaphore(NULL, 0, ACP_SINT32_MAX, NULL);

    if (aCond->mSemaphore == NULL)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    /*
     * cond_signal/cond_broadcast가
     * cond_wait쓰레드 깨우기가 완료될 때까지 기다리기 위한 event
     */
    aCond->mWaitDone = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (aCond->mWaitDone == NULL)
    {
        sRC = ACP_RC_GET_OS_ERROR();

        (void)CloseHandle(aCond->mSemaphore);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrCondDestroy(acp_thr_cond_t *aCond)
{
    (void)CloseHandle(aCond->mWaitDone);
    (void)CloseHandle(aCond->mSemaphore);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrCondWait(acp_thr_cond_t  *aCond,
                                   acp_thr_mutex_t *aMutex)
{
    if (aCond == NULL || aMutex == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    return acpThrCondTimedWaitInternal(aCond, aMutex, INFINITE);
}

ACP_EXPORT acp_rc_t acpThrCondTimedWait(acp_thr_cond_t  *aCond,
                                        acp_thr_mutex_t *aMutex,
                                        acp_time_t       aTimeout,
                                        acp_time_type_t  aTimeoutType)
{
    DWORD sMsec;

    if (aCond == NULL || aMutex == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    /*
     * microsecond단위의 timeout을 millisecond단위로 변환
     */
    if (aTimeout == ACP_TIME_INFINITE)
    {
        sMsec = INFINITE;
    }
    else
    {
        /*
         * absolute timeout을 relative timeout으로 변환
         */
        if (aTimeoutType == ACP_TIME_ABS)
        {
            aTimeout -= acpTimeNow();

            if (aTimeout < 0)
            {
                aTimeout = 0;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }

        sMsec = (DWORD)((aTimeout + 999) / 1000);
    }

    return acpThrCondTimedWaitInternal(aCond, aMutex, sMsec);
}

ACP_EXPORT acp_rc_t acpThrCondSignal(acp_thr_cond_t *aCond)
{
    BOOL sRet;

    if (aCond == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    /*
     * cond_wait하는 쓰레드가 없으면 아무것도 하지 않음
     */
    if (acpAtomicGet32(&aCond->mWaiterCount) > 0)
    {
        /*
         * cond_wait하는 쓰레드 중 하나만 깨움
         */
        sRet = ReleaseSemaphore(aCond->mSemaphore, 1, 0);

        if (sRet == 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* do nothing */
        }

        /*
         * cond_wait쓰레드가 깨어날 때까지 대기
         */
        (void)WaitForSingleObject(aCond->mWaitDone, INFINITE);
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrCondBroadcast(acp_thr_cond_t *aCond)
{
    acp_sint32_t sWaiterCount;
    BOOL         sRet;

    if (aCond == NULL)
    {
        return ACP_RC_EFAULT;
    }
    else
    {
        /* do nothing */
    }

    /*
     * cond_wait하는 쓰레드가 없으면 아무것도 하지 않음
     */
    sWaiterCount = acpAtomicGet32(&aCond->mWaiterCount);

    if (sWaiterCount > 0)
    {
        /*
         * 모든 cond_wait하는 쓰레드가 깨어난 후 WaitDone event를 발생시키도록
         * IsBroadcast flag를 1로 세팅
         */
        (void)acpAtomicSet32(&aCond->mIsBroadcast, 1);

        /*
         * 현재 cond_wait하고 있는 쓰레드를 모두 깨움
         */
        sRet = ReleaseSemaphore(aCond->mSemaphore, sWaiterCount, 0);

        if (sRet == 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* do nothing */
        }

        /*
         * 모든 cond_wait쓰레드가 깨어날 때까지 대기
         */
        (void)WaitForSingleObject(aCond->mWaitDone, INFINITE);

        /*
         * IsBroadcast flag를 0으로 원복
         */
        (void)acpAtomicSet32(&aCond->mIsBroadcast, 0);
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;
}

#endif
