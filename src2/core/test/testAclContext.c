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
 * $Id: testAclContext.c 1578 2008-01-04 14:11:23Z shsuh $
 ******************************************************************************/

#include <act.h>
#include <aclContext.h>


typedef struct testContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;

    acp_sint32_t mInt;
} testContext;


acp_sint32_t main(void)
{
    ACL_CONTEXT_DECLARE(testContext, sContext);

    ACT_TEST_BEGIN();

    ACL_CONTEXT_INIT();

    ACL_CONTEXT_FINAL();

    ACT_TEST_END();

    return 0;
}
