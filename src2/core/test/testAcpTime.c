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
 * $Id: testAcpTime.c 8351 2009-10-26 01:26:41Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpTime.h>
#include <acpPrintf.h>

#define LOOP_COUNT 100

static void testAcpTime(void)
{
    acp_rc_t        sRC;
    acp_time_t      sTime;
    acp_time_t      sCvtTimeFromGm;
    acp_time_t      sCvtTimeFromLocal;
    acp_time_exp_t  sExpTime;
    acp_time_exp_t  sLocalTime;

    sTime = acpTimeNow();
    acpTimeGetGmTime(sTime, &sExpTime);
    acpTimeGetLocalTime(sTime, &sLocalTime);

    sRC = acpTimeMakeTime(&sExpTime, &sCvtTimeFromGm);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpTimeMakeTime(&sLocalTime, &sCvtTimeFromLocal);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(sCvtTimeFromLocal == sTime);

    /* GMT = Korea local time - 9h */
    ACT_CHECK(sCvtTimeFromGm == sTime - ACP_SINT64_LITERAL(32400000000));
}

static void testTimeTranslator(const acp_time_t *aBaseTime,
                               const acp_time_exp_t *aBaseExpTime)
{
    ACT_CHECK(acpTimeGetSec(*aBaseTime) == aBaseExpTime->mSec);
    ACT_CHECK(acpTimeGetMsec(*aBaseTime) ==
              aBaseExpTime->mUsec / ACP_SINT64_LITERAL(1000));
    ACT_CHECK(acpTimeGetUsec(*aBaseTime) == aBaseExpTime->mUsec);

    ACT_CHECK(acpTimeToSec(*aBaseTime) == ACP_SINT64_LITERAL(993911585));
    ACT_CHECK(acpTimeToMsec(*aBaseTime) == ACP_SINT64_LITERAL(993911585487));
    ACT_CHECK(acpTimeToUsec(*aBaseTime) == ACP_SINT64_LITERAL(993911585487704));
}

static void testHiResTimeNow(void)
{
    acp_hrtime_t sPrevious = 0;
    acp_hrtime_t sCurrent;
    acp_sint32_t i;

    for (i = 0; i < LOOP_COUNT; i++)
    {
        sCurrent = acpTimeHiResNow();

        ACT_CHECK_DESC(sCurrent > sPrevious,
                       ("sCurrent should be larger than sPrevious"
                        ": sCurrent - sPrevious = %lld (%d)",
                        sCurrent - sPrevious,
                        i));

        sPrevious = sCurrent;

        if (i % 100 == 1)
        {
            acpTimeHiResRevise();
        }
        else
        {
        }
    }
}

static void testHiResTimeNowWithSleep(void)
{
    acp_hrtime_t sPrevious = 0;
    acp_hrtime_t sCurrent;
    acp_sint32_t i;

    for (i = 0; i < LOOP_COUNT; i++)
    {
        sCurrent = acpTimeHiResNow();

        ACT_CHECK_DESC(sCurrent > sPrevious,
                       ("sCurrent should be larger than sPrevious"
                        ": sCurrent - sPrevious = %lld (%d)",
                        sCurrent - sPrevious,
                        i));

        sPrevious = sCurrent;

        acpSleepMsec(200);

        if (i % 100 == 1)
        {
            acpTimeHiResRevise();
        }
        else
        {
        }
    }
}

static void testMonotonic(void)
{
    /* test acp hi resolution */
    testHiResTimeNow();

    /* test acp hi resolution with sleep*/
    testHiResTimeNowWithSleep();
}

static void testAcpTimeNowTimeZoneBugRegressionHelper()
{
    acp_char_t    sBuffer[512];
    acp_uint32_t  i;
    for ( i = 0 ; i < 512 ; i++ )
    {
        sBuffer[i] = '0';
    }
}

/*
 * BUG-38709 perform regression test for the acpTimeNow bug on SPARC
 * Solaris.
 */
static void testAcpTimeNowTimeZoneBugRegression()
{
    acp_time_t    sTime;

    sTime = acpTimeNow();
    testAcpTimeNowTimeZoneBugRegressionHelper();
    sTime = acpTimeNow() - sTime;

    ACT_ASSERT( sTime > 0 );
}

