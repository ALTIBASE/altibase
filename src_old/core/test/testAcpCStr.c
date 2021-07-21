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
 * $Id: testAcpCStr.c 11231 2010-06-11 06:14:42Z vsebelev $
 ******************************************************************************/

/*******************************************************************************
 * A Skeleton Code for Testcases
*******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <acpMath.h>
#include <acpTest.h>

typedef struct testInt32Type
{
    acp_sint32_t mSign;
    acp_uint32_t mNumber;
    acp_sint32_t mRadix;
    acp_size_t   mLength;
    acp_char_t*  mString;
    acp_char_t*  mEnd;
    acp_rc_t     mResult;
} testInt32Type;

typedef struct testInt64Type
{
    acp_sint32_t mSign;
    acp_uint64_t mNumber;
    acp_sint32_t mRadix;
    acp_size_t   mLength;
    acp_char_t*  mString;
    acp_char_t*  mEnd;
    acp_rc_t     mResult;
} testInt64Type;

typedef struct testDouble
{
    acp_double_t mNumber;
    acp_size_t   mLength;
    acp_char_t*  mString;
    acp_char_t*  mEnd;
    acp_rc_t     mResult;
} testDouble;

#define TEST_INT32(aSign, aNumber, aRadix, aLength, aString, aEnd, aResult) \
{ aSign, aNumber, aRadix, aLength, aString, aEnd, aResult }

#define TEST_INT64(aSign, aNumber, aRadix, aLength, aString, aEnd, aResult) \
{ aSign, ACP_UINT64_LITERAL(aNumber), aRadix, aLength, aString, aEnd, aResult }

#define TEST_DOUBLE_NUMBER(aNumber, aLength, aEnd) \
{(acp_double_t)aNumber, aLength, #aNumber aEnd, aEnd, ACP_RC_SUCCESS}

#define TEST_DOUBLE_STRING(aNumber, aLength, aString, aEnd) \
{(acp_double_t)aNumber, aLength, aString, aEnd, ACP_RC_SUCCESS}

#define TEST_DOUBLE_FAIL(aNumber, aLength, aString, aResult) \
{(acp_double_t)aNumber, aLength, aString, "", aResult}

#define TEST_INTEGER_SENTINEL {0, 0, 0, 0, NULL, NULL, ACP_RC_SUCCESS}
#define TEST_DOUBLE_SENTINEL {0., 0, NULL, NULL, ACP_RC_SUCCESS}

#define TEST_NOT_END(aItem) (NULL != aItem.mString)

#define TESTSTR1 ("Nikolai Ivanovich Lobachevskii")
#define TESTSTR2 ("The Quick Brown Fox Jumps Over The Lazy Dog")

static void testCStrLen(void)
{
    ACT_CHECK(14 == acpCStrLen("SherlockHolmes", 100));
    ACT_CHECK(14 == acpCStrLen("SherlockHolmes", 14));
    ACT_CHECK(10 == acpCStrLen("SherlockHolmes", 10));
}

static void testCStrCmp(void)
{
    ACT_CHECK(acpCStrCmp("SherlockHolmes", "JohnWatson", 10) > 0);
    ACT_CHECK(acpCStrCmp("JohnWatson", "SherlockHolmes", 10) < 0);
    ACT_CHECK(acpCStrCmp("VanessaMae", "VanessaWilliams", 10) < 0);
    ACT_CHECK(acpCStrCmp("VanessaWilliams", "VanessaMae", 10) > 0);
    ACT_CHECK(acpCStrCmp("VanessaWilliams", "VanessaMae", 7) == 0);
}

static void testCStrCaseCmp(void)
{
    ACT_CHECK(acpCStrCaseCmp("AAA", "aaa", 3) == 0);
    ACT_CHECK(acpCStrCaseCmp("BBB", "aaa", 3) > 0);
    ACT_CHECK(acpCStrCaseCmp("aaa", "BBB", 3) < 0);
    ACT_CHECK(acpCStrCaseCmp("aaa", "aaB", 3) < 0);
    ACT_CHECK(acpCStrCaseCmp("", "aaB", 3) < 0);
    ACT_CHECK(acpCStrCaseCmp("aaB", "", 3) > 0);
    ACT_CHECK(acpCStrCaseCmp("", "", 1) == 0);
    ACT_CHECK(acpCStrCaseCmp("aB", "a", 2) > 0);
    ACT_CHECK(acpCStrCaseCmp("a", "aB", 2) < 0);

    ACT_CHECK(acpCStrCaseCmp("SherlockHolmes", "JohnWatson", 10) > 0);
    ACT_CHECK(acpCStrCaseCmp("JohnWatson", "SherlockHolmes", 10) < 0);
    ACT_CHECK(acpCStrCaseCmp("VanessaMae", "VanessaWilliams", 10) < 0);
    ACT_CHECK(acpCStrCaseCmp("VanessaWilliams", "VanessaMae", 10) > 0);
    ACT_CHECK(acpCStrCaseCmp("VanessaWilliams", "VanessaMae", 7) == 0);
}

static void testCStrCpy(void)
{
    acp_char_t      sStr1[1024];
    acp_char_t      sStr2[1024];
    acp_rc_t        sRC;
    acp_sint32_t    i;

    /* sStrX Will be TESTSTRX */
    (void)acpCStrCpy(sStr1, 1024, TESTSTR1, 100);
    (void)acpCStrCpy(sStr2, 1024, TESTSTR2, 100);

    ACT_CHECK(acpCStrLen(sStr1, 100) == acpCStrLen(TESTSTR1, 100));
    ACT_CHECK(acpCStrLen(sStr2, 100) == acpCStrLen(TESTSTR2, 100));

    /* Check whether copy done well */
    ACT_CHECK(0 == acpCStrCmp(TESTSTR1, sStr1, 100));
    ACT_CHECK(0 == acpCStrCmp(TESTSTR2, sStr2, 100));

    ACT_CHECK(ACP_RC_IS_EFAULT(acpCStrCpy(NULL, 0, TESTSTR1, 0)));
    ACT_CHECK(ACP_RC_IS_EFAULT(acpCStrCpy(sStr1, 0, NULL, 0)));
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrCpy(sStr1, 0, TESTSTR1, 100)));
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrCpy(sStr1, 5, TESTSTR1, 100)));

    /* Check Null-termination */
    ACT_CHECK(0 != acpCStrCmp(sStr1, TESTSTR1, 5));
    ACT_CHECK(0 == acpCStrCmp(sStr1, TESTSTR1, 4));

    /* Check acpCStrCat */
    (void)acpCStrCpy(sStr1, 1024, TESTSTR1, 100);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrCat(sStr1, 1024, TESTSTR1, 100)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrCat(sStr1, 1024, TESTSTR1, 100)));
    ACT_CHECK(acpCStrLen(sStr1, 1024) == (acpCStrLen(TESTSTR1, 100) * 3));

    while(1)
    {
        sRC = acpCStrCat(sStr1, 1024, TESTSTR1, 100);
        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            break;
        }
        else
        {
            /* Do nothing */
        }
    }
    ACT_CHECK(ACP_RC_IS_ETRUNC(sRC));
    ACT_CHECK(1023 == (acpCStrLen(sStr1, 1024)));

    sStr1[0] = ACP_CSTR_TERM;
    ACT_CHECK(ACP_RC_IS_EFAULT(acpCStrCat(NULL, 0, TESTSTR1, 0)));
    ACT_CHECK(ACP_RC_IS_EFAULT(acpCStrCat(sStr1, 0, NULL, 0)));
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrCat(sStr1, 0, TESTSTR1, 100)));
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrCat(sStr1, 5, TESTSTR1, 100)));

    /* Check Null-termination */
    ACT_CHECK(0 != acpCStrCmp(sStr1, TESTSTR1, 5));
    ACT_CHECK(0 == acpCStrCmp(sStr1, TESTSTR1, 4));

    for(i = 0; i < 1024; i++)
    {
        sStr1[i] = 'A';
    }
    ACT_CHECK(1024 == (acpCStrLen(sStr1, 1024)));
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrCat(sStr1, 1024, TESTSTR1, 100)));
    ACT_CHECK(1024 == (acpCStrLen(sStr1, 1024)));
}

