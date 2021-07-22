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
 * $Id: testAclCodeUTF8.c 4486 2009-02-10 07:37:20Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpStr.h>
#include <aclCode.h>

typedef struct acpUTF8ToUint32Test
{
    acp_str_t       mString;
    acp_uint8_t     mUTF8[7];
    acp_uint32_t    mValue;
    acp_uint32_t    mLength;
    acp_rc_t        mResult;
} acpUTF8ToUint32Test;

/* Successful Testcases */
acpUTF8ToUint32Test gTestValues[12] = 
{
    {
        ACP_STR_CONST("23"),
        "\x23\x00\x00\x00\x00\x00",
        35, 1, 0
    },
    {
        ACP_STR_CONST("7F"),
        "\x7F\x00\x00\x00\x00\x00",
        127, 1, 0
    },
    {
        ACP_STR_CONST("C3BF"),
        "\xC3\xBF\x00\x00\x00\x00",
        255, 2, 0
    },
    {
        ACP_STR_CONST("DFBF"),
        "\xDF\xBF\x00\x00\x00\x00",
        2047, 2, 0
    },
    {
        ACP_STR_CONST("E289A0"),
        "\xE2\x89\xA0\x00\x00\x00",
        8800, 3, 0
    },
    {
        ACP_STR_CONST("EC9C84"),
        "\xEC\x9C\x84\x00\x00\x00",
        50948, 3, 0
    },
    {
        ACP_STR_CONST("F380AA83"),
        "\xF3\x80\xAA\x83\x00\x00",
        789123, 4, 0
    },
    {
        ACP_STR_CONST("F3B6A8BB"),
        "\xF3\xB6\xA8\xBB\x00\x00",
        1010235, 4, 0
    },
    {
        ACP_STR_CONST("F9909AAFA3"),
        "\xF9\x90\x9A\xAF\xA3\x00",
        21081059, 5, 0
    },
    {
        ACP_STR_CONST("F99B8F9BA3"),
        "\xF9\x9B\x8F\x9B\xA3\x00",
        23918307, 5, 0
    },
    {
        ACP_STR_CONST("FC85A0B78EBD"),
        "\xFC\x85\xA0\xB7\x8E\xBD",
        92500925, 6, 0
    },
    {ACP_STR_CONST("FD868A9F9D88"),
        "\xFD\x86\x8A\x9F\x9D\x88",
        1177155400, 6, 0
    }
};

/* Errornous Testcases */
acpUTF8ToUint32Test gErrorValues[12] = 
{
    {
        ACP_STR_CONST("E289A0"),
        "\xE2\x89\xA0\x00\x00\x00",
        8800,
        1,
        ACP_RC_ETRUNC
    },
    {
        ACP_STR_CONST("FC85A0B78EBD"),
        "\xFC\x85\xA0\xB7\x8E\xBD",
        92500925,
        2,
        ACP_RC_ETRUNC
    },
    {
        ACP_STR_CONST("FC85A0B78EBD"),
        "\xFC\x85\xA0\xB7\x8E\xBD",
        92500925,
        3,
        ACP_RC_ETRUNC
    },
    {
        ACP_STR_CONST("EFBFBE"),
        "\xEF\xBF\xBE\x00\x00\x00",
        0xFFFE,
        5,
        ACP_RC_ENOTSUP
    },
    {
        ACP_STR_CONST("EFBFBF"),
        "\xEF\xBF\xBF\x00\x00\x00",
        0xFFFF,
        5,
        ACP_RC_ENOTSUP
    },
    {
        ACP_STR_CONST("EDA1B6"),
        "\xED\xA1\xB6\x00\x00\x00",
        0xD876,
        5,
        ACP_RC_ENOTSUP
    },
    {
        ACP_STR_CONST("DFDF"),
        "\xDF\xDF\x00\x00\x00\x00",
        0xD876,
        5,
        ACP_RC_ENOTSUP
    },
    {
        ACP_STR_CONST("E0808A"),
        "\xE0\x80\x8A\x00\x00\x00",
        0x0A,
        5,
        ACP_RC_ENOTSUP
    },
};

