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
 * $Id: acpPrintfCore.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpChar.h>
#include <acpPrintfPrivate.h>


#define ACP_PRINTF_ARG_MAX     100


#define ACP_PRINTF_IS_INTEGER_LITERAL(aCursor)          \
    ((*(aCursor) >= '1') && (*(aCursor) <= '9'))

#define ACP_PRINTF_READ_INTEGER_LITERAL(aValue, aCursor)        \
    (aValue) = 0;                                               \
    do                                                          \
    {                                                           \
        (aValue) *= 10;                                         \
        (aValue) += (acp_sint32_t)(*(aCursor) - '0');           \
        (aCursor)++;                                            \
    } while (acpCharIsDigit(*(aCursor)) == ACP_TRUE)


static acp_rc_t acpPrintfStoreWrittenLength(acpPrintfOutput *aOutput,
                                            acpPrintfConv *aConv)
{
    void *sPtr = aConv->mArg->mValue.mPointer;

    switch (aConv->mLength)
    {
        case ACP_PRINTF_LENGTH_CHAR:
            *(acp_sint8_t *)sPtr = (acp_sint8_t)aOutput->mWritten;
            break;
        case ACP_PRINTF_LENGTH_SHORT:
            *(acp_sint16_t *)sPtr = (acp_sint16_t)aOutput->mWritten;
            break;
        case ACP_PRINTF_LENGTH_DEFAULT:
            *(acp_sint32_t *)sPtr = (acp_sint32_t)aOutput->mWritten;
            break;
        case ACP_PRINTF_LENGTH_LONG:
            *(acp_sint64_t *)sPtr = (acp_sint64_t)aOutput->mWritten;
            break;
        case ACP_PRINTF_LENGTH_VINT:
            *(acp_slong_t *)sPtr = (acp_slong_t)aOutput->mWritten;
            break;
        case ACP_PRINTF_LENGTH_SIZE:
            *(acp_ssize_t *)sPtr = (acp_ssize_t)aOutput->mWritten;
            break;
        case ACP_PRINTF_LENGTH_PTRDIFF:
            *(acp_ptrdiff_t *)sPtr = (acp_ptrdiff_t)aOutput->mWritten;
            break;
    }

    return ACP_RC_SUCCESS;
}

ACP_INLINE void acpPrintfGetOptFlag(const acp_char_t **aCursor,
                                    acpPrintfConv     *aConv)
{
    while (1)
    {
        switch (**aCursor)
        {
            case '#':
                aConv->mFlag |= ACP_PRINTF_FLAG_ALT_FORM;
                (*aCursor)++;
                break;
            case '0':
                aConv->mPadder = '0';
                (*aCursor)++;
                break;
            case '-':
                aConv->mFlag |= ACP_PRINTF_FLAG_LEFT_ALIGN;
                (*aCursor)++;
                break;
            case ' ':
                aConv->mFlag |= ACP_PRINTF_FLAG_SIGN_SPACE;
                (*aCursor)++;
                break;
            case '+':
                aConv->mFlag |= ACP_PRINTF_FLAG_SIGN_PLUS;
                (*aCursor)++;
                break;
            default:
                return;
        }
    }
}

ACP_INLINE void acpPrintfGetOptIntValue(const acp_char_t **aCursor,
                                        acp_sint32_t      *aValue,
                                        acpPrintfArg     **aArgPtr,
                                        acpPrintfArg      *aArgs,
                                        acp_sint32_t      *aArgIndex,
                                        acp_sint32_t      *aArgCount)
{
    const acp_char_t *sPtr;
    acp_sint32_t      sTmp;

    if ((acpCharIsDigit(**aCursor)) == ACP_TRUE)
    {
        ACP_PRINTF_READ_INTEGER_LITERAL(sTmp, *aCursor);

        *aValue  = sTmp;
        *aArgPtr = NULL;
    }
    else
    {
        if (**aCursor == '*')
        {
            (*aCursor)++;

            if (ACP_PRINTF_IS_INTEGER_LITERAL(*aCursor))
            {
                sPtr = *aCursor;

                ACP_PRINTF_READ_INTEGER_LITERAL(sTmp, *aCursor);

                if (**aCursor == '$')
                {
                    (*aCursor)++;

                    *aArgIndex = sTmp--;

                    aArgs[sTmp].mType = ACP_PRINTF_ARG_INT;
                    *aArgPtr          = &aArgs[sTmp];
                }
                else
                {
                    *aCursor = sPtr;

                    *aValue  = 0;
                    *aArgPtr = NULL;
                }
            }
            else
            {
                aArgs[*aArgIndex].mType = ACP_PRINTF_ARG_INT;
                *aArgPtr                = &aArgs[*aArgIndex];

                *aArgIndex = *aArgIndex + 1;
            }

            *aArgCount = (*aArgCount < *aArgIndex) ? *aArgIndex : *aArgCount;
        }
        else
        {
            *aValue  = 0;
            *aArgPtr = NULL;
        }
    }
}

