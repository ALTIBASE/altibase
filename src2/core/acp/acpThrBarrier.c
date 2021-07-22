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
 * $Id: acpThrBarrier.c 11192 2010-06-08 01:24:21Z djin $
 ******************************************************************************/

#include <acpThrBarrier.h>


/**
 * creates a thread barrier
 * @param aBarrier pointer to the #acp_thr_barrier_t
 * @return result code
 */
ACP_EXPORT acp_rc_t acpThrBarrierCreate(acp_thr_barrier_t *aBarrier)
{
    acp_rc_t sRC;

    sRC = acpThrCondCreate(&aBarrier->mCond);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    sRC = acpThrMutexCreate(&aBarrier->mMutex, ACP_THR_MUTEX_DEFAULT);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    aBarrier->mCreated = ACP_THR_BARRIER_MAGIC;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * destroys a thread barrier
 * @param aBarrier pointer to the #acp_thr_barrier_t
 * @return result code
 */
ACP_EXPORT acp_rc_t acpThrBarrierDestroy(acp_thr_barrier_t *aBarrier)
{
    acp_rc_t sRC;

    sRC = acpThrCondDestroy(&aBarrier->mCond);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    sRC = acpThrMutexDestroy(&aBarrier->mMutex);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    aBarrier->mCreated = 0;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * increases the counter of the thread barrier and broadcasts to the others
 * @param aBarrier pointer to the #acp_thr_barrier_t
 * @return result code
 */
ACP_EXPORT acp_rc_t acpThrBarrierTouch(acp_thr_barrier_t *aBarrier)
{
    acp_rc_t sRC;

    ACP_TEST_RAISE(ACP_THR_BARRIER_MAGIC != aBarrier->mCreated, E_NOTCREATED);

    sRC = acpThrMutexLock(&aBarrier->mMutex);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    aBarrier->mCount++;
    sRC = acpThrCondBroadcast(&aBarrier->mCond);
    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpThrMutexUnlock(&aBarrier->mMutex);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpThrMutexUnlock(&aBarrier->mMutex);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NOTCREATED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * waits for the others to touch the counter of thread barrier
 * @param aBarrier pointer to the #acp_thr_barrier_t
 * @param aCounter counter to wait
 * @return result code
 */
ACP_EXPORT acp_rc_t acpThrBarrierWait(acp_thr_barrier_t *aBarrier,
                                      acp_uint32_t       aCounter)
{
    acp_rc_t sRC;

    ACP_TEST_RAISE(ACP_THR_BARRIER_MAGIC != aBarrier->mCreated, E_NOTCREATED);

    sRC = acpThrMutexLock(&aBarrier->mMutex);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    while (aBarrier->mCount < aCounter)
    {
        sRC = acpThrCondWait(&aBarrier->mCond, &aBarrier->mMutex);

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC)   &&
            ACP_RC_NOT_ETIMEDOUT(sRC) &&
            ACP_RC_NOT_EINTR(sRC), E_OTHER);
        ACP_MEM_BARRIER();
    }

    sRC = acpThrMutexUnlock(&aBarrier->mMutex);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NOTCREATED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_OTHER)
    {
        (void)acpThrMutexUnlock(&aBarrier->mMutex);
    }

    ACP_EXCEPTION_END;
    return sRC;
}
