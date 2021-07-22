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
 * $Id: testAcpProcMutex.c 5075 2009-04-01 10:57:45Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpProc.h>
#include <acpProcMutex.h>
#include <acpSleep.h>
#include <acpStr.h>

acp_key_t gTestMutexKey;

static void testParent(acp_char_t *aProcPath)
{
    acp_char_t        sMutexKey[16];
    acp_char_t       *sArgs[3];
    acp_proc_t        sProc;
    acp_proc_wait_t   sWaitResult;
    acp_proc_mutex_t  sMutex;
    acp_rc_t          sRC;

    (void)acpSnprintf(sMutexKey,
                      sizeof(sMutexKey),
                      "%u",
                      (acp_uint32_t)gTestMutexKey);

    sArgs[0] = sMutexKey;
    sArgs[2] = NULL;

    /*
     * open mutex
     */
    sRC = acpProcMutexOpen(&sMutex, gTestMutexKey);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcMutexOpen should return SUCCESS but %d", sRC));

    /*
     * simple lock/unlock
     */
    sRC = acpProcMutexLock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcMutexUnlock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * EBUSY if other process trys lock
     */
    sRC = acpProcMutexLock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sArgs[1] = "1";

    sRC = acpProcForkExec(&sProc, aProcPath, sArgs, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcMutexUnlock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * mutex consistency; process termination after lock
     */
    sArgs[1] = "2";

    sRC = acpProcForkExec(&sProc, aProcPath, sArgs, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sWaitResult.mState == ACP_PROC_EXITED);

    sRC = acpProcMutexTryLock(&sMutex);
    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACT_ERROR(("acpProcMutexLock should return SUCCESS but %d", sRC));
        return;
    }
    else
    {
    }

    sRC = acpProcMutexUnlock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * mutex consistency; process termination after unlock
     */
    sArgs[1] = "3";

    sRC = acpProcForkExec(&sProc, aProcPath, sArgs, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sWaitResult.mState == ACP_PROC_EXITED);

    sRC = acpProcMutexTryLock(&sMutex);
    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACT_ERROR(("acpProcMutexLock should return SUCCESS but %d", sRC));
        return;
    }
    else
    {
    }

    sRC = acpProcMutexUnlock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * mutex consistency; process kill
     */
    sArgs[1] = "4";

    sRC = acpProcForkExec(&sProc, aProcPath, sArgs, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while (1)
    {
        sRC = acpProcMutexTryLock(&sMutex);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpProcMutexUnlock(&sMutex);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            acpSleepMsec(100);
        }
        else if (ACP_RC_IS_EBUSY(sRC))
        {
            break;
        }
        else
        {
            ACT_ERROR(("acpProcMutexLock returned unexpected error %d ", sRC));
            break;
        }
    }

    sRC = acpProcKill(&sProc);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sWaitResult.mState == ACP_PROC_EXITED);

    sRC = acpProcMutexTryLock(&sMutex);
    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACT_ERROR(("acpProcMutexLock should return SUCCESS but %d", sRC));
        return;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpProcMutexUnlock(&sMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * close mutex
     */
    sRC = acpProcMutexClose(&sMutex);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcMutexClose should return SUCCESS but %d", sRC));
}

static void testChild(acp_key_t aMutexKey, acp_sint32_t aCase)
{
    acp_proc_mutex_t sMutex;
    acp_rc_t         sRC;

    sRC = acpProcMutexOpen(&sMutex, aMutexKey);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcMutexOpen should return SUCCESS but %d", sRC));

    switch (aCase)
    {
        case 1:
            sRC = acpProcMutexTryLock(&sMutex);
            ACT_CHECK(ACP_RC_IS_EBUSY(sRC));
            break;
        case 2:
            sRC = acpProcMutexLock(&sMutex);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            acpProcExit(0);
            break;
        case 3:
            sRC = acpProcMutexLock(&sMutex);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            sRC = acpProcMutexUnlock(&sMutex);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            acpProcExit(0);
            break;
        case 4:
            sRC = acpProcMutexLock(&sMutex);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            acpSleepSec(10);
            break;
        default:
            break;
    }

    sRC = acpProcMutexClose(&sMutex);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcMutexClose should return SUCCESS but %d", sRC));
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACT_TEST_BEGIN();
    gTestMutexKey = (acp_key_t)acpProcGetSelfID();

    if (aArgc == 3)
    {
        acp_str_t    sStr1 = ACP_STR_CONST(aArgv[1]);
        acp_str_t    sStr2 = ACP_STR_CONST(aArgv[2]);
        acp_sint32_t sSign;
        acp_uint64_t sNum;
        acp_key_t    sMutexKey;
        acp_sint32_t sCase;
        acp_rc_t     sRC;

        sRC = acpStrToInteger(&sStr1, &sSign, &sNum, NULL, 0, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sMutexKey = (acp_key_t)((acp_sint32_t)sNum * sSign);

        sRC = acpStrToInteger(&sStr2, &sSign, &sNum, NULL, 0, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sCase = (acp_sint32_t)sNum * sSign;

        testChild(sMutexKey, sCase);
    }
    else
    {
        acp_rc_t sRC;

        do {
            sRC = acpProcMutexCreate(gTestMutexKey);
            if(ACP_RC_IS_EEXIST(sRC))
            {
                /* Loop until available key found */
                gTestMutexKey++;
            }
            else
            {
                /* Explicitly exits loop */
                break;
            }
        } while(ACP_RC_IS_EEXIST(sRC));

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            ACT_ERROR(
                ("acpProcMutexCreate should return SUCCESS but %d\n", sRC)
                );
        }
        else
        {
            /* do nothing */
        }

        testParent(aArgv[0]);

        sRC = acpProcMutexDestroy(gTestMutexKey);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcMutexDestroy should return SUCCESS but %d",
                        sRC));
    }

    ACT_TEST_END();

    return 0;
}
