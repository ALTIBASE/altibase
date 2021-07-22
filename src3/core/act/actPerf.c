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
 * $Id: testAcpAtomicMT.c 5907 2009-06-08 04:46:50Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpAtomic.h>
#include <acpEnv.h>
#include <acpThr.h>
#include <acpTime.h>
#include <acpSys.h>
#include <acpStd.h>

static act_barrier_t gActBarrier;

/* ------------------------------------------------
 *  Framework Main Body
 * ----------------------------------------------*/
#define ACT_PERF_MAX_THREAD 512

static acp_sint32_t actPerfFrmBody(void *aArg)
{
    acp_rc_t            sRC;
    acp_sint64_t        sGap;
    actPerfFrmThrObj  *sThrObj = (actPerfFrmThrObj *)aArg;

    ACT_BARRIER_TOUCH_AND_WAIT(&gActBarrier, sThrObj->mObj.mFrm->mParallel);

    /* ------------------------------------------------
     * User's Body Calling
     * ----------------------------------------------*/

    sThrObj->mObj.mResult.mBeginTime = acpTimeNow();
    sRC = (*sThrObj->mObj.mFrm->mBody)(aArg);
    sThrObj->mObj.mResult.mEndTime   = acpTimeNow();

    /* ------------------------------------------------
     *  Statistics Calculation
     * ----------------------------------------------*/

    sGap = acpTimeToUsec(sThrObj->mObj.mResult.mEndTime) -
                        acpTimeToUsec(sThrObj->mObj.mResult.mBeginTime);

    /* op per second */
    sThrObj->mObj.mResult.mOPS = ((acp_double_t)sThrObj->mObj.mFrm->mLoopCount /
                                                (acp_double_t)sGap) * 1000000;

    return (sRC == ACP_RC_SUCCESS) ? 0 : -1;
}

/* ------------------------------------------------
 *  Logging
 * ----------------------------------------------*/
static acp_std_file_t gLogFile;

static acp_rc_t actPerfFrameworkOpenLog(acp_char_t *aLogFileName)
{
    acp_rc_t   sRC;
    acp_char_t sFileName[1024] = {0 ,};

    (void)acpSnprintf(sFileName, sizeof(sFileName),
                      "%s.pfi", aLogFileName);

    sRC = acpStdOpen(&gLogFile, sFileName, "a");
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    (void)acpFprintf(ACP_STD_ERR, "error in OpenLog : %d\n", sRC);

    return sRC;
}

static acp_rc_t actPerfFrameworkCloseLog(void)
{
    acp_rc_t   sRC;

    sRC = acpStdClose(&gLogFile);
    ACP_TEST(sRC != ACP_RC_SUCCESS);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    (void)acpFprintf(ACP_STD_ERR, "error in CloseLog : %d\n", sRC);
    return sRC;
}

