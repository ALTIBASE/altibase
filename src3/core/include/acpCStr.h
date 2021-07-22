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
 * $Id: acpCStr.h 11218 2010-06-10 08:13:05Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_CSTR_H_)
#define _O_ACP_CSTR_H_

/**
 * @file
 * @ingroup CoreChar
 *
 *  Altibase Core Const C-Style String
 */

#include <acpError.h>
#include <acpTypes.h>
#include <acpConfig.h>
#include <acpChar.h>

#include <acpTest.h>

ACP_EXTERN_C_BEGIN

#define ACP_CSTR_TERM '\0'


/**
 * compare or search in case sensitive manner
 */
#define ACP_CSTR_CASE_SENSITIVE   0x00
/**
 * compare or search in case insensitive manner
 */
#define ACP_CSTR_CASE_INSENSITIVE 0x01

/**
 * search forward
 */
#define ACP_CSTR_SEARCH_FORWARD   0x00
/**
 * search backward
 */
#define ACP_CSTR_SEARCH_BACKWARD  0x02


#if defined(ALTI_CFG_OS_LINUX)
#  define ACP_CSTR_STRNLEN_USE_NATIVE
#elif defined(ALTI_CFG_OS_WINDOWS)
# if (_MSC_VER >= 1400) /* VC 2005 */
#  define ACP_CSTR_STRNLEN_USE_NATIVE
# else /* below VC 2005 */
# endif
#else /* all others use our function*/
#endif


/**
 * Returns the number of characters in the string pointed to by @a aStr,
 * not including the terminating NULL character, but at most maxlen.
 * In doing this, #acpCStrLen looks only at the first maxlen characters
 * at @a aStr and never beyond @a aStr + @a aMaxLen.
 * @param aStr a NUL terminated string
 * @param aMaxLen maximum length to find NULL
 * @return length of @a aStr. Or @a aMaxLen, if there is no NULL
 * in the first @a aMaxLen character of @a aStr.
 */
ACP_EXPORT acp_size_t acpCStrLen(const acp_char_t*   aStr,
                                 acp_size_t          aMaxLen);

    

/**
 * Compares @a aStr1 and @a aStr2 in lexicographical order.
 * @param aStr1 a string
 * @param aStr2 a string
 * @param aLen length of string to compare
 * @return 
 * 0 if @a aStr1 == @a aStr2 or @a aLen == 0,
 * negative if @a aStr1 < @a aStr2,
 * positive if @a aStr1 > @a aStr2
 */
ACP_INLINE acp_sint32_t acpCStrCmp(const acp_char_t*   aStr1,
                                   const acp_char_t*   aStr2,
                                   acp_size_t          aLen)
{
    /*
     * The return value of strncmp varies with platform.
     * e.g. in Linux, strncmp("A", "K", 1) returns 'A' - 'K',
     * bug in Digital Unix, same code returns -1
     */
    return strncmp(aStr1, aStr2, aLen);
}

 /**
  * Compares @a aStr1 and @a aStr2 in lexicographical order
  * with case insensitive manner.
  * @param aStr1 a String
  * @param aStr2 a String
  * @param aLen Length of string to compare
  * @return 0 if @a aStr1 == @a aStr2, negative if @a aStr1 < @a aStr2,
  * positive if @a aStr1 > @a aStr2
  */
ACP_INLINE acp_sint32_t acpCStrCaseCmp(
    const acp_char_t*   aStr1,
    const acp_char_t*   aStr2,
    acp_size_t          aLen)

{
    acp_uint8_t sChar1;
    acp_uint8_t sChar2;

    if (aLen != 0)
    {
        do
        {
            sChar1 = (acp_uint8_t)acpCharToLower(*aStr1);
            sChar2 = (acp_uint8_t)acpCharToLower(*aStr2);

            if (sChar1 != sChar2)
            {
                return (acp_sint32_t)sChar1 - (acp_sint32_t)sChar2;
            }
            else if (sChar1 == ACP_CSTR_TERM)
            {
                break;
            }
            else
            {
                ++aStr1;
                ++aStr2;
            }

        } while (--aLen != 0);
    }
    else
    {
        /* Do nothing, return 0 */
    }

    return 0;
}

