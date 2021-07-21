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
 * $Id: testAcpStdPerf.c 11010 2010-05-06 02:40:10Z gurugio $
 ******************************************************************************/

/*******************************************************************************
 * A Skeleton Code for Testcases
*******************************************************************************/

#include <act.h>
#include <acpStd.h>
#include <acpMem.h>
#include <acpRand.h>
#include <acpProc.h>
#include <acpMem.h>

#define TEST_SIZE  (1024)

acp_char_t*     gFilename5 = "testAcpStd5";
acp_uint32_t    gSeed = 0;
acp_std_file_t  gFile5;
acp_byte_t*     gLargeBuffer;

void testVeryLargeFile(void)
{
    acp_sint32_t i;
    acp_size_t k;
    acp_offset_t sOffset;
    acp_offset_t sOffset2 = 0;
    acp_offset_t sFileSize = 0;
    acp_rc_t sRC;

    for(i = 0; i < TEST_SIZE * TEST_SIZE; i++)
    {
        gLargeBuffer[i] = (acp_byte_t)(i % 128);
    }

    /* Write 1GByte of file */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile5, gFilename5, ACP_STD_OPEN_WRITE_TRUNC)
            ));
    for(i = 0; i < TEST_SIZE; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdWrite(&gFile5, (void*)gLargeBuffer,
                        TEST_SIZE, TEST_SIZE,
                        &k)
            ));
        ACT_CHECK(TEST_SIZE == k);

        if(127 == i % 128)
        {
            (void)acpPrintf("acpStd Performance::[%4d]MBytes were written\n",
                      i + 1);
        }
        else
        {
            /* Do nothing */
        }
    }

    /* sOffset = TEST_SIZE * TEST_SIZE * TEST_SIZE */
    sOffset  = TEST_SIZE;
    sOffset *= TEST_SIZE;
    sOffset *= TEST_SIZE;

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile5, 0, ACP_STD_SEEKSET, &sOffset2)
            ));
    ACT_CHECK(0 == sOffset2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile5, 0, ACP_STD_SEEKEND, &sOffset2)
            ));
    ACT_CHECK(sOffset2 == sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFile5)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile5)));
    
    /* Read 1GByte of file */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile5, gFilename5, ACP_STD_OPEN_READ)
            ));
    for(i = 0; i < TEST_SIZE; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile5, (void*)gLargeBuffer,
                       TEST_SIZE, TEST_SIZE,
                       &k)
            ));
        ACT_CHECK(TEST_SIZE == k);

        if(127 == i % 128)
        {
            (void)acpPrintf("acpStd Performance::[%4d]MBytes were read\n",
                      i + 1);
        }
        else
        {
            /* Do nothing */
        }
    }
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile5)));

    /* Move to random position and read 1MBytes */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile5, gFilename5, ACP_STD_OPEN_READ)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile5, 0, ACP_STD_SEEKEND, &sFileSize)
            ));

    for(i = 0; i < TEST_SIZE; i++)
    {
        sOffset2 = acpRand(&gSeed) % sOffset;
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile5, sOffset2, ACP_STD_SEEKSET, NULL)
            ));
        sRC = acpStdRead(&gFile5, (void*)gLargeBuffer,
                         TEST_SIZE, TEST_SIZE,
                         &k);
        if((acp_offset_t)(sOffset2 + (TEST_SIZE * TEST_SIZE)) <= sFileSize)
        {
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            ACT_CHECK_DESC(ACP_RC_IS_EOF(sRC),
                           ("Read offset is [%lld (+ %d), %lld) > %lld,"
                            " result must be EOF but %d. (%d bytes read)\n",
                            (acp_sint64_t)sOffset2, 
                            (acp_sint64_t)sOffset2 + TEST_SIZE, TEST_SIZE,
                            (acp_sint64_t)sFileSize,
                            (acp_sint32_t)sRC,
                            (acp_sint32_t)k));
        }

        if(127 == i % 128)
        {
            (void)acpPrintf("acpStd Performance::Random access"
                            " [%4d] times successful\n",
                      i + 1);
        }
        else
        {
            /* Do nothing */
        }
    }

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile5)));
    (void)acpPrintf("acpStd Performance::Read and write of 1G done\n");

}

void testDeleteFiles(void)
{
    /* Delete test files */
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilename5)));
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    /* Initialize Random Seed */
    gSeed = acpRandSeedAuto();

    /* Allocate memory to test large amount of read and write */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
        acpMemAlloc((void**)(&gLargeBuffer), TEST_SIZE * TEST_SIZE)
        ));

    testVeryLargeFile();
    testDeleteFiles();

    /* Free used memory */
    acpMemFree(gLargeBuffer);

    ACT_TEST_END();

    return 0;
}
