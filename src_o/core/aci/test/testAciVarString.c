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
 * $Id: testAciVarString.c 10969 2010-04-29 09:04:59Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acl.h>
#include <acpCStr.h>
#include <acpList.h>
#include <aciVarString.h>

#define TEST_ELEMENT_COUNT 10
#define TEST_ELEMENT_SIZE 256 

#define TEST_STRING "test"
#define TEST_STRING_SIZE 4

#define TEST_STRING2 "test2"
#define TEST_STRING_SIZE2 5


void testAciVarString(void)
{
    acl_mem_pool_t   sPool;
    acp_rc_t         sRC;
    aci_var_string_t sString;
    acp_uint32_t     sLen = 0;

    /* Init */
    sRC = aclMemPoolCreate(&sPool, TEST_ELEMENT_SIZE, TEST_ELEMENT_COUNT, 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(aciVarStringInitialize(&sString,
                                     &sPool,
                                     TEST_ELEMENT_SIZE) == ACE_RC_SUCCESS);
    
    /* Test  */
    sLen = aciVarStringGetLength(&sString);
    ACT_CHECK(sLen == 0);


    ACT_CHECK( aciVarStringAppend(&sString,TEST_STRING) == ACE_RC_SUCCESS);

    sLen = aciVarStringGetLength(&sString);
    ACT_CHECK(sLen == TEST_STRING_SIZE);

    ACT_CHECK(aciVarStringPrint(&sString, TEST_STRING2) == ACE_RC_SUCCESS);

    sLen = aciVarStringGetLength(&sString);
    ACT_CHECK(sLen == TEST_STRING_SIZE2);


    ACT_CHECK(aciVarStringTruncate(&sString, ACP_FALSE) == ACE_RC_SUCCESS);


    ACT_CHECK(aciVarStringAppendFormat(&sString, "%s", TEST_STRING) ==
              ACE_RC_SUCCESS);
    ACT_CHECK(aciVarStringAppendFormat(&sString, "%s", TEST_STRING2) ==
              ACE_RC_SUCCESS);

    sLen = aciVarStringGetLength(&sString);

    ACT_CHECK(sLen == (TEST_STRING_SIZE + TEST_STRING_SIZE2));

    ACT_CHECK(aciVarStringPrintFormat(&sString,"%s",TEST_STRING) ==
              ACE_RC_SUCCESS);

    sLen = aciVarStringGetLength(&sString);
    ACT_CHECK(sLen == TEST_STRING_SIZE);

    /* Finalize string */
    ACT_CHECK(aciVarStringFinalize(&sString) == ACE_RC_SUCCESS);

    aclMemPoolDestroy(&sPool);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();
    
    testAciVarString();

    ACT_TEST_END();

    return 0;
}
