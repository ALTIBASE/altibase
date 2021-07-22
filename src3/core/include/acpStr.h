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
 * $Id: acpStr.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_STR_H_)
#define _O_ACP_STR_H_

/**
 * @file
 * @ingroup CoreChar
 *
 * Altibase Core String Object
 */

#include <acpAlign.h>
#include <acpMem.h>
#include <acpCStr.h>
#include <aceAssert.h>


ACP_EXTERN_C_BEGIN


/**
 * compare or search in case sensitive manner
 * @see acpStrCmp() acpStrFindChar() acpStrFindCharSet()
 * acpStrFindString() acpStrFindCString()
 */
#define ACP_STR_CASE_SENSITIVE   0x00
/**
 * compare or search in case insensitive manner
 * @see acpStrCmp() acpStrFindChar() acpStrFindCharSet()
 * acpStrFindString() acpStrFindCString()
 */
#define ACP_STR_CASE_INSENSITIVE 0x01

/**
 * search forward
 * @see acpStrFindChar() acpStrFindCharSet()
 * acpStrFindString() acpStrFindCString()
 */
#define ACP_STR_SEARCH_FORWARD   0x00
/**
 * search backward
 * @see acpStrFindChar() acpStrFindCharSet()
 * acpStrFindString() acpStrFindCString()
 */
#define ACP_STR_SEARCH_BACKWARD  0x02

/**
 * search index initializer
 * @see acpStrFindChar() acpStrFindCharSet()
 * acpStrFindString() acpStrFindCString()
 */
#define ACP_STR_INDEX_INITIALIZER -1


/**
 * string object
 */
typedef struct acp_str_t
{
    acp_sint32_t  mExtendSize; /* > 0,   dynamic buffer extend size  */
    acp_sint32_t  mSize;       /* > 0,   size of dynamic buffer
                                * == -1, constant buffer
                                * < -1,  size of static buffer       */
    acp_sint32_t  mLength;     /* length of string, -1 means unknown */

    acp_char_t   *mString;     /* string buffer pointer              */
} acp_str_t;


/*
 * string object initializer
 */
/**
 * initializer for the constant string object
 * @param aStr the constant character string
 */
#define ACP_STR_CONST(aStr) { 0, -1, -1, (aStr) }

/*
 * declaration of string object
 */
/**
 * declares a constant string object
 * @param aObj the string to declare
 */
#define ACP_STR_DECLARE_CONST(aObj) acp_str_t aObj
/**
 * declares a static buffered string object
 * @param aObj the string to declare
 * @param aLen maximum length of the string
 */
#define ACP_STR_DECLARE_STATIC(aObj, aLen)      \
    acp_char_t aObj ## _buffer [(aLen) + 1];    \
    acp_str_t  aObj
/**
 * declares a dynamic buffered string object
 * @param aObj the string to declare
 */
#define ACP_STR_DECLARE_DYNAMIC(aObj) acp_str_t aObj

/*
 * initialization of string object
 */
/**
 * initializes a constant string object
 * @param aObj the string to initialize
 */
#define ACP_STR_INIT_CONST(aObj)                                               \
    do                                                                         \
    {                                                                          \
        (aObj).mExtendSize = 0;                                                \
        (aObj).mSize       = -1;                                               \
        (aObj).mLength     = -1;                                               \
        (aObj).mString     = NULL;                                             \
    } while (0)
/**
 * initializes a static buffered string object
 * @param aObj the string to initialize
 */
