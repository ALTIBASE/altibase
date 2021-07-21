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
 * $Id: acpStrCmp.c 9440 2010-01-08 02:21:11Z denisb $
 ******************************************************************************/

#include <acpChar.h>
#include <acpStr.h>


ACP_INLINE acp_sint32_t acpStrCmpCaseSensitive0(const acp_char_t *aStr1,
                                                const acp_char_t *aStr2)
{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    while (1)
    {
        sChar1 = (acp_uint8_t)(*aStr1);
        sChar2 = (acp_uint8_t)(*aStr2);

        if (sChar1 == sChar2)
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                return 0;
            }
            else
            {
                aStr1++;
                aStr2++;
            }
        }
        else
        {
            break;
        }
    }

    return ((acp_sint32_t)sChar1) - ((acp_sint32_t)sChar2);
}

ACP_INLINE acp_sint32_t acpStrCmpCaseSensitive1(const acp_char_t *aStr1,
                                                const acp_char_t *aStr2,
                                                acp_sint32_t      aLen1)
{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    while (1)
    {
        sChar1 = (acp_uint8_t)(*aStr1);
        sChar2 = (acp_uint8_t)(*aStr2);

        if (sChar1 == sChar2)
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                return 0;
            }
            else
            {
                aStr1++;
                aStr2++;
                aLen1--;

                if (aLen1 == 0)
                {
                    break;
                }
                else
                {
                    /* continue */
                }
            }
        }
        else
        {
            break;
        }
    }

    return ((acp_sint32_t)sChar1) - ((acp_sint32_t)sChar2);
}

ACP_INLINE acp_sint32_t acpStrCmpCaseSensitive2(const acp_char_t *aStr1,
                                                const acp_char_t *aStr2,
                                                acp_sint32_t      aLen1,
                                                acp_sint32_t      aLen2)
{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    while (1)
    {
        sChar1 = (acp_uint8_t)(*aStr1);
        sChar2 = (acp_uint8_t)(*aStr2);

        if (sChar1 == sChar2)
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                return 0;
            }
            else
            {
                aStr1++;
                aStr2++;
                aLen1--;
                aLen2--;

                if (aLen1 == 0)
                {
                    return (aLen2 == 0) ? 0 : -1;
                }
                else if (aLen2 == 0)
                {
                    return 1;
                }
                else
                {
                    /* continue */
                }
            }
        }
        else
        {
            break;
        }
    }

    return ((acp_sint32_t)sChar1) - ((acp_sint32_t)sChar2);
}

ACP_INLINE acp_sint32_t acpStrCmpCaseInsensitive0(const acp_char_t *aStr1,
                                                  const acp_char_t *aStr2)
{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    while (1)
    {
        sChar1 = (acp_uint8_t)acpCharToLower(*aStr1);
        sChar2 = (acp_uint8_t)acpCharToLower(*aStr2);

        if (sChar1 == sChar2)
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                return 0;
            }
            else
            {
                aStr1++;
                aStr2++;
            }
        }
        else
        {
            break;
        }
    }

    return (acp_sint32_t)sChar1 - (acp_sint32_t)sChar2;
}

ACP_INLINE acp_sint32_t acpStrCmpCaseInsensitive1(const acp_char_t *aStr1,
                                                  const acp_char_t *aStr2,
                                                  acp_sint32_t      aLen)
{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    while (1)
    {
        sChar1 = (acp_uint8_t)acpCharToLower(*aStr1);
        sChar2 = (acp_uint8_t)acpCharToLower(*aStr2);

        if (sChar1 == sChar2)
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                return 0;
            }
            else
            {
                aStr1++;
                aStr2++;
                aLen--;

                if (aLen == 0)
                {
                    return (*aStr2 == ACP_CSTR_TERM) ? 0 : -1;
                }
                else
                {
                    /* continue */
                }
            }
        }
        else
        {
            break;
        }
    }

    return (acp_sint32_t)sChar1 - (acp_sint32_t)sChar2;
}

ACP_INLINE acp_sint32_t acpStrCmpCaseInsensitive2(const acp_char_t *aStr1,
                                                  const acp_char_t *aStr2,
                                                  acp_sint32_t      aLen1,
                                                  acp_sint32_t      aLen2)
{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    while (1)
    {
        sChar1 = (acp_uint8_t)acpCharToLower(*aStr1);
        sChar2 = (acp_uint8_t)acpCharToLower(*aStr2);

        if (sChar1 == sChar2)
        {
            if (sChar1 == ACP_CSTR_TERM)
            {
                return 0;
            }
            else
            {
                aStr1++;
                aStr2++;
                aLen1--;
                aLen2--;

                if (aLen1 == 0)
                {
                    return (aLen2 == 0) ? 0 : -1;
                }
                else if (aLen2 == 0)
                {
                    return 1;
                }
                else
                {
                    /* continue */
                }
            }
        }
        else
        {
            break;
        }
    }

    return (acp_sint32_t)sChar1 - (acp_sint32_t)sChar2;
}

ACP_INLINE acp_bool_t acpStrCmpIsNull(const acp_char_t *aCStr1,
                                      const acp_char_t *aCStr2,
                                      acp_sint32_t     *aRet)
{
    if (aCStr1 == NULL)
    {
        if (aCStr2 == NULL)
        {
            *aRet = 0;
        }
        else
        {
            *aRet = (*aCStr2 == ACP_CSTR_TERM) ? 0 : -1;
        }
    }
    else
    {
        if (aCStr2 == NULL)
        {
            *aRet = (*aCStr1 == ACP_CSTR_TERM) ? 0 : 1;
        }
        else
        {
            return ACP_FALSE;
        }
    }

    return ACP_TRUE;
}