ACP_INLINE void acpPrintfGetLengthModifier(const acp_char_t **aCursor,
                                           acpPrintfConv     *aConv)
{
    switch (**aCursor)
    {
        case 'h':
            if (*(*aCursor + 1) == 'h')
            {
                aConv->mArg->mType = ACP_PRINTF_ARG_INT;
                aConv->mLength     = ACP_PRINTF_LENGTH_CHAR;
                *aCursor += 2;
            }
            else
            {
                aConv->mArg->mType = ACP_PRINTF_ARG_INT;
                aConv->mLength     = ACP_PRINTF_LENGTH_SHORT;
                (*aCursor)++;
            }
            break;
        case 'l':
            if (*(*aCursor + 1) == 'l')
            {
                aConv->mArg->mType = ACP_PRINTF_ARG_LONG;
                aConv->mLength     = ACP_PRINTF_LENGTH_LONG;
                *aCursor += 2;
            }
            else
            {
                aConv->mArg->mType = ACP_PRINTF_ARG_VINT;
                aConv->mLength     = ACP_PRINTF_LENGTH_VINT;
                (*aCursor)++;
            }
            break;
        case 'z':
            aConv->mArg->mType = ACP_PRINTF_ARG_SIZE;
            aConv->mLength     = ACP_PRINTF_LENGTH_SIZE;
            (*aCursor)++;
            break;
        case 't':
            aConv->mArg->mType = ACP_PRINTF_ARG_PTRDIFF;
            aConv->mLength     = ACP_PRINTF_LENGTH_PTRDIFF;
            (*aCursor)++;
            break;
        default:
            aConv->mArg->mType = ACP_PRINTF_ARG_INT;
            aConv->mLength     = ACP_PRINTF_LENGTH_DEFAULT;
            break;
    }
}

ACP_INLINE acpPrintfConvType acpPrintfGetSpecifier(const acp_char_t *aCursor,
                                                   acpPrintfConv    *aConv)
{
    acpPrintfConvType sConvType;

    switch (*aCursor)
    {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            aConv->mEnd       = aCursor + 1;
            aConv->mRender    = acpPrintfRenderInt;
            aConv->mSpecifier = *aCursor;
            sConvType         = ACP_PRINTF_CONV_GENERIC;
            break;

        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
            aConv->mEnd        = aCursor + 1;
            aConv->mRender     = acpPrintfRenderFloat;
            aConv->mArg->mType = ACP_PRINTF_ARG_DOUBLE;
            aConv->mSpecifier  = *aCursor;
            sConvType          = ACP_PRINTF_CONV_GENERIC;
            break;

        case 'c':
            aConv->mEnd        = aCursor + 1;
            aConv->mRender     = acpPrintfRenderChar;
            aConv->mArg->mType = ACP_PRINTF_ARG_INT;
            aConv->mSpecifier  = *aCursor;
            sConvType          = ACP_PRINTF_CONV_GENERIC;
            break;

        case 's':
        case 'S':
            aConv->mEnd        = aCursor + 1;
            aConv->mRender     = acpPrintfRenderString;
            aConv->mArg->mType = ACP_PRINTF_ARG_POINTER;
            aConv->mSpecifier  = *aCursor;
            sConvType          = ACP_PRINTF_CONV_GENERIC;
            break;

        case 'p':
            aConv->mFlag       |= ACP_PRINTF_FLAG_ALT_FORM;
            aConv->mArg->mType  = ACP_PRINTF_ARG_VINT;
            aConv->mLength      = ACP_PRINTF_LENGTH_VINT;
            aConv->mEnd         = aCursor + 1;
            aConv->mRender      = acpPrintfRenderInt;
            aConv->mSpecifier   = 'x';
            sConvType           = ACP_PRINTF_CONV_GENERIC;
            break;

        case 'n':
            aConv->mEnd        = aCursor + 1;
            aConv->mRender     = acpPrintfStoreWrittenLength;
            aConv->mArg->mType = ACP_PRINTF_ARG_POINTER;
            aConv->mSpecifier  = *aCursor;
            sConvType          = ACP_PRINTF_CONV_GENERIC;
            break;

        case '%':
            aConv->mEnd       = aCursor + 1;
            aConv->mRender    = acpPrintfRenderChar;
            aConv->mArg       = NULL;
            aConv->mSpecifier = *aCursor;
            sConvType         = ACP_PRINTF_CONV_NOCONV;
            break;

        default:
            sConvType = ACP_PRINTF_CONV_UNKNOWN;
            break;
    }

    return sConvType;
}