#define ACP_STR_INIT_STATIC(aObj)                                              \
    do                                                                         \
    {                                                                          \
        (aObj).mExtendSize = 0;                                                \
        (aObj).mSize       = -(acp_sint32_t)sizeof(aObj ## _buffer) - 1;       \
        (aObj).mLength     = 0;                                                \
        (aObj).mString     = aObj ## _buffer;                                  \
    } while (0)
/**
 * initializes a static buffered string object
 * @param aObj the string to initialize
 * @param aInitialSize initial allocation size
 * @param aExtendSize extend size to reallocate buffer
 */
#define ACP_STR_INIT_DYNAMIC(aObj, aInitialSize, aExtendSize)                  \
    do                                                                         \
    {                                                                          \
        (aObj).mExtendSize = (aExtendSize);                                 \
        (aObj).mSize       = (aInitialSize);                                \
        (aObj).mLength     = 0;                                                \
        (aObj).mString     = NULL;                                             \
    } while (0)

/**
 * finalizes the string object
 * @param aObj the string to finalize
 */
#define ACP_STR_FINAL(aObj)                                             \
    do                                                                  \
    {                                                                   \
        if (((aObj).mSize > 0) && ((aObj).mString != NULL))             \
        {                                                               \
            acpMemFree((aObj).mString);                                 \
        }                                                               \
        else                                                            \
        {                                                               \
        }                                                               \
    } while (0)


/*
 * private functions
 */
ACP_INLINE acp_rc_t acpStrAllocBuffer(acp_str_t *aStr)
{
    acp_rc_t sRC;

    if ((aStr->mSize > 0) && (aStr->mString == NULL))
    {
        sRC = acpMemAlloc((void **)&aStr->mString, aStr->mSize);
        if (sRC == ACP_RC_SUCCESS)
        {
            acpMemSet(aStr->mString, 0, aStr->mSize);
        }
        else
        {
            /* error situation */
        }
        return sRC;
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpStrExtendBuffer(acp_str_t *aStr, acp_size_t aSize)
{
    acp_rc_t sRC;

    aSize = ACP_ALIGN_ALL(aSize, aStr->mExtendSize);

    if (aSize > (acp_size_t)ACP_SINT32_MAX)
    {
        return ACP_RC_ENOMEM;
    }
    else
    {
        sRC = acpMemRealloc((void **)&aStr->mString, aSize);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            aStr->mSize = (acp_sint32_t)aSize;

            return ACP_RC_SUCCESS;
        }
    }
}

ACP_INLINE void acpStrResetLength(acp_str_t *aStr)
{
    aStr->mLength = -1;
}

/*
 * get
 */
/**
 * returns the length of string in bytes
 * @param aStr pointer to the string object
 * @return the length of string
 */
ACP_INLINE acp_size_t acpStrGetLength(acp_str_t *aStr)
{
    if (aStr->mLength == -1)
    {
        if (aStr->mString == NULL)
        {
            aStr->mLength = 0;
        }
        else
        {
            aStr->mLength = (acp_sint32_t)strlen(aStr->mString);
        }
    }
    else
    {
        /* do nothing */
    }

    return (acp_size_t)aStr->mLength;
}

/**
 * returns the buffer size of string object
 * @param aStr pointer to the string object
 * @return the size of buffer
 */
ACP_INLINE acp_size_t acpStrGetBufferSize(acp_str_t *aStr)
{
    if (aStr->mSize < 0)
    {
        return (acp_size_t)(-(aStr->mSize + 1));
    }
    else
    {
        return (acp_size_t)(aStr->mSize);
    }
}

/**
 * returns the pointer to the character array
 * @param aStr pointer to the string object
 * @return pointer to the buffer
 *
 * it returns NULL, if the dynamic buffered string cannot allocate buffer
 */
ACP_INLINE acp_char_t *acpStrGetBuffer(acp_str_t *aStr)
{
    (void)acpStrAllocBuffer(aStr);

    return aStr->mString;
}

/**
 * returns the character at @a index
 * @param aStr pointer to the string object
 * @param aIndex index of the character
 * @return the character at @a index
 */
ACP_INLINE acp_char_t acpStrGetChar(acp_str_t *aStr, acp_size_t aIndex)
{
    if (aIndex < acpStrGetLength(aStr))
    {
        return aStr->mString[aIndex];
    }
    else
    {
        return ACP_CSTR_TERM;
    }
}

/*
 * set
 */
/**
 * replaces character at @a aIndex to @a aChr
 * @param aStr pointer to the string object
 * @param aChr character to be replaced with
 * @param aIndex index of the character
 * @return result code
 *
 * it returns #ACP_RC_ERANGE if @a aIndex is greater than the length of @a aStr
 */
ACP_INLINE acp_rc_t acpStrSetChar(acp_str_t *aStr,
                                  acp_char_t aChr,
                                  acp_size_t aIndex)
{
    ACE_DASSERT(aStr->mSize != -1);

    if (aIndex < acpStrGetLength(aStr))
    {
        aStr->mString[aIndex] = aChr;

        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_ERANGE;
    }
}

/**
 * sets a constant C style null terminated string
 * @param aStr pointer to the string object
 * @param aCString null terminated string (pointer to the character array)
 *
 * it will cause a memory leak if @a aStr has allocated buffer,
 * so, in this case, @a aStr should be freed via acpStrFree()
 */
ACP_INLINE void acpStrSetConstCString(acp_str_t        *aStr,
                                      const acp_char_t *aCString)
{
    ACE_DASSERT(aStr->mExtendSize == 0);
    ACE_DASSERT(aStr->mSize == -1);

    aStr->mLength     = -1;
    aStr->mString     = (acp_char_t *)aCString;
}

/**
 * sets a constant C style null terminated string
 * @param aStr pointer to the string object
 * @param aCString string (pointer to the character array)
 * @param aLength length of string
 *
 * it will cause a memory leak if @a aStr has allocated buffer,
 * so, in this case, @a aStr should be freed via acpStrFree()
 *
 * @a aLength should be less than #ACP_SINT32_MAX
 */
ACP_INLINE void acpStrSetConstCStringWithLen(acp_str_t        *aStr,
                                             const acp_char_t *aCString,
                                             acp_size_t        aLength)
{
    ACE_DASSERT(aStr->mExtendSize == 0);
    ACE_DASSERT(aStr->mSize == -1);

    aStr->mLength     = (acp_sint32_t)aLength;
    aStr->mString     = (acp_char_t *)aCString;
}

/**
 * makes empty string
 * @param aStr pointer to the string object
 */
ACP_INLINE void acpStrClear(acp_str_t *aStr)
{
    aStr->mLength = 0;
}

/**
 * truncates the string to the specified length
 * @param aStr pointer to the string object
 * @param aLength length to trucate
 *
 * @a aLength should be less than or equal to the current length of string
 */
ACP_INLINE void acpStrTruncate(acp_str_t *aStr, acp_size_t aLength)
{
    acp_size_t sCurLen = acpStrGetLength(aStr);

    if (sCurLen > aLength)
    {
        aStr->mLength = (acp_sint32_t)aLength;
    }
    else
    {
        /* do nothing */
    }
}

/*
 * copy
 */
ACP_EXPORT acp_rc_t acpStrCpyBuffer(acp_str_t        *aStr,
                                    const acp_char_t *aBuffer,
                                    acp_size_t        aSize);

/**
 * copies string of @a aSrcStr to @a aDstStr
 *
 * @param aDstStr pointer to the string object
 * @param aSrcStr pointer to the string object to be copied
 * @return result code
 *
 * @see acpStrCpyBuffer()
 */
ACP_INLINE acp_rc_t acpStrCpy(acp_str_t *aDstStr, acp_str_t *aSrcStr)
{
    if (aSrcStr->mString == NULL)
    {
        /* source data can not be a uninitialized state*/
        return ACP_RC_EINVAL;
    }
    else
    {
        if (acpStrGetLength(aSrcStr) > 0)
        {
            acp_char_t *sBuf = acpStrGetBuffer(aSrcStr);

            if (sBuf != NULL)
            {
                return acpStrCpyBuffer(aDstStr,
                                       sBuf,
                                       acpStrGetLength(aSrcStr));
            }
            else
            {
                /* No memory state */
                return ACP_RC_ENOMEM;
            }
        }
        else
        {
            ACE_DASSERT(aDstStr->mSize != -1);

            acpStrClear(aDstStr);

            return ACP_RC_SUCCESS;
        }
    }
}

/**
 * copies C style null terminated string @a aCString to the string object
 *
 * @param aStr pointer to the string object
 * @param aCString null terminated string (pointer to the character array)
 * @return result code
 *
 * @see acpStrCpyBuffer()
 */
ACP_INLINE acp_rc_t acpStrCpyCString(acp_str_t        *aStr,
                                     const acp_char_t *aCString)
{
    if (aCString != NULL)
    {
        return acpStrCpyBuffer(aStr, aCString, strlen(aCString));
    }
    else
    {
        ACE_DASSERT(aStr->mSize != -1);

        acpStrClear(aStr);

        return ACP_RC_SUCCESS;
    }
}

/*@printflike@*/
ACP_EXPORT acp_rc_t acpStrCpyFormat(acp_str_t        *aStr,
                                    const acp_char_t *aFormat, ...);
ACP_EXPORT acp_rc_t acpStrCpyFormatArgs(acp_str_t        *aStr,
                                        const acp_char_t *aFormat,
                                        va_list           aArgs);

/*
 * concatenate
 */
ACP_EXPORT acp_rc_t acpStrCatBuffer(acp_str_t        *aStr,
                                    const acp_char_t *aBuffer,
                                    acp_size_t        aSize);

/**
 * appends string of @a aSrcStr to @a aDstStr
 *
 * @param aDstStr pointer to the string object
 * @param aSrcStr pointer to the string object to be copied
 * @return result code
 *
 * @see acpStrCatBuffer()
 */
ACP_INLINE acp_rc_t acpStrCat(acp_str_t *aDstStr, acp_str_t *aSrcStr)
{
    if (acpStrGetLength(aSrcStr) > 0)
    {
        return acpStrCatBuffer(aDstStr,
                               acpStrGetBuffer(aSrcStr),
                               acpStrGetLength(aSrcStr));
    }
    else
    {
        ACE_DASSERT(aDstStr->mSize != -1);

        return ACP_RC_SUCCESS;
    }
}

/**
 * appends C style null terminated string @a aCString to the string object
 *
 * @param aStr pointer to the string object
 * @param aCString null terminated string (pointer to the character array)
 * @return result code
 *
 * @see acpStrCatBuffer()
 */
ACP_INLINE acp_rc_t acpStrCatCString(acp_str_t        *aStr,
                                     const acp_char_t *aCString)
{
    if (aCString != NULL)
    {
        return acpStrCatBuffer(aStr, aCString, strlen(aCString));
    }
    else
    {
        ACE_DASSERT(aStr->mSize != -1);

        return ACP_RC_SUCCESS;
    }
}

/*@printflike@*/
ACP_EXPORT acp_rc_t acpStrCatFormat(acp_str_t        *aStr,
                                    const acp_char_t *aFormat, ...);
ACP_EXPORT acp_rc_t acpStrCatFormatArgs(acp_str_t        *aStr,
                                        const acp_char_t *aFormat,
                                        va_list           aArgs);

/*
 * compare
 */
ACP_EXPORT acp_sint32_t acpStrCmp(acp_str_t    *aStr1,
                                  acp_str_t    *aStr2,
                                  acp_sint32_t  aFlag);
ACP_EXPORT acp_sint32_t acpStrCmpCString(acp_str_t        *aStr,
                                         const acp_char_t *aCString,
                                         acp_sint32_t      aFlag);

/**
 * checks whether two strings are identical
 *
 * @param aStr1 pointer to the string object to compare
 * @param aStr2 pointer to the string object to compare
 * @param aFlag bit flags for compare option
 * @return #ACP_TRUE if the two strings are identical, otherwise #ACP_FALSE
 *
 * the flag can be one of #ACP_STR_CASE_SENSITIVE, #ACP_STR_CASE_INSENSITIVE.
 * if the flag is zero, it uses #ACP_STR_CASE_SENSITIVE as default.
 */
ACP_INLINE acp_bool_t acpStrEqual(acp_str_t    *aStr1,
                                  acp_str_t    *aStr2,
                                  acp_sint32_t  aFlag)
{
    acp_sint32_t sLen1 = aStr1->mLength;
    acp_sint32_t sLen2 = aStr2->mLength;

    if ((sLen1 >= 0) && (sLen2 >= 0) && (sLen1 != sLen2))
    {
        return ACP_FALSE;
    }
    else
    {
        return (acpStrCmp(aStr1, aStr2, aFlag) == 0) ? ACP_TRUE : ACP_FALSE;
    }
}

/*
 * search
 */
ACP_EXPORT acp_rc_t acpStrFindChar(acp_str_t    *aStr,
                                   acp_char_t    aFindChar,
                                   acp_sint32_t *aFoundIndex,
                                   acp_sint32_t  aFromIndex,
                                   acp_sint32_t  aFlag);
ACP_EXPORT acp_rc_t acpStrFindCharSet(acp_str_t        *aStr,
                                      const acp_char_t *aFindCharSet,
                                      acp_sint32_t     *aFoundIndex,
                                      acp_sint32_t      aFromIndex,
                                      acp_sint32_t      aFlag);
ACP_EXPORT acp_rc_t acpStrFindCString(acp_str_t        *aStr,
                                      const acp_char_t *aFindCStr,
                                      acp_sint32_t     *aFoundIndex,
                                      acp_sint32_t      aFromIndex,
                                      acp_sint32_t      aFlag);

/**
 * searches a string
 *
 * @param aStr pointer to the string object
 * @param aFindStr a string to search
 * @param aFoundIndex pointer to a variable to store found index
 * @param aFromIndex index to search start excluding the character at the index
 * @param aFlag bit flag for search option
 * @return result code
 *
 * @see acpStrFindCString() acpStrFindChar() acpStrFindCharSet()
 */
ACP_INLINE acp_rc_t acpStrFindString(acp_str_t    *aStr,
                                     acp_str_t    *aFindStr,
                                     acp_sint32_t *aFoundIndex,
                                     acp_sint32_t  aFromIndex,
                                     acp_sint32_t  aFlag)
{
    acp_sint32_t sFindLen = (acp_sint32_t)acpStrGetLength(aFindStr);

    if (sFindLen == 0)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    return acpStrFindCString(aStr,
                             acpStrGetBuffer(aFindStr),
                             aFoundIndex,
                             aFromIndex,
                             aFlag);
}

/*
 * replace
 */
ACP_EXPORT acp_rc_t acpStrReplaceChar(acp_str_t    *aStr,
                                      acp_char_t    aFindChar,
                                      acp_char_t    aReplaceChar,
                                      acp_sint32_t  aRangeFrom,
                                      acp_sint32_t  aRangeLength);

/*
 * convert
 */
ACP_EXPORT acp_rc_t acpStrToInteger(acp_str_t    *aStr,
                                    acp_sint32_t *aSign,
                                    acp_uint64_t *aValue,
                                    acp_sint32_t *aEndIndex,
                                    acp_sint32_t  aFromIndex,
                                    acp_sint32_t  aBase);

ACP_EXPORT acp_rc_t acpStrUpper(acp_str_t *aStr);
ACP_EXPORT acp_rc_t acpStrLower(acp_str_t *aStr);


ACP_EXTERN_C_END


#endif