/*
 * Test copying a 0-length string.
 */
static void testCStrCpyEmptyString()
{
    acp_char_t  sSource[ 1024 ] = "";
    acp_char_t  sDest[ 1024 ]   = "A";

    /* ACT_ASSERT( ACP_RC_IS_SUCCESS( acpCStrCpy( sDest, 1024, sSource, 1024 ) ) ); */
    strncpy( sDest, sSource, 1024 );

    ACT_ASSERT( 0 == acpCStrLen( sDest, 1024 ) );
    ACT_ASSERT( 0 == acpCStrCmp( sSource, sDest, 1024 ) );
}

static testInt32Type gTestInt32[] =
{
    TEST_INT32( 1,          1, 10,  1, "1"          , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,         10, 10,  2, "10"         , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,        128, 10,  3, "128"        , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,        077,  8,  2, "77"         , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,        077,  0,  3, "077"        , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,          0, 16,  2, "0x"         , "x"    , ACP_RC_SUCCESS),
    TEST_INT32( 1,          0, 16,  6, "0xZKZB"     , "xZKZB", ACP_RC_SUCCESS),
    TEST_INT32( 1,       0xFF, 16,  2, "FF"         , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,       0xFF, 16,  4, "0xFF"       , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,       0xFF,  0,  4, "0xFF"       , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1,        128, 10,  6, "128ABC"     , "ABC"  , ACP_RC_SUCCESS),
    TEST_INT32( 1,         11, 10,  2, "11"         , ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1, 1073741824, 10, 10, "1073741824" , ""     , ACP_RC_SUCCESS),
    TEST_INT32(-1, 1073741824, 10, 11, "-1073741824", ""     , ACP_RC_SUCCESS),
    TEST_INT32( 1, 1073741824, 10, 10, "10737418240", "0"  , ACP_RC_SUCCESS),
    TEST_INT32( 1, 1073741824, 10, 11, "+10737418240", "0" , ACP_RC_SUCCESS),
    TEST_INT32(-1, 1073741824, 10, 11, "-10737418240", "0" , ACP_RC_SUCCESS),
    /* So far 17 */

    /* Various kind of endpoints */
    TEST_INT32( 1, 0, 0, 11, "+", "+", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0, 0, 11, "+ABC", "+ABC", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0, 0, 11, "+0ABC", "ABC", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0, 0, 11, "0xZZZ", "xZZZ", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0xABC, 16, 11, "+0xABC", "", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0, 0, 11, "0x+ABCK", "x+ABCK", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0xABC, 16, 11, "+ABC", "", ACP_RC_SUCCESS),
    TEST_INT32( 1, 0xABC, 16, 11, "+ABCK", "K", ACP_RC_SUCCESS),
    /* So far 23 */

    /* Various kind of ranges */
    TEST_INT32( 1, ACP_UINT32_MAX, 10, 11,
                "10737418240" , ""   , ACP_RC_ERANGE ),
    TEST_INT32(-1, ACP_UINT32_MAX, 10, 12,
                "-10737418240", ""   , ACP_RC_ERANGE ),
    TEST_INT32( 1,          0,  1,  0, "Humbug"     ,
                "Humbug"   , ACP_RC_EINVAL),
    TEST_INT32( 1,          0, 37,  0, "Humbug",
                "Humbug"   , ACP_RC_EINVAL),
    TEST_INT32( 1, 0,  1,  0, "", "", ACP_RC_EINVAL),
    TEST_INT32( 1, 0, 37,  0, "", "", ACP_RC_EINVAL),
    /* So far 29 */
    TEST_INTEGER_SENTINEL
};

