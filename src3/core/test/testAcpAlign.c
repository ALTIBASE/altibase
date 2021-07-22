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
 * $Id: testAcpAlign.c 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#include <act.h>
#include <acpAlign.h>


#if defined(ACP_CFG_COMPILE_64BIT)
#define ACP_ALIGN_TEST_DEFAULT 8
#else
#define ACP_ALIGN_TEST_DEFAULT 4
#endif

typedef struct acpAlignTestStruct
{
    acp_sint8_t  mInt8;
    acp_sint16_t mInt16;
    acp_sint32_t mInt32;
    acp_sint64_t mInt64;
} acpAlignTestStruct;


static void testInt8(void)
{
    acp_sint8_t  sInt8;
    acp_sint8_t  sInt8Aligned;
    acp_sint32_t sAlign;

    for (sInt8 = 0; sInt8 < ACP_SINT8_MAX - 20; sInt8++)
    {
        sInt8Aligned = ACP_ALIGN(sInt8);
        ACT_CHECK(sInt8Aligned >= sInt8);
        ACT_CHECK((sInt8Aligned % ACP_ALIGN_TEST_DEFAULT) == 0);
        ACT_CHECK((sInt8Aligned - ACP_ALIGN_TEST_DEFAULT) < sInt8);

        sInt8Aligned = ACP_ALIGN4(sInt8);
        ACT_CHECK(sInt8Aligned >= sInt8);
        ACT_CHECK((sInt8Aligned % 4) == 0);
        ACT_CHECK((sInt8Aligned - 4) < sInt8);

        sInt8Aligned = ACP_ALIGN8(sInt8);
        ACT_CHECK(sInt8Aligned >= sInt8);
        ACT_CHECK((sInt8Aligned % 8) == 0);
        ACT_CHECK((sInt8Aligned - 8) < sInt8);

        for (sAlign = 1; sAlign < 20; sAlign++)
        {
            sInt8Aligned = ACP_ALIGN_ALL(sInt8, sAlign);

            ACT_CHECK(sInt8Aligned >= sInt8);
            ACT_CHECK((sInt8Aligned % sAlign) == 0);
            ACT_CHECK((sInt8Aligned - sAlign) < sInt8);
        }
    }
}

static void testInt16(void)
{
    acp_sint16_t sInt16;
    acp_sint16_t sInt16Aligned;
    acp_sint32_t sAlign;

    for (sInt16 = ACP_SINT16_MAX - 200; sInt16 < ACP_SINT16_MAX - 100; sInt16++)
    {
        sInt16Aligned = (acp_sint16_t)ACP_ALIGN(sInt16);
        ACT_CHECK(sInt16Aligned >= sInt16);
        ACT_CHECK((sInt16Aligned % ACP_ALIGN_TEST_DEFAULT) == 0);
        ACT_CHECK((sInt16Aligned - ACP_ALIGN_TEST_DEFAULT) < sInt16);

        sInt16Aligned = (acp_sint16_t)ACP_ALIGN4(sInt16);
        ACT_CHECK(sInt16Aligned >= sInt16);
        ACT_CHECK((sInt16Aligned % 4) == 0);
        ACT_CHECK((sInt16Aligned - 4) < sInt16);

        sInt16Aligned = (acp_sint16_t)ACP_ALIGN8(sInt16);
        ACT_CHECK(sInt16Aligned >= sInt16);
        ACT_CHECK((sInt16Aligned % 8) == 0);
        ACT_CHECK((sInt16Aligned - 8) < sInt16);

        for (sAlign = 1; sAlign < 100; sAlign++)
        {
            sInt16Aligned = (acp_sint16_t)ACP_ALIGN_ALL(sInt16, sAlign);

            ACT_CHECK(sInt16Aligned >= sInt16);
            ACT_CHECK((sInt16Aligned % sAlign) == 0);
            ACT_CHECK((sInt16Aligned - sAlign) < sInt16);
        }
    }
}

static void testInt32(void)
{
    acp_sint32_t sInt32;
    acp_sint32_t sInt32Aligned;
    acp_sint32_t sAlign;

    for (sInt32 = ACP_SINT32_MAX - 200; sInt32 < ACP_SINT32_MAX - 100; sInt32++)
    {
        sInt32Aligned = ACP_ALIGN(sInt32);
        ACT_CHECK(sInt32Aligned >= sInt32);
        ACT_CHECK((sInt32Aligned % ACP_ALIGN_TEST_DEFAULT) == 0);
        ACT_CHECK((sInt32Aligned - ACP_ALIGN_TEST_DEFAULT) < sInt32);

        sInt32Aligned = ACP_ALIGN4(sInt32);
        ACT_CHECK(sInt32Aligned >= sInt32);
        ACT_CHECK((sInt32Aligned % 4) == 0);
        ACT_CHECK((sInt32Aligned - 4) < sInt32);

        sInt32Aligned = ACP_ALIGN8(sInt32);
        ACT_CHECK(sInt32Aligned >= sInt32);
        ACT_CHECK((sInt32Aligned % 8) == 0);
        ACT_CHECK((sInt32Aligned - 8) < sInt32);

        for (sAlign = 1; sAlign < 100; sAlign++)
        {
            sInt32Aligned = ACP_ALIGN_ALL(sInt32, sAlign);

            ACT_CHECK(sInt32Aligned >= sInt32);
            ACT_CHECK((sInt32Aligned % sAlign) == 0);
            ACT_CHECK((sInt32Aligned - sAlign) < sInt32);
        }
    }
}

