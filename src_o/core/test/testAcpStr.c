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
 * $Id: testAcpStr.c 5980 2009-06-11 12:28:22Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpStr.h>
#include <acpCStr.h>


#define TEST_STRING1  "Hello, World of "
#define TEST_LENGTH1  16
#define TEST_STRING2  "String"
#define TEST_LENGTH2  6
#define TEST_STRING3  "ALTIBASEaltibaseALTIBASEaltibaseALTIBASEaltibase"
#define TEST_LENGTH3  48
#define TEST_STRING4  "AltiBase"
#define TEST_LENGTH4  8
#define TEST_UPPER1   "HELLO, WORLD OF "
#define TEST_LOWER1   "hello, world of "

static struct
{
    acp_char_t   mChar;
    acp_sint32_t mFlag;
    acp_sint32_t mIndex[4];
} gTestStrChr[] =
{
    {
        ' ',
        ACP_STR_SEARCH_FORWARD | ACP_STR_CASE_INSENSITIVE,
        {
            6, 12, 15, -1
        }
    },
    {
        ' ',
        ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_SENSITIVE,
        {
            15, 12, 6, -1
        }
    },
    {
        'o',
        ACP_STR_SEARCH_FORWARD | ACP_STR_CASE_INSENSITIVE,
        {
            4, 8, 13, -1
        }
    },
    {
        'o',
        ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_SENSITIVE,
        {
            13, 8, 4, -1
        }
    },
    {
        'h',
        ACP_STR_SEARCH_FORWARD | ACP_STR_CASE_INSENSITIVE,
        {
            0, -1, -1, -1
        }
    },
    {
        'h',
        ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_SENSITIVE,
        {
            -1, -1, -1, -1
        }
    },
    {
        '\0',
        0,
        {
            -1, -1, -1, -1
        }
    }
};

static struct
{
    acp_str_t    mString;
    acp_sint32_t mFlag;
    acp_sint32_t mIndex[7];
} gTestStrStr[] =
{
    {
        ACP_STR_CONST("ALTIBASE"),
        ACP_STR_SEARCH_FORWARD | ACP_STR_CASE_INSENSITIVE,
        {
            0, 8, 16, 24, 32, 40, -1
        }
    },
    {
        ACP_STR_CONST("ALTIBASE"),
        ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_SENSITIVE,
        {
            32, 16, 0, -1, -1, -1, -1
        }
    },
    {
        ACP_STR_CONST("altibase"),
        ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_INSENSITIVE,
        {
            40, 32, 24, 16, 8, 0, -1
        }
    },
    {
        ACP_STR_CONST("altibase"),
        ACP_STR_SEARCH_FORWARD | ACP_STR_CASE_SENSITIVE,
        {
            8, 24, 40, -1, -1, -1, -1
        }
    },
    {
        ACP_STR_CONST("AltiBase"),
        ACP_STR_SEARCH_FORWARD | ACP_STR_CASE_INSENSITIVE,
        {
            0, 8, 16, 24, 32, 40, -1
        }
    },
    {
        ACP_STR_CONST("AltiBase"),
        ACP_STR_SEARCH_BACKWARD | ACP_STR_CASE_SENSITIVE,
        {
            -1, -1, -1, -1, -1, -1, -1
        }
    },
    {
        ACP_STR_CONST(""),
        0,
        {
            -1, -1, -1, -1, -1, -1, -1
        }
    }
};

