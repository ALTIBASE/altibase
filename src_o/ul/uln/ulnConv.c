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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnCharSet.h>
#include <ulnConv.h>
#include <ulnConvToCHAR.h>
#include <ulnConvToWCHAR.h>
#include <ulnConvToSLONG.h>
#include <ulnConvToULONG.h>
#include <ulnConvToSSHORT.h>
#include <ulnConvToUSHORT.h>
#include <ulnConvToSTINYINT.h>
#include <ulnConvToUTINYINT.h>
#include <ulnConvToSBIGINT.h>
#include <ulnConvToUBIGINT.h>
#include <ulnConvToBIT.h>
#include <ulnConvToFLOAT.h>
#include <ulnConvToDOUBLE.h>
#include <ulnConvToBINARY.h>
#include <ulnConvToDATE.h>
#include <ulnConvToTIME.h>
#include <ulnConvToTIMESTAMP.h>
#include <ulnConvToNUMERIC.h>
#include <ulnConvToLOCATOR.h>
#include <ulnConvToFILE.h>

static ulnConvFunction *ulnConvTable_MTYPE_SQLC [ULN_CTYPE_MAX] [ULN_MTYPE_MAX] =
{
    /* ULN_CTYPE_NULL */
    {
    ulncNULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL
    },

    /* ULN_CTYPE_DEFAULT */
    {
    ulncNULL,
    ulncCHAR_CHAR,         ulncVARCHAR_CHAR,      ulncNUMERIC_CHAR,      ulncNUMERIC_CHAR,
    ulncBIT_BIT,           ulncSMALLINT_SSHORT,   ulncINTEGER_SLONG,     ulncBIGINT_SBIGINT,
                           // bug-21574
    ulncREAL_FLOAT,        ulncNUMERIC_DOUBLE,    ulncDOUBLE_DOUBLE,     ulncBINARY_BINARY,
    ulncVARBIT_CHAR,       ulncNIBBLE_CHAR,       ulncBYTE_BINARY,       ulncBYTE_BINARY,
    ulncDATE_TIMESTAMP,    NULL,                  NULL,
    NULL,                  ulncBLOB_BINARY,       ulncCLOB_CHAR,         NULL,
    NULL,                  ulncBINARY_BINARY,     ulncNCHAR_WCHAR,       ulncNVARCHAR_WCHAR
    },

    /* ULN_CTYPE_CHAR */
    {
    ulncNULL,
    ulncCHAR_CHAR,         ulncVARCHAR_CHAR,      ulncNUMERIC_CHAR,      ulncNUMERIC_CHAR,
    ulncBIT_CHAR,          ulncSMALLINT_CHAR,     ulncINTEGER_CHAR,      ulncBIGINT_CHAR,
    ulncREAL_CHAR,         ulncNUMERIC_CHAR,      ulncDOUBLE_CHAR,       ulncBINARY_CHAR,
    ulncVARBIT_CHAR,       ulncNIBBLE_CHAR,       ulncBYTE_CHAR,         ulncBYTE_CHAR,
    ulncDATE_CHAR,         NULL,                  NULL,
    ulncINTERVAL_CHAR,     ulncBLOB_CHAR,         ulncCLOB_CHAR,         NULL,
    NULL,                  ulncBINARY_CHAR,       ulncNCHAR_CHAR,        ulncNVARCHAR_CHAR
    },

    /* ULN_CTYPE_NUMERIC */
    {
    ulncNULL,
    ulncCHAR_NUMERIC,      ulncVARCHAR_NUMERIC,   ulncNUMERIC_NUMERIC,   ulncNUMERIC_NUMERIC,
    ulncBIT_NUMERIC,       ulncSMALLINT_NUMERIC,  ulncINTEGER_NUMERIC,   ulncBIGINT_NUMERIC,
    ulncREAL_NUMERIC,      ulncNUMERIC_NUMERIC,   ulncDOUBLE_NUMERIC,    NULL,
    ulncVARBIT_NUMERIC,    NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_NUMERIC,  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_NUMERIC,     ulncNVARCHAR_NUMERIC
    },

    /* ULN_CTYPE_BIT */
    {
    ulncNULL,
    ulncCHAR_BIT,          ulncVARCHAR_BIT,       ulncNUMERIC_BIT,       ulncNUMERIC_BIT,
    ulncBIT_BIT,           ulncSMALLINT_BIT,      ulncINTEGER_BIT,       ulncBIGINT_BIT,
    ulncREAL_BIT,          ulncNUMERIC_BIT,       ulncDOUBLE_BIT,        NULL,
    ulncVARBIT_BIT,        NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_BIT,      NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_BIT,         ulncNVARCHAR_BIT
    },

    /* ULN_CTYPE_STINYINT */
    {
    ulncNULL,
    ulncCHAR_STINYINT,     ulncVARCHAR_STINYINT,  ulncNUMERIC_STINYINT,  ulncNUMERIC_STINYINT,
    ulncBIT_STINYINT,      ulncSMALLINT_STINYINT, ulncINTEGER_STINYINT,  ulncBIGINT_STINYINT,
    ulncREAL_STINYINT,     ulncNUMERIC_STINYINT,  ulncDOUBLE_STINYINT,   NULL,
    ulncVARBIT_STINYINT,   NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_STINYINT, NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_STINYINT,    ulncNVARCHAR_STINYINT
    },

    /* ULN_CTYPE_UTINYINT */
    {
    ulncNULL,
    ulncCHAR_UTINYINT,     ulncVARCHAR_UTINYINT,  ulncNUMERIC_UTINYINT,  ulncNUMERIC_UTINYINT,
    ulncBIT_UTINYINT,      ulncSMALLINT_UTINYINT, ulncINTEGER_UTINYINT,  ulncBIGINT_UTINYINT,
    ulncREAL_UTINYINT,     ulncNUMERIC_UTINYINT,  ulncDOUBLE_UTINYINT,   NULL,
    ulncVARBIT_UTINYINT,   NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_UTINYINT, NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_UTINYINT,    ulncNVARCHAR_UTINYINT
    },

    /* ULN_CTYPE_SSHORT */
    {
    ulncNULL,
    ulncCHAR_SSHORT,       ulncVARCHAR_SSHORT,    ulncNUMERIC_SSHORT,    ulncNUMERIC_SSHORT,
    ulncBIT_SSHORT,        ulncSMALLINT_SSHORT,   ulncINTEGER_SSHORT,    ulncBIGINT_SSHORT,
    ulncREAL_SSHORT,       ulncNUMERIC_SSHORT,    ulncDOUBLE_SSHORT,     NULL,
    ulncVARBIT_SSHORT,     NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_SSHORT,   NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_SSHORT,      ulncVARCHAR_SSHORT
    },

    /* ULN_CTYPE_USHORT */
    {
    ulncNULL,
    ulncCHAR_USHORT,       ulncVARCHAR_USHORT,    ulncNUMERIC_USHORT,    ulncNUMERIC_USHORT,
    ulncBIT_USHORT,        ulncSMALLINT_USHORT,   ulncINTEGER_USHORT,    ulncBIGINT_USHORT,
    ulncREAL_USHORT,       ulncNUMERIC_USHORT,    ulncDOUBLE_USHORT,     NULL,
    ulncVARBIT_USHORT,     NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_USHORT,   NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_USHORT,      ulncNVARCHAR_USHORT
    },

    /* ULN_CTYPE_SLONG */
    {
    ulncNULL,
    ulncCHAR_SLONG,        ulncVARCHAR_SLONG,     ulncNUMERIC_SLONG,     ulncNUMERIC_SLONG,
    ulncBIT_SLONG,         ulncSMALLINT_SLONG,    ulncINTEGER_SLONG,     ulncBIGINT_SLONG,
    ulncREAL_SLONG,        ulncNUMERIC_SLONG,     ulncDOUBLE_SLONG,      NULL,
    ulncVARBIT_SLONG,      NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_SLONG,    NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_SLONG,       ulncNVARCHAR_SLONG
    },

    /* ULN_CTYPE_ULONG */
    {
    ulncNULL,
    ulncCHAR_ULONG,        ulncVARCHAR_ULONG,     ulncNUMERIC_ULONG,     ulncNUMERIC_ULONG,
    ulncBIT_ULONG,         ulncSMALLINT_ULONG,    ulncINTEGER_ULONG,     ulncBIGINT_ULONG,
    ulncREAL_ULONG,        ulncNUMERIC_ULONG,     ulncDOUBLE_ULONG,      NULL,
    ulncVARBIT_ULONG,      NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_ULONG,    NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_ULONG,       ulncNVARCHAR_ULONG
    },

    /* ULN_CTYPE_SBIGINT */
    {
    ulncNULL,
    ulncCHAR_SBIGINT,        ulncVARCHAR_SBIGINT,     ulncNUMERIC_SBIGINT,     ulncNUMERIC_SBIGINT,
    ulncBIT_SBIGINT,         ulncSMALLINT_SBIGINT,    ulncINTEGER_SBIGINT,     ulncBIGINT_SBIGINT,
    ulncREAL_SBIGINT,        ulncNUMERIC_SBIGINT,     ulncDOUBLE_SBIGINT,      NULL,
    ulncVARBIT_SBIGINT,      NULL,                    NULL,                    NULL,
    NULL,                    NULL,                    NULL,
    ulncINTERVAL_SBIGINT,    NULL,                    NULL,                    NULL,
    NULL,                    NULL,                    ulncNCHAR_SBIGINT,       ulncNVARCHAR_SBIGINT
    },

    /* ULN_CTYPE_UBIGINT */
    {
    ulncNULL,
    ulncCHAR_UBIGINT,        ulncVARCHAR_UBIGINT,     ulncNUMERIC_UBIGINT,     ulncNUMERIC_UBIGINT,
    ulncBIT_UBIGINT,         ulncSMALLINT_UBIGINT,    ulncINTEGER_UBIGINT,     ulncBIGINT_UBIGINT,
    ulncREAL_UBIGINT,        ulncNUMERIC_UBIGINT,     ulncDOUBLE_UBIGINT,      NULL,
    ulncVARBIT_UBIGINT,      NULL,                    NULL,                    NULL,
    NULL,                    NULL,                    NULL,
    ulncINTERVAL_UBIGINT,    NULL,                    NULL,                    NULL,
    NULL,                    NULL,                    ulncNCHAR_UBIGINT,       ulncNVARCHAR_UBIGINT
    },

    /* ULN_CTYPE_FLOAT */
    {
    ulncNULL,
    ulncCHAR_FLOAT,        ulncVARCHAR_FLOAT,     ulncNUMERIC_FLOAT,     ulncNUMERIC_FLOAT,
    ulncBIT_FLOAT,         ulncSMALLINT_FLOAT,    ulncINTEGER_FLOAT,     ulncBIGINT_FLOAT,
    ulncREAL_FLOAT,        ulncNUMERIC_FLOAT,     ulncDOUBLE_FLOAT,      NULL,
    ulncVARBIT_FLOAT,      NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_FLOAT,    NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_FLOAT,       ulncNVARCHAR_FLOAT
    },

    /* ULN_CTYPE_DOUBLE */
    {
    ulncNULL,
    ulncCHAR_DOUBLE,       ulncVARCHAR_DOUBLE,    ulncNUMERIC_DOUBLE,    ulncNUMERIC_DOUBLE,
    ulncBIT_DOUBLE,        ulncSMALLINT_DOUBLE,   ulncINTEGER_DOUBLE,    ulncBIGINT_DOUBLE,
    ulncREAL_DOUBLE,       ulncNUMERIC_DOUBLE,    ulncDOUBLE_DOUBLE,     NULL,
    ulncVARBIT_DOUBLE,     NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    ulncINTERVAL_DOUBLE,   NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_DOUBLE,      ulncNVARCHAR_DOUBLE
    },

    /* ULN_CTYPE_BINARY */
    {
    ulncNULL,
    ulncCHAR_BINARY,       ulncVARCHAR_BINARY,    ulncNUMERIC_BINARY,    ulncNUMERIC_BINARY,
    ulncBIT_BINARY,        ulncSMALLINT_BINARY,   ulncINTEGER_BINARY,    ulncBIGINT_BINARY,
    ulncREAL_BINARY,       ulncNUMERIC_BINARY,    ulncDOUBLE_BINARY,     ulncBINARY_BINARY,
    ulncVARBIT_BINARY,     ulncNIBBLE_BINARY,     ulncBYTE_BINARY,       ulncBYTE_BINARY,
    ulncDATE_BINARY,       NULL,                  NULL,
    ulncINTERVAL_BINARY,   ulncBLOB_BINARY,       ulncCLOB_BINARY,       NULL,
    NULL,                  ulncBINARY_BINARY,     ulncNCHAR_BINARY,      ulncNVARCHAR_BINARY
    },

    /* ULN_CTYPE_DATE */
    {
    ulncNULL,
    ulncCHAR_DATE,         ulncVARCHAR_DATE,      NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    ulncDATE_DATE,         NULL,                  NULL,
    ulncINTERVAL_DATE,     NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_DATE,        ulncNVARCHAR_DATE
    },

    /* ULN_CTYPE_TIME */
    {
    ulncNULL,
    ulncCHAR_TIME,         ulncVARCHAR_TIME,      NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    ulncDATE_TIME,         NULL,                  NULL,
    ulncINTERVAL_TIME,     NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_TIME,        ulncNVARCHAR_TIME
    },

    /* ULN_CTYPE_TIMESTAMP */
    {
    ulncNULL,
    ulncCHAR_TIMESTAMP,    ulncVARCHAR_TIMESTAMP, NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    ulncDATE_TIMESTAMP,    NULL,                  NULL,
    ulncINTERVAL_TIMESTAMP,NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  ulncNCHAR_TIMESTAMP,   ulncNVARCHAR_TIMESTAMP
    },

    /* ULN_CTYPE_INTERVAL */
    {
    ulncNULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL
    },

    /* ULN_CTYPE_BLOB_LOCATOR */
    {
    ulncNULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    NULL,                  ulncBLOB_LOCATOR,      ulncCLOB_LOCATOR,      ulncBLOB_LOCATOR,
    NULL,                  NULL,                  NULL,                  NULL
    },

    /* ULN_CTYPE_CLOB_LOCATOR */
    {
    ulncNULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    NULL,                  ulncBLOB_LOCATOR,      ulncCLOB_LOCATOR,      NULL,
    ulncCLOB_LOCATOR,      NULL,                  NULL,                  NULL
    },

    /* ULN_CTYPE_FILE */
    {
    ulncNULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,                  NULL,
    NULL,                  NULL,                  NULL,
    NULL,                  ulncBLOB_FILE,         ulncCLOB_FILE,         NULL,
    NULL,                  NULL,                  NULL,                  NULL
    },

    /* ULN_CTYPE_WCHAR */
    {
    ulncNULL,
    ulncCHAR_WCHAR,        ulncVARCHAR_WCHAR,     ulncNUMERIC_WCHAR,     ulncNUMERIC_WCHAR,
    ulncBIT_WCHAR,         ulncSMALLINT_WCHAR,    ulncINTEGER_WCHAR,     ulncBIGINT_WCHAR,
    ulncREAL_WCHAR,        ulncNUMERIC_WCHAR,     ulncDOUBLE_WCHAR,      ulncBINARY_WCHAR,
    ulncVARBIT_WCHAR,      ulncNIBBLE_WCHAR,      ulncBYTE_WCHAR,        ulncBYTE_WCHAR,
    ulncDATE_WCHAR,        NULL,                  NULL,
    ulncINTERVAL_WCHAR,    ulncBLOB_WCHAR,        ulncCLOB_WCHAR,        NULL,
    NULL,                  ulncBINARY_WCHAR,      ulncNCHAR_WCHAR,       ulncNVARCHAR_WCHAR
    }
};