void testUTF8ToUint32(void)
{
    acp_sint32_t i;
    acp_uint32_t sUint32;
    acp_uint32_t sLength;

    for(i=0; i<12; i++)
    {
        ACT_CHECK(
            ACP_RC_IS_SUCCESS(
                aclCodeUTF8ToUint32(gTestValues[i].mUTF8,
                                    11,
                                    &sUint32,
                                    &sLength)
                )
            );
        ACT_CHECK_DESC(sLength == gTestValues[i].mLength,
                       ("acpUTF8ToUint32 Testcase(%d) (%S) length"
                        " should be %u, but %d appeared", 
                        i,
                        &(gTestValues[i].mString),
                        gTestValues[i].mLength,
                        sLength)
                       );
        ACT_CHECK_DESC(sUint32 == gTestValues[i].mValue,
                       ("acpUTF8ToUint32 Testcase(%d) (%S)"
                        " should be %u(%08X), but %u(%08X) appeared", 
                        i,
                        &(gTestValues[i].mString),
                        gTestValues[i].mValue,
                        gTestValues[i].mValue,
                        sUint32,
                        sUint32)
                       );
    }
}

void testErrornousUTF8ToUint32(void)
{
    acp_sint32_t i;
    acp_uint32_t sUint32;
    acp_uint32_t sLength;
    acp_rc_t    sResult;

    for(i=0; i<8; i++)
    {
        sResult = aclCodeUTF8ToUint32(gErrorValues[i].mUTF8,
                                      gErrorValues[i].mLength,
                                      &sUint32,
                                      &sLength
                                      );
        ACT_CHECK_DESC(sResult == gErrorValues[i].mResult,
                       ("acpUTF8ToUint32 Errorcase(%d) (%S) should"
                        " return %d, but %X and %d appeared", 
                        i,
                        &(gErrorValues[i].mString),
                        gErrorValues[i].mResult,
                        sUint32,
                        sResult
                        )
                       );
    }
}

void testUint32ToUTF8(void)
{
    acp_sint32_t i;
    acp_sint32_t j;
    acp_uint32_t sLength;
    acp_uint8_t sUTF8[11];
    acp_char_t sUTF8String[21];
    acp_char_t* sIndex;

    for(i=0; i<12; i++)
    {
        for(j=0; j<11; j++)
        {
            sUTF8[j] = 0;
        }
        for(j=0; j<21; j++)
        {
            sUTF8String[j] = 0;
        }

        ACT_CHECK(
            ACP_RC_IS_SUCCESS(
                aclCodeUint32ToUTF8(sUTF8, 11, gTestValues[i].mValue, &sLength)
                )
            );

        for(j=0, sIndex=sUTF8String; j<10 && sUTF8[j]!=0; j++, sIndex+=2)
        {
            (void)acpSnprintf(sIndex, 3, "%02X", sUTF8[j]);
        }

        ACT_CHECK_DESC(0 == acpStrCmpCString(&(gTestValues[i].mString),
                                             sUTF8String,
                                             ACP_STR_CASE_SENSITIVE),
                       ("acpUint32ToUTF8 Testcase(%d) (%d) length"
                        " should be %d, but %d appeared", 
                        i,
                        gTestValues[i].mValue,
                        gTestValues[i].mLength,
                        sLength)
                       );

        ACT_CHECK_DESC(sLength == gTestValues[i].mLength,
                       ("acpUint32ToUTF8 Testcase(%d) (%d[%08X])"
                        " should be %S, but (%s) appeared", 
                        i,
                        gTestValues[i].mValue,
                        gTestValues[i].mValue,
                        &(gTestValues[i].mString),
                        sUTF8String)
                       );
    }
}

void testErrornousUint32ToUTF8(void)
{
    acp_sint32_t i;
    acp_uint32_t sLength;
    acp_rc_t     sResult;

    for(i=0; i<6; i++)
    {
        sResult = aclCodeUint32ToUTF8(gErrorValues[i].mUTF8,
                                      gErrorValues[i].mLength,
                                      gErrorValues[i].mValue,
                                      &sLength
                                      );
        ACT_CHECK_DESC(sResult == gErrorValues[i].mResult,
                       ("acpUint32ToUTF8 Errorcase(%d) (%X)"
                        " should return %d, but %d appeared", 
                        i,
                        gErrorValues[i].mValue,
                        gErrorValues[i].mResult,
                        sResult
                        )
                       );
    }
}


acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    /* Test Successful Conditions */
    testUTF8ToUint32();
    testUint32ToUTF8();

    /* Test Errornous Conditions */
    testErrornousUTF8ToUint32();
    testErrornousUint32ToUTF8();

    ACT_TEST_END();

    return 0;
}
