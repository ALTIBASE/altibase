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
 * $Id: testAcpThrRwlock.c 2355 2008-05-20 07:34:00Z shsuh $
 ******************************************************************************/

#include <act.h>
#include <acpThr.h>
#include <acpThrRwlock.h>


#define TEST_THREAD_COUNT 10


static acp_thr_rwlock_t gRwlock;


static acp_sint32_t testLockRead(void *aArg)
{
    acp_rc_t sRC;

    ACP_UNUSED(aArg);

    sRC = acpThrRwlockLockRead(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

static acp_sint32_t testLockWrite(void *aArg)
{
    acp_rc_t sRC;

    ACP_UNUSED(aArg);

    sRC = acpThrRwlockLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

static acp_sint32_t testTryLocks(void *aArg)
{
    acp_rc_t sRC;

    ACP_UNUSED(aArg);

    sRC = acpThrRwlockTryLockRead(&gRwlock);
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));

    sRC = acpThrRwlockTryLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));

    return 0;
}


acp_sint32_t main(void)
{
    acp_thr_t    sThr[TEST_THREAD_COUNT];
    acp_rc_t     sRC;
    acp_sint32_t i;

    ACT_TEST_BEGIN();

    /*
     * create rwlock
     */
    sRC = acpThrRwlockCreate(&gRwlock, ACP_THR_RWLOCK_DEFAULT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * read lock
     */
    sRC = acpThrRwlockLockRead(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * write lock
     */
    sRC = acpThrRwlockLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * try write lock while holding read lock
     */
    sRC = acpThrRwlockLockRead(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrRwlockTryLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * try read lock while holding write lock
     */
    sRC = acpThrRwlockLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrRwlockTryLockRead(&gRwlock);
#if defined(ALTI_CFG_OS_TRU64)
    ACT_CHECK(ACP_RC_IS_EDEADLK(sRC));
#else
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));
#endif

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * hold multiple read lock
     */
    sRC = acpThrRwlockLockRead(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testLockRead, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * and waiting write lock
     */
    sRC = acpThrCreate(&sThr[0], NULL, testLockWrite, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    acpSleepMsec(100);

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThr[0], NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * hold one write lock
     */
    sRC = acpThrRwlockLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testTryLocks, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * and waiting for read lock
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testLockRead, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    acpSleepMsec(100);

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * and waiting for write lock
     */
    sRC = acpThrRwlockLockWrite(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCreate(&sThr[0], NULL, testLockWrite, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    acpSleepMsec(100);

    sRC = acpThrRwlockUnlock(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThr[0], NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * destroy rwlock
     */
    sRC = acpThrRwlockDestroy(&gRwlock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_TEST_END();

    return 0;
}
