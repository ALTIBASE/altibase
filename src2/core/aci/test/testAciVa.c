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
 * $Id: testAciVa.c 11151 2010-05-31 06:28:13Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aciVa.h>

#define TEST_UNIX_PATH_DIR "a/b/c/d"
#define TEST_UNIX_PATH_BASE "e"
#define TEST_UNIX_PATH_EXT "out"

#define TEST_WIN_PATH_DIR "a\\b\\c\\d"
#define TEST_WIN_PATH_BASE "e"
#define TEST_WIN_PATH_EXT "exe"


void testAciVaBasename(void)
{

#if defined(ALTI_CFG_OS_WINDOWS)
    acp_char_t *sWinPath =
        TEST_WIN_PATH_DIR"\\"TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT;
    
    ACT_CHECK(acpCStrCmp(aciVaBasename(sWinPath),
                         TEST_WIN_PATH_BASE"."TEST_WIN_PATH_EXT,
                         128) == 0);
#else
    acp_char_t *sUnixPath =
        TEST_UNIX_PATH_DIR"/"TEST_UNIX_PATH_BASE"."TEST_UNIX_PATH_EXT;

    ACT_CHECK(acpCStrCmp(aciVaBasename(sUnixPath),
                         TEST_UNIX_PATH_BASE"."TEST_UNIX_PATH_EXT,
                         128) == 0);
#endif
    return;
}


typedef struct acpPrintfTestFormat
{
    const acp_char_t *mFormat;
    const acp_char_t *mOldStr;
    const acp_char_t *mNewStr;
    const acp_char_t *mResult;
    const acp_sint32_t mReturn;
} acpPrintfTestFormat;


static acpPrintfTestFormat gTestAppendTable[] =
{
    {
        " %10s",
        "test",
        "hello",
        "test      hell",
        -1
    },
    {
        " %-10s",
        "test",
        "hello",
        "test hello    ",
        -1
    },
    {
        " %010s",
        "test",
        "hello",
        "test 00000hell",
        -1
    },
    {
        " %-010s",
        "test",
        "hello",
        "test hello    ",
        -1
    },
    {
        " %3s",
        "test",
        "hello",
        "test hello",
        10
    },
    {
        " %-3s",
        "test",
        "hello",
        "test hello",
        10
    },
    {
        " %03s",
        "test",
        "hello",
        "test hello",
        10
    },
    {
        " %0-3s",
        "test",
        "hello",
        "test hello",
        10
    },
    {
        "%s",
        "test",
        "",
        "test",
        4
    },
    {
        "%s",
        "test1",
        "hello",
        "test1hello",
        10
    },
    {
        "%s",
        "test12",
        "hello",
        "test12hello",
        11
    },
    {
        "%s",
        "test123",
        "hello",
        "test123hello",
        12
    },
    {
        "%s",
        "test1234",
        "hello",
        "test1234hello",
        13
    },
    {
        "%s",
        "test12345",
        "hello",
        "test12345hello",
        14
    },
    {
        "%s",
        "test123456",
        "hello",
        "test123456hell",
        -1
    },
    {
        "%s",
        "test1234567",
        "hello",
        "test1234567hel",
        -1
    },
    {
        NULL,
        NULL,
        NULL,
        NULL,
        0
    }
};

#define TEST_APPEND_BUF_SIZE 15