static ulnConvEndianFunc *ulnConvEndianMap [2] =
{
    ulnConvEndian_ADJUST,
    ulnConvEndian_NONE
};

#define ULN_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

/*
 * Note : 아래의 함수는 Strtoll() 이라는 cli2 의 함수를 그대로 가져다가 복사해 둔 것이다.
 *
 * BUGBUG : 스트링의 경계검사를 안한다 -_-;
 */

// This code from gnu c++ library
// to check the boundary
#ifndef ULONG_LONG_MAX
#define	ULONG_LONG_MAX	((acp_uint64_t)(0xFFFFFFFFFFFFFFFFLL))
#endif

#ifndef LONG_LONG_MAX
#define	LONG_LONG_MAX	((acp_uint64_t)(ULONG_LONG_MAX >> 1))
#endif

#ifndef LONG_LONG_MIN
#define	LONG_LONG_MIN	((acp_uint64_t)(~LONG_LONG_MAX))
#endif

acp_sint64_t ulncStrToSLong(const acp_char_t *aString, acp_char_t **aEndPointer, acp_sint32_t aBase)
{
    const acp_char_t *sCurrentPointer;
    acp_uint64_t      sCurrentSLongValue;
    acp_sint32_t      sCurrentCharacter;
    acp_uint64_t      sCutOff;
    acp_sint32_t      sCutLimit;

    acp_sint32_t      sSign = 0;
    acp_sint32_t      sAnyDigitsConsumed;

    sCurrentPointer = aString;

    /*
     * 화이트 스페이스 없애기
     */
    do
    {
        sCurrentCharacter = *sCurrentPointer++;
    } while (sCurrentCharacter == ' ' || sCurrentCharacter == '\t');

    /*
     * 부호 체크
     */
    if (sCurrentCharacter == '-')
    {
        sSign = 1;
        sCurrentCharacter = *sCurrentPointer++;
    }
    else if (sCurrentCharacter == '+')
    {
        sCurrentCharacter = *sCurrentPointer++;
    }

#if 0
    /*
     * 0x 체크 혹은 8진수 0 체크 : 필요없어서 일단 주석처리
     */
    if ((aBase == 0 || aBase == 16) &&
        sCurrentCharacter == '0' && (*sCurrentPointer == 'x' || *sCurrentPointer == 'X'))
    {
        sCurrentCharacter = sCurrentPointer[1];
        sCurrentPointer += 2;
        aBase = 16;
    }
    if (aBase == 0)
    {
        aBase = sCurrentCharacter == '0' ? 8 : 10;
    }
#endif

    /*
     * Compute the sCutOff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * aBase.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for long longs is
     * [-2147483648..2147483647] and the input aBase is 10,
     * sCutOff will be set to 214748364 and sCutLimit to either
     * 7 (sSign==0) or 8 (sSign==1), meaning that if we have accumulated
     * a value > 214748364, or equal but the next digit is > 7 (or 8),
     * the number is too big, and we will return a range error.
     *
     * Set sAnyDigitsConsumed if sAnyDigitsConsumed `digits' consumed; make it negative to indicate
     * overflow.
     */
    sCutOff    = sSign ? -(acp_uint64_t)LONG_LONG_MIN : LONG_LONG_MAX;
    sCutLimit  = sCutOff % (acp_uint64_t)aBase;
    sCutOff   /= (acp_uint64_t)aBase;

    for (sCurrentSLongValue = 0, sAnyDigitsConsumed = 0; ;sCurrentCharacter = *sCurrentPointer++)
    {
        if (ULN_IS_DIGIT(sCurrentCharacter))
        {
            sCurrentCharacter -= '0';
        }
        else
        {
            break;
        }

        if (sCurrentCharacter >= aBase)
        {
            break;
        }

        if ((sAnyDigitsConsumed < 0) ||
            (sCurrentSLongValue > sCutOff) ||
            ((sCurrentSLongValue == sCutOff) && (sCurrentCharacter > sCutLimit)))
        {
            sAnyDigitsConsumed = -1;
        }
        else
        {
            sAnyDigitsConsumed  = 1;
            sCurrentSLongValue *= aBase;
            sCurrentSLongValue += sCurrentCharacter;
        }
    }

    if (sAnyDigitsConsumed < 0)
    {
        sCurrentSLongValue = sSign ? LONG_LONG_MIN : LONG_LONG_MAX;
        errno = ERANGE;
    }
    else if (sSign)
    {
        sCurrentSLongValue = -sCurrentSLongValue;
    }

    if (aEndPointer != 0)
    {
        *aEndPointer = (acp_char_t *)(sAnyDigitsConsumed ? sCurrentPointer - 1 : aString);
    }

    return sCurrentSLongValue;
}

