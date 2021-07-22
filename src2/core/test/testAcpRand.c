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
 * $Id: testAcpRand.c 5730 2009-05-21 06:05:53Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpRand.h>


#define TEST_COUNT 100

acp_sint32_t main(void)
{
    acp_sint32_t sValueBefore;
    acp_sint32_t sValueAfter;
    acp_uint32_t sSeed = acpRandSeedAuto();
    acp_sint32_t i;

    ACT_TEST_BEGIN();

    for (i = 0; i < TEST_COUNT; i++)
    {
        sValueBefore = acpRand(&sSeed);
        sValueAfter = acpRand(&sSeed);
    
        ACT_ASSERT(sValueBefore != sValueAfter);
    }
    
    ACT_TEST_END();

    return 0;
}