static struct
{
    acp_char_t   *mString;
    acp_uint64_t  mValue;
    acp_sint32_t  mSign;
    acp_sint32_t  mEndIndex;
    acp_sint32_t  mFromIndex;
    acp_sint32_t  mBase;
    acp_rc_t      mRC;
} gTestInteger[] =
{
    /*
     * basic test
     */
    {
        "  12345678x  ",
        12345678,
        1,
        10,
        0,
        0,
        ACP_RC_SUCCESS
    },
    {
        "  -12345678x  ",
        12345678,
        -1,
        11,
        0,
        0,
        ACP_RC_SUCCESS
    },
    {
        "  +12345678x  ",
        12345678,
        1,
        11,
        0,
        0,
        ACP_RC_SUCCESS
    },
    /*
     * base test
     */
    {
        "  0x12345678  ",
        0x12345678,
        1,
        12,
        0,
        0,
        ACP_RC_SUCCESS
    },
    {
        "  -0x12345678  ",
        0x12345678,
        -1,
        13,
        0,
        16,
        ACP_RC_SUCCESS
    },
    {
        "  +0x12345678  ",
        0x12345678,
        1,
        13,
        0,
        16,
        ACP_RC_SUCCESS
    },
    {
        "  012345678  ",
        01234567,
        1,
        10,
        0,
        0,
        ACP_RC_SUCCESS
    },
    {
        "  -012345678  ",
        01234567,
        -1,
        11,
        0,
        8,
        ACP_RC_SUCCESS
    },
    {
        "  +012345678  ",
        01234567,
        1,
        11,
        0,
        8,
        ACP_RC_SUCCESS
    },
    /*
     * test error
     */
    {
        "  12345678  ",
        0,
        1,
        0,
        0,
        1,
        ACP_RC_EINVAL
    },
    {
        "  12345678  ",
        0,
        1,
        0,
        0,
        -1,
        ACP_RC_EINVAL
    },
    {
        "  12345678  ",
        0,
        1,
        0,
        0,
        37,
        ACP_RC_EINVAL
    },
    {
        "  12345678  ",
        0,
        1,
        0,
        13,
        0,
        ACP_RC_EINVAL
    },
    {
        "  12345678  ",
        12345678,
        1,
        10,
        -1,
        0,
        ACP_RC_SUCCESS
    },
    {
        "  0x12345678  ",
        0,
        1,
        3,
        0,
        10,
        ACP_RC_SUCCESS
    },
    {
        "  -  ",
        0,
        -1,
        3,
        0,
        10,
        ACP_RC_EINVAL
    },
    {
        "  +  ",
        0,
        1,
        3,
        0,
        10,
        ACP_RC_EINVAL
    },
    {
        "  a  ",
        0,
        1,
        2,
        0,
        10,
        ACP_RC_EINVAL
    },
    /*
     * test overflow of range
     */
    {
        "18446744073709551615",
        ACP_UINT64_LITERAL(18446744073709551615),
        1,
        20,
        0,
        0,
        ACP_RC_SUCCESS
    },
    {
        "18446744073709551616",
        ACP_UINT64_LITERAL(1844674407370955161),
        1,
        19,
        0,
        0,
        ACP_RC_ERANGE
    },
    {
        "18446744073709551621",
        ACP_UINT64_LITERAL(1844674407370955162),
        1,
        19,
        0,
        0,
        ACP_RC_ERANGE
    },
    {
        "28446744073709551611",
        ACP_UINT64_LITERAL(2844674407370955161),
        1,
        19,
        0,
        0,
        ACP_RC_ERANGE
    },
    /*
     * end of integer test case
     */
    {
        NULL,
        0,
        0,
        0,
        0,
        0,
        ACP_RC_SUCCESS
    }
};

#if defined(ALTI_CFG_OS_WINDOWS)
#define TEST_DIR_SEP "\\"
#else
#define TEST_DIR_SEP "/"
#endif