/**
 * Copies @a aSrc into @a aDest until NUL terminator,
 * at most @a aSrcLen characters.
 * After #acpCStrCpy, @a aDest is always NUL terminated
 * If return value is #ACP_RC_ETRUNC, @a aDest will contain
 * leading @a aDestSize - 1 characters of @a aSrc and NUL terminator.
 * @param aDest a buffer to store @a aSrc
 * @param aDestSize size of @a aDest
 * @param aSrc a string
 * @param aSrcLen max length of string to copy
 * @return #ACP_RC_SUCCESS when successful
 * @return #ACP_RC_EFAULT when @a aDest or @a aSrc is NULL,
 * @return #ACP_RC_ETRUNC when strnlen(@a aSrc, @a aSrcLen) >= @a aDestSize,
 */
ACP_INLINE acp_rc_t acpCStrCpy(
    acp_char_t*         aDest,
    acp_size_t          aDestSize,
    const acp_char_t*   aSrc,
    acp_size_t          aSrcLen)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if ( (aDest == NULL) || (aSrc == NULL) )
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        if ( aSrcLen <= 0 )
        {
            if ( aDestSize == 0 )
            {
                sRC = ACP_RC_ETRUNC;
            }
            else
            {
                *aDest = ACP_CSTR_TERM;
                sRC = ACP_RC_SUCCESS;
            }
        }
        else if ( aDestSize == 0 )
        {
            sRC = ACP_RC_ETRUNC;
        }
        else
        {
            /* Assume success */
            for ( ; aSrcLen > 0; )
            {
                aDestSize--;
                aSrcLen--;

                /* Reached end of destination */
                if ( aDestSize == 0 )
                {
                    /* Ensure Null-terminated */
                    *aDest = ACP_CSTR_TERM;
                    sRC = ACP_RC_ETRUNC;
                    break;
                }
                else
                {
                    /* Continue copy */
                    *aDest = *aSrc;
                    aDest++;
                    aSrc++;
                }

                /* Reached end of source */
                if ( (aSrcLen == 0) || ((*aSrc) == ACP_CSTR_TERM) )
                {
                    /* Terminator! */
                    *aDest = ACP_CSTR_TERM;
                    sRC = ACP_RC_SUCCESS;
                    break;
                }
                else
                {
                    /* Continue */
                }
            }
        }
    }

    return sRC;
}

/**
 * Concatenates @a aSrc into @a aDest, at most @a aSrcLen characters
 * After #acpCStrCat, @a aDest is always NUL terminated
 *
 * if @a aDestSize == strlen(@a aDest),
 * there is no space to copy @a aSrc
 * and the return value will be #ACP_RC_ETRUNC.
 *
 * If strlen(@a aDest) < @a aDestSize and return value is #ACP_RC_ETRUNC,
 * @a aDest will be concatenated with
 * leading (@a aDestSize - strlen(@a aDest) - 1) characters of @a aSrc and
 * NUL terminator.
 *
 * @param aDest a Buffer to concat @a aSrc
 * @param aDestSize Size of @a aDest
 * @param aSrc a NUL terminated String
 * @param aSrcLen Max length of string to concatenate
 * @return #ACP_RC_SUCCESS when successful,
 * @return #ACP_RC_EFAULT when @a aDest or @a aSrc is NULL,
 * @return #ACP_RC_ETRUNC when 
 * strnlen(@a aSrc, @a aSrcLen) > @a aDestSize - strlen(@a aDest)
 */
ACP_INLINE acp_rc_t acpCStrCat(
    acp_char_t*         aDest,
    acp_size_t          aDestSize,
    const acp_char_t*   aSrc,
    acp_size_t          aSrcLen)
{
    acp_rc_t    sRC;
    acp_size_t  sDestEnd;
    acp_size_t  sRemainLen;

    if((NULL == aDest) || (NULL == aSrc))
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
        sDestEnd = acpCStrLen(aDest, aDestSize);
        if(sDestEnd == aDestSize)
        {
            sRC = ACP_RC_ETRUNC;
        }
        else
        {
            sRemainLen = aDestSize - sDestEnd;
            sRC = acpCStrCpy(aDest + sDestEnd, sRemainLen, aSrc, aSrcLen);
        }
    }

    return sRC;
}

/**
 * Converts a 32bit unsigned integer into string at max @a aLen
 * length with base 10.
 * @param aNumber input number.
 * @param aStr string to store result. Always null terminated after conversion.
 * @param aLen max length of string.
 * @return #ACP_RC_SUCCESS successful.
 * @return #ACP_RC_ETRUNC @a aLen is too short to contain @a aNumber.
 * @return #ACP_RC_EINVAL @a aStr is NULL or aLen is 0.
 */
