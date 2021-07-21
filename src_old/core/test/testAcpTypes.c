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
 * $Id: testAcpTypes.c 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpTypes.h>
#include <acpVersion.h>
#include <acpTest.h>


typedef enum testAcpDoubleImpl
{
    UNKNOWN = 0,
    IEEE_BIG,
    IEEE_LITTLE,
    IBM,
    VAX,
    CRAY
} testAcpDoubleImpl;


static testAcpDoubleImpl testAcpDoubleCheckWithInt(void)
{
    union
    {
        acp_double_t mDouble;
        acp_sint32_t mInt[2];
    } sUnion;

    sUnion.mInt[0] = 0;
    sUnion.mInt[1] = 0;
    sUnion.mDouble = 1e13;

    if ((sUnion.mInt[0] == 1117925532) && (sUnion.mInt[1] == -448790528))
    {
        return IEEE_BIG;
    }
    else if ((sUnion.mInt[1] == 1117925532) && (sUnion.mInt[0] == -448790528))
    {
        return IEEE_LITTLE;
    }
    else if ((sUnion.mInt[0] == -2065213935) && (sUnion.mInt[1] == 10752))
    {
        return VAX;
    }
    else if ((sUnion.mInt[0] == 1267827943) && (sUnion.mInt[1] == 704643072))
    {
        return IBM;
    }
    else
    {
        return UNKNOWN;
    }
}

static acp_bool_t testAcpDoubleCheckSuddenUnderflow(void)
{
#if !defined(ALINT)
    acp_double_t sDouble1;
    acp_double_t sDouble2;
    acp_sint32_t i;

    sDouble1 = 1.;
    sDouble2 = .1;

    for(i = 155;; sDouble2 *= sDouble2, i >>= 1)
    {
        if ((i & 1) != 0)
        {
            sDouble1 *= sDouble2;

            if (i == 1)
            {
                break;
            }
            else
            {
                /* continue */
            }
        }
        else
        {
            /* continue */
        }
    }

    sDouble2 = sDouble1 * sDouble1;

    if (sDouble2 == 0.)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
#else
    return ACP_FALSE;
#endif
}

static void testAcpDoubleCheck(void)
{
    testAcpDoubleImpl sDoubleImpl;

    if (sizeof(double) == 2 * sizeof(acp_sint32_t))
    {
        sDoubleImpl = testAcpDoubleCheckWithInt();

#if defined(ACP_CFG_BIG_ENDIAN)
        ACT_CHECK_DESC(sDoubleImpl == IEEE_BIG,
                       ("double implementation is not IEEE-754 format"));
#elif defined(ACP_CFG_LITTLE_ENDIAN)
        ACT_CHECK_DESC(sDoubleImpl == IEEE_LITTLE,
                       ("double implementation is not IEEE-754 format"));
#else
#error unknown byte order
#endif

#if defined(ALTI_CFG_CPU_ALPHA)
        /*
         * when double has sudden underflow,
         * acpCStrDouble.c should define Sudden_Underflow
         */
        ACT_CHECK_DESC(testAcpDoubleCheckSuddenUnderflow() == ACP_TRUE,
                       ("double implementation has sudden underflow"));
#else
        ACT_CHECK_DESC(testAcpDoubleCheckSuddenUnderflow() == ACP_FALSE,
                       ("double implementation has sudden underflow"));
#endif
    }
    else
    {
        ACT_ERROR(("unknwon double size %zd", sizeof(double)));
    }
}


