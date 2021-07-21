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
 * $Id: testAcpPrintf.c 10912 2010-04-23 01:44:15Z djin $
 ******************************************************************************/

#include <aciTypes.h>
#include <act.h>
#include <acpFile.h>
#include <acpMem.h>
#include <acpError.h>
#include <acpPrintf.h>


#if 0
#define FPRINTF  fprintf
#define SNPRINTF snprintf
#define CHECK_RC ACP_FALSE
#else
#define FPRINTF  acpFprintf
#define SNPRINTF acpSnprintf
#define CHECK_RC ACP_TRUE
#endif


#if defined(ALTI_CFG_OS_TRU64)
#define FLOAT_PINF       (1.0 / 0.0)
#define FLOAT_NINF       (-1.0 / 0.0)
#define FLOAT_NAN        (0.0 / 0.0)
#else
#define FLOAT_PINF       9991
#define FLOAT_NINF       9992
#define FLOAT_NAN        9993
#endif

#define FLOAT_PINF_VALUE (1.0 / sZero)
#define FLOAT_NINF_VALUE (-1.0 / sZero)
#define FLOAT_NAN_VALUE  (0.0 / sZero)


#define BUFFER_SIZE      200

#define TEST_OUT_FILE    "testAcpPrintf.out"


static acp_char_t *gTestOutFile = TEST_OUT_FILE;


typedef struct acpPrintfTestFormat
{
    const acp_char_t *mFormat;
    const acp_char_t *mResult;
} acpPrintfTestFormat;

typedef struct acpPrintfTestInt
{
    const acp_char_t *mFormat;
    const acp_char_t *mResult;
    acp_sint32_t      mValue;
} acpPrintfTestInt;

typedef struct acpPrintfTestFloat
{
    const acp_char_t *mFormat;
    const acp_char_t *mResult;
    acp_float_t       mValue;
} acpPrintfTestFloat;

typedef struct acpPrintfTestDouble
{
    const acp_char_t *mFormat;
    const acp_char_t *mResult;
    acp_double_t      mValue;
} acpPrintfTestDouble;

typedef struct acpPrintfTestChar
{
    const acp_char_t *mFormat;
    const acp_char_t *mResult;
    acp_char_t        mValue;
} acpPrintfTestChar;

typedef struct acpPrintfTestString
{
    const acp_char_t *mFormat;
    const acp_char_t *mResult;
    const acp_char_t *mValue;
} acpPrintfTestString;


static acpPrintfTestFormat gTestBasic[] =
{
    {
        "%b%zn",
        "%b"
    },
    {
        "%%%zn",
        "%"
    },
    {
        "%b %%%zn",
        "%b %"
    },
    {
        "%% %b%zn",
        "% %b"
    },
    {
        " %b %% %zn",
        " %b % "
    },
    {
        " %% %b %zn",
        " % %b "
    },
    {
        " %10% %b %zn",
        "          % %b "
    },
    {
        " %-10% %b %zn",
        " %          %b "
    },
    {
        " %07% %b %zn",
        " 000000% %b "
    },
    {
        " %-07% %b %zn",
        " %       %b "
    },
    {
        NULL,
        NULL
    }
};

