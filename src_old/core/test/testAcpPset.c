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
 * $Id: testAcpPset.c 5245 2009-04-14 02:02:23Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpPset.h>

static acp_sint32_t testThread(void *aArg)
{
    acp_rc_t      sRC;
    acp_uint32_t *sDataPtr = (acp_uint32_t *)aArg;
    acp_uint32_t  sDataVal = *sDataPtr;
    acp_pset_t    sPset;

    ACP_PSET_ZERO(&sPset);

    ACP_PSET_SET(&sPset, sDataVal);

    /* bind thread to cpu */
    sRC = acpPsetBindThread(&sPset);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
/*     (void)acpPrintf("[%lld] Bind Thread to %d CPU. \n", */
/*                     acpThrGetSelfID(), sDataVal); */

    acpSleepSec(3);

    /* unbind thread */
    sRC = acpPsetUnbindThread();
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    return 0;
}

#define TEST_THREAD_COUNT ACP_PSET_MAX_CPU

void testThreadTest(void)
{
    acp_rc_t       sRC;
    acp_sint32_t   sThrRet;
    acp_thr_attr_t sAttr;
    acp_sint32_t   i;
    acp_sint32_t   j;
    acp_sint32_t   sCPUCount;
    acp_pset_t     sPset;

    acp_sint32_t  *sTestData;
    acp_thr_t     *sThr;

    acp_bool_t     sIsSupportBind;
    acp_bool_t     sIsSupportMultiBind;

    /* get CPU */
    sRC = acpPsetGetConf(&sPset, &sIsSupportBind, &sIsSupportMultiBind);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    sCPUCount = (acp_uint32_t)ACP_PSET_COUNT(&sPset);

    ACT_CHECK(acpMemAlloc((void *)&sThr, sCPUCount * sizeof(acp_thr_t))
              == ACP_RC_SUCCESS);
    ACT_CHECK(acpMemAlloc((void *)&sTestData, sCPUCount * sizeof(acp_sint32_t))
              == ACP_RC_SUCCESS);

    /*
     * create thread attribute
     */
    sRC = acpThrAttrCreate(&sAttr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * set other attribute value
     */
    sRC = acpThrAttrSetBound(&sAttr, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * thread create w or w/o attribute
     */
    for (i = 0, j = 0; i < ACP_PSET_MAX_CPU; i++)
    {
        if (ACP_PSET_ISSET(&sPset, i) != 0)
        {
            sTestData[j] = i; /* cpu number */
            sRC = acpThrCreate(&sThr[j],
                               &sAttr,
                               testThread,
                               &sTestData[j]);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            if (++j == sCPUCount) /* end of cpu number */
            {
                break;
            }
            else
            {
                /* going continue */
            }

        }
        else
        {
            /* no cpu */
        }
    }

    for (i = 0, j = 0; i < ACP_PSET_MAX_CPU; i++)
    {
        if (ACP_PSET_ISSET(&sPset, i) != 0)
        {
            sRC = acpThrJoin(&sThr[j], &sThrRet);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            if (++j == sCPUCount) /* end of cpu number */
            {
                break;
            }
            else
            {
                /* going continue */
            }

        }
        else
        {
            /* no cpu */
        }
    }

    /*
     * destroy thread attribute
     */
    sRC = acpThrAttrDestroy(&sAttr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

acp_sint32_t main(void)
{
    acp_rc_t   sRC;
    acp_uint32_t i;

    acp_pset_t   sPset;
    acp_uint32_t sCnt;

    acp_pset_t   sOrgSet;
    acp_pset_t   sDupSet;

    acp_bool_t     sIsSupportBind;
    acp_bool_t     sIsSupportMultiBind;

    ACT_TEST_BEGIN();

    ACP_PSET_ZERO(&sPset);

    /* ------------------------------------------------
     *  Bit Set Testing
     * ----------------------------------------------*/
    for (i = 0 ; i < ACP_PSET_MAX_CPU; i++)
    {
        ACP_PSET_SET(&sPset, i);

        sCnt = (acp_uint32_t)ACP_PSET_COUNT(&sPset);

        ACT_CHECK_DESC(sCnt != i,
                       ("index = %d, bit count = %d", i, sCnt));
        ACT_CHECK(ACP_PSET_ISSET(&sPset, i) != 0);
    }

    /* ------------------------------------------------
     *  Bit Set Isset : for full
     * ----------------------------------------------*/
    for (i = 0 ; i < ACP_PSET_MAX_CPU; i++)
    {
        ACT_CHECK(ACP_PSET_ISSET(&sPset, i) != 0);
    }

    /* ------------------------------------------------
     *  Bit Clr Testing
     * ----------------------------------------------*/
    for (i = 0 ; i < ACP_PSET_MAX_CPU; i++)
    {
        ACP_PSET_CLR(&sPset, i);

        sCnt = (acp_uint32_t)ACP_PSET_COUNT(&sPset);

        ACT_CHECK_DESC(sCnt != (ACP_PSET_MAX_CPU - i),
                       ("index = %d, bit count = %d",
                       (acp_uint32_t)(ACP_PSET_MAX_CPU - i), sCnt));
        ACT_CHECK(ACP_PSET_ISSET(&sPset, i) == 0);
    }

    /* ------------------------------------------------
     *  Bit Set Isset : for full
     * ----------------------------------------------*/
    for (i = 0 ; i < ACP_PSET_MAX_CPU; i++)
    {
        ACT_CHECK(ACP_PSET_ISSET(&sPset, i) == 0);
    }

    /* ------------------------------------------------
     *  Getting Conf info Testing
     * ----------------------------------------------*/
    {
        acp_uint32_t sCpuCount = 0;

        sRC = acpPsetGetConf(&sOrgSet, &sIsSupportBind, &sIsSupportMultiBind);

        ACT_CHECK(sRC == ACP_RC_SUCCESS);

        sCnt = (acp_uint32_t)ACP_PSET_COUNT(&sOrgSet);

        (void)acpSysGetCPUCount(&sCpuCount);

        ACT_CHECK(sCnt == sCpuCount);

/*         (void)acpPrintf("This platform has %d CPUs. \n", sCpuCount); */

    }

    /* ------------------------------------------------
     *  Verify Cache Data
     * ----------------------------------------------*/

    {
        acp_bool_t     sIsSupportBind2;
        acp_bool_t     sIsSupportMultiBind2;

        sRC = acpPsetGetConf(&sDupSet, &sIsSupportBind2, &sIsSupportMultiBind2);

        ACT_CHECK(sRC == ACP_RC_SUCCESS);

        ACT_CHECK(acpMemCmp(&sOrgSet, &sDupSet, sizeof(acp_pset_t)) == 0);
    }

    /* ------------------------------------------------
     * Bind Process
     * ----------------------------------------------*/

    sPset = sOrgSet; /* copy for testing */
    {
        acp_pset_t sMySet;

        for (i = 0 ; i < ACP_PSET_MAX_CPU; i++)
        {
            ACP_PSET_ZERO(&sMySet);

            if (ACP_PSET_ISSET(&sPset, i) != 0)
            {
                ACP_PSET_SET(&sMySet, i);

                sRC = acpPsetBindProcess(&sMySet);

                ACT_CHECK_DESC(sRC == ACP_RC_SUCCESS, ("error CPU %d \n", i));
/*                 (void)acpPrintf("[%lld] Bind Process to %d CPU. \n", */
/*                                 acpProcGetSelfID(),  i); */
            }
            else
            {
                /* no cpu on this id */
            }
        }
        /* recover */
        ACT_CHECK(acpPsetUnbindProcess() == ACP_RC_SUCCESS);
    }

    testThreadTest();

    /* ------------------------------------------------
     *  Multiple CPU-Set  test
     * ----------------------------------------------*/

    if (ACP_PSET_COUNT(&sPset) > 1)
    {
        if (sIsSupportMultiBind == ACP_TRUE)
        {
/*             (void)acpPrintf("[YES] CPUSETs Support \n"); */

            sRC = acpPsetBindProcess(&sPset);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

        }
        else
        {
            sRC = acpPsetBindProcess(&sPset);
            ACT_CHECK(sRC == ACP_RC_EINVAL);
/*             (void)acpPrintf("[NO] CPUSETs Support = %d\n", sRC); */
        }
    }
    else
    {
        /* no testing for 1-CPU */
    }

    ACT_TEST_END();

    return 0;
}
