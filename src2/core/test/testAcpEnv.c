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
 * $Id: testAcpEnv.c 9886 2010-02-03 08:51:40Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpEnv.h>


acp_sint32_t main(void)
{
    acp_char_t*   sName   = "__TEST_ACP_ENV_NAME";
    acp_char_t*   sValue1 = "__TEST_ACP_ENV_VALUE1";
    acp_char_t*   sValue2 = "__TEST_ACP_ENV_VALUE2";
    acp_char_t*   sValue3 = "__TEST_ACP_ENV_VALUE3";
    acp_char_t*   sNoExistName = "__NO_EXIST_NAME";

    acp_char_t*   sNoPath = "a/b\\c/d\\e";/* broken path */
    acp_char_t*   sUnixPath = "a/b/c/d/e";
    acp_char_t*   sWinPath = "a\\b\\c\\d\\e";
    
    acp_rc_t      sRC;
    acp_char_t   *sValue;
    

    ACT_TEST_BEGIN();

    /*
     * test successful case for setting normal string values
     */
    
    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpEnvSet(sName, sValue1, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sValue, sValue1, 128) == 0);

    sRC = acpEnvSet(sName, sValue2, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sValue, sValue1, 128) == 0);

    sRC = acpEnvSet(sName, sValue3, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sValue, sValue3, 128) == 0);

    /*
     * test for path value setting,
     * acpEnvSet, acpEnvGet do not recognize path value.
     */
    
    sRC = acpEnvSet(sName, sNoPath, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sValue, sNoPath, 128) == 0);
    
    sRC = acpEnvSet(sName, sUnixPath, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sValue, sUnixPath, 128) == 0);
    
    sRC = acpEnvSet(sName, sWinPath, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpEnvGet(sName, &sValue);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sValue, sWinPath, 128) == 0);

    /*
     * test for failure cases
     */

    sRC = acpEnvGet(sNoExistName, &sValue);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    
    ACT_TEST_END();

    return 0;
}