static struct
{
    acp_str_t         mStr;
    const acp_char_t *mPath;
    const acp_char_t *mDir;
    const acp_char_t *mLast;
    const acp_char_t *mBase;
    const acp_char_t *mExt;
} gTestPath[] =
{
    {
        ACP_STR_CONST("tmp/scratch.tiff"),
        "tmp" TEST_DIR_SEP "scratch.tiff",
        "tmp",
        "scratch.tiff",
        "tmp" TEST_DIR_SEP "scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("/scratch.tiff"),
        "" TEST_DIR_SEP "scratch.tiff",
        "",
        "scratch.tiff",
        "" TEST_DIR_SEP "scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("scratch.tiff"),
        "scratch.tiff",
        ".",
        "scratch.tiff",
        "scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("scratch.tiff"),
        "scratch.tiff",
        ".",
        "scratch.tiff",
        "scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("scratch."),
        "scratch.",
        ".",
        "scratch.",
        "scratch",
        ""
    },
    {
        ACP_STR_CONST("scratch"),
        "scratch",
        ".",
        "scratch",
        "scratch",
        ""
    },
#if defined(ALTI_CFG_OS_WINDOWS)
    {
        ACP_STR_CONST("tmp\\scratch.tiff"),
        "tmp\\scratch.tiff",
        "tmp",
        "scratch.tiff",
        "tmp\\scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("\\scratch.tiff"),
        "\\scratch.tiff",
        "",
        "scratch.tiff",
        "\\scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("scratch.tiff"),
        "scratch.tiff",
        ".",
        "scratch.tiff",
        "scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("scratch.tiff"),
        "scratch.tiff",
        ".",
        "scratch.tiff",
        "scratch",
        "tiff"
    },
    {
        ACP_STR_CONST("scratch."),
        "scratch.",
        ".",
        "scratch.",
        "scratch",
        ""
    },
    {
        ACP_STR_CONST("scratch"),
        "scratch",
        ".",
        "scratch",
        "scratch",
        ""
    },
#endif
    {
        ACP_STR_CONST(""),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};


static acp_str_t gString1 = ACP_STR_CONST(TEST_STRING1);
static acp_str_t gString2 = ACP_STR_CONST(TEST_STRING2);

static acp_str_t gString[2] =
{
    ACP_STR_CONST(TEST_STRING1),
    ACP_STR_CONST(TEST_STRING2)
};


typedef struct testStrStruct
{
    ACP_STR_DECLARE_STATIC(mStr, 100);
} testStrStruct;


static void initStrSt(testStrStruct *aSt)
{
    ACP_STR_INIT_STATIC(aSt->mStr);
}

static void testStructMember(void)
{
    testStrStruct sStrSt;

    initStrSt(&sStrSt);

    ACT_CHECK(sStrSt.mStr.mExtendSize == 0);
    ACT_CHECK(sStrSt.mStr.mSize       == -102);
    ACT_CHECK(sStrSt.mStr.mLength     == 0);
    ACT_CHECK(sStrSt.mStr.mString     == sStrSt.mStr_buffer);
}

static void testInternal(void)
{
    ACP_STR_DECLARE_STATIC(sString1, 100);
    ACP_STR_DECLARE_DYNAMIC(sString2);
    ACP_STR_DECLARE_DYNAMIC(sString3);

    ACP_STR_INIT_STATIC(sString1);
    ACP_STR_INIT_DYNAMIC(sString2, 0, 10);
    ACP_STR_INIT_DYNAMIC(sString3, 10, 20);

    /*
     * check acp_str_t
     */
    ACT_CHECK(gString1.mExtendSize             == 0);
    ACT_CHECK(gString1.mSize                   == -1);
    ACT_CHECK(gString1.mLength                 == -1);
    ACT_CHECK(acpStrGetBufferSize(&gString1)   == 0);
    ACT_CHECK(acpStrGetLength(&gString1)       == TEST_LENGTH1);
    ACT_CHECK(gString1.mLength                 == TEST_LENGTH1);

    ACT_CHECK(gString2.mExtendSize             == 0);
    ACT_CHECK(gString2.mSize                   == -1);
    ACT_CHECK(gString2.mLength                 == -1);
    ACT_CHECK(acpStrGetBufferSize(&gString2)   == 0);
    ACT_CHECK(acpStrGetLength(&gString2)       == TEST_LENGTH2);
    ACT_CHECK(gString2.mLength                 == TEST_LENGTH2);

    ACT_CHECK(gString[0].mExtendSize           == 0);
    ACT_CHECK(gString[0].mSize                 == -1);
    ACT_CHECK(gString[0].mLength               == -1);
    ACT_CHECK(acpStrGetBufferSize(&gString[0]) == 0);
    ACT_CHECK(acpStrGetLength(&gString[0])     == TEST_LENGTH1);
    ACT_CHECK(gString[0].mLength               == TEST_LENGTH1);

    ACT_CHECK(gString[1].mExtendSize           == 0);
    ACT_CHECK(gString[1].mSize                 == -1);
    ACT_CHECK(gString[1].mLength               == -1);
    ACT_CHECK(acpStrGetBufferSize(&gString[1]) == 0);
    ACT_CHECK(acpStrGetLength(&gString[1])     == TEST_LENGTH2);
    ACT_CHECK(gString[1].mLength               == TEST_LENGTH2);

    ACT_CHECK(sString1.mExtendSize             == 0);
    ACT_CHECK(sString1.mSize                   == -102);
    ACT_CHECK(sString1.mLength                 == 0);
    ACT_CHECK(acpStrGetBufferSize(&sString1)   == 101);
    ACT_CHECK(acpStrGetLength(&sString1)       == 0);

    ACT_CHECK(sString2.mExtendSize             == 10);
    ACT_CHECK(sString2.mSize                   == 0);
    ACT_CHECK(sString2.mLength                 == 0);
    ACT_CHECK(acpStrGetBufferSize(&sString2)   == 0);
    ACT_CHECK(acpStrGetLength(&sString2)       == 0);

    ACT_CHECK(sString3.mExtendSize             == 20);
    ACT_CHECK(sString3.mSize                   == 10);
    ACT_CHECK(sString3.mLength                 == 0);
    ACT_CHECK(acpStrGetBufferSize(&sString3)   == 10);
    ACT_CHECK(acpStrGetLength(&sString3)       == 0);
}

static void testPrintf(void)
{
    acp_char_t sBuffer1[1024];
    acp_char_t sBuffer2[1024];

    /*
     * print with acpPrintf
     */
    (void)acpSnprintf(sBuffer1,
                      sizeof(sBuffer1),
                      "%s%s",
                      TEST_STRING1,
                      TEST_STRING2);

    (void)acpSnprintf(sBuffer2,
                      sizeof(sBuffer2),
                      "%S%S",
                      &gString1,
                      &gString2);
    ACT_CHECK(acpCStrCmp(sBuffer1, sBuffer2, sizeof(sBuffer1)) == 0);

    (void)acpSnprintf(sBuffer2,
                      sizeof(sBuffer2),
                      "%S%S",
                      &gString[0],
                      &gString[1]);
    ACT_CHECK(acpCStrCmp(sBuffer1, sBuffer2, sizeof(sBuffer1)) == 0);
}

static void testCpy(void)
{
    acp_rc_t sRC;

    ACP_STR_DECLARE_STATIC(sString, 100);
    ACP_STR_INIT_STATIC(sString);

    /*
     * acpStrCpy
     */
    sRC = acpStrCpy(&sString, &gString1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpStrGetLength(&sString) == acpStrGetLength(&gString1));
    ACT_CHECK(acpStrCmp(&sString, &gString1, ACP_STR_CASE_SENSITIVE) == 0);
}

static void testCat(void)
{
}

static void testCmp(void)
{
}

static void testFindChar(void)
{
    acp_rc_t     sRC;
    acp_sint32_t sIndex;
    acp_sint32_t i;
    acp_sint32_t j;

    ACP_STR_DECLARE_STATIC(sString, 100);
    ACP_STR_INIT_STATIC(sString);

    /*
     * acpStrFindChar
     */
    sIndex = ACP_STR_INDEX_INITIALIZER;

    sRC = acpStrFindChar(&sString,
                         '\0',
                         &sIndex,
                         sIndex,
                         ACP_STR_SEARCH_BACKWARD |
                         ACP_STR_CASE_INSENSITIVE);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    for (i = 0; gTestStrChr[i].mChar != '\0'; i++)
    {
        sIndex = ACP_STR_INDEX_INITIALIZER;

        for (j = 0; gTestStrChr[i].mIndex[j] >= 0; j++)
        {
            sRC = acpStrFindChar(&gString1,
                                 gTestStrChr[i].mChar,
                                 &sIndex,
                                 sIndex,
                                 gTestStrChr[i].mFlag);

            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                           ("acpStrFindChar should return 0 but %d "
                            "at case %d,%d",
                            sRC,
                            i,
                            j));
            ACT_CHECK_DESC(sIndex == gTestStrChr[i].mIndex[j],
                           ("acpStrFindChar should find at index %d but %d "
                            "at case %d,%d",
                            gTestStrChr[i].mIndex[j],
                            sIndex,
                            i,
                            j));
        }

        sRC = acpStrFindChar(&gString1,
                             gTestStrChr[i].mChar,
                             &sIndex,
                             sIndex,
                             gTestStrChr[i].mFlag);
        ACT_CHECK_DESC(ACP_RC_IS_ENOENT(sRC),
                       ("acpStrFindChar should return %d but %d at case %d,%d",
                        ACP_RC_ENOENT,
                        sRC,
                        i,
                        j));
    }
}