static void testInt64(void)
{
    acp_sint64_t sInt64;
    acp_sint64_t sInt64Aligned;
    acp_sint32_t sAlign;

    for (sInt64 = ACP_SINT64_MAX - 200; sInt64 < ACP_SINT64_MAX - 100; sInt64++)
    {
        sInt64Aligned = ACP_ALIGN(sInt64);
        ACT_CHECK(sInt64Aligned >= sInt64);
        ACT_CHECK((sInt64Aligned % ACP_ALIGN_TEST_DEFAULT) == 0);
        ACT_CHECK((sInt64Aligned - ACP_ALIGN_TEST_DEFAULT) < sInt64);

        sInt64Aligned = ACP_ALIGN4(sInt64);
        ACT_CHECK(sInt64Aligned >= sInt64);
        ACT_CHECK((sInt64Aligned % 4) == 0);
        ACT_CHECK((sInt64Aligned - 4) < sInt64);

        sInt64Aligned = ACP_ALIGN8(sInt64);
        ACT_CHECK(sInt64Aligned >= sInt64);
        ACT_CHECK((sInt64Aligned % 8) == 0);
        ACT_CHECK((sInt64Aligned - 8) < sInt64);

        for (sAlign = 1; sAlign < 100; sAlign++)
        {
            sInt64Aligned = ACP_ALIGN_ALL(sInt64, sAlign);

            ACT_CHECK(sInt64Aligned >= sInt64);
            ACT_CHECK((sInt64Aligned % sAlign) == 0);
            ACT_CHECK((sInt64Aligned - sAlign) < sInt64);
        }
    }
}

static void testPtr(void)
{
    acpAlignTestStruct *sPtr;
    acpAlignTestStruct *sPtrAligned;
    acp_sint32_t        sAlign;

    for (sPtr = 0; sPtr < (acpAlignTestStruct *)100; sPtr++)
    {
        sPtrAligned = ACP_ALIGN_PTR(sPtr);
        ACT_CHECK(sPtrAligned >= sPtr);
        ACT_CHECK(((acp_ulong_t)sPtrAligned % ACP_ALIGN_TEST_DEFAULT) == 0);
        if (sPtrAligned != NULL)
        {
            ACT_CHECK(((acp_ulong_t)sPtrAligned - ACP_ALIGN_TEST_DEFAULT) <
                      (acp_ulong_t)sPtr);
        }
        else
        {
        }

        sPtrAligned = ACP_ALIGN4_PTR(sPtr);
        ACT_CHECK(sPtrAligned >= sPtr);
        ACT_CHECK(((acp_ulong_t)sPtrAligned % 4) == 0);
        if (sPtrAligned != NULL)
        {
            ACT_CHECK(((acp_ulong_t)sPtrAligned - 4) < (acp_ulong_t)sPtr);
        }
        else
        {
        }

        sPtrAligned = ACP_ALIGN8_PTR(sPtr);
        ACT_CHECK(sPtrAligned >= sPtr);
        ACT_CHECK(((acp_ulong_t)sPtrAligned % 8) == 0);
        if (sPtrAligned != NULL)
        {
            ACT_CHECK(((acp_ulong_t)sPtrAligned - 8) < (acp_ulong_t)sPtr);
        }
        else
        {
        }

        for (sAlign = 1; sAlign < 100; sAlign++)
        {
            sPtrAligned = ACP_ALIGN_ALL_PTR(sPtr, sAlign);

            ACT_CHECK(sPtrAligned >= sPtr);
            ACT_CHECK(((acp_ulong_t)sPtrAligned % sAlign) == 0);
            if (sPtrAligned != NULL)
            {
                ACT_CHECK(((acp_ulong_t)sPtrAligned - sAlign) <
                          (acp_ulong_t)sPtr);
            }
            else
            {
            }
        }
    }
}


acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testInt8();
    testInt16();
    testInt32();
    testInt64();
    testPtr();

    ACT_TEST_END();

    return 0;
}
