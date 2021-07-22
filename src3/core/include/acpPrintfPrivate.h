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
 * $Id: acpPrintfPrivate.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_PRINTF_PRIVATE_H_)
#define _O_ACP_PRINTF_PRIVATE_H_

#include <acpChar.h>
#include <acpStd.h>


ACP_EXTERN_C_BEGIN


/*
 * printing null
 */
#define ACP_PRINTF_NULL_STRING "(null)"
#define ACP_PRINTF_NULL_LENGTH ((acp_size_t)6)


/*
 * structures used by functions
 */
struct acpPrintfConv;
struct acpPrintfOutput;

/*
 * argument data type
 */
typedef enum acpPrintfArgType
{
    ACP_PRINTF_ARG_NONE = 0,
    ACP_PRINTF_ARG_INT,
    ACP_PRINTF_ARG_LONG,
    ACP_PRINTF_ARG_VINT,
    ACP_PRINTF_ARG_SIZE,
    ACP_PRINTF_ARG_PTRDIFF,
    ACP_PRINTF_ARG_DOUBLE,
    ACP_PRINTF_ARG_POINTER
} acpPrintfArgType;

/*
 * argument value
 */
typedef union acpPrintfArgValue
{
    acp_sint64_t  mLong;
    acp_double_t  mDouble;
    void         *mPointer;
} acpPrintfArgValue;

/*
 * argument
 */
typedef struct acpPrintfArg
{
    acpPrintfArgType  mType;
    acpPrintfArgValue mValue;
} acpPrintfArg;

/*
 * conversion flags
 */
#define ACP_PRINTF_FLAG_ALT_FORM   1 /* '#' */
#define ACP_PRINTF_FLAG_LEFT_ALIGN 2 /* '-' */
#define ACP_PRINTF_FLAG_SIGN_SPACE 4 /* ' ' */
#define ACP_PRINTF_FLAG_SIGN_PLUS  8 /* '+' */

/*
 * conversion length modifier
 */
typedef enum acpPrintfLength
{
    ACP_PRINTF_LENGTH_DEFAULT = 0, /*    (acp_sint32_t)  */
    ACP_PRINTF_LENGTH_CHAR,        /* hh (acp_sint8_t)   */
    ACP_PRINTF_LENGTH_SHORT,       /* h  (acp_sint16_t)  */
    ACP_PRINTF_LENGTH_VINT,        /* l  (acp_slong_t)   */
    ACP_PRINTF_LENGTH_LONG,        /* ll (acp_sint64_t)  */
    ACP_PRINTF_LENGTH_SIZE,        /* z  (acp_ssize_t)   */
    ACP_PRINTF_LENGTH_PTRDIFF      /* t  (acp_ptrdiff_t) */
} acpPrintfLength;

/*
 * render function type
 */
typedef acp_rc_t acpPrintfRender(struct acpPrintfOutput *aOutput,
                                 struct acpPrintfConv   *aConv);

/*
 * conversion specification
 */
typedef struct acpPrintfConv
{
    acp_char_t         mSpecifier;     /* conversion specifier     */
    acp_char_t         mPadder;        /* padder                   */
    acp_uint8_t        mFlag;          /* flag                     */
    acp_sint32_t       mFieldWidth;    /* field width              */
    acp_sint32_t       mPrecision;     /* precision                */
    acpPrintfLength    mLength;        /* length modifier          */
    const acp_char_t  *mBegin;         /* first char               */
    const acp_char_t  *mEnd;           /* last char + 1            */
    acpPrintfArg      *mArg;           /* argument to convert      */
    acpPrintfArg      *mArgFieldWidth; /* argument for field width */
    acpPrintfArg      *mArgPrecision;  /* argument for precision   */
    acpPrintfRender   *mRender;        /* render function          */
} acpPrintfConv;

/*
 * conversion type
 */
typedef enum acpPrintfConvType
{
    ACP_PRINTF_CONV_UNKNOWN,
    ACP_PRINTF_CONV_GENERIC,
    ACP_PRINTF_CONV_NOCONV
} acpPrintfConvType;

/*
 * output operation
 */
typedef struct acpPrintfOutputOp
{
    acp_rc_t (*mPutChr)(struct acpPrintfOutput *aOutput,
                        acp_char_t              aChar);
    acp_rc_t (*mPutStr)(struct acpPrintfOutput *aOutput,
                        const acp_char_t       *aString,
                        acp_size_t              aLen);
    acp_rc_t (*mPutPad)(struct acpPrintfOutput *aOutput,
                        acp_char_t              aPadder,
                        acp_sint32_t            aLen);
} acpPrintfOutputOp;

/*
 * output specification
 */
typedef struct acpPrintfOutput
{
    acp_size_t         mWritten;

    acp_std_file_t    *mFile;
    acp_size_t         mBufferWritten;

    acp_char_t        *mBuffer;
    acp_size_t         mBufferSize;

    acp_str_t         *mString;

    acpPrintfOutputOp *mOp;
} acpPrintfOutput;


/*
 * printing format core
 */
acp_rc_t acpPrintfCore(acpPrintfOutput  *aOutput,
                       const acp_char_t *aFormat,
                       va_list           aArgs);

/*
 * render functions
 */
acp_rc_t acpPrintfRenderInt(acpPrintfOutput *aOutput, acpPrintfConv *aConv);
acp_rc_t acpPrintfRenderFloat(acpPrintfOutput *aOutput, acpPrintfConv *aConv);
acp_rc_t acpPrintfRenderChar(acpPrintfOutput *aOutput, acpPrintfConv *aConv);
acp_rc_t acpPrintfRenderString(acpPrintfOutput *aOutput, acpPrintfConv *aConv);


ACP_EXTERN_C_END


#endif
