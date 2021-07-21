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
 * $Id: testAcpAtomicPerf.c 5558 2009-05-11 04:29:01Z jykim $
 ******************************************************************************/

#include <acpSpinWait.h>
#include <acpSpinLock.h>
#include <act.h>

/* ------------------------------------------------
 * Main Body
 * ----------------------------------------------*/


/* ------------------------------------------------
 *  SpinLockPerf
 * ----------------------------------------------*/

acp_rc_t testSpinLockPerfInit(actPerfFrmThrObj *aThrObj)
{
    acp_spin_lock_t*    sSpinLock;
    acp_rc_t            sRC;
    acp_uint32_t        i = 0;

    sRC = acpMemAlloc((void*)&sSpinLock, sizeof(acp_spin_lock_t));
    ACP_TEST_RAISE(sRC != ACP_RC_SUCCESS, NO_MEMORY);

    acpSpinLockInit(sSpinLock, -1);

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = sSpinLock;
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NO_MEMORY)
    {
        ACT_ERROR(("Cannot allocate memory!\n"));
    }

    ACP_EXCEPTION_END;
    return sRC;
}

acp_rc_t testSpinLockPerfTestLock(actPerfFrmThrObj *aThrObj)
{
    acp_spin_lock_t*    sSpinLock = (acp_spin_lock_t *)aThrObj->mObj.mData;

    acp_uint64_t        i;
    acp_rc_t            sRC;
    
    ACP_TEST_RAISE(NULL == sSpinLock, NO_MEMORY);

    for (i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        acpSpinLockLock(sSpinLock);
        acpSpinLockUnlock(sSpinLock);
    }
    
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NO_MEMORY)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

#if 0
ACP_INLINE void testSpinLockLockWhenAvailable(acp_spin_lock_t *aLock)
{
    acp_sint32_t sSpinLoop;
    acp_sint32_t sSpinCount = aLock->mSpinCount;
    acp_uint32_t sSpinSleepTime;

    if (sSpinCount < 0)
    {
        sSpinCount = acpSpinWaitGetDefaultSpinCount();
    }
    else
    {
        /* Do nothing */
    }

    for (sSpinLoop = 0; sSpinLoop < sSpinCount; sSpinLoop++)
    {
        if(ACP_FALSE == acpSpinLockIsLocked(sSpinLock));
        {
            if(ACP_TRUE == acpSpinLockTryLock(aLock))
            {
                break;
            }
            else
            {
                /* Loop! */
            }
        }
        else
        {
            /* Loop! */
        }
    }

    if (sSpinLoop >= sSpinCount)
    {
        sSpinSleepTime = ACP_SPIN_WAIT_SLEEP_TIME_MIN;

        while (1)
        {
            if(ACP_FALSE == acpSpinLockIsLocked(sSpinLock));
            {
                if(ACP_TRUE == acpSpinLockTryLock(aLock))
                {
                    break;
                }
                else
                {
                    /* Loop! */
                }
            }
            else
            {
                /* Loop! */
            }

            acpSleepUsec(sSpinSleepTime);
            if (sSpinSleepTime >= (ACP_SPIN_WAIT_SLEEP_TIME_MAX / 2))
            {
                sSpinSleepTime = ACP_SPIN_WAIT_SLEEP_TIME_MAX;
            }
            else
            {
                sSpinSleepTime *= 2;
            }
        }
    }
    else
    {
        /* Loop! */
    }    
}
#endif

acp_rc_t testSpinLockPerfFinal(actPerfFrmThrObj *aThrObj)
{
    acp_spin_lock_t*    sSpinLock = (acp_spin_lock_t *)aThrObj->mObj.mData;
    /* result verify */
    
    if(NULL != sSpinLock)
    {
        /* Check all locks are unlocked */
        ACT_CHECK(ACP_FALSE == acpSpinLockIsLocked(sSpinLock));
        acpMemFree(sSpinLock);
    }
    else
    {
        /* Nothing to verify */
    }
    
    return ACP_RC_SUCCESS;
/*     ACP_EXCEPTION_END; */
/*     return sRC; */
    
}

/* ------------------------------------------------
 *  Data Descriptor 
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "SpinLockPerf";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* Spin Lock */
    {
        "acpSpinLock",
        1,
        ACP_UINT64_LITERAL(100000000),
        testSpinLockPerfInit,
        testSpinLockPerfTestLock,
        testSpinLockPerfFinal
    } ,
    {
        "acpSpinLock",
        2,
        ACP_UINT64_LITERAL(100000000),
        testSpinLockPerfInit,
        testSpinLockPerfTestLock,
        testSpinLockPerfFinal
    } ,
    {
        "acpSpinLock",
        4,
        ACP_UINT64_LITERAL(100000000),
        testSpinLockPerfInit,
        testSpinLockPerfTestLock,
        testSpinLockPerfFinal
    } ,
    {
        "acpSpinLock",
        8,
        ACP_UINT64_LITERAL(100000000),
        testSpinLockPerfInit,
        testSpinLockPerfTestLock,
        testSpinLockPerfFinal
    } ,
    {
        "acpSpinLock",
        16,
        ACP_UINT64_LITERAL(100000000),
        testSpinLockPerfInit,
        testSpinLockPerfTestLock,
        testSpinLockPerfFinal
    } ,
    ACT_PERF_FRM_SENTINEL 
};