static acpPrintfTestFormat gTestIntType[] =
{
    {
        "%hhd %hhd %hhd %hhd %hhd %hhd %lld %lld%zn",
        "-123 123 -57 57 46 -46 -1234567890123456789 1234567890123456789"
    },
    {
        "%hd %hd %hd %hd %hd %hd %lld %lld%zn",
        "-123 123 -12345 12345 -722 722 -1234567890123456789 1234567890123456789"
    },
    {
        "%d %d %d %d %d %d %lld %lld%zn",
        "-123 123 -12345 12345 -1234567890 1234567890 -1234567890123456789 "
        "1234567890123456789"
    },
    {
        "%hhu %hhu %hhu %hhu %hhu %hhu %llu %llu%zn",
        "133 123 199 57 46 210 17212176183586094827 1234567890123456789"
    },
    {
        "%hu %hu %hu %hu %hu %hu %llu %llu%zn",
        "65413 123 53191 12345 64814 722 17212176183586094827 "
        "1234567890123456789"
    },
    {
        "%u %u %u %u %u %u %llu %llu%zn",
        "4294967173 123 4294954951 12345 3060399406 1234567890 "
        "17212176183586094827 1234567890123456789"
    },
    {
        "%hho %hho %hho %hho %hho %hho %llo %llo%zn",
        "205 173 307 71 56 322 1673357360560205477353 104420417217572300425"
    },
    {
        "%ho %ho %ho %ho %ho %ho %llo %llo%zn",
        "177605 173 147707 30071 176456 1322 1673357360560205477353 "
        "104420417217572300425"
    },
    {
        "%o %o %o %o %o %o %llo %llo%zn",
        "37777777605 173 37777747707 30071 26632376456 11145401322 "
        "1673357360560205477353 104420417217572300425"
    },
    {
        "%hhx %hhx %hhx %hhx %hhx %hhx %llx %llx%zn",
        "85 7b c7 39 2e d2 eeddef0b82167eeb 112210f47de98115"
    },
    {
        "%hx %hx %hx %hx %hx %hx %llx %llx%zn",
        "ff85 7b cfc7 3039 fd2e 2d2 eeddef0b82167eeb 112210f47de98115"
    },
    {
        "%x %x %x %x %x %x %llx %llx%zn",
        "ffffff85 7b ffffcfc7 3039 b669fd2e 499602d2 eeddef0b82167eeb "
        "112210f47de98115"
    },
    {
        "%hhX %hhX %hhX %hhX %hhX %hhX %llX %llX%zn",
        "85 7B C7 39 2E D2 EEDDEF0B82167EEB 112210F47DE98115"
    },
    {
        "%hX %hX %hX %hX %hX %hX %llX %llX%zn",
        "FF85 7B CFC7 3039 FD2E 2D2 EEDDEF0B82167EEB 112210F47DE98115"
    },
    {
        "%X %X %X %X %X %X %llX %llX%zn",
        "FFFFFF85 7B FFFFCFC7 3039 B669FD2E 499602D2 EEDDEF0B82167EEB "
        "112210F47DE98115"
    },
    {
        NULL,
        NULL
    }
};

static acpPrintfTestInt gTestInt[] =
{
    {
        "%10d%zn",
        "      -123",
        -123
    },
    {
        "%-10d%zn",
        "-123      ",
        -123
    },
    {
        "%3d%zn",
        "-123",
        -123
    },
    {
        "%-3d%zn",
        "-123",
        -123
    },
    {
        "%010d%zn",
        "-000000123",
        -123
    },
    {
        "%-010d%zn",
        "-123      ",
        -123
    },
    {
        "%03d%zn",
        "-123",
        -123
    },
    {
        "%0-3d%zn",
        "-123",
        -123
    },
    {
        "%10d%zn",
        "       123",
        123
    },
    {
        "%-10d%zn",
        "123       ",
        123
    },
    {
        "%3d%zn",
        "123",
        123
    },
    {
        "%-3d%zn",
        "123",
        123
    },
    {
        "%010d%zn",
        "0000000123",
        123
    },
    {
        "%-010d%zn",
        "123       ",
        123
    },
    {
        "%03d%zn",
        "123",
        123
    },
    {
        "%0-3d%zn",
        "123",
        123
    },
    {
        NULL,
        NULL,
        0
    }
};

static acpPrintfTestFloat gTestFloat[] =
{
    {
        NULL,
        NULL,
        0.0
    }
};