ACP_INLINE void acpPrintfReadArguments(acp_sint32_t  aArgCount,
                                       acpPrintfArg *aArgTarget,
                                       va_list       aArgSource)
{
    acp_sint32_t i;

    for (i = 0; i < aArgCount; i++)
    {
        switch (aArgTarget[i].mType)
        {
            case ACP_PRINTF_ARG_LONG:
                aArgTarget[i].mValue.mLong = va_arg(aArgSource, acp_sint64_t);
                break;
            case ACP_PRINTF_ARG_VINT:
                aArgTarget[i].mValue.mLong = va_arg(aArgSource, acp_slong_t);
                break;
            case ACP_PRINTF_ARG_SIZE:
#if !defined(ALINT)
                aArgTarget[i].mValue.mLong = va_arg(aArgSource, acp_ssize_t);
#endif
                break;
            case ACP_PRINTF_ARG_PTRDIFF:
                aArgTarget[i].mValue.mLong =
                    (acp_sint64_t)va_arg(aArgSource, acp_ptrdiff_t);
                break;
            case ACP_PRINTF_ARG_DOUBLE:
                aArgTarget[i].mValue.mDouble = va_arg(aArgSource, acp_double_t);
                break;
            case ACP_PRINTF_ARG_POINTER:
                aArgTarget[i].mValue.mPointer = va_arg(aArgSource, void *);
                break;
            case ACP_PRINTF_ARG_INT:
            default:
                aArgTarget[i].mValue.mLong = va_arg(aArgSource, acp_sint32_t);
                break;
        }
    }
}

