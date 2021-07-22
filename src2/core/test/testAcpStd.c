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
 * $Id: testAcpStd.c 11055 2010-05-11 07:07:56Z gurugio $
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
#include <aclCode.h>

#define TEST_NEWLINE_FILE   "testAcpStdText"
#define TEST_LINE_MAXLEN    (1024)

acp_char_t *gFilename1   = "testAcpStd1";
acp_char_t *gFilename2   = "testAcpStd2";
acp_char_t *gFilename3   = "testAcpStd3";
acp_char_t *gFilename4   = "testAcpStd4";
acp_char_t *gFilename5   = "testAcpStd5";
acp_char_t *gFilenameUTF = "testAcpStdUTF";

acp_std_file_t gFile1;
acp_std_file_t gFile2;
acp_std_file_t gFile3;
acp_std_file_t gFile4;
acp_std_file_t gFileUTF;

acp_byte_t gBuffer[1024];
acp_byte_t gBuffer1[1024];
acp_byte_t gBuffer2[1024];
acp_byte_t* gLargeBuffer;

acp_uint32_t gSeed = 0;

void testFileCreation(void)
{
    acp_sint32_t j;
    acp_size_t k;
    acp_offset_t sOffset = 0;

    /* Test writing 256 buffer 256 times */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile1, gFilename1, ACP_STD_OPEN_WRITE_TRUNC)
            ));
    for(j = 0; j < 256; j++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdWrite(&gFile1, (void*)gBuffer, 1, 256, &k)
            ));
        ACT_CHECK(256 == k);
    }
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile1, 0, ACP_STD_SEEKSET, &sOffset)
            ));
    ACT_CHECK_DESC(0 == sOffset,
                   ("File offset should be 0 but %ld\n",
                    (acp_ulong_t)sOffset));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile1, 0, ACP_STD_SEEKEND, &sOffset)
            ));
    ACT_CHECK_DESC(256 * 256 == sOffset,
                   ("File offset must be %d but %ld\n",
                    256 * 256, (acp_ulong_t)sOffset));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFile1)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile1)));

    /* Test writing 1024 buffer 1024 times */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile2, gFilename2, ACP_STD_OPEN_WRITE_TRUNC)
            ));
    for(j = 0; j < 1024; j++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdWrite(&gFile2, (void*)gBuffer, 1, 1024, &k)
            ));
        ACT_CHECK(1024 == k);
    }
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile2, 0, ACP_STD_SEEKSET, &sOffset)
            ));
    ACT_CHECK(0 == sOffset);
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile2, 0, ACP_STD_SEEKEND, &sOffset)
            ));
    ACT_CHECK_DESC(1024 * 1024 == sOffset,
                   ("File offset must be %d but %ld\n",
                    1024 * 1024, (acp_ulong_t)sOffset));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFile2)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile2)));
}

#define TEST_RW_LOOP 256

/* PutCString calls PutChar and PutByte, */
/* and also GetCString calls GetChar and GetByte. */
/* Testing PutCString and GetCString includes testing others. */
void testRWCString(void)
{
    acp_sint32_t j;
    acp_size_t k;
    acp_offset_t sOffset = 0;
    acp_char_t sPutBuf[] = "I Love SNSD";
    acp_char_t sGetBuf[32];
    acp_size_t sTestLen;

    ACT_CHECK(ACP_RC_IS_SUCCESS(
                  acpStdOpen(&gFile1, gFilename5,
                             ACP_STD_OPEN_READWRITE_TRUNC_TEXT)));

    sTestLen = acpCStrLen(sPutBuf, 256);
    ACT_CHECK(sTestLen != 0);
    if (sTestLen == 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    for(j = 0; j < TEST_RW_LOOP; j++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdPutCString(&gFile1, sPutBuf, sTestLen, &k)
            ));
        ACT_CHECK(sTestLen == k);
    }
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFile1)));
    
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile1, 0, ACP_STD_SEEKEND, &sOffset)
            ));
    ACT_CHECK_DESC((acp_slong_t)(TEST_RW_LOOP * sTestLen) ==
                   (acp_slong_t)sOffset,
                   ("File offset must be %d but %ld\n",
                    (acp_sint32_t)(TEST_RW_LOOP * sTestLen),
                    (acp_ulong_t)sOffset));

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile1, 0, ACP_STD_SEEKSET, &sOffset)
            ));
    ACT_CHECK_DESC(0 == sOffset,
                   ("File offset should be 0 but %ld\n",
                    (acp_ulong_t)sOffset));

    for(j = 0; j < TEST_RW_LOOP; j++)
    {
        /* read one sentence at a time */
        ACT_CHECK(ACP_RC_IS_SUCCESS(
                      acpStdGetCString(&gFile1, sGetBuf,
                          sTestLen + 1)
            ));
        ACT_CHECK_DESC(acpCStrCmp(sPutBuf, sGetBuf, sTestLen) == 0,
                       ("File must contain \"%s\", but \"%s\"",
                        sPutBuf, sGetBuf));
    }

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile1)));
}