static acpPrintfTestDouble gTestDouble[] =
{
    {
        "%10e%zn",
        "       inf",
        FLOAT_PINF
    },
    {
        "%10e%zn",
        "      -inf",
        FLOAT_NINF
    },
    {
        "%10e%zn",
        "       nan",
        FLOAT_NAN
    },
    {
        "%20e%zn",
        "        1.000000e+00",
        1.0
    },
    {
        "%20e%zn",
        "        0.000000e+00",
        0.0
    },
    {
        "%20e%zn",
        "        1.234000e+00",
        1.234
    },
    {
        "%020e%zn",
        "000000001.230000e-01",
        0.123
    },
    {
        "%020e%zn",
        "000000001.234000e+01",
        12.34
    },
    {
        "%20e%zn",
        "        1.200000e-02",
        0.012
    },
    {
        "%20e%zn",
        "        1.234568e+03",
        1234.56789
    },
    {
        "%020e%zn",
        "-00000001.234568e+03",
        -1234.56789
    },
    {
        "%10.0e%zn",
        "     1e+00",
        1.234
    },
    {
        "%10.0e%zn",
        "     1e-01",
        0.123
    },
    {
        "%#10.0e%zn",
        "    1.e+01",
        12.34
    },
    {
        "%#010.0e%zn",
        "00001.e-02",
        0.012
    },
    {
        "%10.1e%zn",
        "   1.2e+00",
        1.234
    },
    {
        "%010.1e%zn",
        "0001.2e-01",
        0.123
    },
    {
        "%10.1e%zn",
        "   1.2e+01",
        12.34
    },
    {
        "%#10.1e%zn",
        "   1.2e-02",
        0.012
    },
    {
        "% -10E%zn",
        " INF      ",
        FLOAT_PINF
    },
    {
        "%#-10E%zn",
        "-INF      ",
        FLOAT_NINF
    },
    {
        "% -10E%zn",
        " NAN      ",
        FLOAT_NAN
    },
    {
        "%20E%zn",
        "        1.000000E+00",
        1.0
    },
    {
        "%#-20E%zn",
        "0.000000E+00        ",
        0.0
    },
    {
        "%20E%zn",
        "        1.234000E+00",
        1.234
    },
    {
        "%020E%zn",
        "000000001.230000E-01",
        0.123
    },
    {
        "%10f%zn",
        "       inf",
        FLOAT_PINF
    },
    {
        "%10f%zn",
        "      -inf",
        FLOAT_NINF
    },
    {
        "% -10f%zn",
        " nan      ",
        FLOAT_NAN
    },
    {
        "%20f%zn",
        "            1.000000",
        1.0
    },
    {
        "%020f%zn",
        "0000000000000.000000",
        0.0
    },
    {
        "%20f%zn",
        "            1.234000",
        1.234
    },
    {
        "%20f%zn",
        "            0.123000",
        0.123
    },
    {
        "%020f%zn",
        "0000000000012.340000",
        12.34
    },
    {
        "%0-20f%zn",
        "0.012000            ",
        0.012
    },
    {
        "%20f%zn",
        "           12.345679",
        12.3456789
    },
    {
        "%020f%zn",
        "-000000000012.345679",
        -12.3456789
    },
    {
        "%20f%zn",
        "    120000000.000000",
        120000000.0
    },
    {
        "%10.0f%zn",
        "         1",
        1.234
    },
    {
        "%-10.0f%zn",
        "0         ",
        0.123
    },
    {
        "%010.0f%zn",
        "0000000012",
        12.34
    },
    {
        "%010.0f%zn",
        "0000000000",
        0.012
    },
    {
        "%10.1f%zn",
        "       1.2",
        1.234
    },
    {
        "%-10.1f%zn",
        "0.1       ",
        0.123
    },
    {
        "%-10.1f%zn",
        "12.3      ",
        12.34
    },
    {
        "%10.1f%zn",
        "       0.0",
        0.012
    },
    {
        "%10F%zn",
        "       INF",
        FLOAT_PINF
    },
    {
        "%10F%zn",
        "      -INF",
        FLOAT_NINF
    },
    {
        "% -10F%zn",
        " NAN      ",
        FLOAT_NAN
    },
    {
        "%010F%zn",
        "001.000000",
        1.0
    },
    {
        "%10F%zn",
        "  0.000000",
        0.0
    },
    {
        "%-10F%zn",
        "1.234000  ",
        1.234
    },
    {
        "%-10F%zn",
        "0.123000  ",
        0.123
    },
    {
        "%10g%zn",
        "       inf",
        FLOAT_PINF
    },
    {
        "%10g%zn",
        "      -inf",
        FLOAT_NINF
    },
    {
        "% -10g%zn",
        " nan      ",
        FLOAT_NAN
    },
    {
        "%010g%zn",
        "0000000001",
        1.0
    },
    {
        "%10g%zn",
        "         0",
        0.0
    },
    {
        "%-10g%zn",
        "1.234     ",
        1.234
    },
    {
        "%10g%zn",
        "     0.123",
        0.123
    },
    {
        "%010g%zn",
        "0000012.34",
        12.34
    },
    {
        "%10g%zn",
        "     0.012",
        0.012
    },
    {
        "%010.0g%zn",
        "0000000001",
        1.234
    },
    {
        "%010.0g%zn",
        "00000000.1",
        0.123
    },
    {
        "%010.0g%zn",
        "000001e+01",
        12.34
    },
    {
        "%10.0g%zn",
        "      0.01",
        0.012
    },
    {
        "%10.0g%zn",
        "     1e+21",
        12.34E20
    },
    {
        "%10.5g%zn",
        " 1.234e+21",
        12.34E20
    },
    {
        "%10.1g%zn",
        "         1",
        1.234
    },
    {
        "%-10.1g%zn",
        "0.1       ",
        0.123
    },
    {
        "%-10.1g%zn",
        "1e+01     ",
        12.34
    },
    {
        "%-10.1g%zn",
        "0.01      ",
        0.012
    },
    {
        "% -10G%zn",
        " INF      ",
        FLOAT_PINF
    },
    {
        "%010G%zn",
        "      -INF",
        FLOAT_NINF
    },
    {
        "%-010G%zn",
        "NAN       ",
        FLOAT_NAN
    },
    {
        "%10G%zn",
        "         1",
        1.0
    },
    {
        "%-10G%zn",
        "0         ",
        0.0
    },
    {
        "%10G%zn",
        "     1.234",
        1.234
    },
    {
        "%-10G%zn",
        "0.123     ",
        0.123
    },
    {
        "%"ACI_FLOAT_G_FMT"%zn",
        "-1.175494E-37",
        -1.175494351E-37
    },
    {
        NULL,
        NULL,
        0.0
    }
};

