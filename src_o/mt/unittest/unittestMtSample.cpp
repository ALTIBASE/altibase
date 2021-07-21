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
 
#include <idl.h>
#include <act.h>
/*
 * Function to demonstrate UnitTest framework
 */
SInt unittestMtGetOne()
{
    return 1;
}

/*
 * ACT_CHECK and ACT_CHECK_DESC demonstration function
 */
void unittestMtTestCheck()
{
    SInt sRet;

    ACT_CHECK( unittestMtGetOne() == 1 );

    sRet = unittestMtGetOne();
    ACT_CHECK_DESC( sRet == 1, ( "unittestMtGetOne() should return 1 but %d", sRet ) );
}

/*
 * ACT_ERROR demonstration function
 */
void unittestMtTestError()
{
    if ( unittestMtGetOne() == 1 )
    {
        ACT_CHECK( unittestMtGetOne() == 1 );
    }
    else
    {
        ACT_ERROR( ( "Error: unittestMtGetOne() should return 1 but %d", unittestMtGetOne() ) );
    }
}

/*
 * ACT_ASSERT and ACT_ASSERT_DESC demonstration function
 */
void unittestMtTestAssert()
{
    SInt sRet;

    ACT_ASSERT( unittestMtGetOne() == 1 );

    sRet = unittestMtGetOne();
    ACT_ASSERT_DESC( sRet == 1, ( "unittestMtGetOne() should return 1 but %d", sRet ) );
}

/*
 * ACT_DUMP and ACT_DUMP_DESC demonstration function
 */
void unittestMtTestDump()
{
    UChar sBuf[] = { 'A', 'B', 'C', 'D', 'E',
                     'A', 'B', 'C', 'D', 'E',
                     'A', 'B', 'C', 'D', 'E',
                     'A', 'B', 'C', 'D', 'E',
                     'A', 'B', 'C', 'D', 'E',
                     'A', 'B', 'C', 'D', 'E',
                   };

    if ( unittestMtGetOne() != 1 )
    {
        ACT_DUMP( &sBuf, sizeof( sBuf ) );

        ACT_DUMP_DESC( &sBuf, sizeof( sBuf ), ( "Dump description." ) );
    }
}

int main()
{
    ACT_TEST_BEGIN();

    /* Run function to check ACT_CHECK ACT_CHECK_DESC use */
    unittestMtTestCheck();

    /* Run function to check ACT_ERROR use */
    unittestMtTestError();

    /* Run function to check ACT_ASSERT ACT_ASSERT_DESC use */
    unittestMtTestAssert();

    /* Run function to check ACT_DUMP ACT_DUMP_DESC use */
    unittestMtTestDump();

    ACT_TEST_END();

    return 0;
}
