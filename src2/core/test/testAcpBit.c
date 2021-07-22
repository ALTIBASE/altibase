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
 * $Id: testAcpBit.c 3329 2008-10-20 04:01:14Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpBit.h>


acp_sint32_t main(void)
{
    acp_uint64_t s64Int;
    acp_sint32_t i;
    acp_sint32_t j;

    ACT_TEST_BEGIN();

    ACT_CHECK(acpBitFfs32(0) == -1);
    ACT_CHECK(acpBitFls32(0) == -1);

    ACT_CHECK(acpBitFfs32(0x80000000) == 31);
    ACT_CHECK(acpBitFls32(0x00000001) == 0);

    ACT_CHECK(acpBitFfs32(0x80000002) == 1);
    ACT_CHECK(acpBitFls32(0x80000002) == 31);

    ACT_CHECK(acpBitFfs32(0x00800200) == 9);
    ACT_CHECK(acpBitFls32(0x00800200) == 23);

    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0)) == -1);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0)) == -1);

    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0x8000000000000002)) == 1);
    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0x0000000000000002)) == 1);
    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0x8000000000000000)) == 63);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0x8000000000000002)) == 63);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0x0000000000000002)) == 1);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0x8000000000000000)) == 63);

    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0x0080000000000200)) == 9);
    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0x0000000000000200)) == 9);
    ACT_CHECK(acpBitFfs64(ACP_UINT64_LITERAL(0x0080000000000000)) == 55);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0x0080000000000200)) == 55);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0x0000000000000200)) == 9);
    ACT_CHECK(acpBitFls64(ACP_UINT64_LITERAL(0x0080000000000000)) == 55);

    /* Testcase 1 */
    for(i = 0, s64Int=ACP_UINT64_LITERAL(0x8000000000000000);
        i <= 64; i++, s64Int >>= 1)
    {
        j = acpBitFfs64(s64Int);
        ACT_CHECK_DESC((63 - i == j),
                       (
                           "acpBitFfs64(%016llX) must be %d but returned %d",
                           s64Int, 63 - i, j
                       )
                      );
        j = acpBitFls64(s64Int);
        ACT_CHECK_DESC((63 - i == j),
                       (
                           "acpBitFls64(%016llX) must be %d but returned %d",
                           s64Int, 63 - i, j
                       )
                      );
    }


    /* Testcase 2 */
    for(i = 0, s64Int=ACP_UINT64_LITERAL(0xFFFFFFFFFFFFFFFF);
        i < 64; i++, s64Int>>=1)
    {
        j = acpBitFfs64(s64Int);
        ACT_CHECK_DESC((0 == j),
                       (
                           "acpBitFfs64(%016llX) must be %d but returned %d",
                           s64Int, 0, j
                       )
                      );
        j = acpBitFls64(s64Int);
        ACT_CHECK_DESC((63 - i == j),
                       (
                           "acpBitFls64(%016llX) must be %d but returned %d",
                           s64Int, 63 - i, j
                       )
                      );
    }

    ACT_TEST_END();

    return 0;
}