acp_sint32_t main(void)
{
    acp_time_t     sBaseTime;
    acp_time_t     sCurrentTime;
    acp_hrtime_t   sPrevious;
    acp_hrtime_t   sCurrent;
    acp_time_exp_t sBaseExpTime;
    acp_time_exp_t sCurrentExpTime;

    sBaseTime = ACP_SINT64_LITERAL(993911585487704);

    sBaseExpTime.mYear      = 2001;
    sBaseExpTime.mMonth     = 6;
    sBaseExpTime.mDay       = 30;
    sBaseExpTime.mHour      = 14;
    sBaseExpTime.mMin       = 33;
    sBaseExpTime.mSec       = 5;
    sBaseExpTime.mUsec      = 487704;
    sBaseExpTime.mDayOfWeek = 6;
    sBaseExpTime.mDayOfYear = 181;

    ACT_TEST_BEGIN();

    testAcpTime();
    testAcpTimeNowTimeZoneBugRegression();
    testTimeTranslator((const acp_time_t *)&sBaseTime,
                       (const acp_time_exp_t *)&sBaseExpTime);
        
    /* BUG-21847 [CORE] acpTimeHiResNow coredump */
    acpTimeHiResRevise();

    sPrevious = acpTimeHiResNow();

    /* dummy functions to spend time */
    (void)acpProcGetSelfID();

    sCurrent = acpTimeHiResNow();

    ACT_CHECK_DESC(sPrevious < sCurrent,
                   ("the difference is %d\n", sCurrent - sPrevious));

    sCurrentTime = acpTimeNow();
    ACT_CHECK_DESC(sCurrentTime > sBaseTime,
                   ("acpTimeNow => "
                    "sCurrentTime - sBaseTime is (%lld)",
                    (sCurrentTime - sBaseTime)));

    acpTimeGetGmTime(sBaseTime, &sCurrentExpTime);
    ACT_CHECK_DESC(sBaseExpTime.mYear == sCurrentExpTime.mYear,
                   ("sBaseExpTime.mYear: %d, sCurrentExpTime.mYear: %d",
                    sBaseExpTime.mYear, sCurrentExpTime.mYear));
    ACT_CHECK_DESC(sBaseExpTime.mMonth == sCurrentExpTime.mMonth,
                   ("sBaseExpTime.mMonth: %d, sCurrentExpTime.mMonth: %d",
                    sBaseExpTime.mMonth, sCurrentExpTime.mMonth));
    ACT_CHECK_DESC(sBaseExpTime.mDay == sCurrentExpTime.mDay,
                   ("sBaseExpTime.mDay: %d, sCurrentExpTime.mDay: %d",
                    sBaseExpTime.mDay, sCurrentExpTime.mDay));
    ACT_CHECK_DESC(sBaseExpTime.mHour == sCurrentExpTime.mHour,
                   ("sBaseExpTime.mHour: %d, sCurrentExpTime.mHour: %d",
                    sBaseExpTime.mHour, sCurrentExpTime.mHour));
    ACT_CHECK_DESC(sBaseExpTime.mMin == sCurrentExpTime.mMin,
                   ("sBaseExpTime.mMin: %d, sCurrentExpTime.mMin: %d",
                    sBaseExpTime.mMin, sCurrentExpTime.mMin));
    ACT_CHECK_DESC(sBaseExpTime.mSec == sCurrentExpTime.mSec,
                   ("sBaseExpTime.mSec: %d, sCurrentExpTime.mSec: %d",
                    sBaseExpTime.mSec, sCurrentExpTime.mSec));
    ACT_CHECK_DESC(sBaseExpTime.mUsec == sCurrentExpTime.mUsec,
                   ("sBaseExpTime.mUsec: %d, sCurrentExpTime.mUsec: %d",
                    sBaseExpTime.mUsec, sCurrentExpTime.mUsec));
    ACT_CHECK_DESC(sBaseExpTime.mDayOfWeek == sCurrentExpTime.mDayOfWeek,
                   ("sBaseExpTime.mDayOfWeek: %d, "
                    "sCurrentExpTime.mDayOfWeek: %d",
                    sBaseExpTime.mDayOfWeek, sCurrentExpTime.mDayOfWeek));
    ACT_CHECK_DESC(sBaseExpTime.mDayOfYear == sCurrentExpTime.mDayOfYear,
                   ("sBaseExpTime.mDayOfYear: %d, "
                    "sCurrentExpTime.mDayOfYear: %d",
                    sBaseExpTime.mDayOfYear, sCurrentExpTime.mDayOfYear));

    testMonotonic();

    ACT_TEST_END();

    return 0;
}