static acp_rc_t actPerfFrameworkWriteLog(actPerfFrmThrObj *aObj, 
                                         actPerfFrm *aDesc)
{
    acp_uint32_t      j;
    acp_double_t      sOpsAvg = 0;
    acp_rc_t          sRC;
    acp_uint64_t      sMinBeginTime = ACP_UINT64_MAX;
    acp_uint64_t      sMaxEndTime   = 0;

    /* 1. ops calc */
    for (j = 0; j < aDesc->mParallel; j++)
    {
        sOpsAvg += aObj[j].mObj.mResult.mOPS;
    }
    sOpsAvg /= aDesc->mParallel;

    /* 2. time calc */
    for (j = 0; j < aDesc->mParallel; j++)
    {
        acp_uint64_t sCurBeginTime;
        acp_uint64_t sCurEndTime;

        sCurBeginTime = acpTimeToUsec(aObj[j].mObj.mResult.mBeginTime);
        sCurEndTime   = acpTimeToUsec(aObj[j].mObj.mResult.mEndTime);

        if (sMinBeginTime > sCurBeginTime)
        {
            sMinBeginTime = sCurBeginTime;
        }
        else
        {
            /* nothing to do */
        }

        if (sMaxEndTime < sCurEndTime)
        {
            sMaxEndTime = sCurEndTime;
        }
        else
        {
            /* nothing to do */
        }
    }


    /* ------------------------------------------------
     * Output Generation
     * ----------------------------------------------*/

    /* write results in a file */
    sRC = acpFprintf(&gLogFile,
                     "- \"%s\" \n"
                     "- %d-client\n"
                     "- output:\n"
                     "  - OPS : %.1f\n"
                     "  - TIME : %.1f\n"
                     "  active:\n"
                     "  - OPS\n"
                     "  - TIME\n"
                     "---\n"
                     ,
                     aDesc->mTitle,
                     aDesc->mParallel,
                     sOpsAvg,
                     (acp_double_t)(sMaxEndTime - sMinBeginTime) / 1000000
                     );

    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    (void)acpStdFlush(&gLogFile);

    /* print out results on a stdout*/
    (void)acpPrintf(" \"%s\" -> "
                    " %d-client, OPS : %.1f, TIME : %.1f\n",
                    aDesc->mTitle,
                    aDesc->mParallel,
                    sOpsAvg,
                    (acp_double_t)(sMaxEndTime - sMinBeginTime) / 1000000);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    {
        acp_char_t sErrMsg[1024];
        acpErrorString(sRC, sErrMsg, 1024);
        (void)acpFprintf(ACP_STD_ERR, "error in WriteLog : %d(%s)\n",
                         sRC, sErrMsg);
    }
    return sRC;

}

ACP_EXPORT act_barrier_t* actPerfGetBarrier(void)
{
    return &gActBarrier;
}

ACP_EXPORT acp_rc_t actPerfFrameworkRun(acp_char_t *aLogFileName,
                             actPerfFrm *aDesc)
{

    acp_uint32_t      i;
    acp_uint32_t      sCount = 0;
    acp_rc_t          sRC    = ACP_RC_SUCCESS;
    acp_thr_t         sThr[ACT_PERF_MAX_THREAD]; /* maximum */
    actPerfFrmThrObj sThrObj[ACT_PERF_MAX_THREAD];

    ACP_TEST(actPerfFrameworkOpenLog(aLogFileName) != ACP_RC_SUCCESS);

    ACT_BARRIER_CREATE(&gActBarrier);

    while(aDesc ->mTitle != NULL)
    {
        /* Init thread objects */
        for (i = 0; i < ACT_PERF_MAX_THREAD; i++)
        {
            sThrObj[i].mObj.mFrm  = aDesc;
            sThrObj[i].mObj.mData = NULL;
        }

        /* Init test case  */
        sRC = (*aDesc->mInit)(&sThrObj[0]);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        ACP_TEST(aDesc->mParallel < 0);
        ACP_TEST(aDesc->mParallel > ACT_PERF_MAX_THREAD);

        ACT_BARRIER_INIT(&gActBarrier);

        for (i = 0; i < aDesc->mParallel; i++) 
        {
            /* thr create */
            sThrObj[i].mNumber = i;
        ;
            sRC = acpThrCreate(&sThr[i],
                               NULL,
                               actPerfFrmBody,
                               (void *)&sThrObj[i]);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
        }

        ACT_BARRIER_WAIT(&gActBarrier, aDesc->mParallel);

        for (i = 0; i < aDesc->mParallel; i++)
        {
            sRC = acpThrJoin(&sThr[i], NULL);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
        }

        sRC = (*aDesc->mFinal)(&sThrObj[0]);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        /* logging */
        ACP_TEST(ACP_RC_NOT_SUCCESS(actPerfFrameworkWriteLog(&sThrObj[0], 
                                    aDesc)));

        aDesc++;
        sCount++;
    }

    ACT_BARRIER_DESTROY(&gActBarrier);

    ACP_TEST(actPerfFrameworkCloseLog() != ACP_RC_SUCCESS);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;

    ACT_BARRIER_DESTROY(&gActBarrier);

    return sRC;

}