/*
 * Note : 4.3.9 의 strtonumCheck() 함수를 그대로 복사해 와서
 *
 *          경계검사를 함
 *          trailing white space 도 적법한 숫자문자열이라고 판단함
 *
 *        으로 수정함.
 *
 * 참조 : ODBC 의 Numeric Literal 은 다음과 같은 문법을 가진다 :
 *
 *  numeric-literal       ::= signed-numeric-literal | unsigned-numeric-literal
 *
 *  signed-numeric-literal   ::= [sign] unsigned-numeric-literal
 *
 *  unsigned-numeric-literal ::= exact-numeric-literal | approximate-numeric-literal
 *
 *  exact-numeric-literal ::= unsigned-integer [period[unsigned-integer]] | period unsigned-integer
 *
 *  approximate-numeric-literal ::= mantissa E exponent
 *  mantissa                    ::= exact-numeric-literal
 *  exponent                    ::= signed-integer
 *
 *  signed-integer   ::= [sign] unsigned-integer
 *  unsigned-integer ::= digit...
 *
 *  sign             ::= plus-sign | minus-sign
 *  plus-sign        ::= +
 *  minus-sign       ::= -
 *  digit            ::= 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0
 *  period           ::= .
 */

acp_bool_t ulncIsValidNumericLiterals(acp_char_t *aString, acp_uint32_t aBufferSize, acp_sint32_t *aScale)
{
    acp_bool_t   sReturnValue   = ACP_FALSE;

    acp_sint32_t sScaleIncrease = 0;  // 소숫점 이하의 숫자인지의 여부
    acp_sint32_t sScale         = 0;  // 소숫점 이하의 숫자의 갯수

    acp_sint32_t sExponentSign  = 1;
    acp_sint32_t sExponent      = 0;

    acp_uint32_t sPos = 0;

    ACI_TEST_RAISE(aString[sPos] == '\0', NUMERIC_LITERAL);

    while(aString[sPos] == ' ' || aString[sPos] == '\t')
    {
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NUMERIC_LITERAL);
    }

    // [+/-]
    if (aString[sPos] == '-' || aString[sPos] == '+')
    {
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NOT_NUMERIC_LITERAL);
    }

    // 맨 앞 혹은 부호 다음에는 반드시 피리어드혹은 숫자가 와야 한다.
    if (aString[sPos] == '.')
    {
        sScaleIncrease = 1;
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NOT_NUMERIC_LITERAL);
    }
    else
    {
        ACI_TEST_RAISE(aString[sPos] < '0' || aString[sPos] > '9', NOT_NUMERIC_LITERAL);
    }

    // [0-9]
    while(aString[sPos] >= '0' && aString[sPos] <= '9')
    {
        sScale += sScaleIncrease;
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NUMERIC_LITERAL);
    }

    if (aString[sPos] == '.')
    {
        // . 이 두번 나오면 안된다.
        ACI_TEST_RAISE(sScaleIncrease != 0, NOT_NUMERIC_LITERAL);

        sScaleIncrease = 1;
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NUMERIC_LITERAL);

        while(aString[sPos] >= '0' && aString[sPos] <= '9')
        {
            sScale += sScaleIncrease;
            sPos++;
            ACI_TEST_RAISE(sPos >= aBufferSize, NUMERIC_LITERAL);
        }
    }

    // . 바로 뒤에 e 가 와도 된다.
    if ((aString[sPos] == 'E') || (aString[sPos] == 'e'))
    {
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NOT_NUMERIC_LITERAL);

        if (aString[sPos] == '+')
        {
            sPos++;
            ACI_TEST_RAISE(sPos >= aBufferSize, NOT_NUMERIC_LITERAL);
        }
        else
        {
            if (aString[sPos] == '-')
            {
                sExponentSign = -1;
                sPos++;
                ACI_TEST_RAISE(sPos >= aBufferSize, NOT_NUMERIC_LITERAL);
            }
            else
            {
                // e 다음에는 부호 혹은 숫자가 와야만 한다.
                ACI_TEST_RAISE(aString[sPos] < '0' || aString[sPos] > '9', NOT_NUMERIC_LITERAL);
            }
        }

        while(aString[sPos] >= '0' && aString[sPos] <= '9')
        {
            sExponent = (sExponent * 10) + aString[sPos] - '0';
            sPos++;
            ACI_TEST_RAISE(sPos >= aBufferSize, NUMERIC_LITERAL);
        }
    }

    /*
     * 여기까지 숫자부분이다.
     */

    while(aString[sPos] == ' ' || aString[sPos] == '\t')
    {
        sPos++;
        ACI_TEST_RAISE(sPos >= aBufferSize, NUMERIC_LITERAL);
    }

    ACI_RAISE(NOT_NUMERIC_LITERAL);

    return sReturnValue;

    ACI_EXCEPTION(NOT_NUMERIC_LITERAL)
    {
        sReturnValue = ACP_FALSE;
    }

    ACI_EXCEPTION(NUMERIC_LITERAL)
    {
        sReturnValue = ACP_TRUE;
    }

    ACI_EXCEPTION_END;

    sExponent *= sExponentSign;

    if (sExponent >= sScale)
    {
        *aScale = 0;
    }
    else
    {
        *aScale = sScale - sExponent;
    }

    return sReturnValue;
}

