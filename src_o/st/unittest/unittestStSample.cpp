/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <idl.h>
#include <act.h>

/*
 * Function to demonstrate UnitTest framework
 */
SInt unittestStGetOne()
{
    return 1;
}

/*
 * ACT_CHECK and ACT_CHECK_DESC demonstration function
 */
void unittestStTestCheck()
{
    SInt sRet;

    ACT_CHECK(unittestStGetOne() == 1);

    sRet = unittestStGetOne();
    ACT_CHECK_DESC(sRet == 1, ("unittestStGetOne() should return 1 but %d", sRet));
}

/*
 * ACT_ERROR demonstration function
 */
void unittestStTestError()
{
    if (unittestStGetOne() == 1)
    {
        ACT_CHECK(unittestStGetOne() == 1);
    }
    else
    {
        ACT_ERROR(("Error: unittestStGetOne() should return 1 but %d", unittestStGetOne()));
    }
}

/*
 * ACT_ASSERT and ACT_ASSERT_DESC demonstration function
 */
void unittestStTestAssert()
{
    SInt sRet;

    ACT_ASSERT(unittestStGetOne() == 1);

    sRet = unittestStGetOne();
    ACT_ASSERT_DESC(sRet == 1, ("unittestStGetOne() should return 1 but %d", sRet));
}

/*
 * ACT_DUMP and ACT_DUMP_DESC demonstration function
 */
void unittestStTestDump()
{
    UChar sBuf[] = {'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                   };

    if (unittestStGetOne() != 1)
    {
        ACT_DUMP(&sBuf, sizeof(sBuf));

        ACT_DUMP_DESC(&sBuf, sizeof(sBuf), ("Dump description."));
    }
}

int main()
{
    ACT_TEST_BEGIN();

    /* Run function to check ACT_CHECK ACT_CHECK_DESC use */
    unittestStTestCheck();

    /* Run function to check ACT_ERROR use */
    unittestStTestError();

    /* Run function to check ACT_ASSERT ACT_ASSERT_DESC use */
    unittestStTestAssert();

    /* Run function to check ACT_DUMP ACT_DUMP_DESC use */
    unittestStTestDump();

    ACT_TEST_END();

    return 0;
}
