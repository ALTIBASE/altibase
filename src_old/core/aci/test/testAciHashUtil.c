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
 * $Id: testAciHashUtil.c 10969 2010-04-29 09:04:59Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <aciHashUtil.h>

#define ACI_TEST_STRING1 "Test string 1"
#define ACI_TEST_INIT_VALUE1 95

#define ACI_TEST_INIT_VALUE2 33
#define ACI_TEST_STRING2 "Test string 2"

#define ACI_TEST_EMPTY_STRING ""

void testHashString(void)
{
    acp_uint32_t sHash1;
    acp_uint32_t sHash2;

    /* Case1 */
    sHash1 = aciHashHashString(ACI_TEST_INIT_VALUE1,
                               (const acp_uint8_t *)ACI_TEST_STRING1,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_STRING1, 256));

    sHash2 = aciHashHashString(ACI_TEST_INIT_VALUE1,
                               (const acp_uint8_t *)ACI_TEST_STRING1,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_STRING1, 256));

    ACT_CHECK(sHash1 == sHash2);

    /* Case2 */
    sHash2 = aciHashHashString(ACI_TEST_INIT_VALUE2,
                               (const acp_uint8_t *)ACI_TEST_STRING1,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_STRING1, 256));

    ACT_CHECK(sHash1 != sHash2);

    /* Case3 */
    sHash1 = aciHashHashString(ACI_TEST_INIT_VALUE1,
                               (const acp_uint8_t *)ACI_TEST_STRING1,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_STRING1, 256));

    sHash2 = aciHashHashString(ACI_TEST_INIT_VALUE1,
                               (const acp_uint8_t *)ACI_TEST_STRING2,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_STRING2, 256));

    ACT_CHECK(sHash1 != sHash2);

    /* Case4 */
    sHash1 = aciHashHashString(ACI_TEST_INIT_VALUE1,
                              (const acp_uint8_t *)ACI_TEST_EMPTY_STRING,
                              (acp_uint32_t)acpCStrLen(ACI_TEST_EMPTY_STRING,
                                                       256));

    sHash2 = aciHashHashString(ACI_TEST_INIT_VALUE1,
                               (const acp_uint8_t *)ACI_TEST_EMPTY_STRING,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_EMPTY_STRING,
                                                        256));

    ACT_CHECK(sHash1 == sHash2);

    /* Case5 */
    sHash2 = aciHashHashString(ACI_TEST_INIT_VALUE2,
                               (const acp_uint8_t *)ACI_TEST_EMPTY_STRING,
                               (acp_uint32_t)acpCStrLen(ACI_TEST_EMPTY_STRING,
                                                        256));

    ACT_CHECK(sHash1 != sHash2);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testHashString();

    ACT_TEST_END();

    return 0;
}
