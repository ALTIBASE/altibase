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
 * $Id: testAcpCStrFind.c 10268 2010-03-04 10:43:25Z gurugio $
 ******************************************************************************/


#include <act.h>
#include <acpCStr.h>
#include <acpTest.h>


static struct
{
    acp_char_t *mTest;
    acp_char_t *mFind;
    acp_sint32_t mFlag;
} gTestCStrFindChar[] =
{
    {
        "abcabdeabecddbdeca",
        "abcde",
        ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_INSENSITIVE,
    },
    {
        "abcabdeabecddbdeca",
        "abcde",
        ACP_CSTR_SEARCH_BACKWARD | ACP_CSTR_CASE_INSENSITIVE,
    },
    {
        "abcaBdEAbEcddBDEcA",
        "abcdeABCDE",
        ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_SENSITIVE,
    },
    {
        "abcaBdEAbEcddBDEcA",
        "abcdeABCDE",
        ACP_CSTR_SEARCH_BACKWARD | ACP_CSTR_CASE_SENSITIVE,
    },
    {
        "",
        "",
        0,
    }
};


static void testCStrFindCharBody(acp_sint32_t aTestNum)
{
    acp_char_t *sTestData = gTestCStrFindChar[aTestNum].mTest;
    acp_char_t *sFindData = gTestCStrFindChar[aTestNum].mFind;

    acp_sint32_t sTestLen = (acp_sint32_t)acpCStrLen(sTestData, 128);
    acp_sint32_t sFindLen = (acp_sint32_t)acpCStrLen(sFindData, 128);

    acp_sint32_t sFlag = gTestCStrFindChar[aTestNum].mFlag;
    
    acp_char_t sRevertData[64];
    acp_sint32_t sFoundIndex[64];
    acp_sint32_t sFoundCount[64] = {0,};
    acp_sint32_t sFromIndex;
    acp_rc_t sRC;
    acp_sint32_t i;
    acp_sint32_t j;

    /* find all occurences of several characters */

    i = 0;
    for (j = 0; j < sFindLen; j++)
    {
        if ((sFlag & ACP_CSTR_SEARCH_BACKWARD) != 0)
        {
            sFromIndex = sTestLen;
        }
        else
        {
            sFromIndex = 0;
        }

        for (; i < sTestLen; i++)
        {
            sRC = acpCStrFindChar(sTestData,
                                  sFindData[j],
                                  &sFoundIndex[i],
                                  sFromIndex,
                                  sFlag);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                break;
            }
            else
            {
                if ((sFlag & ACP_CSTR_SEARCH_BACKWARD) != 0)
                {
                    sFromIndex = sFoundIndex[i] - 1;
                }
                else
                {
                    sFromIndex = sFoundIndex[i] + 1;
                }

                sFoundCount[j]++;
            }
        }
    }

    /* revert original data from finding result */
    {
        /* index of sFoundIndex array */
        acp_sint32_t sIndexFoundIndex = 0;

        /* What character is placed where? */
        
        for (i = 0; i < sFindLen; i++) /* what character? */
        {
            for (j = 0; j < sFoundCount[i]; j++)  /* where? */
            {
                sRevertData[sFoundIndex[sIndexFoundIndex+j]] = sFindData[i];
            }
            sIndexFoundIndex += j;
        }
    }
    sRevertData[sTestLen] = ACP_CSTR_TERM;

    ACT_CHECK_DESC(acpCStrCmp(sTestData, sRevertData, sTestLen) == 0,
                       ("Finding a character failed\n"));

    sRC = acpCStrFindChar(sTestData,
                          sFindData[0],
                          &sFoundIndex[0],
                          sTestLen + 1,/* over-range */
                          sFlag);

    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));
    
    sRC = acpCStrFindChar(sTestData,
                          sFindData[0],
                          &sFoundIndex[0],
                          -1,/* over-range */
                          sFlag);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpCStrFindChar(sTestData,
                          (acp_char_t)0xEF, /* not character */
                          &sFoundIndex[0],
                          0,
                          sFlag);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    
    return;
}

static void testCStrFindChar(void)
{
    acp_sint32_t i;

    for (i = 0; gTestCStrFindChar[i].mTest[0] != ACP_CSTR_TERM; i++)
    {
        testCStrFindCharBody(i);
    }

    return;
}


#define TEST_CSTR_STRING                                \
    "ALTIBASEaltibaseALTIBASEaltibaseALTIBASEaltibase"
#define TEST_CSTR_LENGTH  48

static struct
{
    acp_char_t *mCString;
    acp_sint32_t mFlag;
    acp_sint32_t mIndex[7];
} gTestCStrFindCStr[] =
{
    {
        "ALTIBASE",
        ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_INSENSITIVE,
        {
            0, 8, 16, 24, 32, 40, -1
        }
    },
    {
        "ALTIBASE",
        ACP_CSTR_SEARCH_BACKWARD | ACP_CSTR_CASE_SENSITIVE,
        {
            32, 16, 0, -1, -1, -1, -1
        }
    },
    {
        "altibase",
        ACP_CSTR_SEARCH_BACKWARD | ACP_CSTR_CASE_INSENSITIVE,
        {
            40, 32, 24, 16, 8, 0, -1
        }
    },
    {
        "altibase",
        ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_SENSITIVE,
        {
            8, 24, 40, -1, -1, -1, -1
        }
    },
    {
        "AltiBase",
        ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_INSENSITIVE,
        {
            0, 8, 16, 24, 32, 40, -1
        }
    },
    {
        "AltiBase",
        ACP_CSTR_SEARCH_BACKWARD | ACP_CSTR_CASE_SENSITIVE,
        {
            -1, -1, -1, -1, -1, -1, -1
        }
    },
    {
        "",
        0,
        {
            -1, -1, -1, -1, -1, -1, -1
        }
    }
};


static void testCStrFindCStr(void)
{
    acp_sint32_t sCStrLoop;
    acp_sint32_t sIndexLoop;
    acp_rc_t sRC;
    acp_sint32_t sFoundIndex;
    acp_sint32_t sFromIndex;

    for (sCStrLoop = 0;
         gTestCStrFindCStr[sCStrLoop].mCString[0] != ACP_CSTR_TERM;
         sCStrLoop++)
    {
        if (gTestCStrFindCStr[sCStrLoop].mFlag & ACP_CSTR_SEARCH_BACKWARD)
        {
            sFromIndex = TEST_CSTR_LENGTH;
        }
        else
        {
            sFromIndex = 0;
        }

        for (sIndexLoop = 0;
             gTestCStrFindCStr[sCStrLoop].mIndex[sIndexLoop] >= 0;
             sIndexLoop++)
        {
            sRC = acpCStrFindCStr(TEST_CSTR_STRING,
                                  gTestCStrFindCStr[sCStrLoop].mCString,
                                  &sFoundIndex,
                                  sFromIndex,
                                  gTestCStrFindCStr[sCStrLoop].mFlag);

            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            ACT_CHECK(sFoundIndex ==
                      gTestCStrFindCStr[sCStrLoop].mIndex[sIndexLoop]);


            if (gTestCStrFindCStr[sCStrLoop].mFlag & ACP_CSTR_SEARCH_BACKWARD)
            {
                sFromIndex = sFoundIndex - 1;
            }
            else
            {
                sFromIndex = sFoundIndex + 1;
            }

        }
    }

    return;
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

#if defined(__INSURE__)
    /* Do nothing */
#else
    testCStrFindChar();
    testCStrFindCStr();
#endif

    ACT_TEST_END();

    return 0;
}