static testInt64Type gTestInt64[] =
{
            
    TEST_INT64( 1,                   1 , 10,  1,
                                    "1", ""   , ACP_RC_SUCCESS),
    TEST_INT64( 1, 1152921504606846976 , 10, 19,
                  "1152921504606846976", ""   , ACP_RC_SUCCESS),
    TEST_INT64(-1, 1152921504606846976 , 10, 20,
                 "-1152921504606846976", ""   , ACP_RC_SUCCESS),
    TEST_INT64( 1, 9152921504606846976 , 10, 19,
                  "9152921504606846976", ""   , ACP_RC_SUCCESS),
    TEST_INT64(-1, 9152921504606846976 , 10, 20,
                 "-9152921504606846976", ""   , ACP_RC_SUCCESS),
    TEST_INT64( 1,  0xFFFFFFFFFFFFFFFF , 10, 20,
                 "91529215046068469760", ""   , ACP_RC_ERANGE ),
    TEST_INT64(-1,  0xFFFFFFFFFFFFFFFF , 10, 21,
                "-91529215046068469760", ""   , ACP_RC_ERANGE ),
    TEST_INTEGER_SENTINEL
};

static testDouble gTestDouble[] =
{
    /* Normal numbers and endpoint */
    TEST_DOUBLE_NUMBER(    0.0,   3, ""                  ),
    TEST_DOUBLE_NUMBER(     .0,   3, ""                  ),
    TEST_DOUBLE_NUMBER(    0. ,   3, ""                  ),
    TEST_DOUBLE_NUMBER(    1.0,   3, ""                  ),
    TEST_DOUBLE_NUMBER(    0.1,   3, ""                  ),
    TEST_DOUBLE_NUMBER(    1. ,   3, ""                  ),
    TEST_DOUBLE_NUMBER(     .1,   3, ""                  ),
    TEST_DOUBLE_NUMBER(1.23E10,   7, ""                  ),
    TEST_DOUBLE_NUMBER(1.23E-9,   7, ""                  ),
    TEST_DOUBLE_NUMBER(    1. ,  20, "SCV good to go sir"),
    TEST_DOUBLE_NUMBER(     .1,  19, "Ready to roll out" ),
    TEST_DOUBLE_NUMBER(1.23E99,  23, "Ready for action"  ),
    TEST_DOUBLE_NUMBER(1.23E-7,  24, "Locked and loaded" ),
    TEST_DOUBLE_NUMBER(  1E308,  21, "My life for aiur"  ),
    TEST_DOUBLE_NUMBER( 1E-307,  18, "Adun toridas"      ),
    /* So far 15 */
    
    /* Fractional part exceeds acp_uint64_t */
    TEST_DOUBLE_NUMBER(3.1415926535897932    , 18, ""),
    TEST_DOUBLE_NUMBER(3.14159265358979323   , 19, ""),
    TEST_DOUBLE_NUMBER(3.141592653589793238  , 20, ""),
    TEST_DOUBLE_NUMBER(3.1415926535897932384 , 21, ""),
    TEST_DOUBLE_NUMBER(3.14159265358979323846, 22, ""),
    TEST_DOUBLE_NUMBER(0.9999999999999999    , 18, ""),
    TEST_DOUBLE_NUMBER(0.99999999999999999   , 19, ""),
    TEST_DOUBLE_NUMBER(0.999999999999999999  , 20, ""),
    TEST_DOUBLE_NUMBER(0.9999999999999999999 , 21, ""),
    TEST_DOUBLE_NUMBER(0.99999999999999999999, 22, ""),
    /* So far 25 */

    /* Integral part exceeds acp_uint64_t */
    TEST_DOUBLE_NUMBER(31415926535897932.    , 18, ""),
    TEST_DOUBLE_NUMBER(314159265358979323.   , 19, ""),
    TEST_DOUBLE_NUMBER(3141592653589793384.  , 20, ""),
    TEST_DOUBLE_NUMBER(31415926535897932384. , 21, ""),
    TEST_DOUBLE_NUMBER(314159265358979323846., 22, ""),
    /* So far 30 */

    /* Both exceeds exceeds acp_uint64_t */
    TEST_DOUBLE_NUMBER(31415926535897932.38462643383279502
                       , 37, ""),
    TEST_DOUBLE_NUMBER(314159265358979323.846264338327950288
                       , 39, ""),
    TEST_DOUBLE_NUMBER(3141592653589793238.4626433832795028841
                       , 41, ""),
    TEST_DOUBLE_NUMBER(31415926535897932384.62643383279502884197
                       , 43, ""),
    TEST_DOUBLE_NUMBER(314159265358979323846.264338327950288419716
                       , 45, ""),
    /* So far 35 */

    /* Various kind of endpoint */
    TEST_DOUBLE_STRING(    0.0,   0, ""         , ""       ),
    TEST_DOUBLE_STRING(    0.0,   7, "       "  , ""       ),
    TEST_DOUBLE_STRING(    0.0,   7, "0.0E100"  , ""       ),
    TEST_DOUBLE_STRING(  123.0,   7, "  123AA"  , "AA"     ),
    TEST_DOUBLE_STRING(    0.0,   4, "E111"     , "E111"   ),
    TEST_DOUBLE_STRING(    0.0,   4, "F111"     , "F111"   ),
    TEST_DOUBLE_STRING(    0.0,   5, "ABCD"     , "ABCD"   ),
    TEST_DOUBLE_STRING(   -0.0,  21, " - 7.1234", "- 7.1234"),
    TEST_DOUBLE_STRING(    -7.,   2, "-72.1234" , "2.1234" ),
    TEST_DOUBLE_STRING(    -7.,   2, "-7.1234"  , ".1234"  ),
    TEST_DOUBLE_STRING(    -7.,   2, "-7E10"    , "E10"    ),
    /* So far 46 */

    TEST_DOUBLE_STRING(3.,  2, "3.14159265", "14159265"),
    TEST_DOUBLE_STRING(3.1,  3, "3.14159265", "4159265"),
    TEST_DOUBLE_STRING(3.14,  4, "3.14159265", "159265"),
    TEST_DOUBLE_STRING(3.141,  5, "3.14159265", "59265"),
    TEST_DOUBLE_STRING(3.1415,  6, "3.14159265", "9265"),
    TEST_DOUBLE_STRING(3.14159,  7, "3.14159265", "265"),
    TEST_DOUBLE_STRING(3.141592,  8, "3.14159265", "65"),
    TEST_DOUBLE_STRING(3.1415926,  9, "3.14159265", "5"),
    TEST_DOUBLE_STRING(3.14159265, 10, "3.14159265", ""),
    /* So far 55 */

    TEST_DOUBLE_STRING(3.141592,      8, "3.141592E+100A", "E+100A"),
    TEST_DOUBLE_STRING(3.141592,      9, "3.141592E+100A", "E+100A"),
    TEST_DOUBLE_STRING(3.141592,     10, "3.141592E+100A", "E+100A"),
    TEST_DOUBLE_STRING(3.141592E1,   11, "3.141592E+100A", "00A"   ),
    TEST_DOUBLE_STRING(3.141592E10,  12, "3.141592E+100A", "0A"    ),
    TEST_DOUBLE_STRING(3.141592E100, 13, "3.141592E+100A", "A"     ),
    TEST_DOUBLE_STRING(3.141592E100, 14, "3.141592E+100A", "A"     ),
    TEST_DOUBLE_STRING(3.141592,      8, "3.141592E+ABCA", "E+ABCA"),
    TEST_DOUBLE_STRING(3.141592,      9, "3.141592E+ABCA", "E+ABCA"),
    TEST_DOUBLE_STRING(3.141592,     10, "3.141592E+ABCA", "E+ABCA"),
    TEST_DOUBLE_STRING(3.141592,     11, "3.141592E+ABCA", "E+ABCA"),
    TEST_DOUBLE_STRING(3.141592,     12, "3.141592E+ABCA", "E+ABCA"),
    TEST_DOUBLE_STRING(3.141592,     13, "3.141592E+ABCA", "E+ABCA"),
    TEST_DOUBLE_STRING(3.141592,     14, "3.141592E+ABCA", "E+ABCA"),
    /* So far 62 */
    TEST_DOUBLE_STRING(0., 1, "+", "+"),
    TEST_DOUBLE_STRING(0., 1, "+ABC", "+ABC"),
    TEST_DOUBLE_STRING(0., 1, "-", "-"),
    TEST_DOUBLE_STRING(0., 1, "-EFG", "-EFG"),

    /* Various kind of exponents */
    TEST_DOUBLE_STRING( 1.23E2,   3, "123"      , ""       ),
    TEST_DOUBLE_STRING(   1E10,  11, "10000000000", ""     ),
    TEST_DOUBLE_STRING(   1E19,  20, "10000000000000000000", ""),
    TEST_DOUBLE_STRING(   1E20,  21, "100000000000000000000", ""),
    TEST_DOUBLE_STRING(   1E30,  24, "100000000000000000000E10", ""),
    TEST_DOUBLE_STRING(     1.,  24, ".00000000000000000001E20", ""),
    TEST_DOUBLE_STRING(     1.,  24, "10000000000000000000E-19", ""),
    TEST_DOUBLE_NUMBER(314159265358979323846.E128, 27, ""),
    TEST_DOUBLE_NUMBER(.314159265358979323846E128, 27, ""),
    TEST_DOUBLE_NUMBER(3.14159265358979323846E128, 27, ""),
    TEST_DOUBLE_NUMBER(3141592653589793238462E128, 27, ""),

    /* Very large part of integrals */
    TEST_DOUBLE_STRING(   1E50,  51, "1"
        "00000000000000000000000000000000000000000000000000", ""),
    TEST_DOUBLE_STRING(   1E50, 102, "1"
        "00000000000000000000000000000000000000000000000000."
        "00000000000000000000000000000000000000000000000001", ""),
    TEST_DOUBLE_STRING(  1E100, 101, "1"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000", ""),
    TEST_DOUBLE_STRING( 1E-100, 101, "."
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000001", ""),

    TEST_DOUBLE_STRING(    1.0, 1002,
        "1."
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000001",
        ""),
    TEST_DOUBLE_STRING(  1E-10,   5, "1E-10"  , ""),
    TEST_DOUBLE_STRING( 1E-100,   6, "1E-100" , ""),

    /* Test subnormal values */
    /* Close to normal values */
    TEST_DOUBLE_NUMBER(1E-308,    6, ""),
    /* In the middle */
    TEST_DOUBLE_NUMBER(23.5E-316, 9, ""),
    /* Minimal subnormal value */
    TEST_DOUBLE_NUMBER(5E-324,    6, ""),

    TEST_DOUBLE_SENTINEL
};

#define TEST_DOUBLE_LEN 20
#define TEST_EPSILON (1E-10)
    
static void testCStrIntConversion(void)
{
    static acp_char_t* sVarBase[] = 
    {
        "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "a", "b", "c", "d", "e", "f", "g", "h", "i",
        "j", "k", "l", "m", "n", "o", "p", "q", "r",
        "s", "t", "u", "v", "w", "x", "y", "z"
    };

    acp_uint32_t sValue1;
    acp_uint64_t sValue2;
    acp_sint32_t sSign;
    acp_char_t   sCStr[30];
    acp_uint32_t i;

    acp_char_t*  sEnd = NULL;
    acp_rc_t     sRC;

    for(i = 2; i <= 36; i++)
    {
        sRC = acpCStrToInt32(
            sVarBase[i - 2], 1,
            &sSign, &sValue1,
            i, &sEnd);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(i - 1 == sValue1);
    }

    for(i = 0; TEST_NOT_END(gTestInt32[i]); i++)
    {
        sRC = acpCStrToInt32(
            gTestInt32[i].mString,
            gTestInt32[i].mLength,
            &sSign, &sValue1,
            gTestInt32[i].mRadix,
            &sEnd);

        ACT_CHECK_DESC(gTestInt32[i].mResult == sRC,
            ("acpCStrToInt32 result not match for testcase [%d]\n"
             "\"%s\" -> %d %u \"%s\" (must be %d but %d)",
             i, gTestInt32[i].mString, sSign, sValue1, sEnd,
             gTestInt32[i].mResult, sRC)
            );
        if(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ERANGE(sRC))
        {
            ACT_CHECK_DESC(gTestInt32[i].mSign == sSign,
                ("Conversion mismatch for testcase [%d]!"
                 " : %s(%d) => %lu, \"%s\" <=> %lu, \"%s\"\n",
                 i, gTestInt32[i].mString, (acp_uint32_t)gTestInt32[i].mLength,
                 sValue1, sEnd,
                 gTestInt32[i].mNumber, gTestInt32[i].mEnd)
                );
            ACT_CHECK_DESC(gTestInt32[i].mNumber == sValue1,
                ("Conversion mismatch for testcase [%d]!"
                 " : %s(%d) => %lu, \"%s\" <=> %lu, \"%s\"\n",
                 i, gTestInt32[i].mString, (acp_uint32_t)gTestInt32[i].mLength,
                 sValue1, sEnd,
                 gTestInt32[i].mNumber, gTestInt32[i].mEnd)
                );
            ACT_CHECK_DESC(
                0 == acpCStrCmp(gTestInt32[i].mEnd, sEnd,
                                gTestInt32[i].mLength),
                ("Endpoint mismatch for testcase [%d]!"
                 " : %s(%d) => %lu, \"%s\" <=> %lu, \"%s\"\n",
                 i, gTestInt32[i].mString, (acp_uint32_t)gTestInt32[i].mLength,
                 sValue1, sEnd,
                 gTestInt32[i].mNumber, gTestInt32[i].mEnd)
                );
        }
        else
        {
            /* No need of comparing */
        }
    }

    for(i = 0; TEST_NOT_END(gTestInt64[i]); i++)
    {
        sRC = acpCStrToInt64(
            gTestInt64[i].mString,
            gTestInt64[i].mLength,
            &sSign, &sValue2,
            gTestInt64[i].mRadix,
            &sEnd);

        ACT_CHECK_DESC(gTestInt64[i].mResult == sRC,
            ("acpCStrToInt64 result not match for testcase [%d]\n", i));
        if(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ERANGE(sRC))
        {
            ACT_CHECK(gTestInt64[i].mSign   == sSign);
            ACT_CHECK(gTestInt64[i].mNumber == sValue2);
            ACT_CHECK(0 == acpCStrCmp(
                    gTestInt64[i].mEnd, sEnd, gTestInt64[i].mLength)
                );
        }
        else
        {
            /* No need of comparing */
        }
    }

    for(i=2; i<=36; i++)
    {
        ACT_CHECK(
            ACP_RC_IS_SUCCESS(
                acpCStrToInt64("11", 2, &sSign, &sValue2,
                               (acp_sint32_t)i, NULL) 
                )
            );
        ACT_CHECK(i + 1 == sValue2);
        ACT_CHECK(1 == sSign);
    }

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrUInt32ToCStr10(1073741824, sCStr, 30)));
    ACT_CHECK(acpCStrCmp(sCStr, "1073741824", 6) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrSInt32ToCStr10(-1073741824, sCStr, 30)));
    ACT_CHECK(acpCStrCmp(sCStr, "-1073741824", 7) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrSInt32ToCStr10(0, sCStr, 2)));
    ACT_CHECK(acpCStrCmp(sCStr, "0", 2) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrSInt32ToCStr10(1, sCStr, 2)));
    ACT_CHECK(acpCStrCmp(sCStr, "1", 2) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrSInt32ToCStr10(99, sCStr, 3)));
    ACT_CHECK(acpCStrCmp(sCStr, "99", 3) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpCStrSInt32ToCStr10(100, sCStr, 4)));
    ACT_CHECK(acpCStrCmp(sCStr, "100", 4) == 0);
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrSInt32ToCStr10(1073741824, sCStr, 2)));
    ACT_CHECK(ACP_RC_IS_ETRUNC(acpCStrSInt32ToCStr10(0, sCStr, 1)));

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpCStrSInt64ToCStr10(ACP_SINT64_LITERAL(1152921504606846976),
                                  sCStr, 30)
            ));
    ACT_CHECK(acpCStrCmp(sCStr, "1152921504606846976", 20) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpCStrUInt64ToCStr10(ACP_UINT64_LITERAL(18446744073709551615),
                                  sCStr, 30)
            ));
    ACT_CHECK(acpCStrCmp(sCStr, "18446744073709551615", 21) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpCStrInt64ToCStr16(ACP_UINT64_LITERAL(18446744073709551615),
                                 sCStr, 30, ACP_TRUE)
            ));
    ACT_CHECK(acpCStrCmp(sCStr, "FFFFFFFFFFFFFFFF", 21) == 0);
}

