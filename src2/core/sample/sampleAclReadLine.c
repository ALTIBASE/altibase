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
 * $Id: sampleAclReadLine.c 8570 2009-11-09 05:35:11Z gurugio $
 ******************************************************************************/


#include <acp.h>
#include <acl.h>

acp_char_t *sampleComplete(const acp_char_t *aText,
                           acp_sint32_t aCount)
{
    acp_char_t *sBuf;

    (void)acpPrintf("Get Text->%s\n", aText);
    (void)acpPrintf("Call Count->%d\n", aCount);

    /* execute one time */
    if (aCount > 0)
    {
        (void)acpPrintf("Exit Complete\n");
        sBuf = NULL;
    }
    else
    {
        /* message is allocated here,
         * but freed out there */
        (void)acpMemAlloc((void **)&sBuf, 10);
        (void)acpCStrCpy(sBuf, 9, "hello", 6);

        (void)acpPrintf("Return Text <%s>\n", sBuf);
    }

    return sBuf;
}

acp_sint32_t main(void)
{
    acp_rc_t     sRC;
    ACP_STR_DECLARE_STATIC(sLineBuf, 128);
    acp_str_t sName = ACP_STR_CONST("sample");

    sRC = acpInitialize();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    sRC = aclReadLineInit(&sName);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    aclReadLineSetComplete(sampleComplete);
    ACP_STR_INIT_STATIC(sLineBuf);

    sRC = aclReadLineGetString("PROMPT#", &sLineBuf);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    sRC = aclReadLineFinal();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    ACP_EXCEPTION(PROG_FAIL)
    {
    }
    
    ACP_EXCEPTION_END;
        (void)acpFinalize();
    return 0;
}