static acpPrintfTestChar gTestChar[] =
{
    {
        " %c %zn",
        " t ",
        't'
    },
    {
        " %03c %zn",
        " 00t ",
        't'
    },
    {
        " %-03c %zn",
        " t   ",
        't'
    },
    {
        " %0-3c %zn",
        " t   ",
        't'
    },
    {
        NULL,
        NULL,
        0
    }
};

static acpPrintfTestString gTestString[] =
{
    {
        " %10s %zn",
        "      hello ",
        "hello"
    },
    {
        " %-10s %zn",
        " hello      ",
        "hello"
    },
    {
        " %3s %zn",
        " hello ",
        "hello"
    },
    {
        " %-3s %zn",
        " hello ",
        "hello"
    },
    {
        " %010s %zn",
        " 00000hello ",
        "hello"
    },
    {
        " %-010s %zn",
        " hello      ",
        "hello"
    },
    {
        " %03s %zn",
        " hello ",
        "hello"
    },
    {
        " %0-3s %zn",
        " hello ",
        "hello"
    },
    {
        NULL,
        NULL,
        NULL,
    }
};


#define TEST_SNPRINTF(aTestName, aTestFormat, aPrintfArgs)             \
    do                                                                  \
    {                                                                   \
        for (sSize = 0; sSize < BUFFER_SIZE; sSize++)                   \
        {                                                               \
            sMaxLen = 0;                                                \
                                                                        \
            for (i = 0; (aTestFormat)[i].mFormat != NULL; i++)            \
            {                                                           \
                sRC = SNPRINTF aPrintfArgs;                             \
                                                                        \
                ACT_CHECK_DESC(                                         \
                    strlen((aTestFormat)[i].mResult) == sLen,             \
                    ("acpSnprintf(\"%s\", %zd) should return %zd "      \
                     "but %zd in case " #aTestName ":%d",               \
                     (aTestFormat)[i].mFormat,                            \
                     sSize,                                             \
                     strlen((aTestFormat)[i].mResult),                    \
                     sLen,                                              \
                     i));                                               \
                                                                        \
                if (CHECK_RC)                                           \
                {                                                       \
                    if (sLen < sSize)                                   \
                    {                                                   \
                        ACT_CHECK_DESC(                                 \
                            ACP_RC_IS_SUCCESS(sRC),                     \
                            ("acpSnprintf(\"%s\", %zd) should return "  \
                             "SUCCESS but %d "                          \
                             "in case " #aTestName ":%d",               \
                             (aTestFormat)[i].mFormat,                    \
                             sSize,                                     \
                             sRC,                                       \
                             i));                                       \
                    }                                                   \
                    else                                                \
                    {                                                   \
                        ACT_CHECK_DESC(                                 \
                            ACP_RC_IS_ETRUNC(sRC),                      \
                            ("acpSnprintf(\"%s\", %zd) should return "  \
                             "ETRUNC but %d in case " #aTestName ":%d", \
                             (aTestFormat)[i].mFormat,                    \
                             sSize,                                     \
                             sRC,                                       \
                             i));                                       \
                    }                                                   \
                }                                                       \
                else                                                    \
                {                                                       \
                }                                                       \
                                                                        \
                if (sSize > 0)                                          \
                {                                                       \
                    ACT_CHECK_DESC(                                     \
                        sBuffer[ACP_MIN(sLen, sSize - 1)] == 0,         \
                        ("acpSnprintf(\"%s\", %zd) did not correctly "  \
                         "null-terminate in case " #aTestName ":%d",    \
                         (aTestFormat)[i].mFormat,                        \
                         sSize,                                         \
                         i));                                           \
                    ACT_CHECK_DESC(                                     \
                        strncmp((aTestFormat)[i].mResult,                 \
                                sBuffer,                                \
                                sSize - 1) == 0,                        \
                        ("acpSnprintf(\"%s\", %zd) should generate "    \
                         "\"%.*s\" but \"%s\" "                         \
                         "in case " #aTestName ":%d",                   \
                         (aTestFormat)[i].mFormat,                        \
                         sSize,                                         \
                         (acp_sint32_t)sSize - 1,                       \
                         (aTestFormat)[i].mResult,                        \
                         sBuffer,                                       \
                         i));                                           \
                }                                                       \
                else                                                    \
                {                                                       \
                }                                                       \
                                                                        \
                if (sMaxLen < sLen)                                     \
                {                                                       \
                    sMaxLen = sLen;                                     \
                }                                                       \
                else                                                    \
                {                                                       \
                }                                                       \
            }                                                           \
                                                                        \
            if (sSize  > (sMaxLen + 10))                                \
            {                                                           \
                break;                                                  \
            }                                                           \
            else                                                        \
            {                                                           \
            }                                                           \
        }                                                               \
    } while (0)

