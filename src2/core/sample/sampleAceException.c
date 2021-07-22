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
 
#include <acp.h>
#include <acl.h>


typedef struct sampleContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
} sampleContext;


ace_rc_t doSomething(sampleContext *aContext, acp_sint32_t aValue)
{
    acp_file_t   sFile1;
    acp_file_t   sFile2;
    acp_rc_t     sRC;
    acp_sint32_t sStage = 0;

    ACP_STR_DECLARE_DYNAMIC(sStr1);
    ACP_STR_DECLARE_DYNAMIC(sStr2);

    ACP_STR_INIT_DYNAMIC(sStr1, 10, 10);
    ACP_STR_INIT_DYNAMIC(sStr2, 10, 10);

    sRC = acpStrCpyFormat(&sStr1, "file1.txt");
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), LABEL_STRING_OPERATION_FAIL);

    sRC = acpStrCpyFormat(&sStr2, "file2.txt");
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), LABEL_STRING_OPERATION_FAIL);

    sRC = acpFileOpen(&sFile1, &sStr1, 0, 0);
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), LABEL_FILE_OPEN_FAIL);
    sStage++;

    sRC = acpFileOpen(&sFile2, &sStr2, 0, 0);
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), LABEL_FILE_OPEN_FAIL);
    sStage++;

    /*
     * do something
     */
    ACE_TEST(aValue == 100);

    sStage--;
    sRC = acpFileClose(&sFile2);
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), LABEL_FILE_CLOSE_FAIL);

    sStage--;
    sRC = acpFileClose(&sFile1);
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), LABEL_FILE_CLOSE_FAIL);

    ACP_STR_FINAL(sStr1);
    ACP_STR_FINAL(sStr2);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(LABEL_STRING_OPERATION_FAIL)
    {
        ACE_SET(ACE_SET_ERROR(ACE_LEVEL_0, ULL_ERR_STRING, &sStr1, sRC));
    }
    ACE_EXCEPTION(LABEL_FILE_OPEN_FAIL)
    {
        ACE_SET(ACE_SET_ERROR(ACE_LEVEL_0, ULL_ERR_FILE_OPEN, &sStr1, sRC));
    }
    ACE_EXCEPTION(LABEL_FILE_CLOSE_FAIL)
    {
        ACE_SET(ACE_SET_ERROR(ACE_LEVEL_0, ULL_ERR_FILE_CLOSE, sRC));
    }
    ACE_EXCEPTION_END;

    switch (sStage)
    {
        case 2:
            (void)acpFileClose(&sFile2);
        case 1:
            (void)acpFileClose(&sFile1);
        default:
            ACP_STR_FINAL(sStr1);
            ACP_STR_FINAL(sStr2);
            break;
    }

    return ACE_RC_FAILURE;
}


acp_sint32_t threadMain(void *aArg)
{
    acp_sint32_t i;

    ACL_CONTEXT_DECLARE(sampleContext, sContext);

    ACL_CONTEXT_INIT();

    for (i = 0; i < 100; i++)
    {
        (void)doSomething(&sContext, i);
    }

    ACL_CONTEXT_FINAL();

    return 0;
}
