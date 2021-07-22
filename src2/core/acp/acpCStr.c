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
 * $Id: acpCStr.c 11218 2010-06-10 08:13:05Z djin $
 ******************************************************************************/

#include <acpCStr.h>
#include <acpChar.h>
#include <acpBit.h>

/* Pick up sign character */
ACP_INLINE acp_sint32_t acpCStrInternalGetSign(const acp_char_t** aStr)
{
    if(**aStr == '-')
    {
        (*aStr)++;
        return -1;
    }
    else
    {
        if(**aStr == '+')
        {
            (*aStr)++;
        }
        else
        {
            /* Do nothing */
        }

        return 1;
    }
}

static acp_rc_t acpCStrInternalCharToInt(
    acp_uint32_t*       aValue,
    const acp_char_t    aChar,
    acp_uint32_t        aBase)
{
    /* 
     * Conversion table.
     * 0xFF means it cannot be converted into number
     */
    static const acp_uint32_t sTable[256] =
    {
        /*
         * 0     1     2     3     4     5     6     7
         * 8     9     A     B     C     D     E     F */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x00 ~ 0x07 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x08 ~ 0x0F */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x10 ~ 0x17 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x18 ~ 0x1F */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x20 ~ 0x27 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x28 ~ 0x2F */
           0,    1,    2,    3,    4,    5,    6,    7, /* 0x30 ~ 0x37 */
           8,    9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x38 ~ 0x3F */
        0xFF,   10,   11,   12,   13,   14,   15,   16, /* 0x40 ~ 0x47 */
          17,   18,   19,   20,   21,   22,   23,   24, /* 0x48 ~ 0x4F */
          25,   26,   27,   28,   29,   30,   31,   32, /* 0x50 ~ 0x57 */
          33,   34,   35, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x58 ~ 0x5F */
        0xFF,   10,   11,   12,   13,   14,   15,   16, /* 0x60 ~ 0x67 */
          17,   18,   19,   20,   21,   22,   23,   24, /* 0x68 ~ 0x6F */
          25,   26,   27,   28,   29,   30,   31,   32, /* 0x70 ~ 0x77 */
          33,   34,   35, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x78 ~ 0x7F */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x80 ~ 0x87 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x88 ~ 0x8F */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x90 ~ 0x97 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0x98 ~ 0x9F */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xA0 ~ 0xA7 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xA8 ~ 0xAF */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xB0 ~ 0xB7 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xB8 ~ 0xBF */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xC0 ~ 0xC7 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xC8 ~ 0xCF */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xD0 ~ 0xD7 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xD8 ~ 0xDF */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xE0 ~ 0xE7 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xE8 ~ 0xEF */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 0xF0 ~ 0xF7 */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  /* 0xF8 ~ 0xFF */
    };

    /* '& 0xFF' may be useless,
     * but for removing Codesonar warnings - Buffer overrun/overrun */
    acp_uint32_t sValue = sTable[aChar & 0xFF];
    ACP_TEST(aBase <= sValue);
    *aValue =  sValue;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_ERANGE;
}

ACP_EXPORT acp_rc_t acpCStrToInt32(
    const acp_char_t*   aStr,
    acp_size_t          aStrLen,
    acp_sint32_t*       aSign,
    acp_uint32_t*       aResult,
    acp_sint32_t        aBase,
    acp_char_t**        aEnd)
{
    acp_uint64_t sResult;
    acp_rc_t sRC = acpCStrToInt64(aStr, aStrLen, aSign, &sResult, aBase, aEnd);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Check Range */
        if(sResult > ACP_UINT32_MAX)
        {
            *aResult = ACP_UINT32_MAX;
            sRC = ACP_RC_ERANGE;
        }
        else
        {
            *aResult = (acp_uint32_t)(sResult);
        }
    }
    else
    {
        /* Do nothing */
    }
    
    return sRC;
}

ACP_INLINE void acpCStrToIntGetBase(
    const acp_char_t**  aStr,
    acp_size_t          aStrLen,
    acp_sint32_t*       aBase)
{
    const acp_char_t*   sStr = (*aStr);
    acp_uint32_t        sTemp;

    if((0 == (*aBase)) || (16 == (*aBase)))
    {
        /* aStr is 0xNNNNN..... */
        if(
            (3 <= aStrLen) &&
            ('0' == sStr[0]) &&
            ('x' == acpCharToLower(sStr[1])) &&
            ACP_RC_IS_SUCCESS(acpCStrInternalCharToInt(&sTemp, sStr[2], 16))
          )
        {
            *aBase = 16;
            sStr += 2;
        }
        /* aStr is 0NNNNN..... */
        else if((0 == (*aBase)) && ('0' == sStr[0]))
        {
            *aBase = 8;
        }
        /* Default base is 10 */
        else
        {
            *aBase = (16 == (*aBase))? 16 : 10;
        }
    }
    else
    {
        /* Base is defined */
    }

    *aStr = sStr;
}

