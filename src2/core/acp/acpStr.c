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
 * $Id: acpStr.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

/**
 * @example sampleAcpStr.c
 */
/**
 * @example sampleAcpStrFindChar.c
 */
/**
 * @example sampleAcpStrToInteger.c
 */

#include <acpChar.h>
#include <acpMem.h>
#include <acpPrintfPrivate.h>
#include <acpStr.h>
#include <acpTest.h>

#define ACP_STR_SURE_MODIFIABLE(aStr)           \
    do                                          \
    {                                           \
        /* Filter invalid arguments */          \
        if(NULL == (aStr))                      \
        {                                       \
            /* Invalid Argument */              \
            return ACP_RC_EINVAL;               \
        }                                       \
        else                                    \
        {                                       \
            /* Fall through */                  \
        }                                       \
        if((aStr)->mSize == -1)                 \
        {                                       \
            /* Modification on Constant */      \
            return ACP_RC_EACCES;               \
        }                                       \
        else                                    \
        {                                       \
            /* Fall through */                  \
        }                                       \
        if(((aStr)->mSize == 0) && ((aStr)->mExtendSize <= 0))  \
        {                                                  \
            /* Modification on Empty Dynamic Buffer */     \
            return ACP_RC_EINVAL;               \
        }                                       \
        else                                    \
        {                                       \
            /* Fall through */                  \
        }                                       \
        if(((aStr)->mSize < -1) && ((aStr)->mExtendSize < -1))  \
        {                                                  \
            /* Modification on Empty Dynamic Buffer */     \
            return ACP_RC_EINVAL;               \
        }                                       \
        else                                    \
        {                                       \
            /* Fall through */                  \
        }                                       \
    } while (0)

