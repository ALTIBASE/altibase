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
 * $Id: testAciVersion.c 10969 2010-04-29 09:04:59Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acl.h>
#include <acpCStr.h>
#include <acpList.h>
#include <aciVersion.h>

void testAciVersion(void)
{
    const acp_char_t *sStr = NULL;

    ACT_CHECK(aciMkProductionTimeString() == 0);

    sStr = aciGetProductionTimeString();
    ACT_CHECK(sStr != NULL);
    ACT_CHECK(acpCStrLen(sStr, 256) > 0);

    sStr = aciGetVersionString();
    ACT_CHECK(sStr != NULL);
    ACT_CHECK(acpCStrLen(sStr, 256) > 0);

    ACT_CHECK(aciMkSystemInfoString() == 0);

    sStr = aciGetSystemInfoString();
    ACT_CHECK(sStr != NULL);
    ACT_CHECK(acpCStrLen(sStr, 256) > 0);

    sStr = aciGetPackageInfoString();
    ACT_CHECK(sStr != NULL);
    ACT_CHECK(acpCStrLen(sStr, 256) > 0);
    
    sStr = aciGetCopyRightString();
    ACT_CHECK(sStr != NULL);
    ACT_CHECK(acpCStrLen(sStr, 256) > 0);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testAciVersion();

    ACT_TEST_END();

    return 0;
}