acp_sint32_t main(void)
{
    acp_sint8_t  sSInt8;
    acp_uint8_t  sUInt8;
    acp_sint16_t sSInt16;
    acp_uint16_t sUInt16;
    acp_sint32_t sSInt32;
    acp_uint32_t sUInt32;
    acp_sint64_t sSInt64;
    acp_uint64_t sUInt64;
    acp_slong_t  sSLong;
    acp_ulong_t  sULong;
    
    acp_uint32_t sVersion;

    ACT_TEST_BEGIN();

    sVersion = acpVersionMajor();
    sVersion = acpVersionMinor();
    sVersion = acpVersionPatch();

    /*
     * Test Size of Types
     */
    ACT_CHECK(sizeof(acp_char_t) == 1);
    ACT_CHECK(sizeof(acp_sint8_t) == 1);
    ACT_CHECK(sizeof(acp_uint8_t) == 1);
    ACT_CHECK(sizeof(acp_sint16_t) == 2);
    ACT_CHECK(sizeof(acp_uint16_t) == 2);
    ACT_CHECK(sizeof(acp_sint32_t) == 4);
    ACT_CHECK(sizeof(acp_uint32_t) == 4);
    ACT_CHECK(sizeof(acp_sint64_t) == 8);
    ACT_CHECK(sizeof(acp_uint64_t) == 8);
    ACT_CHECK(sizeof(acp_ulong_t) == (ACP_CFG_COMPILE_BIT/8));
    ACT_CHECK(sizeof(acp_slong_t) == (ACP_CFG_COMPILE_BIT/8));
    ACT_CHECK(sizeof(acp_ulong_t) == (ACP_CFG_COMPILE_BIT/8));
    ACT_CHECK(sizeof(acp_float_t) == sizeof(float));
    ACT_CHECK(sizeof(acp_double_t) == sizeof(double));
    ACT_CHECK(sizeof(acp_offset_t) == sizeof(acp_sint64_t));
    ACT_CHECK(sizeof(acp_size_t) == sizeof(size_t));
#if !defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(sizeof(acp_ssize_t) == sizeof(ssize_t));
#endif
    ACT_CHECK(sizeof(acp_size_t) == sizeof(void *));

#if defined(ACP_CFG_COMPILE_64BIT)
    ACT_CHECK(sizeof(acp_size_t) == sizeof(acp_uint64_t));
    ACT_CHECK(sizeof(acp_ssize_t) == sizeof(acp_sint64_t));
#elif defined(ACP_CFG_COMPILE_32BIT)
    ACT_CHECK(sizeof(acp_size_t) == sizeof(acp_uint32_t));
    ACT_CHECK(sizeof(acp_ssize_t) == sizeof(acp_sint32_t));
#else
#error Unknown compile bit. core library may be unsafe.
#endif

    /*
     * Test Range of Types
     */
    sSInt8 = ACP_SINT8_MIN;
    ACT_CHECK_DESC(--sSInt8 > ACP_SINT8_MIN,
                   ("ACP_SINT8_MIN is not min of acp_sint8_t"));
    sSInt8 = ACP_SINT8_MAX;
    ACT_CHECK_DESC(++sSInt8 < ACP_SINT8_MAX,
                   ("ACP_SINT8_MAX is not max of acp_sint8_t"));
    sUInt8 = ACP_UINT8_MIN;
    ACT_CHECK_DESC(--sUInt8 > ACP_UINT8_MIN,
                   ("ACP_UINT8_MIN is not min of acp_uint8_t"));
    sUInt8 = ACP_UINT8_MAX;
    ACT_CHECK_DESC(++sUInt8 < ACP_UINT8_MAX,
                   ("ACP_UINT8_MAX is not max of acp_uint8_t"));

    sSInt16 = ACP_SINT16_MIN;
    ACT_CHECK_DESC(--sSInt16 > ACP_SINT16_MIN,
                   ("ACP_SINT16_MIN is not min of acp_sint16_t"));
    sSInt16 = ACP_SINT16_MAX;
    ACT_CHECK_DESC(++sSInt16 < ACP_SINT16_MAX,
                   ("ACP_SINT16_MAX is not max of acp_sint16_t"));
    sUInt16 = ACP_UINT16_MIN;
    ACT_CHECK_DESC(--sUInt16 > ACP_UINT16_MIN,
                   ("ACP_UINT16_MIN is not min of acp_uint16_t"));
    sUInt16 = ACP_UINT16_MAX;
    ACT_CHECK_DESC(++sUInt16 < ACP_UINT16_MAX,
                   ("ACP_UINT16_MAX is not max of acp_uint16_t"));

    sSInt32 = ACP_SINT32_MIN;
    ACT_CHECK_DESC(--sSInt32 > ACP_SINT32_MIN,
                   ("ACP_SINT32_MIN is not min of acp_sint32_t"));
    sSInt32 = ACP_SINT32_MAX;
    ACT_CHECK_DESC(++sSInt32 < ACP_SINT32_MAX,
                   ("ACP_SINT32_MAX is not max of acp_sint32_t"));
    sUInt32 = ACP_UINT32_MIN;
    ACT_CHECK_DESC(--sUInt32 > ACP_UINT32_MIN,
                   ("ACP_UINT32_MIN is not min of acp_uint32_t"));
    sUInt32 = ACP_UINT32_MAX;
    ACT_CHECK_DESC(++sUInt32 < ACP_UINT32_MAX,
                   ("ACP_UINT32_MAX is not max of acp_uint32_t"));

    sSInt64 = ACP_SINT64_MIN;
    ACT_CHECK_DESC(--sSInt64 > ACP_SINT64_MIN,
                   ("ACP_SINT64_MIN is not min of acp_sint64_t"));
    sSInt64 = ACP_SINT64_MAX;
    ACT_CHECK_DESC(++sSInt64 < ACP_SINT64_MAX,
                   ("ACP_SINT64_MAX is not max of acp_sint64_t"));
    sUInt64 = ACP_UINT64_MIN;
    ACT_CHECK_DESC(--sUInt64 > ACP_UINT64_MIN,
                   ("ACP_UINT64_MIN is not min of acp_uint64_t"));
    sUInt64 = ACP_UINT64_MAX;
    ACT_CHECK_DESC(++sUInt64 < ACP_UINT64_MAX,
                   ("ACP_UINT64_MAX is not max of acp_uint64_t"));

    sSLong = ACP_SLONG_MIN;
    ACT_CHECK_DESC(--sSLong > ACP_SLONG_MIN,
                   ("ACP_SLONG_MIN is not min of acp_slong_t"));
    sSLong = ACP_SLONG_MAX;
    ACT_CHECK_DESC(++sSLong < ACP_SLONG_MAX,
                   ("ACP_SLONG_MAX is not max of acp_slong_t"));
    sULong = ACP_ULONG_MIN;
    ACT_CHECK_DESC(--sULong > ACP_ULONG_MIN,
                   ("ACP_ULONG_MIN is not min of acp_ulong_t"));
    sULong = ACP_ULONG_MAX;
    ACT_CHECK_DESC(++sULong < ACP_ULONG_MAX,
                   ("ACP_ULONG_MAX is not max of acp_ulong_t"));

    /* check only existence */
    ACT_CHECK_DESC(ACP_FLOAT_MIN < ACP_FLOAT_MAX,
                   ("ACP_FLOAT_MAX and ACP_FLOAT_MIN do not exist"));
    ACT_CHECK_DESC(ACP_DOUBLE_MIN < ACP_DOUBLE_MAX,
                   ("ACP_DOUBLE_MAX and ACP_DOUBLE_MIN do not exist"));
    
    
    /*
     * acp_double_t
     */
    testAcpDoubleCheck();

    ACT_TEST_END();

    return 0;
}