void testFileCopy(void)
{
    acp_sint32_t i;
    acp_size_t k;
    acp_offset_t sOffset;

    /* Test File Copying */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile1, gFilename1, ACP_STD_OPEN_READ)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile3, gFilename3, ACP_STD_OPEN_READWRITE_TRUNC)
            ));

    /* Bulk read */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile1, (void*)gLargeBuffer, 256, 256, &k)
            ));
    ACT_CHECK(256 == k);

    /* Check if the file was written correctly */
    for(i = 0; i < 256; i++)
    {
        ACT_CHECK(
            0 == acpMemCmp(gBuffer, gLargeBuffer + i * 256, 256)
            );
        if(0 != acpMemCmp(gBuffer, gLargeBuffer + i * 256, 256))
        {
            break;
        }
        else
        {
            /* continue; */
        }
    }

    /* Bulk write */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdWrite(&gFile3, (void*)gLargeBuffer, 256, 256, &k)
            ));
    ACT_CHECK(256 == k);

    /* Move to the beginning of file */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile1, 0, ACP_STD_SEEKSET, &sOffset)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile3, 0, ACP_STD_SEEKSET, &sOffset)
            ));

    /* Check if the files are same */
    for(i = 0; i < 256; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile1, (void*)gBuffer1, 1, 256, &k)
            ));
        ACT_CHECK(256 == k);
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile3, (void*)gBuffer2, 1, 256, &k)
            ));
        ACT_CHECK(256 == k);

        ACT_CHECK(
            0 == acpMemCmp(gBuffer1, gBuffer2, 256)
            );

        if(0 != acpMemCmp(gBuffer1, gBuffer2, 256))
        {
            break;
        }
        else
        {
            /* continue; */
        }
    }

    /* Flush and close files */
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFile3)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile1)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile3)));

    /* Test File Copying - larger size */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile2, gFilename2, ACP_STD_OPEN_READ)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile4, gFilename4, ACP_STD_OPEN_READWRITE_TRUNC)
            ));

    /* Bulk read */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile2, (void*)gLargeBuffer, 1024, 1024, &k)
            ));
    ACT_CHECK(1024 == k);

    /* Check if the file was written correctly */
    for(i = 0; i < 1024; i++)
    {
        ACT_CHECK(
            0 == acpMemCmp(gBuffer, gLargeBuffer + i * 1024, 1024)
            );
    }

    /* Bulk write */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdWrite(&gFile4, (void*)gLargeBuffer, 1024, 1024, &k)
            ));
    ACT_CHECK(1024 == k);

    /* Move to the beginning of file */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile2, 0, ACP_STD_SEEKSET, &sOffset)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile4, 0, ACP_STD_SEEKSET, &sOffset)
            ));

    /* Check if the files are same */
    for(i = 0; i < 1024; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile2, (void*)gBuffer1, 1, 1024, &k)
            ));
        ACT_CHECK(1024 == k);
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdRead(&gFile4, (void*)gBuffer2, 1, 1024, &k)
            ));
        ACT_CHECK(1024 == k);

        ACT_CHECK(
            0 == acpMemCmp(gBuffer1, gBuffer2, 1024)
            );
    }

    /* Flush and close files */
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFile4)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile2)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile4)));
}

void testFileAccess(void)
{
    acp_sint32_t i;
    acp_size_t j;

    acp_byte_t sByte = 0;
    acp_offset_t sOffset = 0;
    acp_offset_t sOffset2 = 0;
    acp_bool_t sIsEOF;

    acp_size_t sToRead;
    acp_size_t sRead;

    acp_rc_t sRC;

    /* Sequential Access */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile1, gFilename1, ACP_STD_OPEN_READ)
            ));
    for(i = 0; i < 256 * 256; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdGetByte(&gFile1, &sByte)
            ));
        ACT_CHECK((acp_byte_t)(i % 256) == sByte);
    }

    while(ACP_RC_IS_SUCCESS(acpStdGetByte(&gFile1, &sByte)))
    {
        /* Proceed to the end of File!! */
    }

    /* EOF Check */
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdIsEOF(&gFile1, &sIsEOF)));
    ACT_CHECK(sIsEOF);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile1)));

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile2, gFilename2, ACP_STD_OPEN_READ)
            ));
    for(i = 0; i < 1024 * 1024; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdGetByte(&gFile2, &sByte)
            ));
        ACT_CHECK((acp_byte_t)(i % 256) == sByte);
    }

    while(ACP_RC_IS_SUCCESS(acpStdGetByte(&gFile2, &sByte)))
    {
        /* Proceed to the end of File!! */
    }
    /* EOF Check */
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdIsEOF(&gFile2, &sIsEOF)));
    ACT_CHECK(sIsEOF);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile2)));

    /* Random Access */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile4, gFilename4, ACP_STD_OPEN_READWRITE)
            ));
    for(i = 0; i < 1024; i++)
    {
        sOffset = acpRand(&gSeed) % (1024 * 1024);
        sToRead = acpRand(&gSeed) % 1024;

        ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile4, sOffset, ACP_STD_SEEKSET, &sOffset2)
            ));

        sRC = acpStdRead(&gFile4, gBuffer2, 1, sToRead, &sRead);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_EOF(sRC));
        for(j = 0; j < sRead; j++)
        {
            ACT_CHECK((acp_byte_t)((sOffset + j) % 256) == gBuffer2[j]);

            if((acp_byte_t)((sOffset + j) % 256) == gBuffer2[j])
            {
                /* loop! */
            }
            else
            {
                break;
            }
        }
    }
    
    sToRead = 1024;

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile4, 0, ACP_STD_SEEKEND, &sOffset2)
            ));
    sRC = acpStdRead(&gFile4, gBuffer2, 1, sToRead, &sRead);
    ACT_CHECK_DESC(ACP_RC_IS_EOF(sRC), ("RC=%d\n", sRC));
    ACT_CHECK(sToRead != sRead);
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile4, 0, ACP_STD_SEEKSET, &sOffset2)
            ));
    sRC = acpStdRead(&gFile4, gBuffer2, 1, sToRead, &sRead);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sToRead == sRead);

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile4)));
}

