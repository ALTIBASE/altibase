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
 * $Id: testAcpCStrPerf.c 5910 2009-06-08 08:20:20Z djin $
 ******************************************************************************/

/*******************************************************************************
 * A Skeleton Code for Testcases
*******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <acpTime.h>

#define TESTSTR1 ("Nikolai Ivanovich Lobachevskii")
#define TESTSTR2 ("The Quick Brown Fox Jumps Over The Lazy Dog")

#define TESTLOOPS 1000000L


void testString(void)
{
# if !defined(ALINT)
/* ------------------------------------------------
 *  This test is for comparing the string API performance
 *  between native and acpCStr..().
 *  so, skip ALINT checking.
 * ----------------------------------------------*/

    acp_char_t sStr1[1024];
    acp_char_t sStr2[1024];

    acp_time_t sTime;
    acp_sint32_t i;

#if defined(ACP_CSTR_STRNLEN_USE_NATIVE)
    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        (void)(strnlen("SherlockHolmes", 100));
        (void)(strnlen("SherlockHolmes", 14));
        (void)(strnlen("SherlockHolmes", 10));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("strnlen   : [%10d]  ", acpTimeGetUsec(sTime));
#endif

    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        (void)(strncmp("SherlockHolmes", "JohnWatson", 10));
        (void)(strncmp("JohnWatson", "SherlockHolmes", 10));
        (void)(strncmp("VanessaMae", "VanessaWilliams", 10));
        (void)(strncmp("VanessaWilliams", "VanessaMae", 10));
        (void)(strncmp("VanessaWilliams", "VanessaMae", 7));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("strncmp   : [%10d]  ", acpTimeGetUsec(sTime));

    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        /* sStrX Will be TESTSTRX */
        (void)(strncpy(sStr1, TESTSTR1, 100));
        (void)(strncpy(sStr2, TESTSTR2, 100));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("strncpy   : [%10d]  ", acpTimeGetUsec(sTime));

    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        /* sStrX Will be TESTSTRX */
        (void)(strncpy(sStr1, TESTSTR1, 100));
        (void)(strncpy(sStr2, TESTSTR2, 100));
        (void)(strncat(sStr1, TESTSTR1, 100));
        (void)(strncat(sStr2, TESTSTR2, 100));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("strncat   : [%10d]\n", acpTimeGetUsec(sTime));
#endif
}

void testAcpCString(void)
{
    acp_char_t sStr1[1024];
    acp_char_t sStr2[1024];

    acp_time_t sTime;
    acp_sint32_t i;

#if defined(ACP_CSTR_STRNLEN_USE_NATIVE)
    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        (void)(acpCStrLen("SherlockHolmes", 100));
        (void)(acpCStrLen("SherlockHolmes", 14));
        (void)(acpCStrLen("SherlockHolmes", 10));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("acpStrLen : [%10d]  ", acpTimeGetUsec(sTime));
#endif

    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        (void)(acpCStrCmp("SherlockHolmes", "JohnWatson", 10));
        (void)(acpCStrCmp("JohnWatson", "SherlockHolmes", 10));
        (void)(acpCStrCmp("VanessaMae", "VanessaWilliams", 10));
        (void)(acpCStrCmp("VanessaWilliams", "VanessaMae", 10));
        (void)(acpCStrCmp("VanessaWilliams", "VanessaMae", 7));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("acpStrCmp : [%10d]  ", acpTimeGetUsec(sTime));

    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        /* sStrX Will be TESTSTRX */
        (void)(acpCStrCpy(sStr1, 1024, TESTSTR1, 100));
        (void)(acpCStrCpy(sStr2, 1024, TESTSTR2, 100));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("acpStrCpy : [%10d]  ", acpTimeGetUsec(sTime));

    sTime = acpTimeNow();
    for(i=0; i<TESTLOOPS; i++)
    {
        /* sStrX Will be TESTSTRX */
        (void)(acpCStrCpy(sStr1, 1024, TESTSTR1, 100));
        (void)(acpCStrCpy(sStr2, 1024, TESTSTR2, 100));
        (void)(acpCStrCat(sStr1, 1024, TESTSTR1, 100));
        (void)(acpCStrCat(sStr2, 1024, TESTSTR2, 100));
    }
    sTime = acpTimeNow() - sTime;
    (void)acpPrintf("acpStrCat : [%10d]\n", acpTimeGetUsec(sTime));
}

acp_sint32_t main(void)
{
    acp_sint32_t i;

    for(i=0; i<5; i++)
    {
        testString();
        testAcpCString();
    }

    return 0;
}
