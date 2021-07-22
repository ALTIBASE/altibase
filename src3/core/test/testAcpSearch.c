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
 * $Id: testAcpSearch.c 11173 2010-06-04 02:09:52Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpSearch.h>



struct testCalenda
{
    acp_sint32_t mNum;
    acp_char_t *mName;
} gMonths[] = {
    { 1, "jan" }, { 2, "feb" }, { 3, "mar" }, { 4, "apr" },
    { 5, "may" }, { 6, "jun" }, { 7, "jul" }, { 8, "aug" },
    { 9, "sep" }, {10, "oct" }, {11, "nov" }, {12, "dec" }
};

#define NUMBER_OF_MONTHS (sizeof(gMonths)/sizeof(struct testCalenda))


struct testCalenda gFailCases[] = {
    {0, "ja"}, /* test for partialy same string */
    {0, "feba"},
    {0, " ma"},
    {0, "mar "},
    {0, "abcde"},
    {-1, NULL},
};



acp_sint32_t testComp(const void *aM1, const void *aM2)
{
    /*
     * aM1 -> finding data
     * aM2 -> comparing data
     */
    struct testCalenda *sM1 = (struct testCalenda *) aM1;
    struct testCalenda *sM2 = (struct testCalenda *) aM2;
    acp_size_t sM1Len = acpCStrLen(sM1->mName, 10);
    acp_size_t sM2Len = acpCStrLen(sM2->mName, 10);

    /* strings must be the same completly */
    return acpCStrCmp(sM1->mName, sM2->mName,
                      ACP_MAX(sM1Len, sM2Len));
}


acp_sint32_t main(void)
{
    acp_sint32_t i;
    struct testCalenda *sResult;
    acp_rc_t sRC;
    struct testCalenda sSortedMonths[NUMBER_OF_MONTHS];
    
    ACT_TEST_BEGIN();

    acpMemCpy(sSortedMonths, gMonths, sizeof(gMonths));

    /* Binary Search needs sorted data */
    acpSortQuickSort(sSortedMonths, NUMBER_OF_MONTHS,
                     sizeof(struct testCalenda), testComp);

    /* success cases */
    for (i = 0; i < (acp_sint32_t)NUMBER_OF_MONTHS; i++)
    {
        sRC = acpSearchBinary(&gMonths[i], sSortedMonths,
                              NUMBER_OF_MONTHS, sizeof(struct testCalenda),
                              testComp, &sResult);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC) && sResult != NULL);
        ACT_CHECK_DESC(sResult->mNum - 1 == i,
                       ("Result must be %d, but %d", i, sResult->mNum - 1));

        if (ACP_RC_NOT_ENOENT(sRC) && ACP_RC_NOT_SUCCESS(sRC))
        {
            (void)acpPrintf("acpSearchBinary has critical bug!,"
                            "return value must be SUCCESS or ENOENT\n");
            return 1;
        }
        else
        {
            /* go ahead */
        }
    }

    /* failure cases */
    for (i = 0; gFailCases[i].mNum >= 0; i++)
    {
        sRC = acpSearchBinary(&gFailCases[i], sSortedMonths,
                              NUMBER_OF_MONTHS, sizeof(struct testCalenda),
                              testComp, &sResult);
        ACT_CHECK_DESC(ACP_RC_IS_ENOENT(sRC) && sResult == NULL,
                       ("Finding %s must be failed, but successed",
                        gFailCases[i].mName));

        /* unidentified error */
        if (ACP_RC_NOT_ENOENT(sRC) && ACP_RC_NOT_SUCCESS(sRC))
        {
            (void)acpPrintf("acpSearchBinary has critical bug!,"
                            "return value must be SUCCESS or ENOENT, but %d\n",
                            sRC);
            return 1;
        }
        else
        {
            /* go ahead */
        }
    }        

    ACT_TEST_END();

    return 0;
}