void testFileErrors(void)
{
    acp_byte_t sByte = 'A';
    acp_size_t k;

    /* Write on read-only */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile1, gFilename1, ACP_STD_OPEN_READ)
            ));
    ACT_CHECK(ACP_RC_NOT_SUCCESS(
            acpStdPutByte(&gFile1, sByte)
            ));
    
    ACT_CHECK(ACP_RC_NOT_SUCCESS(acpStdWrite(&gFile1,
                                             (void*)gBuffer,
                                             1,
                                             256,
                                             &k)));
    ACT_CHECK(k == 0);

    /* Read over EOF */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdSeek(&gFile1, 0, ACP_STD_SEEKEND, NULL)
            ));
    ACT_CHECK(ACP_RC_NOT_SUCCESS(
            acpStdGetByte(&gFile1, &sByte)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile1)));

}

void testUTF8ReadWrite(void)
{
    acp_qchar_t sUTF8Codes[20] = 
    {
        /* Ansi Characters */
        'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\n',
        /* KSC-5601 Characters 알티베이스 */
        0xBECB, 0xC6BC, 0xBAA3, 0xC0CC, 0xBDBA, 0
    };
    acp_qchar_t sBuffer[20];
    acp_size_t sRead;
    acp_size_t sWritten;

    /* Test writing UTF-8 Data */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFileUTF, gFilenameUTF, ACP_STD_OPEN_WRITE_TRUNC)
            ));

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            aclCodePutStringUTF8(&gFileUTF, sUTF8Codes, 20, &sWritten)
            ));
    ACT_CHECK(20 == sWritten);

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdFlush(&gFileUTF)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFileUTF)));

    /* Test writing UTF-8 Data */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFileUTF, gFilenameUTF, ACP_STD_OPEN_READ)
            ));

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            aclCodeGetStringUTF8(&gFileUTF, sBuffer, 20, &sRead)
            ));
    ACT_CHECK(20 == sRead);
    /* Check read properly */
    ACT_CHECK(0 == acpMemCmp(sUTF8Codes, sBuffer, 20 * sizeof(acp_qchar_t)));

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFileUTF)));
}

void testDeleteFiles(void)
{
    /* Delete test files */
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilename1)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilename2)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilename3)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilename4)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilename5)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(gFilenameUTF)));
}

void testNewLine(void)
{
    acp_std_file_t  sFile;
    acp_rc_t        sRC;
    acp_char_t      sLine[TEST_LINE_MAXLEN];

    /* 
     * Read two lines.
     * Second line will contain no newline
     */
    sRC = acpStdOpen(&sFile, TEST_NEWLINE_FILE, ACP_STD_OPEN_READ);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, TEST_LINE_MAXLEN);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, TEST_LINE_MAXLEN);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

acp_sint32_t main(void)
{
    acp_sint32_t i;

    /* Fill buffer to test */
    for(i = 0; i < 1024; i++)
    {
        gBuffer[i] = (acp_char_t)(i % 256);
    }

    ACT_TEST_BEGIN();

    /* Initialize Random Seed */
    gSeed = acpRandSeedAuto();

    /* Allocate memory to test large amount of read and write */
    ACT_CHECK(ACP_RC_IS_SUCCESS(
        acpMemAlloc((void**)(&gLargeBuffer), 1024 * 1024)
        ));

    testFileCreation();
    testFileCopy();
    testFileAccess();
    testFileErrors();
    testUTF8ReadWrite();
    testRWCString();

    testNewLine();
    
    /* Checks TTY - not runs on daily build. Commented out.
    ACT_CHECK(ACP_TRUE == acpStdIsTTY(stdin));
    ACT_CHECK(ACP_TRUE == acpStdIsTTY(stdout));
    ACT_CHECK(ACP_TRUE == acpStdIsTTY(stderr));
    */

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpStdOpen(&gFile1, gFilename1, ACP_STD_OPEN_READ)
            ));
    ACT_CHECK(ACP_FALSE == acpStdIsTTY(&gFile1));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(&gFile1)));

    testDeleteFiles();

    /* Free used memory */
    acpMemFree(gLargeBuffer);

    ACT_TEST_END();

    return 0;
}
