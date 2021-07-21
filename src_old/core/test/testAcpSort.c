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
 * $Id: 
 ******************************************************************************/

#include <act.h>
#include <acpStr.h>
#include <acpRand.h>
#include <acpSort.h>

#define TEST_NUMBER 1000

acp_sint32_t testCompare(const void* aE1, const void* aE2)
{
    return (*(const acp_sint32_t*)aE1) - (*(const acp_sint32_t*)aE2);
}

acp_sint32_t main(void)
{
    acp_sint32_t sElements[TEST_NUMBER];
    acp_sint32_t i;
    acp_uint32_t sSeed = 0;
    
    ACT_TEST_BEGIN();

    sSeed = acpRandSeedAuto();
    for(i = 0; i < TEST_NUMBER; i++)
    {
        sElements[i] = acpRand(&sSeed);
    }

    acpSortQuickSort(sElements, TEST_NUMBER, sizeof(acp_sint32_t), testCompare);

    for(i = 1; i < TEST_NUMBER; i++)
    {
        ACT_CHECK_DESC(sElements[i - 1] <= sElements[i],
                       ("Elements are not sorted! %d %d",
                        sElements[i - 1], sElements[i]));
    }

    ACT_TEST_BEGIN();

    return 0;
}