static void testCStrDoubleConversion(void)
{
    acp_uint32_t i;

    acp_double_t sValueD;
    acp_double_t sDouble1;
    acp_double_t sDouble2;
    acp_char_t   sString[TEST_DOUBLE_LEN + 1];

    acp_char_t*  sEnd = NULL;
    acp_rc_t     sRC;

    for(i = 0; TEST_NOT_END(gTestDouble[i]); i++)
    {
        sRC = acpCStrToDouble(
            gTestDouble[i].mString,
            gTestDouble[i].mLength,
            &sValueD,
            &sEnd);

        ACT_CHECK_DESC(
            gTestDouble[i].mResult == sRC,
            ("Result for testcase [%d] must be %d but %d! :" 
             "\n\t\"%s\" -> %30.25lf, \"%s\"\n\t<=> %30.25lf\n",
             i, gTestDouble[i].mResult, sRC,
             gTestDouble[i].mString,
             sValueD,
             sEnd,
             gTestDouble[i].mNumber)
            );
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            ACT_CHECK_DESC(
                (acpMathFastFabs(gTestDouble[i].mNumber - sValueD) <=
                 acpMathFastFabs(TEST_EPSILON * sValueD)
                ),
                ("Conversion mismatch for testcase [%d]! : "
                 "\n\t\"%s\"[%u] ->"
                 "\nConverted : %30.25lf(%g), \"%s\""
                 "\nRequired  : %30.25lf(%g), \"%s\""
                 "\nDifference is %30.25lf(%g)"
                 " which must be less than %30.25lf(%g)\n",
                 i,
                 gTestDouble[i].mString,
                 (acp_uint32_t)gTestDouble[i].mLength,
                 sValueD, sValueD,
                 sEnd,
                 gTestDouble[i].mNumber, gTestDouble[i].mNumber,
                 gTestDouble[i].mEnd,
                 acpMathFastFabs(gTestDouble[i].mNumber - sValueD),
                 acpMathFastFabs(gTestDouble[i].mNumber - sValueD),
                 acpMathFastFabs(TEST_EPSILON * sValueD),
                 acpMathFastFabs(TEST_EPSILON * sValueD)
                 )
                );
            ACT_CHECK_DESC(0 == acpCStrCmp(
                    gTestDouble[i].mEnd, sEnd, gTestDouble[i].mLength),
                ("Endpoint not match for testcase  [%d]! : "
                 "\n\t\"%s\"[%u] ->"
                 "\nConverted : %30.25lf(%g), \"%s\""
                 "\nRequired  : %30.25lf(%g), \"%s\"",
                 i,
                 gTestDouble[i].mString,
                 (acp_uint32_t)gTestDouble[i].mLength,
                 sValueD, sValueD,
                 sEnd,
                 gTestDouble[i].mNumber, gTestDouble[i].mNumber,
                 gTestDouble[i].mEnd
                 )
                );
        }
        else
        {
            /* No need of comparing */
        }
    }

    sDouble1 = 0.0;
    for(i = 0; i < 1000000; i++)
    {
        sDouble1 = 0.1 * i;

        (void)acpSnprintf(sString, TEST_DOUBLE_LEN, "%lf", sDouble1);
        sRC = acpCStrToDouble(sString, TEST_DOUBLE_LEN, &sDouble2, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(acpMathFastFabs(sDouble1 - sDouble2) < TEST_EPSILON,
                       ("Conversion Mismatch! : %30.25lf -> %s -> %30.25lf\n",
                        sDouble1, sString, sDouble2));
    }

    for(i = 0; i < 1000000; i++)
    {
        (void)acpSnprintf(sString, TEST_DOUBLE_LEN, "%d", i);
        sRC = acpCStrToDouble(sString, TEST_DOUBLE_LEN, &sDouble2, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC((acp_double_t)i == sDouble2,
                       ("Conversion Mismatch! : %d -> %s -> %30.25lf\n",
                        i, sString, sDouble2));
    }

    for(i = 0; i < 1000000; i++)
    {
        (void)acpSnprintf(sString, TEST_DOUBLE_LEN, "0.%06d", i);
        sRC = acpCStrToDouble(sString, TEST_DOUBLE_LEN, &sDouble2, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(
            acpMathFastFabs((i * 0.000001) - sDouble2) < TEST_EPSILON,
            ("Conversion Mismatch! : 0.%06d -> %s -> %30.25lf\n",
             i, sString, sDouble2));
    }
}

static testDouble gTestNans[] =
{
    /* NaNs and Infinities */
    TEST_DOUBLE_STRING(0,  3, "NAn"                 , ""),
    TEST_DOUBLE_STRING(0,  4, "+NAN"                , ""),
    TEST_DOUBLE_STRING(0,  4, "-nAn"                , ""),
    TEST_DOUBLE_STRING(0, 13, "NaNI am a boy"       , "I am a boy"),
    TEST_DOUBLE_STRING(0, 19, "+nAN(You are a girl" , "(You are a girl"),
    TEST_DOUBLE_STRING(0, 16, "-Nan(But a string)"  , ""),
    TEST_DOUBLE_SENTINEL
};

static testDouble gTestInfs[] =
{
    TEST_DOUBLE_STRING( 1.0, 3,  "Inf"     , ""),
    TEST_DOUBLE_STRING( 1.0, 4, "+Inf"     , ""),
    TEST_DOUBLE_STRING(-1.0, 4, "-Inf"     , ""),
    TEST_DOUBLE_STRING( 1.0, 8,  "Infinity", "inity"),
    TEST_DOUBLE_STRING(-1.0, 9, "-Infinity", "inity"),

    /* Overflows and Underflows */
    TEST_DOUBLE_FAIL(     1.0,    5,  "1E309", ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL(     0.0,    8, "4.9E-324", ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL(     1.0, 1001,
        "1"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000",
        ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL(     0.0,  1001,
        "."
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000001",
        ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL(     1.0,   200,
        "1E999999999999999999999999999999999999999999999999"
        "99999999999999999999999999999999999999999999999999",
        ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL(     0.0,   200,
        "1E-99999999999999999999999999999999999999999999999"
        "99999999999999999999999999999999999999999999999999",
        ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL  (    0.0,    7, "1E-1000", ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL  (    0.0,   33,
        "1E-123456789012345678901234567890"    , ACP_RC_ERANGE),
    TEST_DOUBLE_FAIL  (    1.0,   33,
        "1E1234567890123456789012345678901"    , ACP_RC_ERANGE),
    TEST_DOUBLE_SENTINEL
};

void testCStrInfAndNan(void)
{ 
    acp_sint32_t i;
    acp_rc_t sRC;
    acp_double_t sValueD;
    acp_char_t* sEnd;

    for(i = 0; TEST_NOT_END(gTestNans[i]); i++)
    {
        sRC = acpCStrToDouble(
            gTestNans[i].mString,
            gTestNans[i].mLength,
            &sValueD,
            &sEnd);

        ACT_CHECK(gTestDouble[i].mResult == sRC);
        ACT_CHECK_DESC(0 != acpMathIsNan(sValueD),
            ("Conversion must be NaN for testcase [%d]\"%s\"!\n",
             i, gTestNans[i].mString)
            );
        ACT_CHECK_DESC(0 == acpCStrCmp(
                gTestNans[i].mEnd, sEnd, gTestNans[i].mLength),
            ("Endpoint not proper for testcase [%d]! : \"%s\" -> \"%s\"\n",
             i, gTestNans[i].mString, sEnd)
            );
    }

    for(i = 0; TEST_NOT_END(gTestInfs[i]); i++)
    {
        sRC = acpCStrToDouble(
            gTestInfs[i].mString,
            gTestInfs[i].mLength,
            &sValueD,
            &sEnd);

        ACT_CHECK(gTestInfs[i].mResult == sRC);
        if(0.0 == gTestInfs[i].mNumber)
        {
            ACT_CHECK_DESC(
                0.0 == sValueD,
                ("Conversion must be 0.0 for testcase [%d]\"%s\" but %g!\n",
                 i, gTestInfs[i].mString, sValueD)
                );
        }
        else
        {
            ACT_CHECK_DESC(
                0 == acpMathIsFinite(sValueD),
                ("Conversion must be Infinity"
                 " for testcase [%d]\"%s\" but %g!\n",
                 i, gTestInfs[i].mString, sValueD)
                );
        }

        ACT_CHECK_DESC(0 == acpCStrCmp(
                gTestInfs[i].mEnd, sEnd, gTestInfs[i].mLength),
            ("Endpoint not proper for testcase [%d]! : \"%s\" -> \"%s\"\n",
             i, gTestInfs[i].mString, sEnd)
            );
    }
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

#if defined(__INSURE__)
    /* Do nothing */
#else
    testCStrLen();
    testCStrCmp();
    testCStrCaseCmp();
    testCStrCpy();
    testCStrCpyEmptyString();
    testCStrIntConversion();
    testCStrDoubleConversion();
    testCStrInfAndNan();
#endif

    ACT_TEST_END();

    return 0;
}