#define TEST_VSNPRINTF_SIZE(aTestName, aTestFormat, aPrintfArgs)        \
    do                                                                  \
    {                                                                   \
        for (sSize = 0; sSize < BUFFER_SIZE; sSize++)                   \
        {                                                               \
            sMaxLen = 0;                                                \
                                                                        \
            for (i = 0; (aTestFormat)[i].mFormat != NULL; i++)          \
            {                                                           \
                sRC = testVsnprintfSize aPrintfArgs;                    \
                                                                        \
                ACT_CHECK_DESC(                                         \
                    strlen((aTestFormat)[i].mResult) == sWritten,       \
                    ("acpVsnprintfSize(\"%s\", %zd) should return %zd " \
                     "but %zd in case " #aTestName ":%d",               \
                     (aTestFormat)[i].mFormat,                          \
                     sSize,                                             \
                     strlen((aTestFormat)[i].mResult),                  \
                     sWritten,                                          \
                     i));                                               \
                                                                        \
                ACT_CHECK_DESC(                                         \
                    strlen((aTestFormat)[i].mResult) == sLen,           \
                    ("acpVsnprintfSize(\"%s\", %zd) should return %zd " \
                     "but %zd in case " #aTestName ":%d",               \
                     (aTestFormat)[i].mFormat,                          \
                     sSize,                                             \
                     strlen((aTestFormat)[i].mResult),                  \
                     sLen,                                              \
                     i));                                               \
                                                                        \
                if (CHECK_RC)                                           \
                {                                                       \
                    if (sLen < sSize)                                   \
                    {                                                   \
                        ACT_CHECK_DESC(                                 \
                            ACP_RC_IS_SUCCESS(sRC),                     \
                            ("acpVsnprintfSize(\"%s\", %zd) should return " \
                             "SUCCESS but %d "                          \
                             "in case " #aTestName ":%d",               \
                             (aTestFormat)[i].mFormat,                  \
                             sSize,                                     \
                             sRC,                                       \
                             i));                                       \
                    }                                                   \
                    else                                                \
                    {                                                   \
                        ACT_CHECK_DESC(                                 \
                            ACP_RC_IS_ETRUNC(sRC),                      \
                            ("acpVsnprintfSize(\"%s\", %zd) should return " \
                             "ETRUNC but %d in case " #aTestName ":%d", \
                             (aTestFormat)[i].mFormat,                  \
                             sSize,                                     \
                             sRC,                                       \
                             i));                                       \
                    }                                                   \
                }                                                       \
                else                                                    \
                {                                                       \
                }                                                       \
                                                                        \
                if (sSize > 0)                                          \
                {                                                       \
                    ACT_CHECK_DESC(                                     \
                        sBuffer[ACP_MIN(sLen, sSize - 1)] == 0,         \
                        ("acpVsnprintfSize(\"%s\", %zd) did not correctly " \
                         "null-terminate in case " #aTestName ":%d",    \
                         (aTestFormat)[i].mFormat,                      \
                         sSize,                                         \
                         i));                                           \
                    ACT_CHECK_DESC(                                     \
                        strncmp((aTestFormat)[i].mResult,               \
                                sBuffer,                                \
                                sSize - 1) == 0,                        \
                        ("acpVsnprintfSize(\"%s\", %zd) should generate " \
                         "\"%.*s\" but \"%s\" "                         \
                         "in case " #aTestName ":%d",                   \
                         (aTestFormat)[i].mFormat,                      \
                         sSize,                                         \
                         (acp_sint32_t)sSize - 1,                       \
                         (aTestFormat)[i].mResult,                      \
                         sBuffer,                                       \
                         i));                                           \
                }                                                       \
                else                                                    \
                {                                                       \
                }                                                       \
                                                                        \
                if (sMaxLen < sLen)                                     \
                {                                                       \
                    sMaxLen = sLen;                                     \
                }                                                       \
                else                                                    \
                {                                                       \
                }                                                       \
            }                                                           \
                                                                        \
            if (sSize  > (sMaxLen + 10))                                \
            {                                                           \
                break;                                                  \
            }                                                           \
            else                                                        \
            {                                                           \
            }                                                           \
        }                                                               \
    } while (0)