void testAciVaAppendFormat(void)
{
    acp_sint32_t i;

    /* test data length is maximum TEST_APPEND_BUF_SIZE
       [TEST_APPEND_BUF_SIZE-1] -> must be NULL
       [TEST_APPEND_BUF_SIZE] -> signature for overflow
       [TEST_APPEND_BUF_SIZE+1] -> null for printing result
    */
    acp_char_t sBuffer[TEST_APPEND_BUF_SIZE + 2];
    acp_sint32_t sRet;
    acp_rc_t sRC;

    sBuffer[TEST_APPEND_BUF_SIZE] = 0xa5;
    
    for (i = 0; gTestAppendTable[i].mFormat != NULL; i++)
    {
        sRC = acpCStrCpy(sBuffer, sizeof(sBuffer),
                         gTestAppendTable[i].mOldStr,
                         acpCStrLen(gTestAppendTable[i].mOldStr, 128));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        
        sRet = aciVaAppendFormat(sBuffer,
                                TEST_APPEND_BUF_SIZE,
                                gTestAppendTable[i].mFormat,
                                gTestAppendTable[i].mNewStr);
        
        ACT_CHECK_DESC(sRet == gTestAppendTable[i].mReturn,
                       ("Test #%d failed:"
                        "Size of appending (%s) into (%s) must be %d,"
                        "but %d\n",
                        i,
                        gTestAppendTable[i].mNewStr,
                        gTestAppendTable[i].mOldStr,
                        gTestAppendTable[i].mReturn,
                        sRet));

        sBuffer[TEST_APPEND_BUF_SIZE+1] = '\0';
        ACT_CHECK_DESC(acpCStrCmp(sBuffer,
                                  gTestAppendTable[i].mResult,
                                  TEST_APPEND_BUF_SIZE) == 0,
                       ("Test #%d failed:"
                        "Result of appending (%s) into (%s) must be (%s),"
                        "but (%s)\n",
                        i,
                        gTestAppendTable[i].mNewStr,
                        gTestAppendTable[i].mOldStr,
                        gTestAppendTable[i].mResult,
                        sBuffer));
        ACT_CHECK_DESC((acp_uint8_t)sBuffer[TEST_APPEND_BUF_SIZE] == 0xa5,
                       ("Buffer-Overflow occurs in aciVaAppendFormat.\n"
                        "Last byte of buffer must be 0xA5, but (0x%X).\n",
                        (acp_uint8_t)sBuffer[TEST_APPEND_BUF_SIZE]));
    }


    return;
}


void testAciFdeof(acp_char_t *aFileName)
{
    acp_rc_t sRC;
    acp_file_t sFile;
    acp_size_t sLen;
    acp_stat_t sStat;

    sRC = acpFileOpen(&sFile, aFileName, ACP_O_RDONLY, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), OPEN_FAIL);

    ACT_CHECK(aciVaFdeof(&sFile) == ACP_FALSE);

    (void)acpFileStat(&sFile, &sStat);

    sLen = 0;
    do
    {
        acp_char_t sBuf[2];
        acp_size_t sTmpLen;
        
        sRC = acpFileRead(&sFile, sBuf, 1, &sTmpLen);
        sLen += sTmpLen;

        if ((acp_sint32_t)sLen < (acp_sint32_t)sStat.mSize)
        {
            ACT_CHECK(aciVaFdeof(&sFile) == ACP_FALSE);
        }
        else
        {
            /* This is TRUE twice when it read the last byte and also
             * read EOF */
            ACT_CHECK(aciVaFdeof(&sFile) == ACP_TRUE);
        }
        
    } while (ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK_DESC(ACP_RC_IS_EOF(sRC), ("Fail to read data file"));
    ACP_TEST_RAISE(ACP_RC_NOT_EOF(sRC), READ_FAIL);

    ACT_CHECK(aciVaFdeof(&sFile) == ACP_TRUE);
    
    (void)acpFileClose(&sFile);

    ACP_EXCEPTION(OPEN_FAIL)
    {

    }

    ACP_EXCEPTION(READ_FAIL)
    {
        (void)acpFileClose(&sFile);
    }

    ACP_EXCEPTION_END;
    return;
}

int main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    ACP_UNUSED(aArgc);

    ACT_TEST_BEGIN();
    
    testAciVaBasename();
    testAciVaAppendFormat();
    testAciFdeof(aArgv[0]);

    ACT_TEST_END();
    return 0;
}