ACP_INLINE acp_rc_t acpCStrUInt32ToCStr10(acp_uint32_t aNumber,
                                          acp_char_t*  aStr,
                                          acp_size_t   aLen)
{
    acp_rc_t        sRC;
    acp_size_t      sLen;
    acp_uint32_t    sRadix;

    ACP_TEST_RAISE((aStr == NULL) || (aLen <= 0), EINVALARG);

    if(aNumber == 0)
    {
        ACP_TEST_RAISE(aLen < 2, ETOOSHORT);
        aStr[0] = '0';
        aStr[1] = ACP_CSTR_TERM;
    }
    else
    {
        for(sRadix = 1, sLen = 0; (aNumber / sRadix) >= 10; sRadix *= 10)
        {
            sLen++;
            ACP_TEST_RAISE(sLen + 1 >= aLen, ETOOSHORT);
        }
        aStr[sLen + 1] = ACP_CSTR_TERM;

        while(aNumber > 0)
        {
            aStr[sLen] = (acp_char_t)('0' + (aNumber % 10));
            aNumber /= 10;
            sLen--;
        }
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EINVALARG)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(ETOOSHORT)
    {
        sRC = ACP_RC_ETRUNC;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Converts a 32bit signed integer into string at max @a aLen
 * length with base 10.
 * @param aNumber input number.
 * @param aStr string to store result. Always null terminated after conversion.
 * @param aLen max length of string.
 * @return #ACP_RC_SUCCESS successful.
 * @return #ACP_RC_ETRUNC @a aLen is too short to contain @a aNumber.
 * @return #ACP_RC_EINVAL @a aStr is NULL or aLen is 0.
 */
ACP_INLINE acp_rc_t acpCStrSInt32ToCStr10(acp_sint32_t aNumber,
                                          acp_char_t*  aStr,
                                          acp_size_t   aLen)
{
    acp_rc_t sRC;
    ACP_TEST_RAISE((aStr == NULL) || (aLen <= 0), EINVALARG);

    if(aNumber < 0)
    {
        *aStr = '-';
        aStr++;
        aLen--;
        aNumber = -aNumber;
    }
    else
    {
        /* do nothing */
    }
        
    sRC = acpCStrUInt32ToCStr10((acp_uint32_t)aNumber, aStr, aLen);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), EFAIL);
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EINVALARG)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(EFAIL);
    {
        /* nothing to do */
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Converts a 32bit integer into string at max @a aLen
 * length with base 16.
 * @param aNumber input number.
 * @param aStr string to store result. Always null terminated after conversion.
 * @param aLen max length of string.
 * @param aCap Convert with capital letters
 * @return #ACP_RC_SUCCESS successful.
 * @return #ACP_RC_ETRUNC @a aLen is too short to contain @a aNumber.
 * @return #ACP_RC_EINVAL @a aStr is NULL or aLen is 0.
 */
ACP_INLINE acp_rc_t acpCStrInt32ToCStr16(acp_uint32_t aNumber,
                                         acp_char_t*  aStr,
                                         acp_size_t   aLen,
                                         acp_bool_t   aCap)
{
    acp_rc_t        sRC;
    acp_size_t      sLen;
    acp_uint32_t    sRadix;

    static acp_char_t sCapDigits[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };
    static acp_char_t sDigits[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'
    };

    ACP_TEST_RAISE((aStr == NULL) || (aLen <= 0), EINVALARG);

    if(aNumber == 0)
    {
        ACP_TEST_RAISE(aLen <= 2, ETOOSHORT);
        aStr[0] = '0';
        aStr[1] = ACP_CSTR_TERM;
    }
    else
    {
        /* pass leading 0s */
        sRadix = 0x10000000;
        while(aNumber / sRadix == 0)
        {
            sRadix /= 16;
        }

        /* conversion begin */
        sLen = 0;
        while(sRadix > 0)
        {
            if(aCap == ACP_TRUE)
            {
                aStr[sLen] = sCapDigits[(aNumber / sRadix) % 16];
            }
            else
            {
                aStr[sLen] = sDigits[(aNumber / sRadix) % 16];
            }
            sLen++;
            sRadix /= 16;
            ACP_TEST_RAISE(sLen >= aLen, ETOOSHORT);
        }

        aStr[sLen] = ACP_CSTR_TERM;
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EINVALARG)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(ETOOSHORT)
    {
        sRC = ACP_RC_ETRUNC;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Converts a 64bit unsigned integer into string at max @a aLen
 * length with base 10.
 * @param aNumber input number.
 * @param aStr string to store result. Always null terminated after conversion.
 * @param aLen max length of string.
 * @return #ACP_RC_SUCCESS successful.
 * @return #ACP_RC_ETRUNC @a aLen is too short to contain @a aNumber.
 * @return #ACP_RC_EINVAL @a aStr is NULL or aLen is 0.
 */
ACP_INLINE acp_rc_t acpCStrUInt64ToCStr10(acp_uint64_t aNumber,
                                          acp_char_t*  aStr,
                                          acp_size_t   aLen)
{
    acp_rc_t        sRC;
    acp_size_t      sLen;
    acp_uint64_t    sRadix;

    ACP_TEST_RAISE((aStr == NULL) || (aLen <= 0), EINVALARG);

    if(aNumber == 0)
    {
        ACP_TEST_RAISE(aLen <= 2, ETOOSHORT);
        aStr[0] = '0';
        aStr[1] = ACP_CSTR_TERM;
    }
    else
    {
        for(sRadix = 1, sLen = 0; (aNumber / sRadix) >= 10; sRadix *= 10)
        {
            sLen++;
            ACP_TEST_RAISE(sLen + 1 >= aLen, ETOOSHORT);
        }
        aStr[sLen + 1] = ACP_CSTR_TERM;

        while(aNumber > 0)
        {
            aStr[sLen] = (acp_char_t)('0' + (aNumber % 10));
            aNumber /= 10;
            sLen--;
        }
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EINVALARG)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(ETOOSHORT)
    {
        sRC = ACP_RC_ETRUNC;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Converts a 64bit signed integer into string at max @a aLen
 * length with base 10.
 * @param aNumber input number.
 * @param aStr string to store result. Always null terminated after conversion.
 * @param aLen max length of string.
 * @return #ACP_RC_SUCCESS successful.
 * @return #ACP_RC_ETRUNC @a aLen is too short to contain @a aNumber.
 * @return #ACP_RC_EINVAL @a aStr is NULL or aLen is 0.
 */
ACP_INLINE acp_rc_t acpCStrSInt64ToCStr10(acp_sint64_t aNumber,
                                          acp_char_t*  aStr,
                                          acp_size_t   aLen)
{
    acp_rc_t sRC;
    ACP_TEST_RAISE((aStr == NULL) || (aLen <= 0), EINVALARG);

    if(aNumber < 0)
    {
        *aStr = '-';
        aStr++;
        aLen--;
        aNumber = -aNumber;
    }
    else
    {
        /* do nothing */
    }
        
    sRC = acpCStrUInt64ToCStr10((acp_uint64_t)aNumber, aStr, aLen);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), EFAIL);
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EINVALARG)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(EFAIL);
    {
        /* nothing to do */
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Converts a 64bit integer into string at max @a aLen
 * length with base 16.
 * @param aNumber input number.
 * @param aStr string to store result. Always null terminated after conversion.
 * @param aLen max length of string.
 * @param aCap Convert with capital letters
 * @return #ACP_RC_SUCCESS successful.
 * @return #ACP_RC_ETRUNC @a aLen is too short to contain @a aNumber.
 * @return #ACP_RC_EINVAL @a aStr is NULL or aLen is 0.
 */
ACP_INLINE acp_rc_t acpCStrInt64ToCStr16(acp_uint64_t aNumber,
                                         acp_char_t*  aStr,
                                         acp_size_t   aLen,
                                         acp_bool_t   aCap)
{
    acp_rc_t        sRC;
    acp_size_t      sLen;
    acp_uint64_t    sRadix;

    static acp_char_t sCapDigits[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };
    static acp_char_t sDigits[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'
    };

    ACP_TEST_RAISE((aStr == NULL) || (aLen <= 0), EINVALARG);

    if(aNumber == 0)
    {
        ACP_TEST_RAISE(aLen <= 2, ETOOSHORT);
        aStr[0] = '0';
        aStr[1] = ACP_CSTR_TERM;
    }
    else
    {
        /* pass leading 0s */
        sRadix = ACP_UINT64_LITERAL(0x1000000000000000);
        while(aNumber / sRadix == 0)
        {
            sRadix /= 16;
        }

        /* conversion begin */
        sLen = 0;
        while(sRadix > 0)
        {
            if(aCap == ACP_TRUE)
            {
                aStr[sLen] = sCapDigits[(aNumber / sRadix) % 16];
            }
            else
            {
                aStr[sLen] = sDigits[(aNumber / sRadix) % 16];
            }
            sLen++;
            sRadix /= 16;
            ACP_TEST_RAISE(sLen >= aLen, ETOOSHORT);
        }

        aStr[sLen] = ACP_CSTR_TERM;
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EINVALARG)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(ETOOSHORT)
    {
        sRC = ACP_RC_ETRUNC;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Converts the string into 32bit integer with signed infomation.
 * If @a aBase is 0, the base will be decided by @a aStr.
 * If @a aStr begins with "0x"("0X"), base will be 16 (hexadecimal).
 * If @a aStr begins with "0", base will be 8 (octal).
 * Otherwise base will be 10
 * *@a aEnd will point the first invalid character after conversion,
 * eg) if @a aStr points "123ABC", *@a aEnd will point "ABC" after conversion.
 * If return value is #ACP_RC_ERANGE, *@a aResult will be #ACP_UINT32_MAX
 * @param aStr input string buffer
 * @param aStrLen length of string
 * @param aSign info of signess (-1 or 1)
 * @param aResult 32bit value converted
 * @param aBase base, 0 or the number from 2 to 36
 * @param aEnd pointer to store the position of the first invalid character
 * @return #ACP_RC_SUCCESS successful
 * @return #ACP_RC_EFAULT when @a aStr, @a aSign and/or @a aResult is NULL
 * @return #ACP_RC_EINVAL when @a aBase is not 0 and not between 2 and 36
 * @return #ACP_RC_ERANGE when conversion result exceeds #ACP_UINT32_MAX
 */
ACP_EXPORT acp_rc_t  acpCStrToInt32(
    const acp_char_t* aStr,
    acp_size_t        aStrLen,
    acp_sint32_t*     aSign,
    acp_uint32_t*     aResult,
    acp_sint32_t      aBase,
    acp_char_t**      aEnd);

/**
 * Converts the string into 64bit integer with signed infomation.
 * If @a aBase is 0, the base will be decided by @a aStr.
 * If @a aStr begins with "0x"("0X"), base will be 16 (hexadecimal).
 * If @a aStr begins with "0", base will be 8 (octal).
 * Otherwise base will be 10
 * *@a aEnd will point the first invalid character after conversion,
 * eg) if @a aStr points "123ABC", *@a aEnd will point "ABC" after conversion.
 * If return value is #ACP_RC_ERANGE, @a aResult will be #ACP_UINT64_MAX
 * @param aStr input string buffer
 * @param aStrLen length of string
 * @param aSign info of signess (-1 or 1)
 * @param aResult 64bit value converted
 * @param aBase base, 0 or the number from 2 to 36
 * @param aEnd pointer to store the position of the first invalid character
 * @return #ACP_RC_SUCCESS successful
 * @return #ACP_RC_EFAULT when @a aStr, @a aSign or @a aResult is NULL
 * @return #ACP_RC_EINVAL when @a aBase is not 0 and not between 2 and 36
 * @return #ACP_RC_ERANGE when conversion result exceeds #ACP_UINT64_MAX
 */
ACP_EXPORT acp_rc_t  acpCStrToInt64(
    const acp_char_t* aStr,
    acp_size_t        aStrLen,
    acp_sint32_t*     aSign,
    acp_uint64_t*     aResult,
    acp_sint32_t      aBase,
    acp_char_t**      aEnd);

/**
 * Converts the string to double data.
 * If @a aStr is "INF", *@a aResult will be infinity.
 * If @a aStr is "-INF", *@a aResult will be -infinity.
 * If @a aStr is "NAN" or "-NAN" *@a aResult will be NAN(not a number).
 * When return value is #ACP_RC_ERANGE and the absolute of the result
 * exceeds double range, *@a aResult will be INF or -INF
 * with regard to the sign.
 * When return value is #ACP_RC_ERANGE and the absolute of the result
 * is smaller than subnormal minimum (approximately 5E-324),
 * *@a aResult will be 0.0 or -0.0 with regard to the sign.
 * @param aStr input string buffer
 * @param aStrLen length of string
 * @param aResult double value converted
 * @param aEnd pointer to store the position of the first invalid character
 * @return #ACP_RC_SUCCESS successful
 * @return #ACP_RC_ERANGE when conversion result cannot be a double value
 */
ACP_EXPORT acp_rc_t  acpCStrToDouble(
    const acp_char_t* aStr,
    acp_size_t        aStrLen,
    acp_double_t*     aResult,
    acp_char_t**      aEnd);

/**
 * Converts the US7ASCII-String to uppercase
 * @param aTarget string buffer
 * @param aLen length of string
 * @return @a aTarget
 */
ACP_INLINE acp_char_t* acpCStrToUpper(acp_char_t* aTarget, acp_uint32_t aLen)
{
    acp_char_t* sTR = aTarget;

    while(ACP_CSTR_TERM != *sTR && aLen > 0)
    {
        *sTR = acpCharToUpper(*sTR);
        sTR++;
        aLen--;
    }
    return aTarget;
}

/**
 * Converts the US7ASCII-String to lowercase
 * @param aTarget string buffer
 * @param aLen length of string
 * @return @a aTarget
 */
ACP_INLINE acp_char_t* acpCStrToLower(acp_char_t* aTarget, acp_uint32_t aLen)
{
    acp_char_t* sTR = aTarget;

    while(ACP_CSTR_TERM != *sTR && aLen > 0)
    {
        *sTR = acpCharToLower(*sTR);
        sTR++;
        aLen--;
    }
    return aTarget;
}

typedef enum acpCStrDoubleType
{
    ACP_DOUBLE_NORMAL,
    ACP_DOUBLE_INF,
    ACP_DOUBLE_NAN
} acpCStrDoubleType;

acpCStrDoubleType acpCStrDoubleToString(acp_double_t   aValue,
                                        acp_sint32_t   aMode,
                                        acp_sint32_t   aPrecision,
                                        acp_sint32_t  *aDecimalPoint,
                                        acp_bool_t    *aNegative,
                                        acp_char_t   **aString,
                                        acp_char_t   **aEndPtr);

void acpCStrDoubleToStringFree( acp_char_t *aString );



/**
 * Returns the index of the first occurrence of @a aFindCStr in @a aCStr
 * @param aCStr pointer to the source C-string
 * @param aFindCStr null terminated string (pointer to the character array)
 * to search
 * @param aFoundIndex pointer to a variable to store found index
 * @param aFromIndex the index to start searching from
 * (excluding the character at the index)
 * @param aFlag bit flag for search option; bitwise ORed value of
 * case sensitivity (#ACP_CSTR_CASE_SENSITIVE, #ACP_CSTR_CASE_INSENSITIVE) and
 * search direction (#ACP_CSTR_SEARCH_FORWARD, #ACP_CSTR_SEARCH_BACKWARD)
 * If this value is zero(0), default option;
 * #ACP_CSTR_CASE_SENSITIVE | #ACP_CSTR_SEARCH_FORWARD shall be used
 * @return #ACP_RC_SUCCESS successful
 * @return #ACP_RC_ENOENT if the string is not found,
 * @return #ACP_RC_EINVAL if the search string is null or empty string.
 *
 * @see acpCStrFindChar()
 */
ACP_EXPORT acp_rc_t acpCStrFindCStr(const acp_char_t *aCStr,
                                    const acp_char_t *aFindCStr,
                                    acp_sint32_t *aFoundIndex,
                                    acp_sint32_t aFromIndex,
                                    acp_sint32_t aFlag);


/**
 * Returns the index of the first occurrence of @a aFindChar in @a aCStr
 * @param aCStr pointer to the source C-string
 * @param aFindChar a character to search
 * @param aFoundIndex pointer to a variable to store found index
 * @param aFromIndex the index to start searching from
 * (excluding the character at the index)
 * @param aFlag bit flag for search option; bitwise ORed value of
 * case sensitivity (#ACP_CSTR_CASE_SENSITIVE, #ACP_CSTR_CASE_INSENSITIVE) and
 * search direction (#ACP_CSTR_SEARCH_FORWARD, #ACP_CSTR_SEARCH_BACKWARD)
 * If this value is zero(0), default option;
 * #ACP_CSTR_CASE_SENSITIVE | #ACP_CSTR_SEARCH_FORWARD shall be used
 * @return #ACP_RC_SUCCESS successful
 * @return #ACP_RC_ENOENT if the character is not found.
 *
 * @see acpCStrFindCString()
 */
ACP_EXPORT acp_rc_t acpCStrFindChar(acp_char_t *aCStr,
                                    acp_char_t aFindChar,
                                    acp_sint32_t *aFoundIndex,
                                    acp_sint32_t aFromIndex,
                                    acp_sint32_t aFlag);




ACP_EXTERN_C_END

#endif /* of _O_ACP_CSTR_H_ */