#define TEST_FPRINTF(aTestName, aTestFormat, aPrintfArgs)               \
    do                                                                  \
    {                                                                   \
        for (i = 0; (aTestFormat)[i].mFormat != NULL; i++)                \
        {                                                               \
            acp_char_t *sCmpBuffer = NULL;                              \
            acp_file_t  sCmpFile;                                       \
            acp_stat_t  sCmpStat;                                       \
                                                                        \
            sRC = acpStdOpen(&sFile, TEST_OUT_FILE, "w+");              \
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                          \
                                                                        \
            sRC = FPRINTF aPrintfArgs;                                  \
                                                                        \
            (void)acpStdClose(&sFile);                                   \
                                                                        \
            ACT_CHECK_DESC(                                             \
                strlen((aTestFormat)[i].mResult) == sLen,                 \
                ("acpPrintf(\"%s\") should return %zd "                 \
                 "but %zd in case " #aTestName ":%d",                   \
                 (aTestFormat)[i].mFormat,                                \
                 strlen((aTestFormat)[i].mResult),                        \
                 sLen,                                                  \
                 i));                                                   \
                                                                        \
            if (CHECK_RC)                                               \
            {                                                           \
                ACT_CHECK_DESC(                                         \
                    ACP_RC_IS_SUCCESS(sRC),                             \
                    ("acpPrintf(\"%s\") should return "                 \
                     "SUCCESS but %d in case " #aTestName ":%d",        \
                     (aTestFormat)[i].mFormat,                            \
                     sRC,                                               \
                     i));                                               \
            }                                                           \
            else                                                        \
            {                                                           \
            }                                                           \
                                                                        \
            sRC = acpFileOpen(&sCmpFile,                                \
                              gTestOutFile,                            \
                              ACP_O_RDONLY,                             \
                              0);                                       \
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                          \
                                                                        \
            sRC = acpFileStat(&sCmpFile, &sCmpStat);                    \
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                          \
            ACT_CHECK((acp_size_t)sCmpStat.mSize == sLen);              \
                                                                        \
            sRC = acpMemCalloc((void **)&sCmpBuffer, sLen + 1, 1);      \
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                          \
                                                                        \
            sRC = acpFileRead(&sCmpFile, sCmpBuffer, sLen, NULL);       \
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                          \
                                                                        \
            ACT_CHECK_DESC(acpMemCmp(sCmpBuffer,                        \
                                     (aTestFormat)[i].mResult,            \
                                     sLen) == 0,                        \
                           ("acpPrintf(\"%s\") should generate \"%s\" " \
                            "but \"%s\" in case " #aTestName ":%d",     \
                            (aTestFormat)[i].mFormat,                     \
                            (aTestFormat)[i].mResult,                     \
                            sCmpBuffer,                                 \
                            i));                                        \
                                                                        \
            acpMemFree(sCmpBuffer);                                     \
                                                                        \
            (void)acpFileClose(&sCmpFile);                              \
        }                                                               \
    } while (0)

