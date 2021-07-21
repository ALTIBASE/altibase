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
 
/******************************************************************************
 * $Id: testAclQueue.c 1878 2008-03-03 05:34:42Z shsuh $
 ******************************************************************************/

#include <act.h>
#include <acpRand.h>
#include <aclCompression.h>

#define TEST_COMP_MEM_SIZE  2048
#define TEST_MAX_LOOP       1000

void testCompDecomp(void)
{
    void        *sOrgMem    = NULL;
    void        *sDestMem   = NULL;
    void        *sDecompMem = NULL;
    void        *sWorkMem;

    acp_uint32_t i;
    acp_uint32_t j;
    acp_uint32_t sDestSize;
    acp_uint32_t sDecompSize;
    acp_uint32_t sSeed = 0;
    
    for (i = 0; i < TEST_MAX_LOOP; i++)
    {
        acp_uint32_t  sBufferSize;

        sSeed = acpRandSeedAuto();
        
        sBufferSize = TEST_COMP_MEM_SIZE + (acpRand(&sSeed) % 2048); 


        ACT_CHECK(acpMemAlloc(&sOrgMem, sBufferSize) == ACP_RC_SUCCESS);
        ACT_CHECK(acpMemAlloc(&sWorkMem, ACL_COMPRESSION_WORK_SIZE) == ACP_RC_SUCCESS);
        ACT_CHECK(acpMemAlloc(&sDestMem, ACL_COMPRESSION_MAX_OUTSIZE(sBufferSize))
                  == ACP_RC_SUCCESS);
        ACT_CHECK(acpMemAlloc(&sDecompMem, sBufferSize) == ACP_RC_SUCCESS);

        /* generate original buffer */
        for (j = 0; j < sBufferSize; j++)
        {
            ((acp_byte_t *)sOrgMem)[j] = (acp_byte_t)(acpRand(&sSeed) % 0xFF);
        }

        ACT_CHECK(aclCompress(sOrgMem,  sBufferSize,
                              sDestMem,IDU_COMPRESSION_MAX_OUTSIZE(sBufferSize),
                              &sDestSize,
                              sWorkMem) == ACP_RC_SUCCESS);
        ACT_CHECK(aclDecompress(sDestMem, sDestSize,
                                sDecompMem,  sBufferSize,
                                &sDecompSize) == ACP_RC_SUCCESS);

        ACT_CHECK(acpMemCmp(sOrgMem, sDecompMem, sBufferSize) == 0);

        ACT_CHECK_DESC(sDecompSize == sBufferSize,
                       ("Error in Size of Compresstion : Org = %d Comp = %d\n",
                        sBufferSize, sDecompSize));

        acpMemFree(sOrgMem);
        acpMemFree(sWorkMem);
        acpMemFree(sDestMem);
        acpMemFree(sDecompMem);

    }

}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testCompDecomp();

    ACT_TEST_END();

    return 0;
}