static acp_rc_t acpCStrToInt64Convert(
    const acp_char_t*   aStr,
    acp_size_t          aStrLen,
    acp_uint64_t*       aResult,
    acp_sint32_t        aBase,
    acp_char_t**        aEnd)
{
    const acp_uint64_t  sMaxValue = (acp_uint64_t)(ACP_UINT64_MAX / aBase);
    const acp_uint64_t  sMaxDigit = (acp_sint64_t)(ACP_UINT64_MAX % aBase);
    acp_uint32_t        sDigit;
    acp_rc_t            sRC = ACP_RC_SUCCESS;
    acp_rc_t            sReturn = ACP_RC_SUCCESS;

    *aResult = 0;
    while(aStrLen > 0 && *aStr != ACP_CSTR_TERM)
    {
        sRC = acpCStrInternalCharToInt(&sDigit, *aStr, aBase);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            if(
                (*aResult > sMaxValue ) ||
                ((sMaxValue == *aResult) && (sDigit > sMaxDigit))
              )
            {
                *aResult = ACP_UINT64_MAX;
                sReturn = ACP_RC_ERANGE;
            }
            else
            {
                *aResult *= aBase;
                *aResult += sDigit;
            }

            aStr++;
            aStrLen--;
        }
        else
        {
            /* Gotcha! First invalid character. */
            break;
        }
    }

    if(NULL != aEnd)
    {
        *aEnd = (acp_char_t*)aStr;
    }
    else
    {
        /* Do nothing */
    }
    return sReturn;
}

