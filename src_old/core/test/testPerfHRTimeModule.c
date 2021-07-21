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
 * $Id$
 ******************************************************************************/

#include <act.h>
#include <acpTime.h>

#define THREAD_COUNT 8

/* Test TimeNow */
acp_rc_t testTimeNowSetup(actPerfFrmThrObj *aThrObj)
{
    ACP_UNUSED(aThrObj);

    return ACP_RC_SUCCESS;
}

acp_rc_t testTimeNowExec(actPerfFrmThrObj *aThrObj)
{
    volatile acp_time_t   sPrev  = 0;
    volatile acp_time_t   sNow;
    acp_uint64_t sCount = aThrObj->mObj.mFrm->mLoopCount;
    acp_uint64_t i;

    for (i = 0; i < sCount; i++)
    {
        sNow = acpTimeNow();

        ACT_CHECK_DESC(sNow >= sPrev,
                       ("[%lld] sNow should be larger or equal than sPrev,"
                        "but, sNow(%lld) - sPrev(%lld) = %lld \n",
                        i, sNow, sPrev, sNow - sPrev));

        sPrev = sNow;
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testTimeNowVerify(actPerfFrmThrObj *aThrObj)
{
    /* we already verified, acpTimeNow
     * should return a linear increasing value */
    ACP_UNUSED(aThrObj);

    return ACP_RC_SUCCESS;
}

/* Test TimeHiResNow */
acp_rc_t testTimeHiResNowSetup(actPerfFrmThrObj *aThrObj)
{
    ACP_UNUSED(aThrObj);

    return ACP_RC_SUCCESS;
}

acp_rc_t testTimeHiResNowExec(actPerfFrmThrObj *aThrObj)
{
    acp_hrtime_t sPrev  = 0;
    acp_hrtime_t sNow;
    acp_uint64_t sCount = aThrObj->mObj.mFrm->mLoopCount;
    acp_uint64_t i;

    for (i = 0; i < sCount; i++)
    {
        sNow = acpTimeHiResNow();

        ACT_CHECK_DESC(sNow >= sPrev,
                       ("sNow should be larger or equal than sPrev, "
                        "but, sNow - sPrev = %lld \n", sNow - sPrev));

        sPrev = sNow;
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t testTimeHiResNowVerify(actPerfFrmThrObj *aThrObj)
{
    /* we already verified, acpTimeHiResNow
     * should return a linear increasing value */
    ACP_UNUSED(aThrObj);

    return ACP_RC_SUCCESS;
}

acp_char_t /*@unused@*/ *gPerName = "HRTimePerf";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    {
        "acpTimeNow     ",
        1,
        ACP_UINT64_LITERAL(10000000),
        testTimeNowSetup,
        testTimeNowExec,
        testTimeNowVerify
    },
    {
        "acpTimeHiResNow",
        1,
        ACP_UINT64_LITERAL(10000000),
        testTimeHiResNowSetup,
        testTimeHiResNowExec,
        testTimeHiResNowVerify
    },
    {
        "acpTimeNow     ",
        THREAD_COUNT,
        ACP_UINT64_LITERAL(10000000),
        testTimeNowSetup,
        testTimeNowExec,
        testTimeNowVerify
    },
    {
        "acpTimeHiResNow",
        THREAD_COUNT,
        ACP_UINT64_LITERAL(10000000),
        testTimeHiResNowSetup,
        testTimeHiResNowExec,
        testTimeHiResNowVerify
    },
    {
        "acpTimeNow     ",
        THREAD_COUNT * 2,
        ACP_UINT64_LITERAL(10000000),
        testTimeNowSetup,
        testTimeNowExec,
        testTimeNowVerify
    },
    {
        "acpTimeHiResNow",
        THREAD_COUNT * 2,
        ACP_UINT64_LITERAL(10000000),
        testTimeHiResNowSetup,
        testTimeHiResNowExec,
        testTimeHiResNowVerify
    },
    {
        NULL,
        0,
        0,
        NULL,
        NULL,
        NULL
    }

};
