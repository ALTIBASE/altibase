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
 * $Id: acpStrFind.c 4923 2009-03-20 02:55:33Z djin $
 ******************************************************************************/

#include <acpChar.h>
#include <acpStr.h>


ACP_INLINE acp_sint32_t acpStrCmpCaseSensitiveLen(const acp_char_t *aStr1,
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

ACP_INLINE acp_sint32_t acpStrCmpCaseInsensitiveLen(const acp_char_t *aStr1,
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

ACP_INLINE acp_rc_t acpStrFindCharFwdCaseSensitive(const acp_char_t *aCStr,
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

ACP_INLINE acp_rc_t acpStrFindCharFwdCaseInsensitive(const acp_char_t *aCStr,
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

ACP_INLINE acp_rc_t acpStrFindCharBwdCaseSensitive(const acp_char_t *aCStr,
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

ACP_INLINE acp_rc_t acpStrFindCharBwdCaseInsensitive(const acp_char_t *aCStr,
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

ACP_INLINE acp_rc_t acpStrFindCharSetFwd(const acp_char_t *aCStr,
                                         acp_sint32_t     *aFound,
                                         acp_sint32_t      aFrom,
                                         acp_sint32_t      aLen,
                                         acp_str_t        *aCharSet,
                                         acp_sint32_t      aFlag)
{
    acp_rc_t     sRC;
    acp_sint32_t sIndex;
    acp_sint32_t i;

    for (i = aFrom; i < aLen; i++)
    {
        sRC = acpStrFindChar(aCharSet,
                             aCStr[i],
                             &sIndex,
                             ACP_STR_INDEX_INITIALIZER,
                             aFlag);

        if (ACP_RC_IS_SUCCESS(sRC))
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

ACP_INLINE acp_rc_t acpStrFindCharSetBwd(const acp_char_t *aCStr,
                                         acp_sint32_t     *aFound,
                                         acp_sint32_t      aFrom,
                                         acp_str_t        *aCharSet,
                                         acp_sint32_t      aFlag)
{
    acp_rc_t     sRC;
    acp_sint32_t sIndex;
    acp_sint32_t i;

    for (i = aFrom; i >= 0; i--)
    {
        sRC = acpStrFindChar(aCharSet,
                             aCStr[i],
                             &sIndex,
                             ACP_STR_INDEX_INITIALIZER,
                             aFlag);

        if (ACP_RC_IS_SUCCESS(sRC))
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

ACP_INLINE acp_rc_t acpStrFindStrFwdCaseSensitive(const acp_char_t *aCStr,
                                                  const acp_char_t *aFindCStr,
                                                  acp_sint32_t     *aFound,
                                                  acp_sint32_t      aFrom,
                                                  acp_sint32_t      aLen,
                                                  acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i <= aLen; i++)
    {
        if (acpStrCmpCaseSensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
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

ACP_INLINE acp_rc_t acpStrFindStrFwdCaseInsensitive(const acp_char_t *aCStr,
                                                    const acp_char_t *aFindCStr,
                                                    acp_sint32_t     *aFound,
                                                    acp_sint32_t      aFrom,
                                                    acp_sint32_t      aLen,
                                                    acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i <= aLen; i++)
    {
        if (acpStrCmpCaseInsensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
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

ACP_INLINE acp_rc_t acpStrFindStrBwdCaseSensitive(const acp_char_t *aCStr,
                                                  const acp_char_t *aFindCStr,
                                                  acp_sint32_t     *aFound,
                                                  acp_sint32_t      aFrom,
                                                  acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i >= 0; i--)
    {
        if (acpStrCmpCaseSensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
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

ACP_INLINE acp_rc_t acpStrFindStrBwdCaseInsensitive(const acp_char_t *aCStr,
                                                    const acp_char_t *aFindCStr,
                                                    acp_sint32_t     *aFound,
                                                    acp_sint32_t      aFrom,
                                                    acp_sint32_t      aFindLen)
{
    acp_sint32_t i;

    for (i = aFrom; i >= 0; i--)
    {
        if (acpStrCmpCaseInsensitiveLen(&aCStr[i], aFindCStr, aFindLen) == 0)
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

/**
 * searches a character
 *
 * @param aStr pointer to the string object
 * @param aFindChar a character to search
 * @param aFoundIndex pointer to a variable to store found index
 * @param aFromIndex index to search start
 * (excluding the character at the index)
 * @param aFlag bit flag for search option
 * @return result code
 *
 * for the first search,
 * @a aFromIndex should be initialized with #ACP_STR_INDEX_INITIALIZER.
 *
 * @a aFlag can be bitwise ORed with case sensitivity
 * (#ACP_STR_CASE_SENSITIVE, #ACP_STR_CASE_INSENSITIVE)
 * and search direction (#ACP_STR_SEARCH_FORWARD, #ACP_STR_SEARCH_BACKWARD).
 * if @a aFlag is zero, it uses default option;
 * #ACP_STR_CASE_SENSITIVE and #ACP_STR_SEARCH_FORWARD.
 *
 * it returns #ACP_RC_ENOENT if the character is not found.
 *
 * @see acpStrFindString() acpStrFindCString()
 */
ACP_EXPORT acp_rc_t acpStrFindChar(acp_str_t    *aStr,
                                   acp_char_t    aFindChar,
                                   acp_sint32_t *aFoundIndex,
                                   acp_sint32_t  aFromIndex,
                                   acp_sint32_t  aFlag)
{
    acp_sint32_t sLen = (acp_sint32_t)acpStrGetLength(aStr);

    if (sLen > 0)
    {
        if ((aFlag & ACP_STR_SEARCH_BACKWARD) != 0)
        {
            /*
             * search start position in backward
             */
            if (aFromIndex == ACP_STR_INDEX_INITIALIZER)
            {
                aFromIndex = sLen - 1;
            }
            else
            {
                if (aFromIndex > sLen)
                {
                    return ACP_RC_EINVAL;
                }
                else
                {
                    aFromIndex--;
                }
            }

            /*
             * search backward
             */
            if ((aFlag & ACP_STR_CASE_INSENSITIVE) != 0)
            {
                return acpStrFindCharBwdCaseInsensitive(aStr->mString,
                                                        aFoundIndex,
                                                        aFromIndex,
                                                        aFindChar);
            }
            else
            {
                return acpStrFindCharBwdCaseSensitive(aStr->mString,
                                                      aFoundIndex,
                                                      aFromIndex,
                                                      aFindChar);
            }
        }
        else
        {
            /*
             * search start position in forward
             */
            if (aFromIndex < ACP_STR_INDEX_INITIALIZER)
            {
                return ACP_RC_EINVAL;
            }
            else
            {
                aFromIndex++;
            }

            /*
             * search forward
             */
            if ((aFlag & ACP_STR_CASE_INSENSITIVE) != 0)
            {
                return acpStrFindCharFwdCaseInsensitive(aStr->mString,
                                                        aFoundIndex,
                                                        aFromIndex,
                                                        sLen,
                                                        aFindChar);
            }
            else
            {
                return acpStrFindCharFwdCaseSensitive(aStr->mString,
                                                      aFoundIndex,
                                                      aFromIndex,
                                                      sLen,
                                                      aFindChar);
            }
        }
    }
    else
    {
        return ACP_RC_ENOENT;
    }
}

/**
 * searches characters
 *
 * @param aStr pointer to the string object
 * @param aFindCharSet characters to search as C style null-terminated string
 * @param aFoundIndex pointer to a variable to store found index
 * @param aFromIndex index to search start
 * (excluding the character at the index)
 * @param aFlag bit flag for search option
 * @return result code
 *
 * for the first search,
 * @a aFromIndex should be initialized with #ACP_STR_INDEX_INITIALIZER.
 *
 * @a aFlag can be bitwise ORed with case sensitivity
 * (#ACP_STR_CASE_SENSITIVE, #ACP_STR_CASE_INSENSITIVE)
 * and search direction (#ACP_STR_SEARCH_FORWARD, #ACP_STR_SEARCH_BACKWARD).
 * if @a aFlag is zero, it uses default option;
 * #ACP_STR_CASE_SENSITIVE and #ACP_STR_SEARCH_FORWARD.
 *
 * it returns #ACP_RC_ENOENT if the character is not found.
 *
 * @see acpStrFindString() acpStrFindCString()
 */
ACP_EXPORT acp_rc_t acpStrFindCharSet(acp_str_t        *aStr,
                                      const acp_char_t *aFindCharSet,
                                      acp_sint32_t     *aFoundIndex,
                                      acp_sint32_t      aFromIndex,
                                      acp_sint32_t      aFlag)
{
    acp_str_t    sFindCharSet = ACP_STR_CONST((acp_char_t *)aFindCharSet);
    acp_sint32_t sLen         = (acp_sint32_t)acpStrGetLength(aStr);

    if (sLen > 0)
    {
        if ((aFlag & ACP_STR_SEARCH_BACKWARD) != 0)
        {
            /*
             * search start position in backward
             */
            if (aFromIndex == ACP_STR_INDEX_INITIALIZER)
            {
                aFromIndex = sLen - 1;
            }
            else
            {
                if (aFromIndex > sLen)
                {
                    return ACP_RC_EINVAL;
                }
                else
                {
                    aFromIndex--;
                }
            }

            /*
             * search backward
             */
            return acpStrFindCharSetBwd(aStr->mString,
                                        aFoundIndex,
                                        aFromIndex,
                                        &sFindCharSet,
                                        aFlag);
        }
        else
        {
            /*
             * search start position in forward
             */
            if (aFromIndex < ACP_STR_INDEX_INITIALIZER)
            {
                return ACP_RC_EINVAL;
            }
            else
            {
                aFromIndex++;
            }

            /*
             * search forward
             */
            return acpStrFindCharSetFwd(aStr->mString,
                                        aFoundIndex,
                                        aFromIndex,
                                        sLen,
                                        &sFindCharSet,
                                        aFlag);
        }
    }
    else
    {
        return ACP_RC_ENOENT;
    }
}

/**
 * searches a string
 *
 * @param aStr pointer to the string object
 * @param aFindCStr null terminated string (pointer to the character array)
 * to search
 * @param aFoundIndex pointer to a variable to store found index
 * @param aFromIndex index to search start
 * (excluding the character at the index)
 * @param aFlag bit flag for search option
 * @return result code
 *
 * for the first search,
 * @a aFromIndex should be initialized with #ACP_STR_INDEX_INITIALIZER.
 *
 * @a aFlag can be bitwise ORed with case sensitivity
 * (#ACP_STR_CASE_SENSITIVE, #ACP_STR_CASE_INSENSITIVE)
 * and search direction (#ACP_STR_SEARCH_FORWARD, #ACP_STR_SEARCH_BACKWARD).
 * if @a aFlag is zero, it uses default option;
 * #ACP_STR_CASE_SENSITIVE and #ACP_STR_SEARCH_FORWARD
 *
 * it returns #ACP_RC_ENOENT if the string is not found,
 * #ACP_RC_EINVAL if the search string is null or empty string.
 *
 * @see acpStrFindString() acpStrFindChar()
 */
ACP_EXPORT acp_rc_t acpStrFindCString(acp_str_t        *aStr,
                                      const acp_char_t *aFindCStr,
                                      acp_sint32_t     *aFoundIndex,
                                      acp_sint32_t      aFromIndex,
                                      acp_sint32_t      aFlag)
{
    acp_sint32_t sLen     = (acp_sint32_t)acpStrGetLength(aStr);
    acp_sint32_t sFindLen;

    if (aFindCStr == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        sFindLen = (acp_sint32_t)strlen(aFindCStr);

        if (sFindLen == 0)
        {
            return ACP_RC_EINVAL;
        }
        else
        {
            /* do nothing */
        }
    }

    if (sLen > 0)
    {
        if (aFlag & ACP_STR_SEARCH_BACKWARD)
        {
            /*
             * search start position in backward
             */
            if (aFromIndex == ACP_STR_INDEX_INITIALIZER)
            {
                aFromIndex = sLen - sFindLen;
            }
            else
            {
                if (aFromIndex > sLen)
                {
                    return ACP_RC_EINVAL;
                }
                else
                {
                    aFromIndex--;
                }
            }

            /*
             * search backward
             */
            if (aFlag & ACP_STR_CASE_INSENSITIVE)
            {
                return acpStrFindStrBwdCaseInsensitive(aStr->mString,
                                                       aFindCStr,
                                                       aFoundIndex,
                                                       aFromIndex,
                                                       sFindLen);
            }
            else
            {
                return acpStrFindStrBwdCaseSensitive(aStr->mString,
                                                     aFindCStr,
                                                     aFoundIndex,
                                                     aFromIndex,
                                                     sFindLen);
            }
        }
        else
        {
            /*
             * search start and end position in forward
             */
            if (aFromIndex < ACP_STR_INDEX_INITIALIZER)
            {
                return ACP_RC_EINVAL;
            }
            else
            {
                aFromIndex++;
                sLen -= sFindLen;
            }

            /*
             * search forward
             */
            if (aFlag & ACP_STR_CASE_INSENSITIVE)
            {
                return acpStrFindStrFwdCaseInsensitive(aStr->mString,
                                                       aFindCStr,
                                                       aFoundIndex,
                                                       aFromIndex,
                                                       sLen,
                                                       sFindLen);
            }
            else
            {
                return acpStrFindStrFwdCaseSensitive(aStr->mString,
                                                     aFindCStr,
                                                     aFoundIndex,
                                                     aFromIndex,
                                                     sLen,
                                                     sFindLen);
            }
        }
    }
    else
    {
        return ACP_RC_ENOENT;
    }
}