#define ACP_STR_SURE_ALLOC_BUFFER(aStr)         \
    do                                          \
    {                                           \
        ACP_STR_SURE_MODIFIABLE((aStr));        \
        sRC = acpStrAllocBuffer((aStr));        \
                                                \
        if (ACP_RC_NOT_SUCCESS(sRC))            \
        {                                       \
            return sRC;                         \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

#define ACP_STR_TOL_RETURN(aRC)                                 \
    if (aEndIndex != NULL)                                      \
    {                                                           \
        if (sPtr != NULL)                                       \
        {                                                       \
            *aEndIndex = (acp_sint32_t)(sPtr - aStr->mString);    \
        }                                                       \
        else                                                    \
        {                                                       \
            *aEndIndex = 0;                                     \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
    }                                                           \
                                                                \
    return (aRC)

#define ACP_STR_TOL_RETURN_NOTNULL(aRC)                         \
    if (aEndIndex != NULL)                                      \
    {                                                           \
        *aEndIndex = (acp_sint32_t)(sPtr - aStr->mString);    \
    }                                                           \
    else                                                        \
    {                                                           \
    }                                                           \
                                                                \
    return (aRC)


static acp_rc_t acpStrPrintfPutChrToString(acpPrintfOutput *aOutput,
                                           acp_char_t       aChar)
{
    acp_str_t  *sStr  = aOutput->mString;
    acp_size_t  sSize = acpStrGetBufferSize(sStr);
    acp_rc_t    sRC;

    if (sSize > (aOutput->mWritten + 2))
    {
        sStr->mString[aOutput->mWritten] = aChar;
        sStr->mLength++;
    }
    else
    {
        if (sStr->mExtendSize > 0)
        {
            sRC = acpStrExtendBuffer(sStr, sSize + 2);

            if (ACP_RC_IS_SUCCESS(sRC))
            {
                sStr->mString[aOutput->mWritten] = aChar;
                sStr->mLength++;
            }
            else
            {
                return sRC;
            }
        }
        else
        {
            /* do nothing */
        }
    }

    aOutput->mWritten++;

    return ACP_RC_SUCCESS;
}

static acp_rc_t acpStrPrintfPutStrToString(acpPrintfOutput  *aOutput,
                                           const acp_char_t *aString,
                                           acp_size_t        aLen)
{
    acp_str_t  *sStr  = aOutput->mString;
    acp_size_t  sSize = acpStrGetBufferSize(sStr);
    acp_size_t  sCopy;
    acp_rc_t    sRC;

    if (aString == NULL)
    {
        aString = ACP_PRINTF_NULL_STRING;
        aLen    = ACP_PRINTF_NULL_LENGTH;
    }
    else
    {
        /* do nothing */
    }

    if (sSize > (aLen + sStr->mLength))
    {
        acpMemCpy(sStr->mString + sStr->mLength, aString, aLen);

        sStr->mLength += (acp_sint32_t)aLen;
    }
    else
    {
        if (sStr->mExtendSize > 0)
        {
            sRC = acpStrExtendBuffer(sStr, aLen + sStr->mLength + 1);

            if (ACP_RC_IS_SUCCESS(sRC))
            {
                acpMemCpy(sStr->mString + sStr->mLength, aString, aLen);

                sStr->mLength += (acp_sint32_t)aLen;
            }
            else
            {
                return sRC;
            }
        }
        else
        {
            sCopy = ACP_MIN((acp_size_t)(sSize - sStr->mLength - 1), aLen);

            acpMemCpy(sStr->mString + sStr->mLength, aString, sCopy);

            sStr->mLength += (acp_sint32_t)sCopy;
        }
    }

    aOutput->mWritten += aLen;

    return ACP_RC_SUCCESS;
}

static acp_rc_t acpStrPrintfPutPadToString(acpPrintfOutput *aOutput,
                                           acp_char_t       aPadder,
                                           acp_sint32_t     aLen)
{
    acp_str_t  *sStr  = aOutput->mString;
    acp_size_t  sSize = acpStrGetBufferSize(sStr);
    acp_size_t  sFill;
    acp_rc_t    sRC;

    if (sSize > (acp_size_t)(aLen + sStr->mLength))
    {
        acpMemSet(sStr->mString + sStr->mLength, aPadder, aLen);

        sStr->mLength += (acp_sint32_t)aLen;
    }
    else
    {
        if (sStr->mExtendSize > 0)
        {
            sRC = acpStrExtendBuffer(sStr, aLen + sStr->mLength + 1);

            if (ACP_RC_IS_SUCCESS(sRC))
            {
                acpMemSet(sStr->mString + sStr->mLength, aPadder, aLen);

                sStr->mLength += (acp_sint32_t)aLen;
            }
            else
            {
                return sRC;
            }
        }
        else
        {
            sFill = ACP_MIN((acp_size_t)(sSize - sStr->mLength - 1),
                            (acp_size_t)aLen);

            acpMemSet(sStr->mString + sStr->mLength, aPadder, sFill);

            sStr->mLength += (acp_sint32_t)sFill;
        }
    }

    aOutput->mWritten += aLen;

    return ACP_RC_SUCCESS;
}

static acpPrintfOutputOp gAcpStrPrintfOutputOpToString =
{
    acpStrPrintfPutChrToString,
    acpStrPrintfPutStrToString,
    acpStrPrintfPutPadToString
};


/**
 * copies the contents of memory buffer to the string object
 *
 * @param aStr pointer to the string object
 * @param aBuffer pointer to the buffer to be copied
 * @param aSize size of buffer
 * @return result code
 *
 * if the string is truncated because of the not enough internal buffer
 * of string object, it returns #ACP_RC_ETRUNC.
 */
ACP_EXPORT acp_rc_t acpStrCpyBuffer(acp_str_t        *aStr,
                                    const acp_char_t *aBuffer,
                                    acp_size_t        aSize)
{
    acp_size_t sSize;
    acp_rc_t   sRC;

    if (aBuffer == NULL || aStr == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* no problem on the argument */
    }
    
    
        
    ACP_STR_SURE_ALLOC_BUFFER(aStr);
    
    sSize = acpStrGetBufferSize(aStr);

    if (aSize >= sSize)
    {
        if (aStr->mExtendSize > 0)
        {
            sRC = acpStrExtendBuffer(aStr, aSize + 1);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                return sRC;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            aSize = (sSize > 0) ? (sSize - 1) : 0;
            sRC   = ACP_RC_ETRUNC;
        }
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

#if defined(__STATIC_ANALYSIS_DOING__)
    /* only to prevent the FALSE ALARM in codesonar */
    if (aStr->mString == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {

    }
    
#endif
    
    acpMemCpy(aStr->mString, aBuffer, aSize);

    aStr->mLength                = (acp_sint32_t)aSize;
    aStr->mString[aStr->mLength] = ACP_CSTR_TERM;

    return sRC;
}

/**
 * prints format string to the string object
 *
 * @param aStr pointer to the string object
 * @param aFormat format string
 * @param ... variable number of arguments to be converted
 * @return result code
 *
 * @see acpPrintf.h
 *
 * if the string is truncated because of the not enough internal buffer
 * of string object, it returns #ACP_RC_ETRUNC.
 */
ACP_EXPORT acp_rc_t acpStrCpyFormat(acp_str_t        *aStr,
                                    const acp_char_t *aFormat, ...)
{
    acp_rc_t sRC;
    va_list  sArgs;

    acpStrClear(aStr);

    va_start(sArgs, aFormat);

    sRC = acpStrCatFormatArgs(aStr, aFormat, sArgs);

    va_end(sArgs);

    return sRC;
}

/**
 * prints format string to the string object
 *
 * @param aStr pointer to the string object
 * @param aFormat format string
 * @param aArgs @c va_list of arguments to be converted
 * @return result code
 *
 * @see acpPrintf.h
 *
 * if the string is truncated because of the not enough internal buffer
 * of string object, it returns #ACP_RC_ETRUNC.
 */
ACP_EXPORT acp_rc_t acpStrCpyFormatArgs(acp_str_t        *aStr,
                                        const acp_char_t *aFormat,
                                        va_list           aArgs)
{
    acpStrClear(aStr);

    return acpStrCatFormatArgs(aStr, aFormat, aArgs);
}

/**
 * appends the contents of memory buffer to the string object
 *
 * @param aStr pointer to the string object
 * @param aBuffer pointer to the buffer to be copied
 * @param aSize size of buffer
 * @return result code
 *
 * if the string is truncated because of the not enough internal buffer
 * of string object, it returns #ACP_RC_ETRUNC.
 *
 * it returns #ACP_RC_EFAULT if @a aStr has constant string
 */
ACP_EXPORT acp_rc_t acpStrCatBuffer(acp_str_t        *aStr,
                                    const acp_char_t *aBuffer,
                                    acp_size_t        aSize)
{
    acp_size_t sSize;
    acp_size_t sNewSize;
    acp_rc_t   sRC;

    if (aBuffer == NULL || aStr == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* no problem on the argument */
    }

    sSize    = acpStrGetBufferSize(aStr);
    sNewSize = acpStrGetLength(aStr) + aSize;
    
    ACP_STR_SURE_ALLOC_BUFFER(aStr);
    
    if (sNewSize >= sSize)
    {
        if (aStr->mExtendSize > 0)
        {
            sRC = acpStrExtendBuffer(aStr, sNewSize + 1);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                return sRC;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            aSize = (sSize > 0) ? (sSize - aStr->mLength - 1) : 0;
            sRC   = ACP_RC_ETRUNC;
        }
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

#if defined(__STATIC_ANALYSIS_DOING__)
    /* only to prevent the FALSE ALARM in codesonar */
    if (aStr->mString == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {

    }
    
#endif

    
    acpMemCpy(aStr->mString + aStr->mLength, aBuffer, aSize);

    aStr->mLength                += (acp_sint32_t)aSize;
    aStr->mString[aStr->mLength]  = ACP_CSTR_TERM;

    return sRC;
}

/**
 * appends format string to the string object
 *
 * @param aStr pointer to the string object
 * @param aFormat format string
 * @param ... variable number of arguments to be converted
 * @return result code
 *
 * @see acpPrintf.h
 *
 * if the string is truncated because of the not enough internal buffer
 * of string object, it returns #ACP_RC_ETRUNC.
 *
 * it returns #ACP_RC_EFAULT if @a aStr has constant string
 */
ACP_EXPORT acp_rc_t acpStrCatFormat(acp_str_t        *aStr,
                                    const acp_char_t *aFormat, ...)
{
    acp_rc_t sRC;
    va_list  sArgs;

    va_start(sArgs, aFormat);

    sRC = acpStrCatFormatArgs(aStr, aFormat, sArgs);

    va_end(sArgs);

    return sRC;
}

/**
 * appends format string to the string object
 *
 * @param aStr pointer to the string object
 * @param aFormat format string
 * @param aArgs @c va_list of arguments to be converted
 * @return result code
 *
 * @see acpPrintf.h
 *
 * if the string is truncated because of the not enough internal buffer
 * of string object, it returns #ACP_RC_ETRUNC.
 *
 * it returns #ACP_RC_EFAULT if @a aStr has constant string
 */
ACP_EXPORT acp_rc_t acpStrCatFormatArgs(acp_str_t        *aStr,
                                        const acp_char_t *aFormat,
                                        va_list           aArgs)
{
    acp_rc_t        sRC;
    acpPrintfOutput sOutput;

    ACP_STR_SURE_ALLOC_BUFFER(aStr);

    sOutput.mWritten    = acpStrGetLength(aStr);
    sOutput.mFile       = NULL;
    sOutput.mBuffer     = NULL;
    sOutput.mBufferSize = (acp_size_t)0;
    sOutput.mString     = aStr;
    sOutput.mOp         = &gAcpStrPrintfOutputOpToString;

    sRC = acpPrintfCore(&sOutput, aFormat, aArgs);

    /*
     * null terminate
     */
    if ((acpStrGetLength(aStr) > 0) && (aStr->mSize != -1))
    {
        aStr->mString[aStr->mLength] = ACP_CSTR_TERM;
    }
    else
    {
        /* do nothing */
    }

    /*
     * error if the printed string is truncated
     */
    if (ACP_RC_IS_SUCCESS(sRC) &&
        (sOutput.mWritten > (acp_size_t)aStr->mLength))
    {
        sRC = ACP_RC_ETRUNC;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * replaces a character in the string
 * @param aStr pointer to the string object
 * @param aFindChar a character to be replaced
 * @param aReplaceChar a character to replace with
 * @param aRangeFrom index to search start including the character at the index
 * @param aRangeLength the length of the replace range
 * @return result code
 *
 * if @a aLength is less than zero, it will replace to the end of the string.
 *
 * it returns #ACP_RC_ENOENT if no replacement occurred
 * it returns #ACP_RC_EACCES if aStr is constant
 */
ACP_EXPORT acp_rc_t acpStrReplaceChar(acp_str_t    *aStr,
                                      acp_char_t    aFindChar,
                                      acp_char_t    aReplaceChar,
                                      acp_sint32_t  aRangeFrom,
                                      acp_sint32_t  aRangeLength)
{
    acp_rc_t     sRC  = ACP_RC_ENOENT;
    acp_sint32_t sLen = (acp_sint32_t)acpStrGetLength(aStr);
    acp_sint32_t sRangeTo;
    acp_sint32_t i;

    ACP_STR_SURE_MODIFIABLE(aStr);

    if (aRangeFrom < 0)
    {
        aRangeFrom = 0;
    }
    else if (aRangeFrom >= sLen)
    {
        return ACP_RC_ERANGE;
    }
    else
    {
        /* do nothing */
    }

    if (aRangeLength < 0)
    {
        sRangeTo = sLen;
    }
    else
    {
        sRangeTo = aRangeFrom + aRangeLength;

        if (sRangeTo > sLen)
        {
            return ACP_RC_ERANGE;
        }
        else
        {
            /* do nothing */
        }
    }

    for (i = aRangeFrom; i < sRangeTo; i++)
    {
        if (aStr->mString[i] == aFindChar)
        {
            aStr->mString[i] = aReplaceChar;

            sRC = ACP_RC_SUCCESS;
        }
        else
        {
            /* do nothing */
        }
    }

    /* case in BUG-19886:
     * refixing the length : in case of 0 replace
     * (void)acpStrReplaceChar(aGetString, '\n', 0, 0, -1);
     *   => in this case, a wrong length will be returned.
     */
    aStr->mLength = (acp_sint32_t)strlen(aStr->mString);

    return sRC;
}

ACP_INLINE acp_rc_t acpStrTolGetBase(const acp_char_t **aPtr,
                                     acp_sint32_t      *aBase)
{
    if (*aBase == 0)
    {
        /*
         * 0x for hex, 0 for octal, else assume decimal
         */
        if (**aPtr == '0')
        {
            if (acpCharToLower(*(*aPtr + 1)) == 'x')
            {
                *aBase = 16;
                *aPtr += 2;
            }
            else
            {
                *aBase = 8;
            }
        }
        else
        {
            *aBase = 10;
        }
    }
    else if (*aBase == 16)
    {
        /*
         * allow 0x if base is already 16
         */
        if ((**aPtr == '0') && (acpCharToLower(*(*aPtr + 1)) == 'x'))
        {
            *aPtr += 2;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        if ((*aBase < 2) || (*aBase > 36))
        {
            return ACP_RC_EINVAL;
        }
        else
        {
            /* do nothing */
        }
    }

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t acpStrTolConvert(const acp_char_t **aPtr,
                                     const acp_char_t  *aEndPtr,
                                     acp_uint64_t      *aValue,
                                     acp_sint32_t       aBase)
{
    acp_uint64_t sMaxValue;
    acp_uint32_t sMaxDigit;
    acp_uint32_t sDigit;

    /*
     * values for check range overflow
     */
    sMaxValue = (acp_uint64_t)(ACP_UINT64_MAX / aBase);
    sMaxDigit = (acp_uint32_t)(ACP_UINT64_MAX % aBase);

    /*
     * convert string to integer
     */
    do
    {
        if (acpCharIsDigit(**aPtr) == ACP_TRUE)
        {
            sDigit = (acp_uint32_t)(**aPtr - '0');
        }
        else if (acpCharIsUpper(**aPtr) == ACP_TRUE)
        {
            sDigit = (acp_uint32_t)(**aPtr - 'A' + 10);
        }
        else if (acpCharIsLower(**aPtr) == ACP_TRUE)
        {
            sDigit = (acp_uint32_t)(**aPtr - 'a' + 10);
        }
        else
        {
            break;
        }

        /*
         * add digit if it is valid
         */
        if (sDigit >= (acp_uint32_t)aBase)
        {
            break;
        }
        else
        {
            if ((*aValue > sMaxValue) ||
                ((*aValue == sMaxValue) && (sDigit > sMaxDigit)))
            {
                return ACP_RC_ERANGE;
            }
            else
            {
                *aValue *= aBase;
                *aValue += sDigit;
            }
        }

        (*aPtr)++;
    } while (*aPtr < aEndPtr);

    return ACP_RC_SUCCESS;
}

/**
 * convert a string to a integer
 *
 * @param aStr pointer to the string object
 * @param aSign pointer to a variable to store sign whose value could be 1 or -1
 * whether the converted value is negative or not
 * @param aValue pointer to a variable to store converted interger value
 * @param aEndIndex pointer to a variable to store
 * the index of the first invalid character
 * @param aFromIndex index to character to be converted
 * (including the character at index)
 * @param aBase base for convert
 * @return result code
 *
 * @a aBase must be between 2 and 36 inclusive, or be the special value 0.
 * if not, it returns #ACP_RC_EINVAL.
 *
 * if @a aBase is zero, it takes the prefix.
 * the base will be 16 for prefix @c '0x', 8 for prefix @c '0',
 * otherwise, the base will be 10.
 */
ACP_EXPORT acp_rc_t acpStrToInteger(acp_str_t    *aStr,
                                    acp_sint32_t *aSign,
                                    acp_uint64_t *aValue,
                                    acp_sint32_t *aEndIndex,
                                    acp_sint32_t  aFromIndex,
                                    acp_sint32_t  aBase)
{
    const acp_char_t *sFirstDigit;
    const acp_char_t *sPtr = NULL;
    acp_sint32_t      sLen;
    acp_rc_t          sRC;

    ACP_TEST_RAISE(
        (aStr == NULL) || (aSign == NULL) || (aValue == NULL),
        NULL_ARGUMENTS
        );

    ACP_TEST_RAISE(
        aStr->mString == NULL,
        INVALID_ARGUMENTS
        );
            
    /* Get string length and Initialize values */
    sLen = (acp_sint32_t)acpStrGetLength(aStr);
    *aSign  = 1;
    *aValue = 0;

    /*
     * find start position
     */
    if (aFromIndex < 0)
    {
        sPtr = acpStrGetBuffer(aStr);
        ACP_TEST_RAISE(NULL == sPtr, NO_MEMORY);
    }
    else
    {
        acp_char_t *sBuf = NULL;
        ACP_TEST_RAISE(aFromIndex >= sLen, INVALID_ARGUMENTS);
            
        sBuf = acpStrGetBuffer(aStr);
        ACP_TEST_RAISE(NULL == sBuf, NO_MEMORY);
        /* success of getting the pointer */
        sPtr = sBuf + aFromIndex;
    }

    /*
     * skip whitespaces
     */
    for (; acpCharIsSpace(*sPtr) == ACP_TRUE; sPtr++)
    {
        /* just skip */
    }

    /*
     * pick up sign
     */
    switch(*sPtr)
    {
    case '-':
        *aSign = -1;
        sPtr++;
        break;
    case '+':
        sPtr++;
        break;
    default:
        break;
    }

    /*
     * get base
     */
    sRC = acpStrTolGetBase(&sPtr, &aBase);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), UNEXPECTED_BASE);

    /*
     * the first digit position
     */
    sFirstDigit = sPtr;

    /*
     * convert string to integer
     */
    sRC = acpStrTolConvert(&sPtr, aStr->mString + aStr->mLength, aValue, aBase);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), UNEXPECTED_STRING_END);

    /*
     * error if there is no digit
     */
    ACP_TEST_RAISE(sFirstDigit == sPtr, NO_DIGIT);

    /*
     * Set end point and return success! 
     */
    ACP_STR_TOL_RETURN_NOTNULL(ACP_RC_SUCCESS);

    /* Exception Handling */
    ACP_EXCEPTION(NULL_ARGUMENTS)
    {
        /* The arguments point outside of current process' area */
        sRC = ACP_RC_EFAULT;
        sPtr = NULL;
    }

    ACP_EXCEPTION(INVALID_ARGUMENTS)
    {
        /* Invalid arguments */
        sRC = ACP_RC_EINVAL;
        sPtr = NULL;
    }

    ACP_EXCEPTION(NO_MEMORY)
    {
        /* Not enough memory available */
        sRC = ACP_RC_ENOMEM;
        sPtr = NULL;
    }

    ACP_EXCEPTION(NO_DIGIT)
    {
        /* Not a proper string */
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(UNEXPECTED_BASE)
    {
        /* String do not start with digit, 0, 0x */
        sPtr = NULL;
    }

    ACP_EXCEPTION(UNEXPECTED_STRING_END)
    {
        /* No need of handling */
    }

    ACP_EXCEPTION_END;
    /* Set end position and return error code */
    ACP_STR_TOL_RETURN(sRC);
}

/**
 * convert string to uppercase
 *
 * @param aStr pointer to the string object
 */
ACP_EXPORT acp_rc_t acpStrUpper(acp_str_t *aStr)
{
    acp_sint32_t sLen;
    acp_sint32_t i;

    ACP_STR_SURE_MODIFIABLE(aStr);

    sLen = (acp_sint32_t)acpStrGetLength(aStr);

    for (i = 0; i < sLen; i++)
    {
        aStr->mString[i] = acpCharToUpper(aStr->mString[i]);
    }

    return ACP_RC_SUCCESS;
}

/**
 * convert string to lowercase
 *
 * @param aStr pointer to the string object
 */
ACP_EXPORT acp_rc_t acpStrLower(acp_str_t *aStr)
{
    acp_sint32_t sLen;
    acp_sint32_t i;

    ACP_STR_SURE_MODIFIABLE(aStr);

    sLen = (acp_sint32_t)acpStrGetLength(aStr);

    for (i = 0; i < sLen; i++)
    {
        aStr->mString[i] = acpCharToLower(aStr->mString[i]);
    }

    return ACP_RC_SUCCESS;
}