ACP_INLINE acp_rc_t acpPrintfPrint(acpPrintfOutput  *aOutput,
                                   const acp_char_t *aFormat,
                                   acpPrintfConv    *aConvs,
                                   acp_sint32_t      aConvCount)
{
    const acp_char_t *sPtr;
    acp_sint32_t      sConvIndex;
    acp_rc_t          sRC;

    for (sPtr = aFormat, sConvIndex = 0; sConvIndex < aConvCount; sConvIndex++)
    {
        /*
         * print format string before the current conversion specifier
         */
        if (sPtr < aConvs[sConvIndex].mBegin)
        {
            sRC = (*aOutput->mOp->mPutStr)(aOutput,
                                           sPtr,
                                           aConvs[sConvIndex].mBegin - sPtr);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                return sRC;
            }
            else
            {
                sPtr = aConvs[sConvIndex].mBegin;
            }
        }
        else
        {
            /* do nothing */
        }

        /*
         * field width
         */
        if (aConvs[sConvIndex].mArgFieldWidth != NULL)
        {
            aConvs[sConvIndex].mFieldWidth =
                (acp_sint32_t)aConvs[sConvIndex].mArgFieldWidth->mValue.mLong;
        }
        else
        {
            /* do nothing */
        }

        /*
         * negative field width
         */
        if ((aConvs[sConvIndex].mFieldWidth > 0) &&
            ((aConvs[sConvIndex].mFlag & ACP_PRINTF_FLAG_LEFT_ALIGN) != 0))
        {
            aConvs[sConvIndex].mFieldWidth = 0 - aConvs[sConvIndex].mFieldWidth;
        }
        else
        {
            /* do nothing */
        }

        /*
         * precision
         */
        if (aConvs[sConvIndex].mArgPrecision != NULL)
        {
            aConvs[sConvIndex].mPrecision =
                (acp_sint32_t)aConvs[sConvIndex].mArgPrecision->mValue.mLong;
        }
        else
        {
            /* do nothing */
        }

        /*
         * render and print the current conversion specifier
         */
        sRC = (*aConvs[sConvIndex].mRender)(aOutput, &aConvs[sConvIndex]);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            sPtr = aConvs[sConvIndex].mEnd;
        }
    }

    /*
     * print remaining format string
     */
    while (*sPtr != '\0')
    {
        if (ACP_RC_NOT_SUCCESS(sRC = (*aOutput->mOp->mPutChr)(aOutput, *sPtr)))
        {
            return sRC;
        }
        else
        {
            sPtr++;
        }
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t acpPrintfCore(acpPrintfOutput  *aOutput,
                       const acp_char_t *aFormat,
                       va_list           aArgs)
{
    const acp_char_t *sCursor;
    acpPrintfArg      sArg[ACP_PRINTF_ARG_MAX];
    acpPrintfConv     sConv[ACP_PRINTF_ARG_MAX];
    acp_sint32_t      sArgPrevIndex;
    acp_sint32_t      sArgIndex;
    acp_sint32_t      sArgCount;
    acp_sint32_t      sConvCount;

    sArgIndex  = 0;
    sArgCount  = 0;
    sConvCount = 0;

    if (aFormat == NULL)
    {
        aFormat = ACP_PRINTF_NULL_STRING;
    }
    else
    {
        /* do nothing */
    }

    for (sCursor = aFormat; *sCursor != '\0'; sCursor++)
    {
        if (*sCursor == '%')
        {
            sArgPrevIndex = sArgIndex;

            /*
             * initialize conversion spec
             */
            sConv[sConvCount].mBegin  = sCursor;
            sConv[sConvCount].mFlag   = (acp_char_t)0;
            sConv[sConvCount].mPadder = ' ';
            sConv[sConvCount].mArg    = NULL;

            /*
             * advance cursor
             */
            sCursor++;

            if (ACP_PRINTF_IS_INTEGER_LITERAL(sCursor))
            {
                /*
                 * [0] if spec begins with numeric literal
                 */
                acp_sint32_t sTmp;

                ACP_PRINTF_READ_INTEGER_LITERAL(sTmp, sCursor);

                if (*sCursor == '$')
                {
                    /*
                     * [1] it is the position of argument to be converted
                     */
                    sCursor++;

                    sConv[sConvCount].mArg = &sArg[sTmp - 1];

                    sArgIndex = sTmp;
                    sArgCount = (sArgCount < sArgIndex) ? sArgIndex : sArgCount;

                    /*
                     * [2] get flags
                     */
                    acpPrintfGetOptFlag(&sCursor, &sConv[sConvCount]);

                    /*
                     * [3] get field width
                     */
                    acpPrintfGetOptIntValue(&sCursor,
                                            &sConv[sConvCount].mFieldWidth,
                                            &sConv[sConvCount].mArgFieldWidth,
                                            sArg,
                                            &sArgIndex,
                                            &sArgCount);
                }
                else
                {
                    /*
                     * [3] it is the field width
                     */
                    sConv[sConvCount].mFieldWidth    = sTmp;
                    sConv[sConvCount].mArgFieldWidth = NULL;
                }
            }
            else
            {
                /*
                 * [2] get flags
                 */
                acpPrintfGetOptFlag(&sCursor, &sConv[sConvCount]);

                /*
                 * [3] get field width
                 */
                acpPrintfGetOptIntValue(&sCursor,
                                        &sConv[sConvCount].mFieldWidth,
                                        &sConv[sConvCount].mArgFieldWidth,
                                        sArg,
                                        &sArgIndex,
                                        &sArgCount);
            }

            /*
             * [4] get precision
             */
            if (*sCursor == '.')
            {
                sCursor++;

                acpPrintfGetOptIntValue(&sCursor,
                                        &sConv[sConvCount].mPrecision,
                                        &sConv[sConvCount].mArgPrecision,
                                        sArg,
                                        &sArgIndex,
                                        &sArgCount);
            }
            else
            {
                sConv[sConvCount].mPrecision    = -1;
                sConv[sConvCount].mArgPrecision = NULL;
            }

            /*
             * [5] set argument pointer if not specified
             */
            if (sConv[sConvCount].mArg == NULL)
            {
                sConv[sConvCount].mArg = &sArg[sArgIndex++];
            }
            else
            {
                /* do nothing */
            }

            /*
             * [6] get length modifier
             */
            acpPrintfGetLengthModifier(&sCursor, &sConv[sConvCount]);

            /*
             * [7] get conversion specifier
             */
            switch (acpPrintfGetSpecifier(sCursor, &sConv[sConvCount]))
            {
                case ACP_PRINTF_CONV_UNKNOWN:
                    sArgIndex = sArgPrevIndex;
                    break;
                case ACP_PRINTF_CONV_GENERIC:
                    sConvCount++;
                    break;
                case ACP_PRINTF_CONV_NOCONV:
                    sArgIndex--;
                    sConvCount++;
                    break;
            }

            sArgCount = (sArgCount < sArgIndex) ? sArgIndex : sArgCount;
        }
        else
        {
            /* continue */
        }
    }

    /*
     * get arguments
     */
    acpPrintfReadArguments(sArgCount, sArg, aArgs);

    /*
     * print
     */
    return acpPrintfPrint(aOutput, aFormat, sConv, sConvCount);
}