/*
 * SrcBuffer 의 내용을 DstBuffer 로 복사한다.
 * 복사한 양을 리턴한다.
 */
acp_uint32_t ulnConvCopy(acp_uint8_t *aDstBuffer, acp_uint32_t aDstSize, acp_uint8_t *aSrcBuffer, acp_uint32_t aSrcLength)
{
    acp_uint32_t sSizeToCopy;

    sSizeToCopy = ACP_MIN(aDstSize, aSrcLength);

    if (aDstBuffer == NULL)
    {
        sSizeToCopy = 0;
    }
    else
    {
        if (sSizeToCopy > 0)
        {
            acpMemCpy(aDstBuffer, aSrcBuffer, sSizeToCopy);
        }
    }

    return sSizeToCopy;
}

// BUG-27515: ulnConvCopyToChar(), ulnConvCopyToWChar() 코드 중복 제거
ACI_RC ulnConvCopyStr(ulnFnContext  *aFnContext,
                      mtlModule     *aSrcCharSet,
                      mtlModule     *aDestCharSet,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      acp_char_t    *aSourceBuffer,
                      acp_uint32_t   aSourceLength,
                      ulnLengthPair *aLength)
{
    acp_uint32_t sDstSize  = 0;
    acp_uint32_t sCopySize = 0;
    ulnCharSet   sCharSet;
    acp_uint32_t sConvPosision = 0;
    acp_sint32_t sConvSrcSize  = 0;
    acp_sint32_t sConvDesSize  = 0;
    acp_sint32_t sConvOption   = CONV_DATA_OUT;

    if ((aAppBuffer->mBuffer == NULL) || (aAppBuffer->mBufferSize <= 0))
    {
        sDstSize = 0;
    }
    else
    {
        if (aDestCharSet->id == MTL_UTF16_ID)
        {
            // BUG-28110: WCHAR로 데이타를 얻을 때는 버퍼가 2의 배수여야한다.
            // 2의 배수가 아닐 경우에는 버퍼의 마지막 바이트를 무시
            if ((aAppBuffer->mBufferSize % 2) == 1)
            {
                aAppBuffer->mBufferSize--;
            }
            sDstSize = aAppBuffer->mBufferSize - 2;
        }
        else
        {
            sDstSize = aAppBuffer->mBufferSize - 1;
        }

        // 전에 다 못주고 남은게 있으면 그것먼저 복사
        if (aColumn->mRemainTextLen > 0)
        {
            sCopySize = ACP_MIN(sDstSize, aColumn->mRemainTextLen);
            acpMemCpy(aAppBuffer->mBuffer, aColumn->mRemainText, sCopySize);

            sDstSize                -= sCopySize;
            aColumn->mRemainTextLen -= sCopySize;
            if ((aColumn->mRemainTextLen > 0) && (sCopySize > 0))
            {
                acpMemCpy(aColumn->mRemainText,
                          aColumn->mRemainText + sCopySize,
                          aColumn->mRemainTextLen);
            }
        }
    }

    // CLOB은 일부만 가져와서 변환하므로
    // CLOB이 아닐 경우에만 전체 문자열 길이 옵션과 GDPosition을 처리를 한다.
    if (aColumn->mMtype != ULN_MTYPE_CLOB)
    {
        sConvOption  |= CONV_CALC_TOTSIZE;
        sConvPosision = aColumn->mGDPosition;
    }

    /* BUG-36775 CodeSonar : Buffer Overrun */
    ACI_TEST( sConvPosision > aSourceLength );

    // PROJ-1579 NCHAR
    // 클라이언트 캐릭터 셋으로 변환한다.
    // BUGBUG: 컨버전에 실패할 수 있으므로 리턴값을 무조건 무시하면 문제가 될수도 있다.
    // SUCCESS가 이닌 경우 SUCCESS_WITH_INFO인지 FAILURE인지 확인할 수 있어야 한다.

    if( (aSrcCharSet      != aDestCharSet) ||
        (aSrcCharSet->id  == MTL_UTF16_ID) ||
        (aDestCharSet->id == MTL_UTF16_ID) )
    {
        ulnCharSetInitialize(&sCharSet);

        (void)ulnCharSetConvertUseBuffer(&sCharSet,
                                        aFnContext,
                                        NULL,
                                        (const mtlModule*)aSrcCharSet,
                                        (const mtlModule*)aDestCharSet,
                                        (void*)(aSourceBuffer + sConvPosision),
                                        aSourceLength - sConvPosision,
                                        aAppBuffer->mBuffer + sCopySize,
                                        sDstSize,
                                        sConvOption);

        aLength->mNeeded         = sCharSet.mDestLen + sCopySize + aColumn->mRemainTextLen;
        aLength->mWritten        = sCharSet.mCopiedDesLen + sCopySize;
        aColumn->mGDPosition    += sCharSet.mConvedSrcLen;

        ulnCharSetFinalize(&sCharSet);

        if (sDstSize > 0)
        {
            ACE_ASSERT(aColumn->mRemainTextLen == 0);
            aColumn->mRemainTextLen  = sCharSet.mRemainTextLen;
            acpMemCpy(aColumn->mRemainText, sCharSet.mRemainText, aColumn->mRemainTextLen);
        }
    }
    else
    {
        sConvDesSize = aSourceLength - sConvPosision;
        sConvSrcSize = ACP_MIN(sConvDesSize, (acp_sint32_t)sDstSize);

        aLength->mNeeded         = sConvDesSize + sCopySize + aColumn->mRemainTextLen;
        aLength->mWritten        = sConvSrcSize + sCopySize;
        aColumn->mGDPosition    += sConvSrcSize;

        // BUG-27515: 동일한 케릭터셋일 경우에도 사용자 버퍼에 복사해야한다.
        acpMemCpy(aAppBuffer->mBuffer + sCopySize,
                  aSourceBuffer + sConvPosision,
                  sConvSrcSize);

        if (sDstSize > 0)
        {
            ACE_ASSERT(aColumn->mRemainTextLen == 0);
            aColumn->mRemainTextLen  = 0;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ulnConvFunction *ulnConvGetFilter(ulnCTypeID aCTYPE, ulnMTypeID aMTYPE)
{
    return ulnConvTable_MTYPE_SQLC[aCTYPE][aMTYPE];
}

ACI_RC ulncNULL(ulnFnContext  *aFnContext,
                ulnAppBuffer  *aAppBuffer,
                ulnColumn     *aColumn,
                ulnLengthPair *aLength,
                acp_uint16_t   aRowNumber)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aAppBuffer);
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aRowNumber);

    aLength->mWritten = SQL_NULL_DATA;
    aLength->mNeeded  = SQL_NULL_DATA;

    return ACI_SUCCESS;
}