#define TEST_PRINTF(aTestName)                                          \
    do                                                                  \
    {                                                                   \
        acp_sint32_t i_MACRO_LOCAL_VAR;                                 \
                                                                            \
        for (i_MACRO_LOCAL_VAR = 0;                                         \
             g ## aTestName[i_MACRO_LOCAL_VAR].mFormat != NULL;             \
             i_MACRO_LOCAL_VAR++)                                           \
        {                                                                   \
            sRC = SNPRINTF(sBuffer,                                         \
                           sizeof(sBuffer),                                 \
                           g ## aTestName[i_MACRO_LOCAL_VAR].mFormat,       \
                           g ## aTestName[i_MACRO_LOCAL_VAR].mValue,        \
                           &sLen);                                          \
                                                                            \
            if (CHECK_RC)                                                   \
            {                                                               \
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                          \
            }                                                               \
            else                                                            \
            {                                                               \
            }                                                               \
                                                                            \
            ACT_CHECK_DESC(strlen(g ## aTestName[i_MACRO_LOCAL_VAR].mResult) == \
                           sLen,                                            \
                           ("acpPrintf(\"%s\") should return %zu "          \
                            "but %zu in case " #aTestName ":%d",            \
                            g ## aTestName[i_MACRO_LOCAL_VAR].mFormat,      \
                            strlen(g ## aTestName[i_MACRO_LOCAL_VAR].mResult), \
                            sLen,                                           \
                            i_MACRO_LOCAL_VAR));                            \
                                                                            \
            ACT_CHECK_DESC(strcmp(g ## aTestName[i_MACRO_LOCAL_VAR].mResult, \
                                  sBuffer) == 0,                            \
                           ("acpPrintf(\"%s\") should generate \"%s\" "     \
                            "but \"%s\" in case " #aTestName ":%d",         \
                            g ## aTestName[i_MACRO_LOCAL_VAR].mFormat,      \
                            g ## aTestName[i_MACRO_LOCAL_VAR].mResult,      \
                            sBuffer,                                        \
                            i_MACRO_LOCAL_VAR));                            \
        }                                                                   \
    } while (0)

static void testDouble(void)
{
    acp_double_t sValue;
    acp_sint32_t sExp;
    acp_char_t   sStrInt[32];
    acp_char_t   sStrDouble[32];

    /* Test exponent */
    for(sExp = -307, sValue=1.23E-307; sExp <= 308; sExp++)
    {
        acpSnprintf(sStrDouble, 31, "%g", sValue);

        if(sExp >= -4 && sExp <= 5)
        {
        }
        else
        {
            if(sExp > 0)
            {
                acpSnprintf(sStrInt, 31, "1.23e+%02d", sExp);
            }
            else
            {
                acpSnprintf(sStrInt, 31, "1.23e-%02d", -sExp);
            }

            ACT_CHECK_DESC(
                0 == acpCStrCmp(sStrDouble, sStrInt, 32),
                ("Conversion from 10^%d should be %s but %s\n",
                 sExp, sStrInt, sStrDouble)
                );
        }

        if(sExp < 308)
	{
            sValue *= 10.;
	}
	else
	{
	    /* To prevent SIGFPE */
	}
    }
}

/*
 * An adapter function to help testing acpVsnprintf2()
 */
acp_rc_t testVsnprintfSize(acp_char_t       *aBuffer,
                           acp_size_t        aSize,
                           acp_size_t       *aWritten,
                           const acp_char_t *aFormat,
                           ...)
{
    va_list  ap;
    acp_rc_t sRC;

    va_start(ap, aFormat);
    sRC = acpVsnprintfSize(aBuffer, aSize, aWritten, aFormat, ap);
    va_end(ap);

    return sRC;
}

acp_sint32_t main(void)
{
    void*           sPtr;
    acp_std_file_t  sFile;
    acp_char_t      sBuffer[BUFFER_SIZE];
    acp_sint8_t     sSInt8  = -123;
    acp_uint8_t     sUInt8  = 123;
    acp_sint16_t    sSInt16 = -12345;
    acp_uint16_t    sUInt16 = 12345;
    acp_sint32_t    sSInt32 = -1234567890;
    acp_uint32_t    sUInt32 = 1234567890;
    acp_sint64_t    sSInt64 = ACP_SINT64_LITERAL(-1234567890123456789);
    acp_uint64_t    sUInt64 = ACP_UINT64_LITERAL(1234567890123456789);
    acp_size_t      sLen    = 0;
    acp_size_t      sMaxLen;
    acp_size_t      sSize;
    acp_rc_t        sRC;
    acp_sint32_t    i;
    acp_size_t      sWritten; /* acpVsnprintfSize() */

    ACT_TEST_BEGIN();

#if !defined(ALTI_CFG_OS_TRU64)
    for (i = 0; gTestDouble[i].mFormat != NULL; i++)
    {
        acp_double_t sZero   = 0.0;

        if (gTestDouble[i].mValue == FLOAT_PINF)
        {
            gTestDouble[i].mValue = FLOAT_PINF_VALUE;
        }
        else
        {
            /* do nothing */
        }
        if (gTestDouble[i].mValue == FLOAT_NINF)
        {
            gTestDouble[i].mValue = FLOAT_NINF_VALUE;
        }
        else
        {
            /* do nothing */
        }
        if (gTestDouble[i].mValue == FLOAT_NAN)
        {
            gTestDouble[i].mValue = FLOAT_NAN_VALUE;
        }
        else
        {
            /* do nothing */
        }
    }
#endif

    /*
     * basic
     */
    TEST_SNPRINTF(Basic, gTestBasic,
                  (sBuffer, sSize, gTestBasic[i].mFormat, &sLen));

    TEST_FPRINTF(Basic, gTestBasic, (&sFile, gTestBasic[i].mFormat, &sLen));

    TEST_VSNPRINTF_SIZE(Basic, gTestBasic,
                        (sBuffer, sSize, &sWritten, gTestBasic[i].mFormat, &sLen));

    /*
     * int types
     */
    TEST_SNPRINTF(IntType, gTestIntType,
                  (sBuffer, sSize, gTestIntType[i].mFormat, sSInt8, sUInt8,
                   sSInt16, sUInt16, sSInt32, sUInt32, sSInt64, sUInt64,
                   &sLen));

    TEST_FPRINTF(IntType, gTestIntType,
                 (&sFile, gTestIntType[i].mFormat, sSInt8, sUInt8,
                  sSInt16, sUInt16, sSInt32, sUInt32, sSInt64, sUInt64, &sLen));

    TEST_VSNPRINTF_SIZE(IntType, gTestIntType,
                        (sBuffer, sSize, &sWritten, gTestIntType[i].mFormat, sSInt8, sUInt8,
                         sSInt16, sUInt16, sSInt32, sUInt32, sSInt64, sUInt64,
                         &sLen));

    /*
     * format for each data type
     */
    TEST_PRINTF(TestInt);
    TEST_PRINTF(TestFloat);
    TEST_PRINTF(TestDouble);
    TEST_PRINTF(TestChar);
    TEST_PRINTF(TestString);

    /*
     * 'p'
     */
    sPtr = NULL;
    sRC = acpSnprintf(sBuffer, sizeof(sBuffer), "%p%zn", sPtr, &sLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(strcmp("0", sBuffer) == 0);
    ACT_CHECK(sLen == 1);

    sPtr = (void *)1;
    sRC = acpSnprintf(sBuffer, sizeof(sBuffer), "%p%zn", sPtr, &sLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(strcmp("0x1", sBuffer) == 0);
    ACT_CHECK(sLen == 3);

    /*
     * unknown specifier
     */
    sRC = acpSnprintf(sBuffer, sizeof(sBuffer),
                      "%d %*.*b %%%*d%%%zn", 1, 10, 100, &sLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(strcmp("1 %*.*b %       100%", sBuffer) == 0);
    ACT_CHECK(sLen == 20);

    (void)acpFileRemove(gTestOutFile);

    testDouble();

    ACT_TEST_END();

    return 0;
}