static void testFindString(void)
{
    acp_sint32_t          sIndex;
    acp_rc_t              sRC;
    acp_sint32_t          i;
    acp_sint32_t          j;

    ACP_STR_DECLARE_STATIC(sString1, 100);
    ACP_STR_DECLARE_DYNAMIC(sString2);

    ACP_STR_INIT_STATIC(sString1);
    ACP_STR_INIT_DYNAMIC(sString2, 0, 10);

    sRC = acpStrCpy(&sString1, &gString1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpStrFindString
     */
    sIndex = ACP_STR_INDEX_INITIALIZER;

    sRC = acpStrFindString(&sString1,
                           &sString2,
                           &sIndex,
                           sIndex,
                           ACP_STR_SEARCH_BACKWARD |
                           ACP_STR_CASE_INSENSITIVE);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sIndex = ACP_STR_INDEX_INITIALIZER;

    sRC = acpStrFindString(&sString2,
                           &sString1,
                           &sIndex,
                           sIndex,
                           ACP_STR_SEARCH_BACKWARD |
                           ACP_STR_CASE_INSENSITIVE);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpStrCpyCString(&sString1, TEST_STRING3);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; acpStrGetLength(&gTestStrStr[i].mString) > 0; i++)
    {
        sIndex = ACP_STR_INDEX_INITIALIZER;

        for (j = 0; gTestStrStr[i].mIndex[j] >= 0; j++)
        {
            sRC = acpStrFindString(&sString1,
                                   &gTestStrStr[i].mString,
                                   &sIndex,
                                   sIndex,
                                   gTestStrStr[i].mFlag);

            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                           ("acpStrFindString should return 0 but %d "
                            "at case %d,%d",
                            sRC,
                            i,
                            j));
            ACT_CHECK_DESC(sIndex == gTestStrStr[i].mIndex[j],
                           ("acpStrFindString should find at index %d but %d "
                            "at case %d,%d",
                            gTestStrStr[i].mIndex[j],
                            sIndex,
                            i,
                            j));
        }

        sRC = acpStrFindString(&sString1,
                               &gTestStrStr[i].mString,
                               &sIndex,
                               sIndex,
                               gTestStrStr[i].mFlag);
        ACT_CHECK_DESC(ACP_RC_IS_ENOENT(sRC),
                       ("acpStrFindString should return %d but %d "
                        "at case %d,%d",
                        ACP_RC_ENOENT,
                        sRC,
                        i,
                        j));
    }
}