/*
 * 여기 row number 는 1 베이스
 */
ACI_RC ulnConvert(ulnFnContext     *aFnContext,
                  ulnAppBuffer     *aUserBuffer,
                  ulnColumn        *aColumn,
                  acp_uint16_t      aUserRowNumber,
                  ulnIndLenPtrPair *aUserIndLenPair)
{
    ulnLengthPair    sLength = {ULN_vLEN(0), ULN_vLEN(0)};
    ulnConvFunction *sFilter = NULL;

    sFilter = ulnConvGetFilter(aUserBuffer->mCTYPE, (ulnMTypeID)aColumn->mMtype);

    /*
     * 컨버젼시에 에러가 발생하더라도 해당 diagnostic record 만 매달고,
     * 해당 row 에 에러가 발생했음을 row status ptr 에 할당한 후에 계속 fetch 를 진행한다.
     */
    if (sFilter != NULL)
    {
        if (aColumn->mDataLength == SQL_NULL_DATA)
        {
            if (ulnBindSetUserIndLenValue(aUserIndLenPair, SQL_NULL_DATA) != ACI_SUCCESS)
            {
                /*
                 * 22002 :
                 *
                 * NULL 이 컬럼에 fetch 되어 와서, SQL_NULL_DATA 를 사용자가 지정한
                 * StrLen_or_IndPtr 에 써 줘야 하는데, 이녀석이 NULL 포인터이다.
                 * 그럴때에 발생시켜 주는 에러.
                 */
                ulnErrorExtended(aFnContext,
                                 aUserRowNumber,
                                 aColumn->mColumnNumber,
                                 ulERR_ABORT_INDICATOR_REQUIRED_BUT_NOT_SUPPLIED_ERR);
            }
        }
        else
        {
            if (sFilter(aFnContext,
                        aUserBuffer,
                        aColumn,
                        &sLength,
                        aUserRowNumber) == ACI_SUCCESS)
            {
                /*
                 * 데이터를 다 가져감. 더 이상 줄 데이터 없음.
                 */
                if ((sLength.mNeeded != SQL_NO_TOTAL) && (sLength.mNeeded <= sLength.mWritten))
                {
                    aColumn->mGDPosition = 0;
                    aColumn->mRemainTextLen = 0;
                }

                /*
                 * 사용자에게 리턴하는 길이 : 남아있는 데이터의 길이이다.
                 */

                if (ulnBindSetUserIndLenValue(aUserIndLenPair, sLength.mNeeded) != ACI_SUCCESS)
                {
                    /*
                     * 22002 :
                     *
                     * NULL 이 컬럼에 fetch 되어 와서, SQL_NULL_DATA 를 사용자가 지정한
                     * StrLen_or_IndPtr 에 써 줘야 하는데, 이녀석이 NULL 포인터이다.
                     * 그럴때에 발생시켜 주는 에러.
                     */
                    ulnErrorExtended(aFnContext,
                                     aUserRowNumber,
                                     aColumn->mColumnNumber,
                                     ulERR_ABORT_INDICATOR_REQUIRED_BUT_NOT_SUPPLIED_ERR);

                    aUserBuffer->mColumnStatus = ULN_ROW_ERROR;
                }
            }
            else
            {
                aUserBuffer->mColumnStatus = ULN_ROW_ERROR;
            }
        }
    }
    else
    {
        /*
         * 07006 : Restricted data type attribute violation
         */
        ulnErrorExtended(aFnContext,
                         aUserRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_CONVERSION);

        aUserBuffer->mColumnStatus = ULN_ROW_ERROR;
    }

    return ACI_SUCCESS;
}

ulnConvEndianFunc *ulnConvGetEndianFunc(acp_uint8_t aIsSameEndian)
{
    return ulnConvEndianMap[aIsSameEndian];
}

void ulnConvEndian_NONE(acp_uint8_t *aSourceBuffer,
                        ulvSLen      aSourceLength)
{
    ACP_UNUSED(aSourceBuffer);
    ACP_UNUSED(aSourceLength);
}

void ulnConvEndian_ADJUST(acp_uint8_t *aSourceBuffer,
                          ulvSLen      aSourceLength)
{
    acp_sint32_t i;
    acp_uint8_t sTemp;

    for (i = 0; i < aSourceLength; i = i + 2)
    {
        sTemp = aSourceBuffer[i];
        aSourceBuffer[i] = aSourceBuffer[i + 1];
        aSourceBuffer[i + 1] = sTemp;
    }
}