ACP_INLINE acp_bool_t acpStrCmpIsEmpty(const acp_char_t *aCStr1,
                                       const acp_char_t *aCStr2,
                                       acp_sint32_t      aLen1,
                                       acp_sint32_t      aLen2,
                                       acp_sint32_t     *aRet)
{
    if (aLen1 == 0)
    {
        if (aLen2 == 0)
        {
            *aRet = 0;
        }
        else
        {
            *aRet = (*aCStr2 == ACP_CSTR_TERM) ? 0 : -1;
        }
    }
    else
    {
        if (aLen2 == 0)
        {
            *aRet = (*aCStr1 == ACP_CSTR_TERM) ? 0 : 1;
        }
        else
        {
            return ACP_FALSE;
        }
    }

    return ACP_TRUE;
}

/**
 * compares two strings
 *
 * @param aStr1 pointer to the string object to compare
 * @param aStr2 pointer to the string object to compare
 * @param aFlag bit flags for compare option
 * @return zero if the two strings are identical,
 * otherwise the difference between the first two different characters
 *
 * the flag can be one of #ACP_STR_CASE_SENSITIVE, #ACP_STR_CASE_INSENSITIVE.
 * if the flag is zero, it uses #ACP_STR_CASE_SENSITIVE as default.
 */
ACP_EXPORT acp_sint32_t acpStrCmp(acp_str_t    *aStr1,
                                  acp_str_t    *aStr2,
                                  acp_sint32_t  aFlag)
{
    acp_sint32_t sLen1 = (acp_sint32_t)acpStrGetLength(aStr1);
    acp_sint32_t sLen2 = (acp_sint32_t)acpStrGetLength(aStr2);
    acp_sint32_t sRet = ACP_RC_SUCCESS;

    /*
     * check whether one or both strings are null
     */
    if (acpStrCmpIsNull(aStr1->mString, aStr2->mString, &sRet) == ACP_TRUE)
    {
        return sRet;
    }
    else
    {
        /* do nothing */
    }

    /*
     * check whether the length of one or both strings are 0
     */
    if (acpStrCmpIsEmpty(aStr1->mString,
                         aStr2->mString,
                         sLen1,
                         sLen2,
                         &sRet) == ACP_TRUE)
    {
        return sRet;
    }
    else
    {
        /* do nothing */
    }

    /*
     * compare two strings
     */
    if (sLen1 > 0)
    {
        if (sLen2 > 0)
        {
            if (aFlag & ACP_STR_CASE_INSENSITIVE)
            {
                return acpStrCmpCaseInsensitive2(aStr1->mString,
                                                 aStr2->mString,
                                                 sLen1,
                                                 sLen2);
            }
            else
            {
                return acpStrCmpCaseSensitive2(aStr1->mString,
                                               aStr2->mString,
                                               sLen1,
                                               sLen2);
            }
        }
        else
        {
            if (aFlag & ACP_STR_CASE_INSENSITIVE)
            {
                return acpStrCmpCaseInsensitive1(aStr1->mString,
                                                 aStr2->mString,
                                                 sLen1);
            }
            else
            {
                return acpStrCmpCaseSensitive1(aStr1->mString,
                                               aStr2->mString,
                                               sLen1);
            }
        }
    }
    else
    {
        if (sLen2 > 0)
        {
            if (aFlag & ACP_STR_CASE_INSENSITIVE)
            {
                return -acpStrCmpCaseInsensitive1(aStr2->mString,
                                                  aStr1->mString,
                                                  sLen2);
            }
            else
            {
                return -acpStrCmpCaseSensitive1(aStr2->mString,
                                                aStr1->mString,
                                                sLen2);
            }
        }
        else
        {
            if (aFlag & ACP_STR_CASE_INSENSITIVE)
            {
                return acpStrCmpCaseInsensitive0(aStr1->mString,
                                                 aStr2->mString);
            }
            else
            {
                return acpStrCmpCaseSensitive0(aStr1->mString,
                                               aStr2->mString);
            }
        }
    }
}

/**
 * compares two strings
 *
 * @param aStr pointer to the string object to compare
 * @param aCString null terminated string (pointer to the character array)
 * @param aFlag bit flags for compare option
 * @return zero if the two strings are identical,
 * otherwise the difference between the first two different characters
 *
 * the flag can be one of #ACP_STR_CASE_SENSITIVE, #ACP_STR_CASE_INSENSITIVE.
 * if the flag is zero, it uses #ACP_STR_CASE_SENSITIVE as default.
 */
ACP_EXPORT acp_sint32_t acpStrCmpCString(acp_str_t        *aStr,
                                         const acp_char_t *aCString,
                                         acp_sint32_t      aFlag)
{
    acp_sint32_t sLen = (acp_sint32_t)acpStrGetLength(aStr);
    acp_sint32_t sRet = ACP_RC_SUCCESS;

    /*
     * check whether one or both strings are null
     */
    if (acpStrCmpIsNull(aStr->mString, aCString, &sRet) == ACP_TRUE)
    {
        return sRet;
    }
    else
    {
        /* do nothing */
    }

    /*
     * compare
     */
    if (sLen == 0)
    {
        return (aCString[0] == ACP_CSTR_TERM) ? 0 : -1;
    }
    else if (sLen > 0)
    {
        if (aFlag & ACP_STR_CASE_INSENSITIVE)
        {
            return acpStrCmpCaseInsensitive1(aStr->mString, aCString, sLen);
        }
        else
        {
            return acpStrCmpCaseSensitive1(aStr->mString, aCString, sLen);
        }
    }
    else
    {
        if (aFlag & ACP_STR_CASE_INSENSITIVE)
        {
            return acpStrCmpCaseInsensitive0(aStr->mString, aCString);
        }
        else
        {
            return acpStrCmpCaseSensitive0(aStr->mString, aCString);
        }
    }
}