static void testReplaceChar(void)
{
    acp_rc_t sRC;

    ACP_STR_DECLARE_STATIC(sString, 100);
    ACP_STR_INIT_STATIC(sString);

    sRC = acpStrCpyCString(&sString, TEST_STRING3);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStrReplaceChar(&sString, 'Y', 'y', 0, -1);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpStrReplaceChar(&sString, 'A', 'O', 0, -1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(
        acpStrCmpCString(&sString,
                         "OLTIBOSEaltibaseOLTIBOSEaltibaseOLTIBOSEaltibase",
                         0) == 0);

    sRC = acpStrReplaceChar(&sString, 'L', 'l', 1, 32);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(
        acpStrCmpCString(&sString,
                         "OlTIBOSEaltibaseOlTIBOSEaltibaseOLTIBOSEaltibase",
                         0) == 0);

    sRC = acpStrReplaceChar(&sString, 'T', 't', 2, 33);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(
        acpStrCmpCString(&sString,
                         "OltIBOSEaltibaseOltIBOSEaltibaseOLtIBOSEaltibase",
                         0) == 0);
}

static void testToInteger(void)
{
    acp_uint64_t sValue;
    acp_sint32_t sSign;
    acp_sint32_t sEndIndex;
    acp_sint32_t i;
    acp_rc_t     sRC;

    ACP_STR_DECLARE_STATIC(sString, 100);
    ACP_STR_INIT_STATIC(sString);

    /*
     * acpStrToInteger
     */
    for (i = 0; gTestInteger[i].mString != NULL; i++)
    {
        sRC = acpStrCpyCString(&sString, gTestInteger[i].mString);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpStrToInteger(&sString,
                              &sSign,
                              &sValue,
                              &sEndIndex,
                              gTestInteger[i].mFromIndex,
                              gTestInteger[i].mBase);

        ACT_CHECK_DESC(sRC == gTestInteger[i].mRC,
                       ("result code should be %d but %d at case %d",
                        gTestInteger[i].mRC,
                        sRC,
                        i));
        ACT_CHECK_DESC(sEndIndex == gTestInteger[i].mEndIndex,
                       ("end index should be %d but %d at case %d",
                        gTestInteger[i].mEndIndex,
                        sEndIndex,
                        i));
        ACT_CHECK_DESC(sValue == gTestInteger[i].mValue,
                       ("value should be %llu but %llu at case %d",
                        gTestInteger[i].mValue,
                        sValue,
                        i));
        ACT_CHECK_DESC(sSign == gTestInteger[i].mSign,
                       ("sign should be %d but %d at case %d",
                        gTestInteger[i].mSign,
                        sSign,
                        i));
    }
}

static void testCase(void)
{
    acp_rc_t sRC;

    ACP_STR_DECLARE_STATIC(sString, 100);
    ACP_STR_INIT_STATIC(sString);

    /*
     * acpStrUpper
     */
    sRC = acpStrCpyCString(&sString, TEST_STRING1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    (void)acpStrUpper(&sString);
    ACT_CHECK(acpCStrCmp(acpStrGetBuffer(&sString), TEST_UPPER1, 100) == 0);

    /*
     * acpStrLower
     */
    sRC = acpStrCpyCString(&sString, TEST_STRING1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    (void)acpStrLower(&sString);
    ACT_CHECK(acpCStrCmp(acpStrGetBuffer(&sString), TEST_LOWER1, 100) == 0);
}

static void testPath(void)
{
    acp_path_pool_t sPool;
    acp_sint32_t    i;
    acp_char_t*     sPath;

    acpPathPoolInit(&sPool);

    for (i = 0; gTestPath[i].mPath != NULL; i++)
    {
        sPath = acpPathFull(acpStrGetBuffer(&gTestPath[i].mStr), &sPool);
        ACT_CHECK(NULL != sPath);
        ACT_CHECK_DESC(acpCStrCmp(sPath,
                           gTestPath[i].mPath,
                           acpStrGetBufferSize(&gTestPath[i].mStr)) == 0,
                       ("acpPathFull(\"%S\") should be \"%s\" but \"%s\"",
                        &gTestPath[i].mStr,
                        gTestPath[i].mPath, sPath));

        sPath = acpPathDir(acpStrGetBuffer(&gTestPath[i].mStr), &sPool);
        ACT_CHECK(NULL != sPath);
        ACT_CHECK_DESC(acpCStrCmp(sPath,
                                 gTestPath[i].mDir,
                                 acpStrGetBufferSize(&gTestPath[i].mStr)) == 0,
                       ("acpPathDir(\"%S\") should be \"%s\" but \"%s\"",
                        &gTestPath[i].mStr,
                        gTestPath[i].mDir,
                        sPath));

        sPath = acpPathLast(acpStrGetBuffer(&gTestPath[i].mStr), &sPool);
        ACT_CHECK(NULL != sPath);
        ACT_CHECK_DESC(acpCStrCmp(sPath,
                                 gTestPath[i].mLast,
                                 acpStrGetBufferSize(&gTestPath[i].mStr)) == 0,
                       ("acpPathLast(\"%S\") should be \"%s\" but \"%s\"",
                        &gTestPath[i].mStr,
                        gTestPath[i].mLast,
                        sPath));

        sPath = acpPathBase(acpStrGetBuffer(&gTestPath[i].mStr), &sPool);
        ACT_CHECK(NULL != sPath);
        ACT_CHECK_DESC(acpCStrCmp(sPath,
                                 gTestPath[i].mBase,
                                 acpStrGetBufferSize(&gTestPath[i].mStr)) == 0,
                       ("acpPathBase(\"%S\") should be \"%s\" but \"%s\"",
                        &gTestPath[i].mStr,
                        gTestPath[i].mBase,
                        sPath));

        sPath = acpPathExt(acpStrGetBuffer(&gTestPath[i].mStr), &sPool);
        ACT_CHECK(NULL != sPath);
        ACT_CHECK_DESC(acpCStrCmp(sPath,
                                   gTestPath[i].mExt,
                                   acpStrGetBufferSize(&gTestPath[i].mStr))== 0,
                       ("acpPathExt(\"%S\") should be \"%s\" but \"%s\"",
                        &gTestPath[i].mStr,
                        gTestPath[i].mExt,
                        sPath));
    }

    acpPathPoolFinal(&sPool);
}

static void testNullTermination(void)
{
    acp_str_t  sAppName0 = ACP_STR_CONST("12345678901");
    acp_str_t  sAppName1 = ACP_STR_CONST("1234567890");
    acp_str_t  sAppName2 = ACP_STR_CONST("123456789");
    acp_str_t  sLineBuffer11;
    acp_str_t  sLineBuffer12;
    acp_str_t  sLineBuffer13;
    
    
    ACP_STR_DECLARE_STATIC(sLineBuffer0, 9);
    ACP_STR_DECLARE_STATIC(sLineBuffer1, 9);
    ACP_STR_DECLARE_STATIC(sLineBuffer2, 9);
    
    ACP_STR_INIT_STATIC(sLineBuffer0);
    ACP_STR_INIT_STATIC(sLineBuffer1);
    ACP_STR_INIT_STATIC(sLineBuffer2);
     
    (void)acpStrCatFormat(&sLineBuffer0, "%S", &sAppName0);
    (void)acpStrCatFormat(&sLineBuffer1, "%S", &sAppName1);
    (void)acpStrCatFormat(&sLineBuffer2, "%S", &sAppName2);

    ACT_CHECK(acpCStrCmp(
                  acpStrGetBuffer(&sLineBuffer0), "123456789", 100) == 0);
    ACT_CHECK(acpCStrCmp(
                  acpStrGetBuffer(&sLineBuffer1), "123456789", 100) == 0);
    ACT_CHECK(acpCStrCmp(
                  acpStrGetBuffer(&sLineBuffer2), "123456789", 100) == 0);
    
    /* ========== */
    
    ACP_STR_INIT_DYNAMIC(sLineBuffer11, 9, 3);
    ACP_STR_INIT_DYNAMIC(sLineBuffer12, 9, 3);
    ACP_STR_INIT_DYNAMIC(sLineBuffer13, 9, 3);

    (void)acpStrCatFormat(&sLineBuffer11, "%S", &sAppName0);
    (void)acpStrCatFormat(&sLineBuffer12, "%S", &sAppName1);
    (void)acpStrCatFormat(&sLineBuffer13, "%S", &sAppName2);

    ACT_CHECK(acpCStrCmp(
                  acpStrGetBuffer(&sLineBuffer11), "12345678901", 100) == 0);
    ACT_CHECK(acpCStrCmp(
                  acpStrGetBuffer(&sLineBuffer12), "1234567890", 100) == 0);
    ACT_CHECK(acpCStrCmp(
                  acpStrGetBuffer(&sLineBuffer13), "123456789", 100) == 0);
    
}

static void testConst(void)
{
    acp_str_t sConst = ACP_STR_CONST("Test String");

    ACT_CHECK(ACP_RC_IS_EACCES(acpStrUpper(&sConst)));
    ACT_CHECK(ACP_RC_IS_EACCES(acpStrLower(&sConst)));
    ACT_CHECK(ACP_RC_IS_EACCES(acpStrCatBuffer(&sConst, "LOOM", 5)));
    ACT_CHECK(ACP_RC_IS_EACCES(acpStrCpyBuffer(&sConst, "LOOM", 5)));
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testInternal();
    testStructMember();
    testPrintf();
    testCpy();
    testCat();
    testCmp();
    testFindChar();
    testFindString();
    testReplaceChar();
    testToInteger();
    testCase();
    testPath();
    testNullTermination();
    testConst();
    

    ACT_TEST_END();

    return 0;
}
