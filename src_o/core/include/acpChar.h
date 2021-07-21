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
 * $Id: acpChar.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_CHAR_H_)
#define _O_ACP_CHAR_H_

/**
 * @file
 * @ingroup CoreChar
 * @ingroup CoreType
 *
 * There are some differences between the stardard ctype functions
 * and the acpChar implementation
 * - standard ctype functions is that the standard ctype functions use parameter
 *   and return type as @c int and treat as <tt>unsigned char</tt>, but acpChar
 *   functions use #acp_char_t which is same as @c char.
 * - ctype test functions (in standard, whose name start with 'is') return
 *   #acp_bool_t whose value can be #ACP_TRUE or #ACP_FALSE.
 * - most of the implementation of standard ctype functions are locale
 *   sensitive, but acpChar implementation is not. acpChar assumes all
 *   characters are ASCII.
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN


/*
 * difference between upper-case letter and lower-case letter
 */
#define ACP_CHAR_CASE_DIFF ('a' - 'A')

/*
 * character class type
 */
typedef acp_uint16_t acpCharClass;

/*
 * character class mask
 */
#define ACP_CHAR_CNTRL ((acpCharClass)(0x0001))
#define ACP_CHAR_PRINT ((acpCharClass)(0x0002))
#define ACP_CHAR_GRAPH ((acpCharClass)(0x0004))
#define ACP_CHAR_DIGIT ((acpCharClass)(0x0008))
#define ACP_CHAR_PUNCT ((acpCharClass)(0x0010))
#define ACP_CHAR_SPACE ((acpCharClass)(0x0020))
#define ACP_CHAR_XDGIT ((acpCharClass)(0x0040))
#define ACP_CHAR_UPPER ((acpCharClass)(0x0080))
#define ACP_CHAR_LOWER ((acpCharClass)(0x0100))
#define ACP_CHAR_ALNUM (ACP_CHAR_DIGIT | ACP_CHAR_UPPER | ACP_CHAR_LOWER)
#define ACP_CHAR_ALPHA (ACP_CHAR_UPPER | ACP_CHAR_LOWER)

/*
 * get character class
 */
ACP_EXPORT acpCharClass acpCharClassOf(acp_char_t aChar);


/**
 * converts character to lower case, if possible.
 * @param aChar character to convert
 * @return converted character, or @a aChar if the conversion was not possible
 */
ACP_INLINE acp_char_t acpCharToLower(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_UPPER) != 0)
    {
        return aChar + ('a' - 'A');
    }
    else
    {
        return aChar;
    }
}

/**
 * converts character to upper case, if possible.
 * @param aChar character to convert
 * @return converted character, or @a aChar if the conversion was not possible
 */
ACP_INLINE acp_char_t acpCharToUpper(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_LOWER) != 0)
    {
        return aChar - ('a' - 'A');
    }
    else
    {
        return aChar;
    }
}

/**
 * checks for an alphanumeric character; it is equivalent to
 * (isalpha(@a aChar) || isdigit(@a aChar))
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsAlnum(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_ALNUM) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for an alphabetic character; it is equivalent to
 * (isupper(@a aChar) || islower(@a aChar))
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsAlpha(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_ALPHA) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks whether @a aChar is a 7-bit ASCII character
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsAscii(acp_char_t aChar)
{
    if ((aChar & 0x80) == 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for a control character
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsCntrl(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_CNTRL) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for a digit (0 through 9)
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsDigit(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_DIGIT) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for any printable character except space
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsGraph(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_GRAPH) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for a lower-case character
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsLower(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_LOWER) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for any printable character including space
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsPrint(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_PRINT) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for any printable character which is not a space or an alphanumeric
 * character
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsPunct(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_PUNCT) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for white-space characters. these are: space, form-feed('\\f'),
 * newline('\\n'), carriage return('\\r'), horizontal tab('\\t'), and
 * vertical tab('\\v')
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsSpace(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_SPACE) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for an upper-case character
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsUpper(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_UPPER) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}

/**
 * checks for a hexadecimal digits,
 * i.e. one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F
 * @param aChar character to check
 * @return #ACP_TRUE if it is, #ACP_FALSE if it is not
 */
ACP_INLINE acp_bool_t acpCharIsXdigit(acp_char_t aChar)
{
    if ((acpCharClassOf(aChar) & ACP_CHAR_XDGIT) != 0)
    {
        return ACP_TRUE;
    }
    else
    {
        return ACP_FALSE;
    }
}


ACP_EXTERN_C_END


#endif