ACP_EXPORT acp_rc_t acpCStrToInt64(
    const acp_char_t*   aStr,
    acp_size_t          aStrLen,
    acp_sint32_t*       aSign,
    acp_uint64_t*       aResult,
    acp_sint32_t        aBase,
    acp_char_t**        aEnd)
{
    const acp_char_t*   sStr = aStr;
    acp_size_t          sStrLen;
    acp_uint32_t        sDigit;
    acp_rc_t            sRC;

    /* NULL Check */
    ACP_TEST_RAISE((NULL == aStr) || (NULL == aSign) || (NULL == aResult),
                   HANDLE_FAULT);

    /* Handle base with 0, 2~36 */
    ACP_TEST_RAISE((aBase != 0) && (aBase < 2 || aBase > 36),
                   HANDLE_BASE);
    /* Empty string */
    ACP_TEST_RAISE((0 == aStrLen) || (ACP_CSTR_TERM == *sStr), HANDLE_EMPTY);

    (*aSign) = acpCStrInternalGetSign(&sStr);
    sStrLen = sStr - aStr;
    /* String is "+", "-" */
    ACP_TEST_RAISE(
        (aStrLen <= sStrLen) || (ACP_CSTR_TERM == *sStr),
        HANDLE_EMPTY);

    sStrLen = aStrLen - (sStr - aStr);
    acpCStrToIntGetBase(&sStr, sStrLen, &aBase);
    sStrLen = sStr - aStr;
    /* String is "0x", "0" */
    ACP_TEST_RAISE(
        (aStrLen <= sStrLen) || (ACP_CSTR_TERM == *sStr),
        HANDLE_EMPTY);
    /* Check first character is convertable */
    ACP_TEST_RAISE(
        ACP_RC_NOT_SUCCESS(acpCStrInternalCharToInt(&sDigit, *sStr, aBase)),
        HANDLE_INVALID);

    sStrLen = aStrLen - (sStr - aStr);
    return acpCStrToInt64Convert(sStr, sStrLen, aResult, aBase, aEnd);

    ACP_EXCEPTION(HANDLE_EMPTY)
    {
        if(NULL != aEnd)
        {
            *aEnd = (acp_char_t*)aStr;
        }
        else
        {
            /* No need of returning endpoint */
        }

        *aSign   = 1;
        *aResult = 0;
        /* Originally, Error handler must return error code,
         * but without this, there would be too many level of
         * nested if's.
         * So, as a special case, the error handler returns success */
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_INVALID)
    {
        if(NULL != aEnd)
        {
            *aEnd = (acp_char_t*)aStr;
        }
        else
        {
            /* No need of returning endpoint */
        }

        *aSign   = 1;
        *aResult = 0;
        /* Success again */
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(HANDLE_FAULT)
    {
        sRC = ACP_RC_EFAULT;
    }

    ACP_EXCEPTION(HANDLE_BASE)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;
    return sRC;
}


ACP_INLINE acp_rc_t acpCStrFindCharFwdCaseSensitive(const acp_char_t *aCStr,
                                                   acp_sint32_t     *aFound,
                                                   acp_sint32_t      aFrom,
                                                   acp_sint32_t      aLen,
                                                   acp_char_t        aChar)
{
    acp_sint32_t i;

    for (i = aFrom; i < aLen; i++)
    {
        if (aCStr[i] == aChar)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t acpCStrFindCharFwdCaseInsensitive(const acp_char_t *aCStr,
                                                     acp_sint32_t     *aFound,
                                                     acp_sint32_t      aFrom,
                                                     acp_sint32_t      aLen,
                                                     acp_char_t        aChar)
{
    acp_sint32_t i;

    aChar = acpCharToLower(aChar);

    for (i = aFrom; i < aLen; i++)
    {
        if (acpCharToLower(aCStr[i]) == aChar)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t acpCStrFindCharBwdCaseSensitive(const acp_char_t *aCStr,
                                                   acp_sint32_t     *aFound,
                                                   acp_sint32_t      aFrom,
                                                   acp_char_t        aChar)
{
    acp_sint32_t i;

    for (i = aFrom; i >= 0; i--)
    {
        if (aCStr[i] == aChar)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t acpCStrFindCharBwdCaseInsensitive(const acp_char_t *aCStr,
                                                     acp_sint32_t     *aFound,
                                                     acp_sint32_t      aFrom,
                                                     acp_char_t        aChar)
{
    acp_sint32_t i;

    aChar = acpCharToLower(aChar);

    for (i = aFrom; i >= 0; i--)
    {
        if (acpCharToLower(aCStr[i]) == aChar)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}


ACP_EXPORT acp_rc_t acpCStrFindChar(acp_char_t *aCStr,
                                    acp_char_t aFindChar,
                                    acp_sint32_t *aFoundIndex,
                                    acp_sint32_t aFromIndex,
                                    acp_sint32_t aFlag)
{
    acp_rc_t sRC;
    acp_sint32_t sLen = (acp_sint32_t)acpCStrLen(aCStr, ACP_SINT32_MAX);


    ACP_TEST_RAISE(sLen <= 0 ||
                   aFromIndex > sLen ||
                   aFromIndex < 0, INVAL_ARGUMENT);

    /*
     * Default action is forward searching.
     * If backward flag is set explicitly,
     * do backward searching.
     */
    if ((aFlag & ACP_CSTR_SEARCH_BACKWARD) != 0)
    {
        /*
         * Default action is case-sensitive searching.
         * If case-insensitive flag is set explicitly,
         * do case-insensitive searching.
         */
        if ((aFlag & ACP_CSTR_CASE_INSENSITIVE) != 0)
        {
            sRC = acpCStrFindCharBwdCaseInsensitive(aCStr,
                                                   aFoundIndex,
                                                   aFromIndex,
                                                   aFindChar);
        }
        else
        {
            sRC = acpCStrFindCharBwdCaseSensitive(aCStr,
                                                 aFoundIndex,
                                                 aFromIndex,
                                                 aFindChar);
        }
    }
    else
    {
        if ((aFlag & ACP_CSTR_CASE_INSENSITIVE) != 0)
        {
            sRC = acpCStrFindCharFwdCaseInsensitive(aCStr,
                                                   aFoundIndex,
                                                   aFromIndex,
                                                   sLen,
                                                   aFindChar);
        }
        else
        {
            sRC = acpCStrFindCharFwdCaseSensitive(aCStr,
                                                 aFoundIndex,
                                                 aFromIndex,
                                                 sLen,
                                                 aFindChar);
        }
    }

    return sRC;

    ACP_EXCEPTION(INVAL_ARGUMENT)
    {
        sRC = ACP_RC_EINVAL;
    }
    
    ACP_EXCEPTION_END;
    return sRC;
}



ACP_INLINE acp_sint32_t acpCStrCmpCaseSensitiveLen(const acp_char_t *aStr1,
                                                  const acp_char_t *aStr2,
                                                  acp_sint32_t      aLen)
{
    acp_sint32_t i;

    for (i = 0; i < aLen; i++)
    {
        if (*aStr1 != *aStr2)
        {
            return ((acp_sint32_t)*aStr1) - ((acp_sint32_t)*aStr2);
        }
        else
        {
            if (*aStr1 == ACP_CSTR_TERM)
            {
                break;
            }
            else
            {
                aStr1++;
                aStr2++;
            }
        }
    }

    return 0;
}

ACP_INLINE acp_sint32_t acpCStrCmpCaseInsensitiveLen(const acp_char_t *aStr1,
                                                    const acp_char_t *aStr2,
                                                    acp_sint32_t      aLen)
{
    acp_char_t   sChar1;
    acp_char_t   sChar2;
    acp_sint32_t i;

    for (i = 0; i < aLen; i++)
    {
        sChar1 = acpCharToLower(*aStr1);
        sChar2 = acpCharToLower(*aStr2);

        if (sChar1 != sChar2)
        {
            return (acp_sint32_t)sChar1 - (acp_sint32_t)sChar2;
        }
        else
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                break;
            }
            else
            {
                aStr1++;
                aStr2++;
            }
        }
    }

    return 0;
}


ACP_INLINE acp_rc_t acpCStrFindCStrFwdCaseSensitive(const acp_char_t *aCStr,
                                                  const acp_char_t *aFindCStr,
                                                  acp_sint32_t     *aFound,
                                                  acp_sint32_t      aFrom,
                                                  acp_sint32_t      aLen,
                                                  acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i <= aLen; i++)
    {
        if (acpCStrCmpCaseSensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t acpCStrFindCStrFwdCaseInsensitive(const acp_char_t *aCStr,
                                                    const acp_char_t *aFindCStr,
                                                    acp_sint32_t     *aFound,
                                                    acp_sint32_t      aFrom,
                                                    acp_sint32_t      aLen,
                                                    acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i <= aLen; i++)
    {
        if (acpCStrCmpCaseInsensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t acpCStrFindCStrBwdCaseSensitive(const acp_char_t *aCStr,
                                                  const acp_char_t *aFindCStr,
                                                  acp_sint32_t     *aFound,
                                                  acp_sint32_t      aFrom,
                                                  acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i >= 0; i--)
    {
        if (acpCStrCmpCaseSensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_INLINE acp_rc_t acpCStrFindCStrBwdCaseInsensitive(const acp_char_t *aCStr,
                                                    const acp_char_t *aFindCStr,
                                                    acp_sint32_t     *aFound,
                                                    acp_sint32_t      aFrom,
                                                    acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i >= 0; i--)
    {
        if (acpCStrCmpCaseInsensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
        {
            *aFound = i;

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}


ACP_EXPORT acp_rc_t acpCStrFindCStr(const acp_char_t *aCStr,
                                    const acp_char_t *aFindCStr,
                                    acp_sint32_t *aFoundIndex,
                                    acp_sint32_t aFromIndex,
                                    acp_sint32_t aFlag)
{
    acp_rc_t sRC;
    acp_sint32_t sLen = (acp_sint32_t)acpCStrLen(aCStr, ACP_SINT32_MAX);
    acp_sint32_t sFindLen = (acp_sint32_t)acpCStrLen(aFindCStr,
                                                     ACP_SINT32_MAX);
    
    ACP_TEST_RAISE(sLen == 0 || aFindCStr == NULL || sFindLen == 0,
                   INVAL_ARGUMENT);

    ACP_TEST_RAISE(aFromIndex > sLen || aFromIndex < 0,
                   INVAL_ARGUMENT);    
    
    if ((aFlag & ACP_CSTR_SEARCH_BACKWARD) != 0)
    {
        if ((aFlag & ACP_CSTR_CASE_INSENSITIVE) != 0)
        {
            sRC = acpCStrFindCStrBwdCaseInsensitive(aCStr,
                                                   aFindCStr,
                                                   aFoundIndex,
                                                   aFromIndex,
                                                   sFindLen);
        }
        else
        {
            sRC = acpCStrFindCStrBwdCaseSensitive(aCStr,
                                                 aFindCStr,
                                                 aFoundIndex,
                                                 aFromIndex,
                                                 sFindLen);
        }
    }
    else
    {
        if ((aFlag & ACP_CSTR_CASE_INSENSITIVE) != 0)
        {
            sRC = acpCStrFindCStrFwdCaseInsensitive(aCStr,
                                                   aFindCStr,
                                                   aFoundIndex,
                                                   aFromIndex,
                                                   sLen,
                                                   sFindLen);
        }
        else
        {
            sRC = acpCStrFindCStrFwdCaseSensitive(aCStr,
                                                 aFindCStr,
                                                 aFoundIndex,
                                                 aFromIndex,
                                                 sLen,
                                                 sFindLen);
        }
    }
    
    return sRC;

    ACP_EXCEPTION(INVAL_ARGUMENT)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;
    return sRC;
}


ACP_EXPORT acp_size_t acpCStrLen(
    const acp_char_t*   aStr,
    acp_size_t          aMaxLen)
{
#if defined(ACP_CSTR_STRNLEN_USE_NATIVE)
    return strnlen(aStr, aMaxLen);
#else
    /* Not all platform support strnlen */
    acp_size_t sLen;
    if (NULL != aStr)
    {
        for(sLen = 0; sLen < aMaxLen; sLen++, aStr++)
        {
            /* Searching for '\0' in aStr */
            if ((*aStr) == ACP_CSTR_TERM)
            {
                break;
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        sLen = 0;
    }

    return sLen;
#endif
}
